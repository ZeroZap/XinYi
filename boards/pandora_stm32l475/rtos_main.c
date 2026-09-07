#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
#include "xy_broker.h"
#include "xy_broker_isr_ingress.h"
#include "xy_device.h"
#include "xy_fota.h"
#include "xy_fota_boot.h"
#include "xy_fota_w25q128.h"
#include "xy_hal_dma.h"
#include "xy_hal_gpio.h"
#include "xy_hal_qspi.h"
#include "xy_hal_spi.h"
#include "xy_hal_uart.h"
#include "xy_os.h"
#include "xy_pm.h"
#include "xy_stdio.h"
#include "xy_w25q128.h"

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
xy_os_semaphore_id_t pandora_dma_sem;
xy_os_semaphore_id_t pandora_ipc_isr_sem;
xy_broker_isr_ingress_t pandora_ipc_isr_ingress;
static xy_broker_isr_msg_t pandora_ipc_isr_storage[4];
volatile uint32_t pandora_ipc_isr_attempts;
volatile uint32_t pandora_ipc_isr_accepted;
volatile uint32_t pandora_ipc_isr_full;
volatile int pandora_ipc_isr_result;
TIM_HandleTypeDef pandora_tim6;
static uint32_t shared_sequence;
static uint32_t ipc_expected_sequence;
static uint32_t ipc_delivered_count;
static uint32_t ipc_probe_sequence = UINT32_MAX;
static bool ipc_probe_reject;
static uint32_t ipc_isr_last_sequence;
static uint32_t ipc_isr_delivered;
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
DMA_HandleTypeDef dma1_channel1;
DMA_HandleTypeDef spi1_tx_dma;
static SPI_HandleTypeDef spi1;
static QSPI_HandleTypeDef qspi;
static xy_w25q128_t w25q128;
static uint32_t dma_source[8];
static uint32_t dma_destination[8];
static volatile xy_hal_dma_event_t dma_event;
static volatile xy_hal_spi_event_t spi_event;
static uint32_t w25q128_mcu_reset_recovery_pending;

#define SYNC_EVENT_DATA_READY (1UL << 0)
#define BLOCKING_TIMEOUT_TICKS 100U
#define BLOCKING_TIMEOUT_TOLERANCE_TICKS 20U
#define MULTI_MESSAGES_PER_PRODUCER 8U
#define MULTI_STOP_PRODUCER 0xFFFFFFFFU
#define IPC_ISR_STREAM_COUNT 16U
#define IPC_ISR_STRESS_CYCLES 12U
#define IPC_ISR_STRESS_INTERVAL_TICKS 44000U
#define IPC_ISR_SEQUENCE_FLAG 0x80000000U
#define W25Q128_TEST_ADDRESS 0x00FFF000U
#define W25Q128_TEST_LENGTH 256U
#define W25Q128_MCU_RESET_MAGIC 0x51315253U
#define W25Q128_FOTA_TEST_ADDRESS 0x00FFE000U
#define W25Q128_FOTA_TEST_SIZE 4096U
#define W25Q128_FOTA_IMAGE_ADDRESS 0x00F00000U
#define W25Q128_FOTA_IMAGE_SIZE 0x00080000U
#define PANDORA_FOTA_CONFIRM_REQUEST_MAGIC 0x46525132U
#define PANDORA_FOTA_CONFIRM_ACK_MAGIC 0x46414332U

extern const uint8_t __flash_image_end;

static const xy_hal_qspi_config_t w25q128_qspi_config = {
    .clock_prescaler = 3U,
    .fifo_threshold = 1U,
    .flash_size_bits = 24U,
    .chip_select_high_cycles = 2U,
    .sample_shift_half_cycle = 0U,
    .clock_mode = 0U,
};

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

static void dma_callback(void *dma, xy_hal_dma_event_t event, void *arg)
{
    (void)dma;
    (void)arg;
    dma_event = event;
    (void)xy_os_semaphore_release_from_isr(pandora_dma_sem);
}

static void spi_callback(void *spi, xy_hal_spi_event_t event, void *arg)
{
    (void)spi;
    (void)arg;
    spi_event = event;
    (void)xy_os_semaphore_release_from_isr(pandora_dma_sem);
}

