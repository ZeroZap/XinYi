/**
 * @file xy_lte_uart_adapter.c
 * @brief Callback-backed UART byte transport adapter for xy_lte.
 */

#include "xy_lte_uart_adapter.h"

static int adapter_write(void *context, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    xy_lte_uart_adapter_t *adapter = (xy_lte_uart_adapter_t *)context;

    if (!adapter || !adapter->uart || !adapter->write || !data || len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }

    return adapter->write(adapter->uart, data, len,
                          timeout_ms ? timeout_ms : adapter->default_timeout_ms);
}

static int adapter_read(void *context, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    xy_lte_uart_adapter_t *adapter = (xy_lte_uart_adapter_t *)context;

    if (!adapter || !adapter->uart || !adapter->read || !data || len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }

    return adapter->read(adapter->uart, data, len,
                         timeout_ms ? timeout_ms : adapter->default_timeout_ms);
}

static int adapter_flush(void *context)
{
    xy_lte_uart_adapter_t *adapter = (xy_lte_uart_adapter_t *)context;

    if (!adapter || !adapter->uart) {
        return XY_LTE_INVALID_PARAM;
    }
    if (!adapter->flush) {
        return XY_LTE_OK;
    }

    return adapter->flush(adapter->uart);
}

int xy_lte_uart_adapter_init(xy_lte_uart_adapter_t *adapter, void *uart, uint8_t *rx_buffer,
                             size_t rx_buffer_len, uint32_t default_timeout_ms,
                             xy_lte_uart_write_fn write, xy_lte_uart_read_fn read,
                             xy_lte_uart_flush_fn flush)
{
    if (!adapter || !uart || !rx_buffer || rx_buffer_len == 0U || !write || !read) {
        return XY_LTE_INVALID_PARAM;
    }

    adapter->uart = uart;
    adapter->default_timeout_ms = default_timeout_ms;
    adapter->rx_buffer = rx_buffer;
    adapter->rx_buffer_len = rx_buffer_len;
    adapter->write = write;
    adapter->read = read;
    adapter->flush = flush;
    return XY_LTE_OK;
}

int xy_lte_uart_adapter_get_transport(xy_lte_uart_adapter_t *adapter,
                                       xy_lte_transport_t *transport)
{
    if (!adapter || !transport || !adapter->uart || !adapter->write || !adapter->read) {
        return XY_LTE_INVALID_PARAM;
    }

    transport->context = adapter;
    transport->write = adapter_write;
    transport->read = adapter_read;
    transport->flush = adapter_flush;
    return XY_LTE_OK;
}
