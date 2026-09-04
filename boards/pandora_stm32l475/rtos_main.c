#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
#include "xy_broker.h"
#include "xy_device.h"
#include "xy_hal_dma.h"
#include "xy_os.h"
#include "xy_pm.h"
#include "xy_stdio.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static xy_os_semaphore_id_t sync_sem;
static xy_os_msgqueue_id_t sync_queue;
static xy_os_event_flags_id_t sync_events;
static xy_os_mutex_id_t sync_mutex;
static xy_os_msgqueue_id_t multi_queue;
xy_os_semaphore_id_t pandora_isr_sem;
xy_os_semaphore_id_t pandora_tim6_sem;
TIM_HandleTypeDef pandora_tim6;
static uint32_t shared_sequence;
static uint32_t ipc_expected_sequence;
static uint32_t ipc_delivered_count;
static uint32_t pm_last_tick;
static uint32_t multi_seen[2];
static uint32_t multi_received_count;
static uint32_t multi_consumer_counts[2];
static uint32_t multi_producers_done;
static uint32_t multi_consumers_done;
static xy_device_t ipc_device = {
    .name = "pandora-ipc",
    .type = XY_DEV_TYPE_MISC,
    .state = XY_DEV_STATE_CLOSED,
};
static uint8_t resource_pool_memory[2U * sizeof(uint32_t)];
static DMA_HandleTypeDef dma1_channel1;
static uint32_t dma_source[8];
static uint32_t dma_destination[8];

#define SYNC_EVENT_DATA_READY (1UL << 0)
#define BLOCKING_TIMEOUT_TICKS 100U
#define BLOCKING_TIMEOUT_TOLERANCE_TICKS 20U
#define MULTI_MESSAGES_PER_PRODUCER 8U
#define MULTI_STOP_PRODUCER 0xFFFFFFFFU

typedef struct {
    uint32_t producer;
    uint32_t sequence;
} multi_message_t;

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
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    while (text[length] != '\0') {
        ++length;
    }
    (void)HAL_UART_Transmit(&uart1, (uint8_t *)text, length, 100U);
    if (primask == 0U) {
        __enable_irq();
    }
}

static void uart_log_text(char *text)
{
    uart_text(text);
}

uint32_t xy_os_tick_get(void)
{
    return xy_os_kernel_get_tick_count();
}

static int ipc_handler(const xy_broker_msg_t *msg, void *user_data)
{
    uint32_t sequence;

    (void)user_data;
    if (msg == NULL || msg->msg_id != XY_BROKER_MSG_SENSOR_DATA ||
        msg->payload_len != sizeof(sequence)) {
        return XY_BROKER_ERROR;
    }
    sequence = (uint32_t)msg->payload[0] | ((uint32_t)msg->payload[1] << 8) |
               ((uint32_t)msg->payload[2] << 16) | ((uint32_t)msg->payload[3] << 24);
    if (sequence != ipc_expected_sequence || xy_device_find("pandora-ipc") != &ipc_device) {
        return XY_BROKER_ERROR;
    }
    ++ipc_expected_sequence;
    ++ipc_delivered_count;
    uart_text("OSAL_DEVICE_LOOKUP\r\n");
    if (xy_printf("[I] OSAL_TRACE_DELIVER\r\n") < 0) {
        uart_text("OSAL_TRACE_ERROR\r\n");
        return XY_BROKER_ERROR;
    }
    uart_text("OSAL_IPC_DELIVER\r\n");
    return XY_BROKER_OK;
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

static void tim6_init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();
    pandora_tim6.Instance = TIM6;
    pandora_tim6.Init.Prescaler = 7999U;
    pandora_tim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    pandora_tim6.Init.Period = 6999U;
    pandora_tim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&pandora_tim6) != HAL_OK) {
        fail();
    }
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
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
        if (xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                               XY_BROKER_MSG_SENSOR_DATA, &sequence, sizeof(sequence),
                               XY_BROKER_PRIORITY_NORMAL) != XY_BROKER_OK) {
            uart_text("OSAL_IPC_ERROR\r\n");
            fail();
        }
        uart_text("OSAL_IPC_SEND\r\n");
        {
            uint32_t pm_tick = xy_pm_tick_get();
            uint32_t os_tick = xy_os_kernel_get_tick_count();
            if (pm_tick < pm_last_tick || pm_tick > os_tick || os_tick - pm_tick > 1U) {
                uart_text("OSAL_PM_ERROR\r\n");
                fail();
            }
            pm_last_tick = pm_tick;
            uart_text("OSAL_PM_TICK\r\n");
        }
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
        if (xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U) != 1 ||
            ipc_delivered_count != expected_sequence + 1U) {
            uart_text("OSAL_IPC_ERROR\r\n");
            fail();
        }
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

