/**
 * @file xy_lte_hal_uart_adapter.c
 * @brief HAL UART-backed byte transport adapter for xy_lte.
 */

#include "xy_lte_hal_uart_adapter.h"

#include "xy_hal_uart.h"

static uint32_t adapter_timeout(const xy_lte_hal_uart_adapter_t *adapter, uint32_t timeout_ms)
{
    return timeout_ms ? timeout_ms : adapter->default_timeout_ms;
}

static int normalize_hal_status(xy_hal_error_t status)
{
    switch (status) {
    case XY_HAL_OK:
        return XY_LTE_OK;
    case XY_HAL_ERROR_INVALID_PARAM:
        return XY_LTE_INVALID_PARAM;
    case XY_HAL_ERROR_TIMEOUT:
        return XY_LTE_TIMEOUT;
    default:
        return XY_LTE_ERROR;
    }
}

static int adapter_write(void *context, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    xy_lte_hal_uart_adapter_t *adapter = (xy_lte_hal_uart_adapter_t *)context;

    if (!adapter || !adapter->uart || !data || len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }

    return normalize_hal_status(
        xy_hal_uart_send(adapter->uart, data, len, adapter_timeout(adapter, timeout_ms)));
}

static int adapter_read(void *context, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    xy_lte_hal_uart_adapter_t *adapter = (xy_lte_hal_uart_adapter_t *)context;
    xy_hal_error_t status;

    if (!adapter || !adapter->uart || !adapter->rx_buffer || adapter->rx_buffer_len == 0U ||
        !data || len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }

    if (len > adapter->rx_buffer_len) {
        len = adapter->rx_buffer_len;
    }

    status = xy_hal_uart_recv(adapter->uart, data, len, adapter_timeout(adapter, timeout_ms));
    if (status != XY_HAL_OK) {
        return normalize_hal_status(status);
    }

    return (int)len;
}

static int adapter_flush(void *context)
{
    xy_lte_hal_uart_adapter_t *adapter = (xy_lte_hal_uart_adapter_t *)context;

    if (!adapter || !adapter->uart) {
        return XY_LTE_INVALID_PARAM;
    }

    return normalize_hal_status(xy_hal_uart_flush(adapter->uart));
}

int xy_lte_hal_uart_adapter_init(xy_lte_hal_uart_adapter_t *adapter, void *uart,
                                  uint8_t *rx_buffer, size_t rx_buffer_len,
                                  uint32_t default_timeout_ms)
{
    if (!adapter || !uart || !rx_buffer || rx_buffer_len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }

    adapter->uart = uart;
    adapter->default_timeout_ms = default_timeout_ms;
    adapter->rx_buffer = rx_buffer;
    adapter->rx_buffer_len = rx_buffer_len;
    return XY_LTE_OK;
}

int xy_lte_hal_uart_adapter_get_transport(xy_lte_hal_uart_adapter_t *adapter,
                                           xy_lte_transport_t *transport)
{
    if (!adapter || !transport || !adapter->uart || !adapter->rx_buffer ||
        adapter->rx_buffer_len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }

    transport->context = adapter;
    transport->write = adapter_write;
    transport->read = adapter_read;
    transport->flush = adapter_flush;
    return XY_LTE_OK;
}
