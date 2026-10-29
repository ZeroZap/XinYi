/**
 * @file xy_hal_spi_pc.c
 * @brief PC/Linux simulation layer for SPI HAL
 *
 * Contract, not hardware: per-handle state so a host test meets the answers a board gives - a
 * transfer on a bus that was never initialised is NOT_INIT, a zero-length or oversized transfer is
 * INVALID_PARAM, an out-of-range config is refused, and `len` counts bytes the same way the real
 * backends count them (on a 16-bit bus the count must be even and the hardware counts frames).
 */

#include "xy_hal_spi.h"
#include <stdint.h>
#include <string.h>

#define PC_SPI_SLOTS 4U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    xy_hal_spi_config_t config;
} pc_spi_ctx_t;

static pc_spi_ctx_t g_spi_ctx[PC_SPI_SLOTS];

static pc_spi_ctx_t *pc_spi_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_SPI_SLOTS; index++) {
        if (g_spi_ctx[index].used != 0U && g_spi_ctx[index].handle == handle) {
            return &g_spi_ctx[index];
        }
    }
    return NULL;
}

static pc_spi_ctx_t *pc_spi_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_SPI_SLOTS; index++) {
        if (g_spi_ctx[index].used == 0U) {
            g_spi_ctx[index].used = 1U;
            return &g_spi_ctx[index];
        }
    }
    return NULL;
}

static int pc_spi_config_valid(const xy_hal_spi_config_t *config)
{
    if (config->mode > XY_HAL_SPI_MODE_3) return 0;
    if (config->datasize > XY_HAL_SPI_DATASIZE_16BIT) return 0;
    if (config->firstbit > XY_HAL_SPI_FIRSTBIT_LSB) return 0;
    if (config->nss > XY_HAL_SPI_NSS_HARD_OUTPUT) return 0;
    if (config->direction > XY_HAL_SPI_DIR_1LINE) return 0;
    if (config->is_master > 1U) return 0;
    return 1;
}

static xy_hal_error_t pc_spi_check_transfer(const void *hspi, size_t len)
{
    pc_spi_ctx_t *ctx = pc_spi_find(hspi);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* `len` is bytes. On a 16-bit bus the items are words, so an odd byte count cannot be one
     * transfer - the real backends refuse it rather than moving half a frame. */
    if (len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx->config.datasize == XY_HAL_SPI_DATASIZE_16BIT && (len % 2U) != 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config)
{
    pc_spi_ctx_t *ctx;

    if (spi == NULL || config == NULL || !pc_spi_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_spi_find(spi);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_spi_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = spi;
    }
    ctx->config = *config;
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_deinit(void *spi)
{
    pc_spi_ctx_t *ctx;

    if (spi == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_spi_find(spi);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit(void *hspi, const uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_error_t status = pc_spi_check_transfer(hspi, len);

    (void)timeout; /* milliseconds; the simulated bus never blocks */
    if (status != XY_HAL_OK) {
        return status;
    }
    return data == NULL ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_receive(void *hspi, uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_error_t status = pc_spi_check_transfer(hspi, len);

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

xy_hal_error_t xy_hal_spi_transmit_receive(void *hspi, const uint8_t *tx_data,
                                           uint8_t *rx_data, size_t len, uint32_t timeout)
{
    xy_hal_error_t status = pc_spi_check_transfer(hspi, len);

    (void)timeout;
    if (status != XY_HAL_OK) {
        return status;
    }
    if (tx_data == NULL || rx_data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memset(rx_data, 0, len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_stop(void *hspi)
{
    pc_spi_ctx_t *ctx;

    if (hspi == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_spi_find(hspi);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return XY_HAL_OK;
}
