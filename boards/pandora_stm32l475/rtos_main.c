#include "stm32l4xx_hal.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static xy_os_semaphore_id_t sync_sem;
static xy_os_msgqueue_id_t sync_queue;
static xy_os_event_flags_id_t sync_events;
static xy_os_mutex_id_t sync_mutex;
xy_os_semaphore_id_t pandora_isr_sem;
static uint32_t shared_sequence;
static uint8_t resource_pool_memory[2U * sizeof(uint32_t)];

#define SYNC_EVENT_DATA_READY (1UL << 0)
#define BLOCKING_TIMEOUT_TICKS 100U
#define BLOCKING_TIMEOUT_TOLERANCE_TICKS 20U

void _init(void) {}
void _fini(void) {}

void vApplicationStackOverflowHook(void *task, char *task_name)
{
    (void)task;
    (void)task_name;
    __disable_irq();
    for (;;) {
    }
}

static void fail(void)
{
    __disable_irq();
    for (;;) {
    }
}

static void uart_text(const char *text)
{
    uint16_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    (void)HAL_UART_Transmit(&uart1, (uint8_t *)text, length, 100U);
}

static void clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        fail();
    }
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 20;
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        fail();
    }
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        fail();
    }
}

static void gpio_uart_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio);

    uart1.Instance = USART1;
    uart1.Init.BaudRate = 115200;
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    uart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&uart1) != HAL_OK) {
        fail();
    }
}

static void fast_task(void *argument)
{
    uint32_t sequence = 0U;

    (void)argument;
    for (;;) {
        if (xy_os_mutex_acquire(sync_mutex, 100U) != XY_OS_OK) {
            uart_text("OSAL_MUTEX_TIMEOUT\r\n");
            fail();
        }
        shared_sequence = sequence;
        uart_text("OSAL_MUTEX_FAST\r\n");
        if (xy_os_mutex_release(sync_mutex) != XY_OS_OK) {
            fail();
        }
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7);
        uart_text("OSAL_TASK_FAST\r\n");
        if (xy_os_semaphore_release(sync_sem) != XY_OS_OK) {
            fail();
        }
        if (xy_os_msgqueue_put(sync_queue, &sequence, 0U, 100U) != XY_OS_OK) {
            fail();
        }
        uart_text("OSAL_QUEUE_SEND\r\n");
        if ((xy_os_event_flags_set(sync_events, SYNC_EVENT_DATA_READY) & SYNC_EVENT_DATA_READY) ==
            0U) {
            fail();
        }
        uart_text("OSAL_EVENT_SET\r\n");
        ++sequence;
        (void)xy_os_delay(500U);
    }
}

static void slow_task(void *argument)
{
    uint32_t expected_sequence = 0U;
    uint32_t sequence;

    (void)argument;
    for (;;) {
        if (xy_os_semaphore_acquire(sync_sem, 1200U) == XY_OS_OK) {
            uart_text("OSAL_SEM_TAKE\r\n");
        } else {
            uart_text("OSAL_SEM_TIMEOUT\r\n");
            fail();
        }
        if ((xy_os_event_flags_wait(sync_events, SYNC_EVENT_DATA_READY, XY_OS_FLAGS_WAIT_ALL,
                                    100U) &
             SYNC_EVENT_DATA_READY) == 0U) {
            uart_text("OSAL_EVENT_MISMATCH\r\n");
            fail();
        }
        uart_text("OSAL_EVENT_WAIT\r\n");
        if (xy_os_msgqueue_get(sync_queue, &sequence, NULL, 100U) != XY_OS_OK ||
            sequence != expected_sequence) {
            uart_text("OSAL_QUEUE_MISMATCH\r\n");
            fail();
        }
        uart_text("OSAL_QUEUE_RECV\r\n");
        if (xy_os_mutex_acquire(sync_mutex, 100U) != XY_OS_OK) {
            uart_text("OSAL_MUTEX_TIMEOUT\r\n");
            fail();
        }
        if (shared_sequence != sequence) {
            uart_text("OSAL_MUTEX_MISMATCH\r\n");
            fail();
        }
        uart_text("OSAL_MUTEX_SLOW\r\n");
        if (xy_os_mutex_release(sync_mutex) != XY_OS_OK) {
            fail();
        }
        ++expected_sequence;
        uart_text("OSAL_TASK_SLOW\r\n");
    }
}

static void isr_task(void *argument)
{
    (void)argument;
    for (;;) {
        if (xy_os_semaphore_acquire(pandora_isr_sem, 1500U) == XY_OS_OK) {
            uart_text("OSAL_ISR_TAKE\r\n");
        } else {
            uart_text("OSAL_ISR_TIMEOUT\r\n");
            fail();
        }
    }
}

