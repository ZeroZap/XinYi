/**
 * @file xy_hal_i2c.c
 * @brief I2C HAL STM32F4 implementation
 *
 * Rewritten against the current public `xy_hal_i2c.h` contract.  The previous
 * version was deferred from the STM32F4 build because it did not compile at
 * all: it referenced an `xy_hal_i2c_handle_t` type that the public HAL never
 * declares, and returned `XY_HAL_I2C_ADDRESSINGMODE_*` symbols instead of the
 * vendor `I2C_ADDRESSINGMODE_*` ones.  Beyond the compile break it declared
 * every entry point `int` instead of `xy_hal_error_t`, shifted `dev_addr`
 * without any range check (so a caller passing an 8-bit address silently
 * addressed a different device), truncated `reg_addr` to 8 bits with no
 * validation, recorded nothing in `register_callback`, and never implemented
 * `xy_hal_i2c_error` / `xy_hal_i2c_set_error_cb`.
 *
 * F4 is the family that still configures the peripheral with an explicit
 * `ClockSpeed`/`DutyCycle` pair (L4+ uses a precomputed `Timing` value), so
 * those two fields are programmed here from the public configuration.
 */

#include "../../inc/xy_hal_i2c.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include "stm32f4xx_hal_i2c_ex.h"
#include <string.h>

#define XY_HAL_I2C_MAX_INSTANCES 4U
#define XY_HAL_I2C_ADDR_7BIT_MAX 0x7FU

typedef struct {
    I2C_HandleTypeDef *handle;
    xy_hal_i2c_callback_t callback;
    void *arg;
    uint8_t initialized;
} xy_hal_i2c_context_t;

static xy_hal_i2c_context_t i2c_contexts[XY_HAL_I2C_MAX_INSTANCES];

static xy_hal_i2c_context_t *i2c_context_find(const void *i2c)
{
    uint32_t index;

    if (i2c == NULL) return NULL;

    for (index = 0U; index < XY_HAL_I2C_MAX_INSTANCES; ++index) {
        if (i2c_contexts[index].initialized != 0U && i2c_contexts[index].handle == i2c) {
            return &i2c_contexts[index];
        }
    }
    return NULL;
}

static xy_hal_i2c_context_t *i2c_context_allocate(void)
{
    uint32_t index;

    for (index = 0U; index < XY_HAL_I2C_MAX_INSTANCES; ++index) {
        if (i2c_contexts[index].initialized == 0U) {
            return &i2c_contexts[index];
        }
    }
    return NULL;
}

static xy_hal_error_t i2c_map_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) return XY_HAL_OK;
    if (status == HAL_TIMEOUT) return XY_HAL_ERROR_TIMEOUT;
    if (status == HAL_BUSY) return XY_HAL_ERROR_BUSY;
    return XY_HAL_ERROR_IO;
}

/* A 7-bit address is shifted left to build the vendor address word; accepting
 * an out-of-range value would silently address a different slave. */
