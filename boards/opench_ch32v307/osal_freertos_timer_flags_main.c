#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

#define TIMER_FLAG 0x1U
#define TIMER_TARGET 20U
/* XY_OS_EVENT_FLAGS_ERROR is defined in xy_os.h; a local copy of the number would keep checking 0x80000000 after the contract moved. */

static xy_os_thread_id_t monitor_id;
static xy_os_timer_id_t timer_id;
static volatile uint32_t callback_count;
static volatile uint32_t worker_count;

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

/* The WCH-Link virtual COM port drops and reorders bytes, so a marker is not
   evidence once it leaves this function. Publish the same facts in RAM, where
   the debugger can read them: how many markers were emitted, whether any of them
   was a failure, and which marker came last. "ERROR" in a marker name is this
   board's convention for a check that failed; no marker carries both ERROR and
   OK, so the substring test is unambiguous. */
volatile uint32_t xy_image_steps;
volatile uint32_t xy_image_failed;
volatile const char* xy_image_last;

static void mark(const char* text) {
    size_t length = 0U;
    size_t index;
    while (text[length] != '\0')
        ++length;
    xy_image_last = text;
    ++xy_image_steps;
    for (index = 0U; index + 5U <= length; ++index) {
        if (text[index] == 'E' && text[index + 1U] == 'R' && text[index + 2U] == 'R' &&
            text[index + 3U] == 'O' && text[index + 4U] == 'R')
            xy_image_failed = 1U;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, UART_TIMEOUT_MS);
}

static void timer_callback(void* argument) {
    (void)argument;
    callback_count++;
    (void)xy_os_thread_flags_set(monitor_id, TIMER_FLAG);
}

static void worker(void* argument) {
    (void)argument;
    for (;;) {
        worker_count++;
        (void)xy_os_delay(10);
    }
}

static void monitor(void* argument) {
    uint32_t observed = 0;
    uint32_t stopped_count;
    (void)argument;
    mark("OPENCH_OSAL_TIMER_FLAGS_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    if (xy_os_timer_start(timer_id, 10) != XY_OS_OK) {
        mark("OPENCH_OSAL_TIMER_FLAGS_START_ERROR_A\r\n");
        for (;;) {
        }
    }
    while (observed < TIMER_TARGET) {
        uint32_t flags = xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 100);
        if ((flags & TIMER_FLAG) == 0U) {
            mark("OPENCH_OSAL_TIMER_FLAGS_WAIT_ERROR_A\r\n");
            continue;
        }
        observed++;
    }
    if (callback_count == TIMER_TARGET && xy_os_timer_is_running(timer_id) == 1U)
        mark("OPENCH_OSAL_TIMER_FLAGS_PERIODIC_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_PERIODIC_ERROR_A\r\n");
    if (xy_os_timer_stop(timer_id) != XY_OS_OK) {
        mark("OPENCH_OSAL_TIMER_FLAGS_STOP_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_delay(30);
    stopped_count = callback_count;
    (void)xy_os_delay(50);
    if (callback_count == stopped_count && xy_os_timer_is_running(timer_id) == 0U)
        mark("OPENCH_OSAL_TIMER_FLAGS_STOP_STABLE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_STOP_STABLE_ERROR_A\r\n");
    if (xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 20) == XY_OS_EVENT_FLAGS_ERROR)
        mark("OPENCH_OSAL_TIMER_FLAGS_TIMEOUT_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_TIMEOUT_ERROR_A\r\n");
    if (worker_count >= 20U)
        mark("OPENCH_OSAL_TIMER_FLAGS_WORKER_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_WORKER_ERROR_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t worker_attr = {
        .name = "worker", .stack_size = 1024, .priority = XY_OS_PRIORITY_LOW};
    const xy_os_thread_attr_t monitor_attr = {
        .name = "monitor", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_TIMER_FLAGS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    monitor_id = xy_os_thread_new(monitor, NULL, &monitor_attr);
    timer_id = xy_os_timer_new(timer_callback, XY_OS_TIMER_PERIODIC, NULL, NULL);
    if (monitor_id == NULL || timer_id == NULL ||
        xy_os_thread_new(worker, NULL, &worker_attr) == NULL) {
        mark("OPENCH_OSAL_TIMER_FLAGS_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_TIMER_FLAGS_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
