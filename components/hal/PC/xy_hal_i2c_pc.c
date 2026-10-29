/**
 * @file xy_hal_i2c_pc.c
 * @brief PC/Linux simulation layer for I2C HAL
 *
 * Contract, not hardware: per-handle state so a host test meets the answers a board gives - a
 * transfer on a bus that was never initialised is NOT_INIT, a zero-length or oversized transfer is
 * INVALID_PARAM, an out-of-range config is refused, and 10-bit addressing is NOT_SUPPORTED rather
 * than accepted, because no real backend implements it and a host test that "passes" 10-bit would be
 * testing a fiction.
 */

#include "xy_hal_i2c.h"
#include <stdint.h>
#include <string.h>

#define PC_I2C_SLOTS 4U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    xy_hal_i2c_config_t config;
} pc_i2c_ctx_t;

static pc_i2c_ctx_t g_i2c_ctx[PC_I2C_SLOTS];

static pc_i2c_ctx_t *pc_i2c_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_I2C_SLOTS; index++) {
        if (g_i2c_ctx[index].used != 0U && g_i2c_ctx[index].handle == handle) {
            return &g_i2c_ctx[index];
        }
    }
    return NULL;
}

static pc_i2c_ctx_t *pc_i2c_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_I2C_SLOTS; index++) {
        if (g_i2c_ctx[index].used == 0U) {
            g_i2c_ctx[index].used = 1U;
            return &g_i2c_ctx[index];
        }
    }
    return NULL;
}

static int pc_i2c_config_valid(const xy_hal_i2c_config_t *config)
{
    if (config->clock_speed == 0U) return 0;
    if (config->addr_mode > XY_HAL_I2C_ADDR_10BIT) return 0;
    if (config->duty_cycle > XY_HAL_I2C_DUTY_16_9) return 0;
    return 1;
}

static xy_hal_error_t pc_i2c_check_transfer(const void *hi2c, uint16_t dev_addr, size_t len)
{
    pc_i2c_ctx_t *ctx = pc_i2c_find(hi2c);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* 7-bit, unshifted - the convention the real backends document and enforce. */
    if (dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    pc_i2c_ctx_t *ctx;

    if (i2c == NULL || config == NULL || !pc_i2c_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    /* Declared for the shape of the API and implemented by no backend; a host test must not be the
     * one place where it appears to work. */
    if (config->addr_mode == XY_HAL_I2C_ADDR_10BIT) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    ctx = pc_i2c_find(i2c);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_i2c_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = i2c;
    }
    ctx->config = *config;
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_deinit(void *i2c)
{
    pc_i2c_ctx_t *ctx;

    if (i2c == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_i2c_find(i2c);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *hi2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    xy_hal_error_t status = pc_i2c_check_transfer(hi2c, dev_addr, len);

    (void)timeout; /* milliseconds; the simulated bus never blocks */
    if (status != XY_HAL_OK) {
        return status;
    }
    return data == NULL ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *hi2c, uint16_t dev_addr,
                                         uint8_t *data, size_t len,
                                         uint32_t timeout)
{
    xy_hal_error_t status = pc_i2c_check_transfer(hi2c, dev_addr, len);

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

xy_hal_error_t xy_hal_i2c_mem_write(void *hi2c, uint16_t dev_addr,
                                    uint16_t mem_addr, const uint8_t *data,
                                    size_t len, uint32_t timeout)
{
    xy_hal_error_t status = pc_i2c_check_transfer(hi2c, dev_addr, len);

    (void)mem_addr; (void)timeout;
    if (status != XY_HAL_OK) {
        return status;
    }
    return data == NULL ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_read(void *hi2c, uint16_t dev_addr,
                                   uint16_t mem_addr, uint8_t *data,
                                   size_t len, uint32_t timeout)
{
    xy_hal_error_t status = pc_i2c_check_transfer(hi2c, dev_addr, len);

    (void)mem_addr; (void)timeout;
    if (status != XY_HAL_OK) {
        return status;
    }
    if (data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memset(data, 0, len);
    return XY_HAL_OK;
}
