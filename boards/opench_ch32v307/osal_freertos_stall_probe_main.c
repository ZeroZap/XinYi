#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

#define READY_FLAG 0x1U
#define UART_TIMEOUT_MS 1000U
/* xy_os_event_flags_wait reports an unsatisfied wait with this sentinel, not 0. */
#define EVENT_FLAGS_ERROR 0x80000000U

/*
 * Progress counters that the debugger reads straight out of RAM.
 *
 * The WCH-Link VCP only holds a couple of hundred bytes, so a one-shot image
 * cannot show over serial where it stopped. Every step below bumps
 * xy_probe_stage before the call and moves it on afterwards, so halting the
 * target and reading this word names the exact call the task never returned
 * from. The *_ret words record what each blocking call actually returned.
 */
volatile uint32_t xy_probe_stage;
volatile uint32_t xy_probe_loops;
volatile uint32_t xy_probe_delay_ret;
volatile uint32_t xy_probe_wait_ret;
volatile uint32_t xy_probe_get_ret;
volatile uint32_t xy_probe_timeout_sentinel_ok;

static xy_os_event_flags_id_t event_id;
static xy_os_msgqueue_id_t queue_id;

static int uart_init(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        return 0;
    return 1;
}

static void mark(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, UART_TIMEOUT_MS);
}

static void probe_task(void* argument) {
    uint32_t index;
    uint32_t value = 0U;
    (void)argument;

    xy_probe_stage = 2U;
    mark("OPENCH_OSAL_PROBE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    /* Stage 3 means "inside xy_os_delay"; stage 4 means it came back. A target
       halted at 3 with xy_probe_loops unchanged proves the delay never expires. */
    for (index = 0U; index < 3U; ++index) {
        xy_probe_stage = 3U;
        xy_probe_delay_ret = (uint32_t)xy_os_delay(100);
        xy_probe_loops = index + 1U;
        xy_probe_stage = 4U;
    }
    mark("OPENCH_OSAL_PROBE_DELAYS_DONE_A\r\n");

    xy_probe_stage = 5U;
    xy_probe_wait_ret = xy_os_event_flags_wait(event_id, READY_FLAG, XY_OS_FLAGS_WAIT_ALL, 200);
    xy_probe_stage = 6U;
    if (xy_probe_wait_ret == EVENT_FLAGS_ERROR)
        xy_probe_timeout_sentinel_ok = 1U;

    xy_probe_stage = 7U;
    xy_probe_get_ret = (uint32_t)xy_os_msgqueue_get(queue_id, &value, NULL, 200);
    xy_probe_stage = 8U;

    mark("OPENCH_OSAL_PROBE_SEQUENCE_DONE_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t probe_attr = {
        .name = "probe", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    if (!uart_init())
        for (;;) {
        }
    mark("OPENCH_OSAL_PROBE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    event_id = xy_os_event_flags_new(NULL);
    queue_id = xy_os_msgqueue_new(4, sizeof(uint32_t), NULL);
    if (event_id == NULL || queue_id == NULL ||
        xy_os_thread_new(probe_task, NULL, &probe_attr) == NULL) {
        mark("OPENCH_OSAL_PROBE_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    xy_probe_stage = 1U;
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_PROBE_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
