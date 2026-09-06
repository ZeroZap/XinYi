/**
 * @file xy_hal_i2c.c
 * @brief I2C HAL STM32L4 implementation
 */

#include "../../inc/xy_hal_i2c.h"

#if defined(STM32L4) || defined(STM32L4xx)

#include "stm32l4xx_hal.h"
#include <string.h>

#define MAX_I2C_INSTANCES 4U

typedef struct {
    I2C_HandleTypeDef *handle;
    xy_hal_i2c_callback_t callback;
    void *arg;
    uint8_t initialized;
} i2c_context_t;

static i2c_context_t i2c_contexts[MAX_I2C_INSTANCES];

static i2c_context_t *find_context(const void *i2c)
{
    for (size_t index = 0U; index < MAX_I2C_INSTANCES; ++index) {
        if (i2c_contexts[index].handle == i2c) {
            return &i2c_contexts[index];
        }
    }
    return NULL;
}

static i2c_context_t *allocate_context(void)
{
    for (size_t index = 0U; index < MAX_I2C_INSTANCES; ++index) {
        if (i2c_contexts[index].handle == NULL) {
            return &i2c_contexts[index];
        }
    }
    return NULL;
}

static xy_hal_error_t validate_transfer(void *i2c, const void *data, size_t len)
{
    i2c_context_t *context;

    if (i2c == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(i2c);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return XY_HAL_OK;
}

static xy_hal_error_t map_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) {
        return XY_HAL_OK;
    }
    if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    }
    if (status == HAL_BUSY) {
        return XY_HAL_ERROR_BUSY;
    }
    return XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    I2C_HandleTypeDef *handle;
    i2c_context_t *context;

    if (i2c == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(i2c);
    if (context != NULL && context->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (context == NULL) {
        context = allocate_context();
        if (context == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    handle = (I2C_HandleTypeDef *)i2c;
    handle->Init.OwnAddress1 = config->own_address;
    handle->Init.AddressingMode = config->addr_mode == XY_HAL_I2C_ADDR_10BIT
                                      ? I2C_ADDRESSINGMODE_10BIT
                                      : I2C_ADDRESSINGMODE_7BIT;
    handle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    handle->Init.OwnAddress2 = 0U;
    handle->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    handle->Init.GeneralCallMode = config->general_call_mode != 0U
                                       ? I2C_GENERALCALL_ENABLE
                                       : I2C_GENERALCALL_DISABLE;
    handle->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(handle) != HAL_OK ||
        HAL_I2CEx_ConfigAnalogFilter(handle, I2C_ANALOGFILTER_ENABLE) != HAL_OK ||
        HAL_I2CEx_ConfigDigitalFilter(handle, 0U) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    context->handle = handle;
    context->callback = NULL;
    context->arg = NULL;
    context->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_deinit(void *i2c)
{
    i2c_context_t *context;

    if (i2c == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(i2c);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_I2C_DeInit((I2C_HandleTypeDef *)i2c) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(context, 0, sizeof(*context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    xy_hal_error_t result = validate_transfer(i2c, data, len);
    if (result != XY_HAL_OK || dev_addr > 0x7FU) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)i2c,
                                              (uint16_t)(dev_addr << 1), (uint8_t *)data,
                                              (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr, uint8_t *data,
                                         size_t len, uint32_t timeout)
{
    xy_hal_error_t result = validate_transfer(i2c, data, len);
    if (result != XY_HAL_OK || dev_addr > 0x7FU) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_I2C_Master_Receive((I2C_HandleTypeDef *)i2c,
                                             (uint16_t)(dev_addr << 1), data,
                                             (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                    const uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_error_t result = validate_transfer(i2c, data, len);
    if (result != XY_HAL_OK || dev_addr > 0x7FU) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_I2C_Mem_Write((I2C_HandleTypeDef *)i2c,
                                        (uint16_t)(dev_addr << 1), reg_addr,
                                        I2C_MEMADD_SIZE_8BIT, (uint8_t *)data,
                                        (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                   uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_error_t result = validate_transfer(i2c, data, len);
    if (result != XY_HAL_OK || dev_addr > 0x7FU) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_I2C_Mem_Read((I2C_HandleTypeDef *)i2c,
                                       (uint16_t)(dev_addr << 1), reg_addr,
                                       I2C_MEMADD_SIZE_8BIT, data, (uint16_t)len,
                                       timeout));
}

xy_hal_error_t xy_hal_i2c_master_transmit_dma(void *i2c, uint16_t dev_addr,
                                              const uint8_t *data, size_t len)
{
    xy_hal_error_t result = validate_transfer(i2c, data, len);
    if (result != XY_HAL_OK || dev_addr > 0x7FU) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_I2C_Master_Transmit_DMA((I2C_HandleTypeDef *)i2c,
                                                  (uint16_t)(dev_addr << 1),
                                                  (uint8_t *)data, (uint16_t)len));
}

xy_hal_error_t xy_hal_i2c_master_receive_dma(void *i2c, uint16_t dev_addr, uint8_t *data,
                                             size_t len)
{
    xy_hal_error_t result = validate_transfer(i2c, data, len);
    if (result != XY_HAL_OK || dev_addr > 0x7FU) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_I2C_Master_Receive_DMA((I2C_HandleTypeDef *)i2c,
                                                 (uint16_t)(dev_addr << 1), data,
                                                 (uint16_t)len));
}

xy_hal_error_t xy_hal_i2c_register_callback(void *i2c, xy_hal_i2c_callback_t callback,
                                            void *arg)
{
    i2c_context_t *context;

    if (i2c == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(i2c);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    context->callback = callback;
    context->arg = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_is_device_ready(void *i2c, uint16_t dev_addr, uint32_t trials,
                                          uint32_t timeout)
{
    i2c_context_t *context;

    if (i2c == NULL || dev_addr > 0x7FU || trials == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(i2c);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return map_status(HAL_I2C_IsDeviceReady((I2C_HandleTypeDef *)i2c,
                                             (uint16_t)(dev_addr << 1), trials, timeout));
}

xy_hal_error_t xy_hal_i2c_error(void *i2c)
{
    if (i2c == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return find_context(i2c) != NULL ? XY_HAL_OK : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_i2c_set_error_cb(void *i2c, xy_hal_i2c_callback_t callback,
                                       void *arg)
{
    return xy_hal_i2c_register_callback(i2c, callback, arg);
}

static void dispatch_event(I2C_HandleTypeDef *handle, xy_hal_i2c_event_t event)
{
    i2c_context_t *context = find_context(handle);
    if (context != NULL && context->callback != NULL) {
        context->callback(handle, event, context->arg);
    }
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *handle)
{
    dispatch_event(handle, XY_HAL_I2C_EVENT_TX_DONE);
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *handle)
{
    dispatch_event(handle, XY_HAL_I2C_EVENT_RX_DONE);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *handle)
{
    dispatch_event(handle, XY_HAL_I2C_EVENT_ERROR);
}

#endif
