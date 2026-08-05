/**
 * @file xy_lte_uart_adapter.h
 * @brief Callback-backed UART byte transport adapter for xy_lte.
 */

#ifndef XY_LTE_UART_ADAPTER_H
#define XY_LTE_UART_ADAPTER_H

#include "xy_lte.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*xy_lte_uart_write_fn)(void *uart, const uint8_t *data, size_t len,
                                    uint32_t timeout_ms);
typedef int (*xy_lte_uart_read_fn)(void *uart, uint8_t *data, size_t len,
                                   uint32_t timeout_ms);
typedef int (*xy_lte_uart_flush_fn)(void *uart);

typedef struct {
    void *uart;
    uint32_t default_timeout_ms;
    uint8_t *rx_buffer;
    size_t rx_buffer_len;
    xy_lte_uart_write_fn write;
    xy_lte_uart_read_fn read;
    xy_lte_uart_flush_fn flush;
} xy_lte_uart_adapter_t;

int xy_lte_uart_adapter_init(xy_lte_uart_adapter_t *adapter, void *uart, uint8_t *rx_buffer,
                             size_t rx_buffer_len, uint32_t default_timeout_ms,
                             xy_lte_uart_write_fn write, xy_lte_uart_read_fn read,
                             xy_lte_uart_flush_fn flush);
int xy_lte_uart_adapter_get_transport(xy_lte_uart_adapter_t *adapter,
                                       xy_lte_transport_t *transport);

#ifdef __cplusplus
}
#endif

#endif /* XY_LTE_UART_ADAPTER_H */
