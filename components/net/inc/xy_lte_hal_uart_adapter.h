/**
 * @file xy_lte_hal_uart_adapter.h
 * @brief HAL UART-backed byte transport adapter for xy_lte.
 */

#ifndef XY_LTE_HAL_UART_ADAPTER_H
#define XY_LTE_HAL_UART_ADAPTER_H

#include "xy_lte.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *uart;
    uint32_t default_timeout_ms;
    uint8_t *rx_buffer;
    size_t rx_buffer_len;
} xy_lte_hal_uart_adapter_t;

int xy_lte_hal_uart_adapter_init(xy_lte_hal_uart_adapter_t *adapter, void *uart,
                                  uint8_t *rx_buffer, size_t rx_buffer_len,
                                  uint32_t default_timeout_ms);
int xy_lte_hal_uart_adapter_get_transport(xy_lte_hal_uart_adapter_t *adapter,
                                           xy_lte_transport_t *transport);

#ifdef __cplusplus
}
#endif

#endif /* XY_LTE_HAL_UART_ADAPTER_H */
