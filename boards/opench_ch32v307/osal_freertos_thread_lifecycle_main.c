#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

#define THREAD_FLAG_READY 0x01U
#define THREAD_FLAGS_ERROR 0x80000000U

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

/* A thread cannot resume itself: suspending "self" means the resume call never
   runs. Suspend and resume a second thread instead, which also makes the state
   transition observable rather than assumed. */
static void worker(void* argument) {
    (void)argument;
    for (;;)
        (void)xy_os_delay(50U);
}

static xy_os_thread_id_t worker_id;

static void monitor(void* argument) {
    xy_os_thread_id_t self;
    const char* name;
    (void)argument;
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    self = xy_os_thread_get_id();
    name = xy_os_thread_get_name(self);
    /* Split what used to be one compound condition: when this self-check fails,
       the marker has to name the property that was wrong, otherwise the only
       evidence is "metadata is wrong" and the reader has to guess. */
    if (self == NULL)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_ID_ERROR_A\r\n");
    if (name == NULL)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_NAME_NULL_ERROR_A\r\n");
    if (name[0] != 'm')
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_NAME_VALUE_ERROR_A\r\n");
    if (xy_os_thread_get_state(self) != XY_OS_THREAD_RUNNING)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_STATE_ERROR_A\r\n");
    if (xy_os_thread_get_stack_size(self) == 0U)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_STACK_ERROR_A\r\n");
    if (xy_os_thread_get_priority(self) != XY_OS_PRIORITY_NORMAL)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_PRIORITY_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_OK_A\r\n");

    /* Same treatment as the metadata block: name the step that failed. */
    if (xy_os_thread_flags_set(self, THREAD_FLAG_READY) != THREAD_FLAG_READY)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_SET_ERROR_A\r\n");
    if (xy_os_thread_flags_get() != THREAD_FLAG_READY)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_GET_ERROR_A\r\n");
    if (xy_os_thread_flags_wait(THREAD_FLAG_READY, XY_OS_FLAGS_WAIT_ALL | XY_OS_FLAGS_NO_CLEAR,
                                0U) != THREAD_FLAG_READY)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_WAIT_ERROR_A\r\n");
    /* The wait clears the bits it consumed unless NO_CLEAR is passed, so asking
       it to consume the flag and then clearing it again can never succeed. Keep
       the flag across the wait, then clear it explicitly and check the residue. */
    if (xy_os_thread_flags_clear(THREAD_FLAG_READY) != THREAD_FLAG_READY)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_CLEAR_ERROR_A\r\n");
    if (xy_os_thread_flags_get() != 0U)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_RESIDUE_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_OK_A\r\n");

    if (xy_os_thread_suspend(worker_id) != XY_OS_OK)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_SUSPEND_ERROR_A\r\n");
    if (xy_os_thread_get_state(worker_id) != XY_OS_THREAD_BLOCKED)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_SUSPEND_STATE_ERROR_A\r\n");
    if (xy_os_thread_resume(worker_id) != XY_OS_OK)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_RESUME_ERROR_A\r\n");
    if (xy_os_thread_get_state(worker_id) != XY_OS_THREAD_READY &&
        xy_os_thread_get_state(worker_id) != XY_OS_THREAD_RUNNING)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_RESUME_STATE_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_SUSPEND_OK_A\r\n");

    if (xy_os_thread_get_count() != 0U || xy_os_thread_enumerate(NULL, 0U) != 0U ||
        xy_os_thread_flags_set(self, 0U) != THREAD_FLAGS_ERROR ||
        xy_os_thread_terminate(NULL) != XY_OS_ERROR_PARAMETER ||
        xy_os_thread_join(NULL) != XY_OS_ERROR_PARAMETER ||
        xy_os_thread_detach(NULL) != XY_OS_ERROR_PARAMETER)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_BOUNDARY_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_BOUNDARY_OK_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_PASS_20261018_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000U);
    }
}

int main(void) {
    const xy_os_thread_attr_t attr = {
        .name = "monitor", .stack_size = 1024U, .priority = XY_OS_PRIORITY_NORMAL};
    const xy_os_thread_attr_t worker_attr = {
        .name = "worker", .stack_size = 512U, .priority = XY_OS_PRIORITY_LOW};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    worker_id = xy_os_thread_new(worker, NULL, &worker_attr);
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL || worker_id == NULL)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_THREAD_ERROR_A\r\n");
    (void)xy_os_kernel_start();
    fail("OPENCH_OSAL_THREAD_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
}
