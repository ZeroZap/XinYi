#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

#define EVENT_FLAG_A 0x01U
#define EVENT_FLAG_B 0x02U
#define EVENT_FLAG_ALL (EVENT_FLAG_A | EVENT_FLAG_B)
#define EVENT_FLAGS_ERROR 0x80000000U

static void uart_init(void) {
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
    (void)xy_hal_gpio_init(GPIOA, 9U, &tx);
    (void)xy_hal_gpio_init(GPIOA, 10U, &rx);
    (void)xy_hal_uart_init(USART1, &uart);
}

static void mark(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, UART_TIMEOUT_MS);
}

static void fail(const char* marker) {
    mark(marker);
    for (;;) {
    }
}

static void monitor(void* argument) {
    xy_os_event_flags_id_t flags;
    uint32_t observed;
    (void)argument;
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    flags = xy_os_event_flags_new(NULL);
    if (flags == NULL || xy_os_event_flags_get(flags) != 0U)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CREATE_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CREATE_OK_A\r\n");

    if (xy_os_event_flags_set(flags, EVENT_FLAG_A) != EVENT_FLAG_A ||
        xy_os_event_flags_get(flags) != EVENT_FLAG_A)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_SET_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_SET_OK_A\r\n");

    observed = xy_os_event_flags_wait(flags, EVENT_FLAG_A, XY_OS_FLAGS_WAIT_ALL, 0U);
    if (observed != EVENT_FLAG_A)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_WAIT_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_WAIT_OK_A\r\n");

    if (xy_os_event_flags_clear(flags, EVENT_FLAG_A) != EVENT_FLAG_A ||
        xy_os_event_flags_get(flags) != 0U)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CLEAR_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CLEAR_OK_A\r\n");

    if (xy_os_event_flags_set(flags, EVENT_FLAG_ALL) != EVENT_FLAG_ALL ||
        xy_os_event_flags_wait(flags, EVENT_FLAG_ALL, XY_OS_FLAGS_WAIT_ALL, 0U) != EVENT_FLAG_ALL)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_ALL_ERROR_A\r\n");
    if (xy_os_event_flags_clear(flags, EVENT_FLAG_ALL) != EVENT_FLAG_ALL)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_ALL_CLEAR_ERROR_A\r\n");
    if (xy_os_event_flags_wait(flags, EVENT_FLAG_A, XY_OS_FLAGS_WAIT_ALL, 0U) != EVENT_FLAGS_ERROR)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_TIMEOUT_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_TIMEOUT_OK_A\r\n");

    if (xy_os_event_flags_delete(flags) != XY_OS_OK ||
        xy_os_event_flags_get(flags) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_set(flags, EVENT_FLAG_A) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_clear(flags, EVENT_FLAG_A) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_wait(flags, EVENT_FLAG_A, XY_OS_FLAGS_WAIT_ALL, 0U) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_delete(flags) != XY_OS_ERROR_PARAMETER)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_STALE_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_STALE_OK_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_PASS_20261018_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000U);
    }
}

int main(void) {
    const xy_os_thread_attr_t attr = {
        .name = "monitor", .stack_size = 1024U, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_THREAD_ERROR_A\r\n");
    (void)xy_os_kernel_start();
    fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
}