static int w25q128_erase_write_read_test(void)
{
    uint8_t expected[W25Q128_TEST_LENGTH];
    uint8_t actual[W25Q128_TEST_LENGTH];

    for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
        expected[index] = (uint8_t)(index ^ 0xA5U);
        actual[index] = 0U;
    }
    if (xy_w25q128_sector_erase(&w25q128, W25Q128_TEST_ADDRESS, 1000U) != XY_W25Q128_OK ||
        xy_w25q128_page_program(&w25q128, W25Q128_TEST_ADDRESS, expected, sizeof(expected),
                                100U) != XY_W25Q128_OK ||
        xy_w25q128_read(&w25q128, W25Q128_TEST_ADDRESS, actual, sizeof(actual)) !=
            XY_W25Q128_OK) {
        return 0;
    }
    for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
        if (actual[index] != expected[index]) {
            return 0;
        }
    }
    return 1;
}

static int w25q128_quad_read_test(void)
{
    uint8_t expected[W25Q128_TEST_LENGTH];
    uint8_t actual[W25Q128_TEST_LENGTH];

    for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
        expected[index] = (uint8_t)(index ^ 0xA5U);
        actual[index] = 0U;
    }
    if (xy_w25q128_quad_read(&w25q128, W25Q128_TEST_ADDRESS, actual, sizeof(actual), 8U) !=
        XY_W25Q128_OK) {
        return 0;
    }
    for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
        if (actual[index] != expected[index]) {
            return 0;
        }
    }
    return 1;
}

static int w25q128_quad_program_test(void)
{
    uint8_t expected[W25Q128_TEST_LENGTH];
    uint8_t actual[W25Q128_TEST_LENGTH];

    for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
        expected[index] = (uint8_t)(index ^ 0x5AU);
        actual[index] = 0U;
    }
    if (xy_w25q128_sector_erase(&w25q128, W25Q128_TEST_ADDRESS, 1000U) != XY_W25Q128_OK ||
        xy_w25q128_quad_page_program(&w25q128, W25Q128_TEST_ADDRESS, expected,
                                     sizeof(expected), 100U) != XY_W25Q128_OK ||
        xy_w25q128_quad_read(&w25q128, W25Q128_TEST_ADDRESS, actual, sizeof(actual), 8U) !=
            XY_W25Q128_OK) {
        return 0;
    }
    for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
        if (actual[index] != expected[index]) {
            return 0;
        }
    }
    return 1;
}

static int w25q128_fota_full_image_test(void)
{
    const xy_fota_flash_ops_t *fota_ops;
#ifdef PANDORA_FOTA_APPLICATION
    const uint8_t *image = (const uint8_t *)0x08008000U;
#else
    const uint8_t *image = (const uint8_t *)0x08000000U;
#endif
    uint32_t image_size = (uint32_t)((uintptr_t)&__flash_image_end - (uintptr_t)image);
    uint32_t expected_crc;
    xy_fota_t fota;
    xy_fota_boot_candidate_header_t candidate_header = {
        .magic = XY_FOTA_BOOT_CANDIDATE_MAGIC,
        .format_version = XY_FOTA_BOOT_CANDIDATE_FORMAT_VERSION,
        .header_size = sizeof(xy_fota_boot_candidate_header_t),
        .image_offset = sizeof(xy_fota_boot_candidate_header_t),
        .image_version = 2U,
    };
    xy_fota_config_t fota_config = {
        .mode = XY_FOTA_MODE_DUAL_BANK,
        .flash_base_addr = W25Q128_FOTA_IMAGE_ADDRESS + sizeof(candidate_header),
        .slot_size = W25Q128_FOTA_IMAGE_SIZE,
        .slot_count = 1U,
        .enable_rollback = true,
        .min_version = 1U,
    };

#ifdef PANDORA_FOTA_APPLICATION
    candidate_header.load_address = 0x08008000U;
#else
    candidate_header.load_address = 0x08000000U;
#endif

    if (image_size == 0U || image_size > W25Q128_FOTA_IMAGE_SIZE - sizeof(candidate_header)) {
        return 0;
    }
    expected_crc = xy_fota_calc_crc32(image, image_size);
    candidate_header.image_size = image_size;
    candidate_header.image_crc32 = expected_crc;
    if (xy_fota_w25q128_bind(&w25q128, W25Q128_FOTA_IMAGE_ADDRESS,
                              W25Q128_FOTA_IMAGE_SIZE, 1000U) != XY_FOTA_OK ||
        (fota_ops = xy_fota_w25q128_ops()) == NULL || fota_ops->init() != XY_FOTA_OK ||
        fota_ops->erase(W25Q128_FOTA_IMAGE_ADDRESS,
                        image_size + sizeof(candidate_header)) != XY_FOTA_OK ||
        fota_ops->write(W25Q128_FOTA_IMAGE_ADDRESS, (const uint8_t *)&candidate_header,
                        sizeof(candidate_header)) != XY_FOTA_OK ||
        xy_fota_init(&fota, &fota_config) != XY_FOTA_OK ||
        xy_fota_set_flash_ops(&fota, fota_ops) != XY_FOTA_OK ||
        xy_fota_start_download(&fota, 2U, image_size, false) != XY_FOTA_OK ||
        xy_fota_set_expected_crc32(&fota, expected_crc) != XY_FOTA_OK) {
        return 0;
    }
    for (uint32_t offset = 0U; offset < image_size; offset += 256U) {
        uint32_t chunk = image_size - offset;
        if (chunk > 256U) {
            chunk = 256U;
        }
        if (xy_fota_download_chunk(&fota, image + offset, chunk) != XY_FOTA_OK) {
            return 0;
        }
    }
    if (xy_fota_finish_download(&fota) != XY_FOTA_OK) {
        return 0;
    }
    {
        xy_fota_boot_candidate_header_t validated_header;
        xy_fota_boot_candidate_config_t boot_config = {
            .storage_address = W25Q128_FOTA_IMAGE_ADDRESS,
            .storage_size = W25Q128_FOTA_IMAGE_SIZE,
            .execution_limit = 0x0807E000U,
            .sram_base = 0x20000000U,
            .sram_limit = 0x20018000U,
            .sram2_base = 0x10000000U,
            .sram2_limit = 0x10008000U,
            .read = fota_ops->read,
        };
#ifdef PANDORA_FOTA_APPLICATION
        boot_config.execution_base = 0x08008000U;
#else
        boot_config.execution_base = 0x08000000U;
#endif
        if (xy_fota_boot_candidate_validate(&boot_config, &validated_header) != XY_FOTA_OK ||
            validated_header.image_version != 2U || validated_header.image_size != image_size) {
            return 0;
        }
    }
    if (fota_ops->deinit() != XY_FOTA_OK) {
        return 0;
    }
    return 1;
}

