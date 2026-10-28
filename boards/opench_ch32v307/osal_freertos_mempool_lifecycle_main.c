#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

static uint32_t external_pool[12];

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

static void monitor(void* argument) {
    xy_os_mempool_id_t pool;
    xy_os_mempool_attr_t attr = {
        .name = "external", .mp_mem = external_pool, .mp_size = sizeof(external_pool)};
    void* blocks[3];
    uint32_t outsider = 0;
    uint32_t index;
    (void)argument;
    mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    pool = xy_os_mempool_new(3, 16, &attr);
    if (pool == NULL || xy_os_mempool_get_capacity(pool) != 3U ||
        xy_os_mempool_get_block_size(pool) != 16U || xy_os_mempool_get_count(pool) != 0U ||
        xy_os_mempool_get_space(pool) != 3U) {
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_METADATA_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_METADATA_OK_A\r\n");
    for (index = 0; index < 3U; ++index) {
        blocks[index] = xy_os_mempool_alloc(pool, 0);
        if (blocks[index] == NULL) {
            mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_ALLOC_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    if (xy_os_mempool_alloc(pool, 0) == NULL && xy_os_mempool_get_count(pool) == 3U &&
        xy_os_mempool_get_space(pool) == 0U)
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_EXHAUST_OK_A\r\n");
    else
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_EXHAUST_ERROR_A\r\n");
    if (xy_os_mempool_delete(pool) != XY_OS_ERROR_RESOURCE)
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_OUTSTANDING_DELETE_ERROR_A\r\n");
    else
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_OUTSTANDING_DELETE_OK_A\r\n");
    if (xy_os_mempool_alloc(pool, 0) == NULL &&
        xy_os_mempool_free(pool, blocks[0]) == XY_OS_OK)
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_DELETE_RACE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_DELETE_RACE_ERROR_A\r\n");
    if (xy_os_mempool_free(pool, &outsider) != XY_OS_ERROR_PARAMETER ||
        xy_os_mempool_free(pool, (uint8_t*)blocks[0] + 1) != XY_OS_ERROR_PARAMETER) {
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_FOREIGN_ERROR_A\r\n");
        for (;;) {
        }
    }
    if (xy_os_mempool_free(pool, blocks[0]) != XY_OS_OK ||
        xy_os_mempool_free(pool, blocks[0]) != XY_OS_ERROR_RESOURCE ||
        xy_os_mempool_get_count(pool) != 2U || xy_os_mempool_get_space(pool) != 1U) {
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_DOUBLE_FREE_ERROR_A\r\n");
        for (;;) {
        }
    }
    blocks[0] = xy_os_mempool_alloc(pool, 0);
    if (blocks[0] != NULL)
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_REUSE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_REUSE_ERROR_A\r\n");
    for (index = 0; index < 3U; ++index)
        (void)xy_os_mempool_free(pool, blocks[index]);
    if (xy_os_mempool_delete(pool) != XY_OS_OK) {
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_DELETE_ERROR_A\r\n");
        for (;;) {
        }
    }
    pool = xy_os_mempool_new(4, 24, NULL);
    if (pool != NULL && xy_os_mempool_delete(pool) == XY_OS_OK)
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_RESOURCE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_RESOURCE_ERROR_A\r\n");
    pool = xy_os_mempool_new(3, 10, NULL);
    if (pool != NULL && xy_os_mempool_get_block_size(pool) == 10U &&
        xy_os_mempool_delete(pool) == XY_OS_OK)
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_UNALIGNED_SIZE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_UNALIGNED_SIZE_ERROR_A\r\n");
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
    mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL) {
        mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