static void resource_task(void *argument)
{
    static const xy_os_mempool_attr_t pool_attr = {
        .name = "stress-pool",
        .mp_mem = resource_pool_memory,
        .mp_size = sizeof(resource_pool_memory),
    };
    uint32_t sequence = 0x12345678U;
    uint32_t received = 0U;
    uint32_t timeout_start;
    uint32_t timeout_elapsed;
    xy_os_mempool_id_t pool;
    xy_os_msgqueue_id_t queue;
    void *block0;
    void *block1;

    (void)argument;
    (void)xy_os_delay(250U);

    pool = xy_os_mempool_new(2U, sizeof(uint32_t), &pool_attr);
    queue = xy_os_msgqueue_new(1U, sizeof(sequence), NULL);
    if (pool == NULL || queue == NULL) {
        uart_text("OSAL_RESOURCE_ERROR\r\n");
        fail();
    }
    block0 = xy_os_mempool_alloc(pool, XY_OS_NO_WAIT);
    block1 = xy_os_mempool_alloc(pool, XY_OS_NO_WAIT);
    if (block0 == NULL || block1 == NULL || xy_os_mempool_alloc(pool, XY_OS_NO_WAIT) != NULL ||
        xy_os_msgqueue_put(queue, &sequence, 0U, XY_OS_NO_WAIT) != XY_OS_OK ||
        xy_os_msgqueue_put(queue, &sequence, 0U, XY_OS_NO_WAIT) == XY_OS_OK) {
        uart_text("OSAL_RESOURCE_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_RESOURCE_EXHAUSTED\r\n");

    timeout_start = xy_os_kernel_get_tick_count();
    if (xy_os_msgqueue_put(queue, &sequence, 0U, BLOCKING_TIMEOUT_TICKS) !=
        XY_OS_ERROR_TIMEOUT) {
        uart_text("OSAL_BLOCKING_TIMEOUT_ERROR\r\n");
        fail();
    }
    timeout_elapsed = xy_os_kernel_get_tick_count() - timeout_start;
    if (timeout_elapsed < BLOCKING_TIMEOUT_TICKS ||
        timeout_elapsed > BLOCKING_TIMEOUT_TICKS + BLOCKING_TIMEOUT_TOLERANCE_TICKS) {
        uart_text("OSAL_BLOCKING_TIMEOUT_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_BLOCKING_TIMEOUT_OK\r\n");

    if (xy_os_mempool_free(pool, block0) != XY_OS_OK ||
        xy_os_mempool_alloc(pool, XY_OS_NO_WAIT) == NULL ||
        xy_os_msgqueue_get(queue, &received, NULL, XY_OS_NO_WAIT) != XY_OS_OK ||
        received != sequence ||
        xy_os_msgqueue_put(queue, &sequence, 0U, XY_OS_NO_WAIT) != XY_OS_OK) {
        uart_text("OSAL_RESOURCE_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_RESOURCE_RECOVERED\r\n");

    if (xy_os_msgqueue_delete(queue) != XY_OS_OK || xy_os_mempool_delete(pool) != XY_OS_OK) {
        uart_text("OSAL_RESOURCE_ERROR\r\n");
        fail();
    }
    pool = xy_os_mempool_new(2U, sizeof(uint32_t), &pool_attr);
    queue = xy_os_msgqueue_new(1U, sizeof(sequence), NULL);
    if (pool == NULL || queue == NULL || xy_os_mempool_delete(pool) != XY_OS_OK ||
        xy_os_msgqueue_delete(queue) != XY_OS_OK) {
        uart_text("OSAL_RESOURCE_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_LIFECYCLE_REINIT\r\n");
    xy_os_thread_exit();
}

int main(void)
{
    static const xy_os_thread_attr_t fast_attr = {
        .name = "osal-fast",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_NORMAL,
    };
    static const xy_os_thread_attr_t slow_attr = {
        .name = "osal-slow",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_LOW,
    };
    static const xy_os_thread_attr_t isr_attr = {
        .name = "osal-isr",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_ABOVE_NORMAL,
    };
    static const xy_os_thread_attr_t resource_attr = {
        .name = "osal-resource",
        .stack_size = 768U,
        .priority = XY_OS_PRIORITY_NORMAL,
    };

    HAL_Init();
    clock_init();
    gpio_uart_init();
    uart_text("PANDORA STM32L475VE XINYI OSAL FREERTOS READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    uart_text("OSAL_STRESS_READY\r\n");

    if (xy_os_kernel_init() != XY_OS_OK ||
        (sync_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        (sync_queue = xy_os_msgqueue_new(2U, sizeof(uint32_t), NULL)) == NULL ||
        (sync_events = xy_os_event_flags_new(NULL)) == NULL ||
        (sync_mutex = xy_os_mutex_new(NULL)) == NULL ||
        (pandora_isr_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        xy_os_thread_new(fast_task, NULL, &fast_attr) == NULL ||
        xy_os_thread_new(slow_task, NULL, &slow_attr) == NULL ||
        xy_os_thread_new(isr_task, NULL, &isr_attr) == NULL ||
        xy_os_thread_new(resource_task, NULL, &resource_attr) == NULL) {
        fail();
    }
    (void)xy_os_kernel_start();
    fail();
}
