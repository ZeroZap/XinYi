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

static void monitor(void* argument) {
    xy_os_msgqueue_id_t queue;
    uint32_t values[3] = {11U, 22U, 33U};
    uint32_t output = 0;
    uint32_t index;
    (void)argument;
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    queue = xy_os_msgqueue_new(3, sizeof(uint32_t), NULL);
    if (queue == NULL || xy_os_msgqueue_get_capacity(queue) != 3U ||
        xy_os_msgqueue_get_msg_size(queue) != sizeof(uint32_t) ||
        xy_os_msgqueue_get_count(queue) != 0U || xy_os_msgqueue_get_space(queue) != 3U) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_METADATA_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_METADATA_OK_A\r\n");
    for (index = 0; index < 3U; ++index) {
        if (xy_os_msgqueue_put(queue, &values[index], 0, 10) != XY_OS_OK) {
            mark("OPENCH_OSAL_QUEUE_LIFECYCLE_PUT_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    if (xy_os_msgqueue_get_count(queue) != 3U || xy_os_msgqueue_get_space(queue) != 0U ||
        xy_os_msgqueue_put(queue, &values[0], 0, 20) != XY_OS_ERROR_TIMEOUT) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_FULL_ERROR_A\r\n");
        for (;;) {
        }
    }
    for (index = 0; index < 3U; ++index) {
        if (xy_os_msgqueue_get(queue, &output, NULL, 10) != XY_OS_OK || output != values[index]) {
            mark("OPENCH_OSAL_QUEUE_LIFECYCLE_FIFO_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_FIFO_OK_A\r\n");
    {
        uint8_t priority = 0xA5U;

        if (xy_os_msgqueue_get(queue, &output, &priority, 20) == XY_OS_ERROR_TIMEOUT &&
            priority == 0xA5U)
            mark("OPENCH_OSAL_QUEUE_LIFECYCLE_TIMEOUT_OUTPUT_OK_A\r\n");
        else {
            mark("OPENCH_OSAL_QUEUE_LIFECYCLE_TIMEOUT_OUTPUT_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    (void)xy_os_msgqueue_put(queue, &values[0], 0, 10);
    if (xy_os_msgqueue_reset(queue) == XY_OS_OK && xy_os_msgqueue_get_count(queue) == 0U &&
        xy_os_msgqueue_get_space(queue) == 3U)
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESET_OK_A\r\n");
    else
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESET_ERROR_A\r\n");
    if (xy_os_msgqueue_delete(queue) != XY_OS_OK) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_DELETE_ERROR_A\r\n");
        for (;;) {
        }
    }
    if (xy_os_msgqueue_get_count(queue) != 0U || xy_os_msgqueue_get_space(queue) != 0U ||
        xy_os_msgqueue_put(queue, &values[0], 0, 0U) != XY_OS_ERROR_PARAMETER ||
        xy_os_msgqueue_get(queue, &output, NULL, 0U) != XY_OS_ERROR_PARAMETER ||
        xy_os_msgqueue_delete(queue) != XY_OS_ERROR_PARAMETER) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_STALE_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_STALE_OK_A\r\n");
    queue = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    if (queue != NULL && xy_os_msgqueue_get_capacity(queue) == 2U &&
        xy_os_msgqueue_get_msg_size(queue) == sizeof(uint32_t) &&
        xy_os_msgqueue_delete(queue) == XY_OS_OK)
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESOURCE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESOURCE_ERROR_A\r\n");
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_PASS_20261018_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t attr = {
        .name = "monitor", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
