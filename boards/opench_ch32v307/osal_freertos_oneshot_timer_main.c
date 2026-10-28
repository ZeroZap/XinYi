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
#define FLAGS_ERROR 0x80000000U

static xy_os_thread_id_t monitor_id;
static volatile uint32_t callback_count;
static volatile uint32_t callback_generation;
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
    callback_generation = *(uint32_t*)argument;
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
    uint32_t generation = 1;
    uint32_t deleted_count;
    xy_os_timer_id_t timer;
    (void)argument;
    mark("OPENCH_OSAL_ONESHOT_TIMER_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    timer = xy_os_timer_new(timer_callback, XY_OS_TIMER_ONCE, &generation, NULL);
    if (timer == NULL || xy_os_timer_start(timer, 20) != XY_OS_OK ||
        (xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 100) & TIMER_FLAG) == 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_FIRST_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_delay(1);
    if (callback_count != 1U || callback_generation != 1U || xy_os_timer_is_running(timer) != 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_FIRST_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_ONESHOT_TIMER_FIRST_OK_A\r\n");

    generation = 2;
    if (xy_os_timer_start(timer, 20) != XY_OS_OK ||
        (xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 100) & TIMER_FLAG) == 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESTART_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_delay(1);
    if (callback_count != 2U || callback_generation != 2U || xy_os_timer_is_running(timer) != 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESTART_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_ONESHOT_TIMER_RESTART_OK_A\r\n");

    generation = 3;
    if (xy_os_timer_start(timer, 100) != XY_OS_OK || xy_os_timer_delete(timer) != XY_OS_OK) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_DELETE_ERROR_A\r\n");
        for (;;) {
        }
    }
    deleted_count = callback_count;
    (void)xy_os_delay(130);
    if (callback_count == deleted_count &&
        xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 20) == FLAGS_ERROR)
        mark("OPENCH_OSAL_ONESHOT_TIMER_DELETE_STABLE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ONESHOT_TIMER_DELETE_STABLE_ERROR_A\r\n");

    timer = xy_os_timer_new(timer_callback, XY_OS_TIMER_ONCE, &generation, NULL);
    if (timer != NULL && xy_os_timer_delete(timer) == XY_OS_OK)
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESOURCE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESOURCE_ERROR_A\r\n");
    if (worker_count >= 15U)
        mark("OPENCH_OSAL_ONESHOT_TIMER_WORKER_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ONESHOT_TIMER_WORKER_ERROR_A\r\n");
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
    mark("OPENCH_OSAL_ONESHOT_TIMER_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    monitor_id = xy_os_thread_new(monitor, NULL, &monitor_attr);
    if (monitor_id == NULL || xy_os_thread_new(worker, NULL, &worker_attr) == NULL) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_ONESHOT_TIMER_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
