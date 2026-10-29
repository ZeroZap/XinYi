/**
 * @file xy_hal_uart_pc.c
 * @brief PC/Linux simulation layer for UART HAL
 *
 * This backend models the contract rather than hardware: it keeps per-handle state so a host test
 * sees the answers a board would give - a transfer on a port that was never initialised is NOT_INIT,
 * a zero-length or oversized transfer is INVALID_PARAM, and a config with an enum outside its range
 * is refused instead of being quietly defaulted. Until now every entry here returned XY_HAL_OK for
 * anything at all, so a host test could pass without touching a single line of the contract.
 */

#include "xy_hal_uart.h"
#include <stdint.h>
#include <string.h>

#define PC_UART_SLOTS 4U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    xy_hal_uart_config_t config;
} pc_uart_ctx_t;

static pc_uart_ctx_t g_uart_ctx[PC_UART_SLOTS];

static pc_uart_ctx_t *pc_uart_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_UART_SLOTS; index++) {
        if (g_uart_ctx[index].used != 0U && g_uart_ctx[index].handle == handle) {
            return &g_uart_ctx[index];
        }
    }
    return NULL;
}

static pc_uart_ctx_t *pc_uart_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_UART_SLOTS; index++) {
        if (g_uart_ctx[index].used == 0U) {
            g_uart_ctx[index].used = 1U;
            return &g_uart_ctx[index];
        }
    }
    return NULL;
}

/* The enums are small and dense, so a range check is the whole validation. A value outside its range
 * would otherwise be read as a bitfield further down and quietly mean something else. */
static int pc_uart_config_valid(const xy_hal_uart_config_t *config)
{
    if (config->baudrate == 0U) return 0;
    if (config->wordlen > XY_HAL_UART_WORDLEN_9B) return 0;
    if (config->stopbits > XY_HAL_UART_STOPBITS_2) return 0;
    if (config->parity > XY_HAL_UART_PARITY_ODD) return 0;
    if (config->flowctrl > XY_HAL_UART_FLOWCTRL_RTS_CTS) return 0;
    if (config->mode != XY_HAL_UART_MODE_TX && config->mode != XY_HAL_UART_MODE_RX &&
        config->mode != XY_HAL_UART_MODE_TX_RX) return 0;
    return 1;
}

static xy_hal_error_t pc_uart_check_transfer(const void *huart, size_t len)
{
    pc_uart_ctx_t *ctx = pc_uart_find(huart);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* Same limit the real backends apply: the transfer count lands in a 16-bit field. */
    if (len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    pc_uart_ctx_t *ctx;

    if (uart == NULL || config == NULL || !pc_uart_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_uart_find(uart);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_uart_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = uart;
    }
    ctx->config = *config;
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_deinit(void *uart)
{
    pc_uart_ctx_t *ctx;

    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_uart_find(uart);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send(void *huart, const uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_error_t status = pc_uart_check_transfer(huart, len);

    (void)timeout; /* milliseconds; the simulated port never blocks */
    if (status != XY_HAL_OK) {
        return status;
    }
    if (data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv(void *huart, uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_error_t status = pc_uart_check_transfer(huart, len);

    (void)timeout;
    if (status != XY_HAL_OK) {
        return status;
    }
    if (data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memset(data, 0, len);
    return XY_HAL_OK;
}