static void uart_text(const char *text)
{
    uint16_t length = 0;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 100U);
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
    if (msg->src_server == XY_BROKER_SERVER_TIMER) {
        uint32_t stream_sequence = sequence & ~IPC_ISR_SEQUENCE_FLAG;

        if ((sequence & IPC_ISR_SEQUENCE_FLAG) == 0U || stream_sequence == 0U ||
            (ipc_isr_delivered > 0U && stream_sequence <= ipc_isr_last_sequence)) {
            return XY_BROKER_ERROR;
        }
        ipc_isr_last_sequence = stream_sequence;
        ++ipc_isr_delivered;
        if (xy_printf("OSAL_IPC_ISR_STREAM_DELIVER %u\r\n", (unsigned int)stream_sequence) < 0) {
            return XY_BROKER_ERROR;
        }
        return XY_BROKER_OK;
    }
    if (sequence == ipc_probe_sequence) {
        return ipc_probe_reject ? XY_BROKER_ERROR : XY_BROKER_OK;
    }
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

static int ipc_isr_wake(void *context)
{
    xy_os_status_t status =
        xy_os_semaphore_release_from_isr((xy_os_semaphore_id_t)context);

    /* A full binary semaphore means a drain wake is already pending. */
    return status == XY_OS_OK || status == XY_OS_ERROR_RESOURCE ? XY_BROKER_OK
                                                                : XY_BROKER_ERROR;
}