static xy_hal_error_t i2c_validate_transfer(void *i2c, uint16_t dev_addr, const void *data,
                                            size_t len)
{
    if (i2c == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (dev_addr > XY_HAL_I2C_ADDR_7BIT_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (i2c_context_find(i2c) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return XY_HAL_OK;
}

static xy_hal_error_t i2c_config_check(const xy_hal_i2c_config_t *config)
{
    if (config->clock_speed == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->addr_mode > XY_HAL_I2C_ADDR_10BIT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->duty_cycle > XY_HAL_I2C_DUTY_16_9) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->general_call_mode > 1U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)i2c;
    xy_hal_i2c_context_t *context;
    xy_hal_error_t status;

    if (handle == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = i2c_config_check(config);
    if (status != XY_HAL_OK) {
        return status;
    }
    if (i2c_context_find(i2c) != NULL) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    context = i2c_context_allocate();
    if (context == NULL) {
        return XY_HAL_ERROR_NO_RESOURCE;
    }

    handle->Init.ClockSpeed = config->clock_speed;
    handle->Init.DutyCycle = (config->duty_cycle == XY_HAL_I2C_DUTY_2)
                                 ? I2C_DUTYCYCLE_2
                                 : I2C_DUTYCYCLE_16_9;
    handle->Init.OwnAddress1 = config->own_address;
    handle->Init.AddressingMode = (config->addr_mode == XY_HAL_I2C_ADDR_10BIT)
                                      ? I2C_ADDRESSINGMODE_10BIT
                                      : I2C_ADDRESSINGMODE_7BIT;
    handle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    handle->Init.OwnAddress2 = 0U;
    handle->Init.GeneralCallMode = (config->general_call_mode != 0U)
                                       ? I2C_GENERALCALL_ENABLE
                                       : I2C_GENERALCALL_DISABLE;
    handle->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(handle) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
#if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
    /* Only the F4 parts that expose the I2C FLTR register can configure the
     * analog/digital filters; on the others (for example STM32F407) the vendor
     * extension does not exist at all and there is nothing to program. */
    if (HAL_I2CEx_ConfigAnalogFilter(handle, I2C_ANALOGFILTER_ENABLE) != HAL_OK ||
        HAL_I2CEx_ConfigDigitalFilter(handle, 0U) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
#endif

    memset(context, 0, sizeof(*context));
    context->handle = handle;
    context->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_deinit(void *i2c)
{
    xy_hal_i2c_context_t *context;

    if (i2c == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = i2c_context_find(i2c);
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_I2C_DeInit((I2C_HandleTypeDef *)i2c) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(context, 0, sizeof(*context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr, const uint8_t *data,
                                          size_t len, uint32_t timeout)
{
    xy_hal_error_t status = i2c_validate_transfer(i2c, dev_addr, data, len);

    if (status != XY_HAL_OK) return status;
    return i2c_map_status(HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)i2c,
                                                  (uint16_t)(dev_addr << 1), (uint8_t *)data,
                                                  (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr, uint8_t *data,
                                         size_t len, uint32_t timeout)
{
    xy_hal_error_t status = i2c_validate_transfer(i2c, dev_addr, data, len);

    if (status != XY_HAL_OK) return status;
    return i2c_map_status(HAL_I2C_Master_Receive((I2C_HandleTypeDef *)i2c,
                                                 (uint16_t)(dev_addr << 1), data, (uint16_t)len,
                                                 timeout));
}

/* The public API exposes one 16-bit register address with no size selector, so
 * the encoding must be chosen from the value itself; truncating a >0xFF address
 * to eight bits addressed the wrong register. */
static uint16_t i2c_mem_address_size(uint16_t reg_addr)
{
    return (reg_addr > 0xFFU) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;
}

xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                    const uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_error_t status = i2c_validate_transfer(i2c, dev_addr, data, len);

    if (status != XY_HAL_OK) return status;
    return i2c_map_status(HAL_I2C_Mem_Write((I2C_HandleTypeDef *)i2c,
                                            (uint16_t)(dev_addr << 1), reg_addr,
                                            i2c_mem_address_size(reg_addr), (uint8_t *)data,
                                            (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr, uint16_t reg_addr, uint8_t *data,
                                   size_t len, uint32_t timeout)
{
    xy_hal_error_t status = i2c_validate_transfer(i2c, dev_addr, data, len);

    if (status != XY_HAL_OK) return status;
    return i2c_map_status(HAL_I2C_Mem_Read((I2C_HandleTypeDef *)i2c,
                                           (uint16_t)(dev_addr << 1), reg_addr,
                                           i2c_mem_address_size(reg_addr), data, (uint16_t)len,
                                           timeout));
}

xy_hal_error_t xy_hal_i2c_master_transmit_dma(void *i2c, uint16_t dev_addr, const uint8_t *data,
                                              size_t len)
{
    xy_hal_error_t status = i2c_validate_transfer(i2c, dev_addr, data, len);

    if (status != XY_HAL_OK) return status;
    return i2c_map_status(HAL_I2C_Master_Transmit_DMA((I2C_HandleTypeDef *)i2c,
                                                      (uint16_t)(dev_addr << 1),
                                                      (uint8_t *)data, (uint16_t)len));
}

xy_hal_error_t xy_hal_i2c_master_receive_dma(void *i2c, uint16_t dev_addr, uint8_t *data,
                                             size_t len)
{
    xy_hal_error_t status = i2c_validate_transfer(i2c, dev_addr, data, len);

    if (status != XY_HAL_OK) return status;
    return i2c_map_status(HAL_I2C_Master_Receive_DMA((I2C_HandleTypeDef *)i2c,
                                                     (uint16_t)(dev_addr << 1), data,
                                                     (uint16_t)len));
}

xy_hal_error_t xy_hal_i2c_register_callback(void *i2c, xy_hal_i2c_callback_t callback, void *arg)
{
    xy_hal_i2c_context_t *context;

    if (i2c == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = i2c_context_find(i2c);
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    context->callback = callback;
    context->arg = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_is_device_ready(void *i2c, uint16_t dev_addr, uint32_t trials,
                                          uint32_t timeout)
{
    if (i2c == NULL || dev_addr > XY_HAL_I2C_ADDR_7BIT_MAX || trials == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (i2c_context_find(i2c) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return i2c_map_status(HAL_I2C_IsDeviceReady((I2C_HandleTypeDef *)i2c,
                                                (uint16_t)(dev_addr << 1), trials, timeout));
}

xy_hal_error_t xy_hal_i2c_error(void *i2c)
{
    if (i2c == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return (i2c_context_find(i2c) != NULL) ? XY_HAL_OK : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_i2c_set_error_cb(void *i2c, xy_hal_i2c_callback_t callback, void *arg)
{
    return xy_hal_i2c_register_callback(i2c, callback, arg);
}

static void i2c_dispatch_event(I2C_HandleTypeDef *handle, xy_hal_i2c_event_t event)
{
    xy_hal_i2c_context_t *context = i2c_context_find(handle);

    if (context != NULL && context->callback != NULL) {
        context->callback(handle, event, context->arg);
    }
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *handle)
{
    i2c_dispatch_event(handle, XY_HAL_I2C_EVENT_TX_DONE);
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *handle)
{
    i2c_dispatch_event(handle, XY_HAL_I2C_EVENT_RX_DONE);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *handle)
{
    i2c_dispatch_event(handle, XY_HAL_I2C_EVENT_ERROR);
}

#endif /* STM32_HAL_ENABLED */
