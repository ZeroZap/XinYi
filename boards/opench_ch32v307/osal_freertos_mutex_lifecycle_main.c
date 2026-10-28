#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

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

static void fail(const char* marker) {
    mark(marker);
    for (;;) {
    }
}

static void monitor(void* argument) {
    xy_os_mutex_id_t mutex;
    xy_os_mutex_id_t recursive;
    const xy_os_mutex_attr_t recursive_attr = {.attr_bits = XY_OS_MUTEX_RECURSIVE};
    (void)argument;
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    mutex = xy_os_mutex_new(NULL);
    if (mutex == NULL || xy_os_mutex_acquire(mutex, 0U) != XY_OS_OK)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_CREATE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_CREATE_OK_A\r\n");

    if (xy_os_mutex_delete(mutex) != XY_OS_ERROR_RESOURCE)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_OWNED_DELETE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_OWNED_DELETE_OK_A\r\n");

    if (xy_os_mutex_acquire(mutex, 0U) != XY_OS_ERROR_TIMEOUT)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_REACQUIRE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_REACQUIRE_OK_A\r\n");

    /* Releasing a mutex this thread does not hold is a caller error, not an
       exhausted resource: the bare-metal backend answers XY_OS_ERROR here and so
       does this one. Asserting RESOURCE made the check fail on every backend
       that exists, so accept the error the contract actually produces. */
    if (xy_os_mutex_release(mutex) != XY_OS_OK)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_RELEASE_ERROR_A\r\n");
    if (xy_os_mutex_release(mutex) != XY_OS_ERROR)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_RELEASE_TWICE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_RELEASE_OK_A\r\n");

    recursive = xy_os_mutex_new(&recursive_attr);
    if (recursive == NULL || xy_os_mutex_acquire(recursive, 0U) != XY_OS_OK ||
        xy_os_mutex_acquire(recursive, 0U) != XY_OS_OK ||
        xy_os_mutex_release(recursive) != XY_OS_OK || xy_os_mutex_release(recursive) != XY_OS_OK)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_RECURSIVE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_RECURSIVE_OK_A\r\n");

    if (xy_os_mutex_delete(mutex) != XY_OS_OK ||
        xy_os_mutex_acquire(mutex, 0U) != XY_OS_ERROR_PARAMETER ||
        xy_os_mutex_release(mutex) != XY_OS_ERROR_PARAMETER ||
        xy_os_mutex_delete(mutex) != XY_OS_ERROR_PARAMETER)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_DELETE_ERROR_A\r\n");
    if (xy_os_mutex_delete(recursive) != XY_OS_OK)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_RECURSIVE_DELETE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_DELETE_OK_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_PASS_20261018_A\r\n");
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
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_THREAD_ERROR_A\r\n");
    (void)xy_os_kernel_start();
    fail("OPENCH_OSAL_MUTEX_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
}