static void ipc_isr_task(void *argument)
{
    uint32_t recovery_cycle;

    (void)argument;
    for (recovery_cycle = 0U; recovery_cycle < IPC_ISR_STRESS_CYCLES; ++recovery_cycle) {
        uint32_t cycle_delivery_target = ipc_isr_delivered + IPC_ISR_STREAM_COUNT;
        uint32_t task_producer_start;

        if (recovery_cycle > 0U) {
            (void)xy_os_delay(IPC_ISR_STRESS_INTERVAL_TICKS);
        }
        if (xy_os_semaphore_acquire(pandora_ipc_isr_sem, 2500U) != XY_OS_OK ||
            pandora_ipc_isr_result == XY_BROKER_ERROR) {
            uart_text("OSAL_IPC_ISR_ERROR\r\n");
            fail();
        }
        task_producer_start = ipc_delivered_count;
        (void)xy_os_delay(6000U);
        if (pandora_ipc_isr_full == 0U) {
            uart_text("OSAL_IPC_ISR_ERROR\r\n");
            fail();
        }
        uart_text("OSAL_IPC_ISR_BACKPRESSURE\r\n");
        while (ipc_isr_delivered < cycle_delivery_target) {
            int result = xy_broker_isr_drain_one(&pandora_ipc_isr_ingress);

            if (result == XY_BROKER_OK) {
                if (xy_broker_process_msgs(XY_BROKER_SERVER_TIMER, 1U) != 1) {
                    uart_text("OSAL_IPC_ISR_ERROR\r\n");
                    fail();
                }
                continue;
            }
            if (result != XY_BROKER_NOT_FOUND ||
                xy_os_semaphore_acquire(pandora_ipc_isr_sem, 1500U) != XY_OS_OK ||
                pandora_ipc_isr_result == XY_BROKER_ERROR) {
                uart_text("OSAL_IPC_ISR_ERROR\r\n");
                fail();
            }
        }
        if (pandora_ipc_isr_accepted != IPC_ISR_STREAM_COUNT ||
            ipc_isr_last_sequence != pandora_ipc_isr_attempts ||
            ipc_delivered_count <= task_producer_start) {
            uart_text("OSAL_IPC_ISR_ERROR\r\n");
            fail();
        }
        uart_text("OSAL_IPC_TASK_PRODUCER_PROGRESS\r\n");
        uart_text("OSAL_IPC_ISR_RECOVERED\r\n");

        __disable_irq();
        pandora_ipc_isr_accepted = 0U;
        pandora_ipc_isr_full = 0U;
        __enable_irq();
    }
    uart_text("OSAL_IPC_ISR_SUSTAINED_OK\r\n");
    xy_os_thread_exit();
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
    const xy_hal_gpio_config_t led_gpio = {
        XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_NONE, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_LOW, 0U,
    };
    const xy_hal_gpio_config_t uart_gpio = {
        XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF7_USART1,
    };
    const xy_hal_uart_config_t uart_config = {
        115200U, XY_HAL_UART_WORDLEN_8B, XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
        XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX,
    };

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    if (xy_hal_gpio_init(GPIOE, 7U, &led_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 9U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_uart_init(&uart1, &uart_config) != XY_HAL_OK) {
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
        (void)xy_hal_gpio_toggle(GPIOE, 7U);
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

    ipc_probe_sequence = sequence;
    ipc_probe_reject = true;
    if (xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                           XY_BROKER_MSG_SENSOR_DATA, &sequence, sizeof(sequence),
                           XY_BROKER_PRIORITY_NORMAL) != XY_BROKER_OK ||
        xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U) != XY_BROKER_ERROR ||
        xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM) != 0) {
        uart_text("OSAL_IPC_HANDLER_RECOVERY_ERROR\r\n");
        fail();
    }
    uart_text("OSAL_IPC_HANDLER_REJECTED\r\n");
    ipc_probe_reject = false;
    if (xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                           XY_BROKER_MSG_SENSOR_DATA, &sequence, sizeof(sequence),
                           XY_BROKER_PRIORITY_NORMAL) != XY_BROKER_OK ||
        xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U) != 1) {
        uart_text("OSAL_IPC_HANDLER_RECOVERY_ERROR\r\n");
        fail();
    }
    ipc_probe_sequence = UINT32_MAX;
    uart_text("OSAL_IPC_HANDLER_RECOVERED\r\n");

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
    if (xy_hal_dma_init(&dma1_channel1, &config) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_INIT_ERROR\r\n");
        fail();
    }
    if (xy_hal_dma_register_callback(&dma1_channel1, XY_HAL_DMA_EVENT_COMPLETE,
                                     dma_callback, NULL) != XY_HAL_OK ||
        xy_hal_dma_register_callback(&dma1_channel1, XY_HAL_DMA_EVENT_ERROR, dma_callback,
                                     NULL) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_CALLBACK_ERROR\r\n");
        fail();
    }
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    if (xy_hal_dma_start(&dma1_channel1, (uint32_t)dma_source, (uint32_t)dma_destination,
                         8U) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_START_ERROR\r\n");
        fail();
    }
    if (xy_os_semaphore_acquire(pandora_dma_sem, 100U) != XY_OS_OK ||
        dma_event != XY_HAL_DMA_EVENT_COMPLETE) {
        uart_text("PANDORA_DMA_IRQ_ERROR\r\n");
        fail();
    }
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    for (uint32_t index = 0U; index < 8U; ++index) {
        if (dma_destination[index] != dma_source[index]) {
            uart_text("PANDORA_DMA_COMPARE_ERROR\r\n");
            fail();
        }
    }
    if (xy_hal_dma_deinit(&dma1_channel1) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_DEINIT_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_DMA_MEM2MEM_OK\r\n");
    uart_text("PANDORA_DMA_IRQ_CALLBACK_OK\r\n");
    for (uint32_t index = 0U; index < 8U; ++index) {
        dma_source[index] = 0xA5A50000U | index;
        dma_destination[index] = 0U;
    }
    if (xy_hal_dma_init(&dma1_channel1, &config) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_REINIT_ERROR\r\n");
        fail();
    }
    if (xy_hal_dma_start(&dma1_channel1, (uint32_t)dma_source, (uint32_t)dma_destination,
                         8U) != XY_HAL_OK ||
        xy_hal_dma_stop(&dma1_channel1) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_STOP_ERROR\r\n");
        fail();
    }
    if (xy_hal_dma_deinit(&dma1_channel1) != XY_HAL_OK ||
        xy_hal_dma_init(&dma1_channel1, &config) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_REINIT_ERROR\r\n");
        fail();
    }
    if (xy_hal_dma_start(&dma1_channel1, (uint32_t)dma_source, (uint32_t)dma_destination,
                         8U) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_RESTART_ERROR\r\n");
        fail();
    }
    if (xy_hal_dma_poll_complete(&dma1_channel1, 100U) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_REPOLL_ERROR\r\n");
        fail();
    }
    for (uint32_t index = 0U; index < 8U; ++index) {
        if (dma_destination[index] != dma_source[index]) {
            uart_text("PANDORA_DMA_RECOMPARE_ERROR\r\n");
            fail();
        }
    }
    if (xy_hal_dma_deinit(&dma1_channel1) != XY_HAL_OK) {
        uart_text("PANDORA_DMA_REDEINIT_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_DMA_STOP_RECOVERY_OK\r\n");

    {
        const xy_hal_gpio_config_t gpio = {
            XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
            XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF10_QUADSPI,
        };

        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_QSPI_CLK_ENABLE();
        for (uint8_t pin = 10U; pin <= 15U; ++pin) {
            if (xy_hal_gpio_init(GPIOE, pin, &gpio) != XY_HAL_OK) {
                uart_text("PANDORA_W25Q128_JEDEC_ID_ERROR\r\n");
                fail();
            }
        }

        qspi.Instance = QUADSPI;
        {
            if (xy_hal_qspi_init(&qspi, &w25q128_qspi_config) != XY_HAL_OK ||
                xy_w25q128_init(&w25q128, &qspi, "pandora-w25q128") != XY_W25Q128_OK ||
                xy_device_find("pandora-w25q128") != &w25q128.device) {
                uart_text("PANDORA_W25Q128_JEDEC_ID_ERROR\r\n");
                fail();
            }
        }
        if (w25q128_mcu_reset_recovery_pending != 0U) {
            uint8_t recovered[W25Q128_TEST_LENGTH];

            if (xy_w25q128_read(&w25q128, W25Q128_TEST_ADDRESS, recovered,
                                 sizeof(recovered)) != XY_W25Q128_OK) {
                uart_text("PANDORA_W25Q128_MCU_RESET_ERROR\r\n");
                fail();
            }
            for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
                if (recovered[index] != (uint8_t)(index ^ 0x5AU)) {
                    uart_text("PANDORA_W25Q128_MCU_RESET_ERROR\r\n");
                    fail();
                }
            }
            uart_text("PANDORA_W25Q128_MCU_RESET_RECOVERED\r\n");
        }
        uart_text("PANDORA_W25Q128_JEDEC_ID_OK\r\n");
        if (!w25q128_erase_write_read_test()) {
            uart_text("PANDORA_W25Q128_ERASE_WRITE_READ_ERROR\r\n");
            fail();
        }
        uart_text("PANDORA_W25Q128_ERASE_WRITE_READ_OK\r\n");
        uart_text("PANDORA_W25Q128_PERSISTENCE_STAGED\r\n");
        if (xy_hal_qspi_deinit(&qspi) != XY_HAL_OK ||
            xy_hal_qspi_init(&qspi, &w25q128_qspi_config) != XY_HAL_OK) {
            uart_text("PANDORA_W25Q128_PERSISTENCE_ERROR\r\n");
            fail();
        }
        {
            uint8_t persisted[W25Q128_TEST_LENGTH];

            if (xy_w25q128_read(&w25q128, W25Q128_TEST_ADDRESS, persisted,
                                 sizeof(persisted)) != XY_W25Q128_OK) {
                uart_text("PANDORA_W25Q128_PERSISTENCE_ERROR\r\n");
                fail();
            }
            for (uint32_t index = 0U; index < W25Q128_TEST_LENGTH; ++index) {
                if (persisted[index] != (uint8_t)(index ^ 0xA5U)) {
                    uart_text("PANDORA_W25Q128_PERSISTENCE_ERROR\r\n");
                    fail();
                }
            }
        }
        uart_text("PANDORA_W25Q128_PERSISTENCE_RECOVERED\r\n");
        if (!w25q128_quad_read_test()) {
            uart_text("PANDORA_W25Q128_QUAD_READ_ERROR\r\n");
            fail();
        }
        uart_text("PANDORA_W25Q128_QUAD_READ_OK\r\n");
        if (!w25q128_quad_program_test()) {
            uart_text("PANDORA_W25Q128_QUAD_PROGRAM_ERROR\r\n");
            fail();
        }
        uart_text("PANDORA_W25Q128_QUAD_PROGRAM_OK\r\n");
        {
            const xy_fota_flash_ops_t *fota_ops;
            xy_fota_t fota;
            xy_fota_config_t fota_config = {
                .mode = XY_FOTA_MODE_DUAL_BANK,
                .flash_base_addr = W25Q128_FOTA_TEST_ADDRESS,
                .slot_size = W25Q128_FOTA_TEST_SIZE,
                .slot_count = 1U,
                .enable_rollback = true,
                .min_version = 1U,
            };
            uint8_t candidate[300];
            uint8_t readback[300] = {0};
            uint32_t candidate_crc;

            for (uint32_t index = 0U; index < sizeof(candidate); ++index) {
                candidate[index] = (uint8_t)(index ^ 0x3CU);
            }
            candidate_crc = xy_fota_calc_crc32(candidate, sizeof(candidate));
            if (xy_fota_w25q128_bind(&w25q128, W25Q128_FOTA_TEST_ADDRESS,
                                      W25Q128_FOTA_TEST_SIZE, 1000U) != XY_FOTA_OK ||
                (fota_ops = xy_fota_w25q128_ops()) == NULL || fota_ops->init() != XY_FOTA_OK ||
                fota_ops->erase(W25Q128_FOTA_TEST_ADDRESS, sizeof(candidate)) != XY_FOTA_OK ||
                xy_fota_init(&fota, &fota_config) != XY_FOTA_OK ||
                xy_fota_set_flash_ops(&fota, fota_ops) != XY_FOTA_OK ||
                xy_fota_start_download(&fota, 2U, sizeof(candidate), false) != XY_FOTA_OK ||
                xy_fota_set_expected_crc32(&fota, candidate_crc) != XY_FOTA_OK ||
                xy_fota_download_chunk(&fota, candidate, 173U) != XY_FOTA_OK ||
                xy_fota_download_chunk(&fota, candidate + 173U, sizeof(candidate) - 173U) !=
                    XY_FOTA_OK ||
                xy_fota_finish_download(&fota) != XY_FOTA_OK ||
                fota_ops->read(W25Q128_FOTA_TEST_ADDRESS, readback, sizeof(readback)) !=
                    XY_FOTA_OK ||
                fota_ops->deinit() != XY_FOTA_OK) {
                uart_text("PANDORA_W25Q128_FOTA_DOWNLOAD_CRC_ERROR\r\n");
                fail();
            }
            for (uint32_t index = 0U; index < sizeof(candidate); ++index) {
                if (readback[index] != candidate[index]) {
                    uart_text("PANDORA_W25Q128_FOTA_DOWNLOAD_CRC_ERROR\r\n");
                    fail();
                }
            }
            uart_text("PANDORA_W25Q128_FOTA_DOWNLOAD_CRC_OK\r\n");
        }
        if (!w25q128_fota_full_image_test()) {
            uart_text("PANDORA_W25Q128_FOTA_FULL_IMAGE_ERROR\r\n");
            uart_text("PANDORA_FOTA_BOOTABLE_CANDIDATE_ERROR\r\n");
            fail();
        }
        uart_text("PANDORA_W25Q128_FOTA_FULL_IMAGE_OK\r\n");
        uart_text("PANDORA_FOTA_BOOTABLE_CANDIDATE_VALIDATED\r\n");
        if (w25q128_mcu_reset_recovery_pending == 0U) {
#ifndef PANDORA_IPC_ISR_STRESS_ONLY
            RTC->BKP0R = W25Q128_MCU_RESET_MAGIC;
            uart_text("PANDORA_W25Q128_MCU_RESET_STAGED\r\n");
            NVIC_SystemReset();
            fail();
#endif
        }
        if (xy_hal_qspi_deinit(&qspi) != XY_HAL_OK) {
            uart_text("PANDORA_W25Q128_PERSISTENCE_ERROR\r\n");
            fail();
        }
    }

    {
        static const xy_hal_spi_config_t spi_config = {
            .mode = XY_HAL_SPI_MODE_0,
            .direction = XY_HAL_SPI_DIR_2LINES,
            .datasize = XY_HAL_SPI_DATASIZE_8BIT,
            .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
            .nss = XY_HAL_SPI_NSS_SOFT,
            .baudrate_prescaler = SPI_BAUDRATEPRESCALER_16,
            .is_master = 1U,
        };
        static uint8_t spi_tx_data[16] = {
            0xA5U, 0x5AU, 0x00U, 0xFFU, 0x11U, 0x22U, 0x33U, 0x44U,
            0x55U, 0x66U, 0x77U, 0x88U, 0x99U, 0xAAU, 0xBBU, 0xCCU,
        };

        __HAL_RCC_SPI1_CLK_ENABLE();
        spi1_tx_dma.Instance = DMA1_Channel3;
        spi1_tx_dma.Init.Request = DMA_REQUEST_1;
        spi1_tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
        spi1_tx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
        spi1_tx_dma.Init.MemInc = DMA_MINC_ENABLE;
        spi1_tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        spi1_tx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        spi1_tx_dma.Init.Mode = DMA_NORMAL;
        spi1_tx_dma.Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(&spi1_tx_dma) != HAL_OK) {
            uart_text("PANDORA_SPI_DMA_ERROR\r\n");
            fail();
        }
        spi1.Instance = SPI1;
        spi1.hdmatx = &spi1_tx_dma;
        spi1_tx_dma.Parent = &spi1;
        HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 6U, 0U);
        HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
        if (xy_hal_spi_init(&spi1, &spi_config) != XY_HAL_OK ||
            xy_hal_spi_register_callback(&spi1, spi_callback, NULL) != XY_HAL_OK ||
            xy_hal_spi_transmit_dma(&spi1, spi_tx_data, sizeof(spi_tx_data)) != XY_HAL_OK ||
            xy_os_semaphore_acquire(pandora_dma_sem, 100U) != XY_OS_OK ||
            spi_event != XY_HAL_SPI_EVENT_TX_DONE ||
            xy_hal_spi_deinit(&spi1) != XY_HAL_OK || HAL_DMA_DeInit(&spi1_tx_dma) != HAL_OK) {
            uart_text("PANDORA_SPI_DMA_ERROR\r\n");
            fail();
        }
        uart_text("PANDORA_SPI_DMA_TX_OK\r\n");
        spi_event = XY_HAL_SPI_EVENT_ERROR;
        if (HAL_DMA_Init(&spi1_tx_dma) != HAL_OK ||
            xy_hal_spi_init(&spi1, &spi_config) != XY_HAL_OK ||
            xy_hal_spi_register_callback(&spi1, spi_callback, NULL) != XY_HAL_OK ||
            xy_hal_spi_transmit_dma(&spi1, spi_tx_data, sizeof(spi_tx_data)) != XY_HAL_OK ||
            xy_os_semaphore_acquire(pandora_dma_sem, 100U) != XY_OS_OK ||
            spi_event != XY_HAL_SPI_EVENT_TX_DONE ||
            xy_hal_spi_deinit(&spi1) != XY_HAL_OK || HAL_DMA_DeInit(&spi1_tx_dma) != HAL_OK) {
            uart_text("PANDORA_SPI_DMA_RECOVERY_ERROR\r\n");
            fail();
        }
        uart_text("PANDORA_SPI_DMA_RECOVERY_OK\r\n");
        spi_event = XY_HAL_SPI_EVENT_ERROR;
        if (HAL_DMA_Init(&spi1_tx_dma) != HAL_OK ||
            xy_hal_spi_init(&spi1, &spi_config) != XY_HAL_OK ||
            xy_hal_spi_register_callback(&spi1, spi_callback, NULL) != XY_HAL_OK ||
            xy_hal_spi_transmit_dma(&spi1, spi_tx_data, sizeof(spi_tx_data)) != XY_HAL_OK ||
            HAL_SPI_DMAStop(&spi1) != HAL_OK || xy_hal_spi_deinit(&spi1) != XY_HAL_OK ||
            HAL_DMA_DeInit(&spi1_tx_dma) != HAL_OK || HAL_DMA_Init(&spi1_tx_dma) != HAL_OK ||
            xy_hal_spi_init(&spi1, &spi_config) != XY_HAL_OK ||
            xy_hal_spi_register_callback(&spi1, spi_callback, NULL) != XY_HAL_OK ||
            xy_hal_spi_transmit_dma(&spi1, spi_tx_data, sizeof(spi_tx_data)) != XY_HAL_OK ||
            xy_os_semaphore_acquire(pandora_dma_sem, 100U) != XY_OS_OK ||
            spi_event != XY_HAL_SPI_EVENT_TX_DONE ||
            xy_hal_spi_deinit(&spi1) != XY_HAL_OK || HAL_DMA_DeInit(&spi1_tx_dma) != HAL_OK) {
            uart_text("PANDORA_SPI_DMA_ABORT_RECOVERY_ERROR\r\n");
            fail();
        }
        uart_text("PANDORA_SPI_DMA_ABORT_RECOVERY_OK\r\n");
    }
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
        .stack_size = 1536U,
        .priority = XY_OS_PRIORITY_NORMAL,
    };