static void tim6_irq_task(void *argument)
{
    (void)argument;
    if (xy_os_semaphore_acquire(pandora_tim6_sem, 1500U) != XY_OS_OK) {
        uart_text("OSAL_TIM6_IRQ_TIMEOUT\r\n");
        fail();
    }
    uart_text("OSAL_TIM6_IRQ_TAKE\r\n");
    if (HAL_TIM_Base_Stop_IT(&pandora_tim6) != HAL_OK) {
        fail();
    }
    while (xy_os_semaphore_acquire(pandora_tim6_sem, XY_OS_NO_WAIT) == XY_OS_OK) {
    }
    if (xy_os_semaphore_acquire(pandora_tim6_sem, 900U) != XY_OS_ERROR_TIMEOUT) {
        uart_text("OSAL_TIM6_IRQ_RECOVERY_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_TIM6_IRQ_TIMEOUT_EXPECTED\r\n");
    if (HAL_TIM_Base_Start_IT(&pandora_tim6) != HAL_OK ||
        xy_os_semaphore_acquire(pandora_tim6_sem, 1500U) != XY_OS_OK) {
        uart_text("OSAL_TIM6_IRQ_RECOVERY_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_TIM6_IRQ_RECOVERED\r\n");
    for (;;) {
        if (xy_os_semaphore_acquire(pandora_tim6_sem, 1500U) == XY_OS_OK) {
            uart_text("OSAL_TIM6_IRQ_TAKE\r\n");
        } else {
            uart_text("OSAL_TIM6_IRQ_TIMEOUT\r\n");
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

    if (xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                           XY_BROKER_MSG_SENSOR_DATA, &sequence, sizeof(sequence),
                           XY_BROKER_PRIORITY_NORMAL) != XY_BROKER_OK ||
        xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                           XY_BROKER_MSG_SENSOR_DATA, &sequence, sizeof(sequence),
                           XY_BROKER_PRIORITY_NORMAL) != XY_BROKER_OK ||
        xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                           XY_BROKER_MSG_SENSOR_DATA, &sequence, sizeof(sequence),
                           XY_BROKER_PRIORITY_NORMAL) != XY_BROKER_QUEUE_FULL) {
        uart_text("OSAL_IPC_SATURATION_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_IPC_SATURATED\r\n");
    if (xy_broker_clear_queue(XY_BROKER_SERVER_SYSTEM) != XY_BROKER_OK) {
        uart_text("OSAL_IPC_SATURATION_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_IPC_RECOVERED\r\n");

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

static void dma_task(void *argument)
{
    static const xy_hal_dma_config_t config = {
        .direction = XY_HAL_DMA_DIR_MEM_TO_MEM,
        .mode = XY_HAL_DMA_MODE_NORMAL,
        .priority = XY_HAL_DMA_PRIORITY_HIGH,
        .periph_width = XY_HAL_DMA_WIDTH_WORD,
        .mem_width = XY_HAL_DMA_WIDTH_WORD,
        .periph_incr = XY_HAL_DMA_INCR_ENABLE,
        .mem_incr = XY_HAL_DMA_INCR_ENABLE,
    };

    (void)argument;
    (void)xy_os_delay(750U);
    __HAL_RCC_DMA1_CLK_ENABLE();
    dma1_channel1.Instance = DMA1_Channel1;
    for (uint32_t index = 0U; index < 8U; ++index) {
        dma_source[index] = 0x5A5A0000U | index;
        dma_destination[index] = 0U;
    }
    if (xy_hal_dma_init(&dma1_channel1, &config) != XY_HAL_OK ||
        xy_hal_dma_start(&dma1_channel1, (uint32_t)dma_source, (uint32_t)dma_destination, 8U) !=
            XY_HAL_OK ||
        xy_hal_dma_poll_complete(&dma1_channel1, 100U) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_MEM2MEM_ERROR\r\n");
        fail();
    }
    for (uint32_t index = 0U; index < 8U; ++index) {
        if (dma_destination[index] != dma_source[index]) {
            uart_text("PANDORA_DMA_MEM2MEM_ERROR\r\n");
            fail();
        }
    }
    if (xy_hal_dma_deinit(&dma1_channel1) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_MEM2MEM_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_DMA_MEM2MEM_OK\r\n");
    xy_os_thread_exit();
}

static void multi_producer_task(void *argument)
{
    multi_message_t message = {.producer = (uint32_t)(uintptr_t)argument};
    uint32_t producer_done;

    (void)xy_os_delay(1500U);
    for (message.sequence = 0U; message.sequence < MULTI_MESSAGES_PER_PRODUCER;
         ++message.sequence) {
        if (xy_os_msgqueue_put(multi_queue, &message, 0U, 1000U) != XY_OS_OK) {
            uart_text("OSAL_MULTI_PRODUCER_ERROR\r\n");
            fail();
        }
        (void)xy_os_delay(7U + message.producer);
    }

    if (xy_os_mutex_acquire(sync_mutex, 100U) != XY_OS_OK) {
        uart_text("OSAL_MULTI_PRODUCER_ERROR\r\n");
        fail();
    }
    producer_done = ++multi_producers_done;
    if (xy_os_mutex_release(sync_mutex) != XY_OS_OK) {
        fail();
    }
    if (producer_done == 2U) {
        message.producer = MULTI_STOP_PRODUCER;
        message.sequence = 0U;
        if (xy_os_msgqueue_put(multi_queue, &message, 0U, 1000U) != XY_OS_OK ||
            xy_os_msgqueue_put(multi_queue, &message, 0U, 1000U) != XY_OS_OK) {
            uart_text("OSAL_MULTI_PRODUCER_ERROR\r\n");
            fail();
        }
    }
    xy_os_thread_exit();
}

static void multi_consumer_task(void *argument)
{
    multi_message_t message;
    uint32_t consumer = (uint32_t)(uintptr_t)argument;
    uint32_t consumer_done;

    if (consumer >= 2U) {
        uart_text("OSAL_MULTI_CONSUMER_ERROR\r\n");
        fail();
    }
    for (;;) {
        if (xy_os_msgqueue_get(multi_queue, &message, NULL, 3000U) != XY_OS_OK) {
            uart_text("OSAL_MULTI_CONSUMER_ERROR\r\n");
            fail();
        }
        if (message.producer == MULTI_STOP_PRODUCER) {
            break;
        }
        if (message.producer >= 2U || message.sequence >= MULTI_MESSAGES_PER_PRODUCER ||
            xy_os_mutex_acquire(sync_mutex, 100U) != XY_OS_OK) {
            uart_text("OSAL_MULTI_CONSUMER_ERROR\r\n");
            fail();
        }
        if ((multi_seen[message.producer] & (1UL << message.sequence)) != 0U) {
            uart_text("OSAL_MULTI_CONSUMER_ERROR\r\n");
            fail();
        }
        multi_seen[message.producer] |= 1UL << message.sequence;
        ++multi_received_count;
        ++multi_consumer_counts[consumer];
        if (xy_os_mutex_release(sync_mutex) != XY_OS_OK) {
            fail();
        }
        uart_text(consumer == 0U ? "OSAL_MULTI_CONSUMER_0_TAKE\r\n"
                                 : "OSAL_MULTI_CONSUMER_1_TAKE\r\n");
        (void)xy_os_delay(1U);
    }

    if (xy_os_mutex_acquire(sync_mutex, 100U) != XY_OS_OK) {
        uart_text("OSAL_MULTI_CONSUMER_ERROR\r\n");
        fail();
    }
    consumer_done = ++multi_consumers_done;
    if (consumer_done == 2U) {
        if (multi_received_count != 2U * MULTI_MESSAGES_PER_PRODUCER ||
            multi_seen[0] != ((1UL << MULTI_MESSAGES_PER_PRODUCER) - 1U) ||
            multi_seen[1] != ((1UL << MULTI_MESSAGES_PER_PRODUCER) - 1U) ||
            multi_consumer_counts[0] == 0U || multi_consumer_counts[1] == 0U) {
            uart_text("OSAL_MULTI_CONSUMER_ERROR\r\n");
            fail();
        }
        uart_text("OSAL_MULTI_CONSUMER_DISTRIBUTED\r\n");
        uart_text("OSAL_MULTI_PRODUCER_OK\r\n");
    }
    if (xy_os_mutex_release(sync_mutex) != XY_OS_OK) {
        fail();
    }
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
    static const xy_os_thread_attr_t tim6_attr = {
        .name = "osal-tim6",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_ABOVE_NORMAL,
    };
    static const xy_os_thread_attr_t multi_attr = {
        .name = "osal-multi",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_NORMAL,
    };
    static const xy_os_thread_attr_t dma_attr = {
        .name = "hal-dma",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_NORMAL,
    };

    HAL_Init();
    clock_init();
    gpio_uart_init();
    tim6_init();
    uart_text("PANDORA STM32L475VE XINYI OSAL FREERTOS READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    uart_text("OSAL_STRESS_READY\r\n");

    xy_stdio_printf_init(uart_log_text);
    if (xy_os_kernel_init() != XY_OS_OK || xy_device_init() != XY_DEVICE_OK ||
        xy_device_register(&ipc_device) != XY_DEVICE_OK ||
        xy_broker_init() != XY_BROKER_OK ||
        xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, ipc_handler, NULL) != XY_BROKER_OK ||
        (sync_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        (sync_queue = xy_os_msgqueue_new(2U, sizeof(uint32_t), NULL)) == NULL ||
        (sync_events = xy_os_event_flags_new(NULL)) == NULL ||
        (sync_mutex = xy_os_mutex_new(NULL)) == NULL ||
        (multi_queue = xy_os_msgqueue_new(4U, sizeof(multi_message_t), NULL)) == NULL ||
        (pandora_isr_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        (pandora_tim6_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        xy_os_thread_new(fast_task, NULL, &fast_attr) == NULL ||
        xy_os_thread_new(slow_task, NULL, &slow_attr) == NULL ||
        xy_os_thread_new(isr_task, NULL, &isr_attr) == NULL ||
        xy_os_thread_new(tim6_irq_task, NULL, &tim6_attr) == NULL ||
        xy_os_thread_new(resource_task, NULL, &resource_attr) == NULL ||
        xy_os_thread_new(dma_task, NULL, &dma_attr) == NULL ||
        xy_os_thread_new(multi_producer_task, (void *)(uintptr_t)0U, &multi_attr) == NULL ||
        xy_os_thread_new(multi_producer_task, (void *)(uintptr_t)1U, &multi_attr) == NULL ||
        xy_os_thread_new(multi_consumer_task, (void *)(uintptr_t)0U, &multi_attr) == NULL ||
        xy_os_thread_new(multi_consumer_task, (void *)(uintptr_t)1U, &multi_attr) == NULL) {
        fail();
    }
    if (HAL_TIM_Base_Start_IT(&pandora_tim6) != HAL_OK) {
        fail();
    }
    (void)xy_os_kernel_start();
    fail();
}