#ifdef PANDORA_FOTA_APPLICATION
    SCB->VTOR = 0x08008000U;
#else
    SCB->VTOR = 0x08000000U;
#endif
    HAL_Init();
    clock_init();
    gpio_uart_init();

    tim6_init();
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    if (RTC->BKP0R == W25Q128_MCU_RESET_MAGIC) {
        w25q128_mcu_reset_recovery_pending = 1U;
        RTC->BKP0R = 0U;
    }
    uart_text("PANDORA STM32L475VE XINYI OSAL FREERTOS READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
#ifdef PANDORA_FOTA_APPLICATION
    if (RTC->BKP1R == PANDORA_FOTA_CONFIRM_ACK_MAGIC) {
        RTC->BKP1R = 0U;
        uart_text("PANDORA_FOTA_CONFIRM_ACKNOWLEDGED\r\n");
    } else if (w25q128_mcu_reset_recovery_pending != 0U) {
        RTC->BKP1R = PANDORA_FOTA_CONFIRM_REQUEST_MAGIC;
        uart_text("PANDORA_FOTA_CONFIRM_REQUESTED\r\n");
        NVIC_SystemReset();
        fail();
    }
#endif
    uart_text("OSAL_STRESS_READY\r\n");

    xy_stdio_printf_init(uart_log_text);
    if (xy_os_kernel_init() != XY_OS_OK || xy_device_init() != XY_DEVICE_OK ||
        xy_device_register(&ipc_device) != XY_DEVICE_OK ||
        xy_broker_init() != XY_BROKER_OK ||
        xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, ipc_handler, NULL) != XY_BROKER_OK ||
        xy_broker_register_server(XY_BROKER_SERVER_TIMER, ipc_handler, NULL) != XY_BROKER_OK ||
        (sync_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        (sync_queue = xy_os_msgqueue_new(2U, sizeof(uint32_t), NULL)) == NULL ||
        (sync_events = xy_os_event_flags_new(NULL)) == NULL ||
        (sync_mutex = xy_os_mutex_new(NULL)) == NULL ||
        (multi_queue = xy_os_msgqueue_new(4U, sizeof(multi_message_t), NULL)) == NULL ||
        (pandora_isr_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        (pandora_tim6_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        (pandora_dma_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        (pandora_ipc_isr_sem = xy_os_semaphore_new(1U, 0U, NULL)) == NULL ||
        xy_broker_isr_ingress_init(&pandora_ipc_isr_ingress, pandora_ipc_isr_storage, 4U,
                                   ipc_isr_wake, pandora_ipc_isr_sem) != XY_BROKER_OK ||
        xy_os_thread_new(fast_task, NULL, &fast_attr) == NULL ||
        xy_os_thread_new(slow_task, NULL, &slow_attr) == NULL ||
        xy_os_thread_new(isr_task, NULL, &isr_attr) == NULL ||
        xy_os_thread_new(ipc_isr_task, NULL, &isr_attr) == NULL ||
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
