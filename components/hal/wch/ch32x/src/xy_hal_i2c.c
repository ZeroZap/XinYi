/**
 * @file xy_hal_i2c.c
 * @brief WCH CH32V30x I2C HAL implementation
 */

#include "xy_hal_i2c.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include "xy_hal_tick_internal.h"

static xy_hal_i2c_callback_t i2c_callbacks[2];
static void* i2c_callback_args[2];
static uint8_t i2c_initialized[2];

static uint32_t i2c_tick_ms(void) {
    return xy_hal_tick_ms_internal();
}

static int i2c_index(void* instance) {
    if (instance == I2C1)
        return 0;
    if (instance == I2C2)
        return 1;
    return -1;
}

static void i2c_enable_clock(void* instance) {
    RCC_APB1PeriphClockCmd(instance == I2C1 ? RCC_APB1Periph_I2C1 : RCC_APB1Periph_I2C2, ENABLE);
}

static void i2c_abort(void* instance) {
    I2C_GenerateSTOP(instance, ENABLE);
    I2C_AcknowledgeConfig(instance, ENABLE);
    I2C_ClearFlag(instance, I2C_FLAG_AF | I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR);
}

static xy_hal_error_t i2c_wait_idle(void* instance, uint32_t timeout) {
    uint32_t start = i2c_tick_ms();
    while (I2C_GetFlagStatus(instance, I2C_FLAG_BUSY) == SET) {
        if (timeout != 0U && (i2c_tick_ms() - start) >= timeout)
            return XY_HAL_ERROR_TIMEOUT;
    }
    return XY_HAL_OK;
}

static xy_hal_error_t i2c_wait_event(void* instance, uint32_t event, uint32_t timeout) {
    uint32_t start = i2c_tick_ms();
    while (!I2C_CheckEvent(instance, event)) {
        if (I2C_GetFlagStatus(instance, I2C_FLAG_AF) == SET) {
            i2c_abort(instance);
            return XY_HAL_ERROR_NOT_FOUND;
        }
        if (I2C_GetFlagStatus(instance, I2C_FLAG_BERR) == SET ||
            I2C_GetFlagStatus(instance, I2C_FLAG_ARLO) == SET ||
            I2C_GetFlagStatus(instance, I2C_FLAG_OVR) == SET) {
            i2c_abort(instance);
            return XY_HAL_ERROR_IO;
        }
        if (timeout != 0U && (i2c_tick_ms() - start) >= timeout) {
            i2c_abort(instance);
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    return XY_HAL_OK;
}

static int i2c_address_valid(uint16_t address) {
    return address <= 0x7FU;
}

static uint8_t i2c_sdk_address(uint16_t address) {
    return (uint8_t)(address << 1U);
}

xy_hal_error_t xy_hal_i2c_init(void* instance, const xy_hal_i2c_config_t* config) {
    I2C_InitTypeDef init = {0};
    int index;
    if (i2c_index(instance) < 0 || config == NULL || config->clock_speed == 0U ||
        config->clock_speed > 400000U || config->addr_mode > XY_HAL_I2C_ADDR_10BIT ||
        config->duty_cycle > XY_HAL_I2C_DUTY_16_9 || config->general_call_mode > 1U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->addr_mode == XY_HAL_I2C_ADDR_10BIT)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    index = i2c_index(instance);
    if (i2c_initialized[index] != 0U)
        return XY_HAL_ERROR_ALREADY_INIT;
    i2c_enable_clock(instance);
    init.I2C_ClockSpeed = config->clock_speed;
    init.I2C_Mode = I2C_Mode_I2C;
    init.I2C_DutyCycle =
        config->duty_cycle == XY_HAL_I2C_DUTY_16_9 ? I2C_DutyCycle_16_9 : I2C_DutyCycle_2;
    init.I2C_OwnAddress1 = config->own_address;
    init.I2C_Ack = I2C_Ack_Enable;
    init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(instance, &init);
    I2C_GeneralCallCmd(instance, config->general_call_mode ? ENABLE : DISABLE);
    I2C_Cmd(instance, ENABLE);
    i2c_initialized[index] = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_deinit(void* instance) {
    int index = i2c_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    I2C_Cmd(instance, DISABLE);
    I2C_DeInit(instance);
    i2c_initialized[index] = 0U;
    i2c_callbacks[index] = NULL;
    i2c_callback_args[index] = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void* instance, uint16_t address, const uint8_t* data,
                                          size_t len, uint32_t timeout) {
    size_t i;
    xy_hal_error_t result;
    int index = i2c_index(instance);
    if (index < 0 || !i2c_address_valid(address) || len == 0U || data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    result = i2c_wait_idle(instance, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_AcknowledgeConfig(instance, ENABLE);
    I2C_GenerateSTART(instance, ENABLE);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_Send7bitAddress(instance, i2c_sdk_address(address), I2C_Direction_Transmitter);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout);
    if (result != XY_HAL_OK)
        return result;
    for (i = 0U; i < len; ++i) {
        I2C_SendData(instance, data[i]);
        result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
        if (result != XY_HAL_OK)
            return result;
    }
    I2C_GenerateSTOP(instance, ENABLE);
    return i2c_wait_idle(instance, timeout);
}

xy_hal_error_t xy_hal_i2c_master_receive(void* instance, uint16_t address, uint8_t* data,
                                         size_t len, uint32_t timeout) {
    size_t i;
    xy_hal_error_t result;
    int index = i2c_index(instance);
    if (index < 0 || !i2c_address_valid(address) || len == 0U || data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    result = i2c_wait_idle(instance, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_AcknowledgeConfig(instance, ENABLE);
    I2C_GenerateSTART(instance, ENABLE);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_Send7bitAddress(instance, i2c_sdk_address(address), I2C_Direction_Receiver);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, timeout);
    if (result != XY_HAL_OK)
        return result;
    for (i = 0U; i < len; ++i) {
        if (i + 1U == len) {
            I2C_AcknowledgeConfig(instance, DISABLE);
        }
        result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_RECEIVED, timeout);
        if (result != XY_HAL_OK)
            return result;
        data[i] = I2C_ReceiveData(instance);
    }
    I2C_GenerateSTOP(instance, ENABLE);
    I2C_AcknowledgeConfig(instance, ENABLE);
    return i2c_wait_idle(instance, timeout);
}

xy_hal_error_t xy_hal_i2c_mem_write(void* instance, uint16_t address, uint16_t reg,
                                    const uint8_t* data, size_t len, uint32_t timeout) {
    size_t i;
    xy_hal_error_t result;
    int index = i2c_index(instance);
    if (index < 0 || !i2c_address_valid(address) || len == 0U || data == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    result = i2c_wait_idle(instance, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_AcknowledgeConfig(instance, ENABLE);
    I2C_GenerateSTART(instance, ENABLE);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_Send7bitAddress(instance, i2c_sdk_address(address), I2C_Direction_Transmitter);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout);
    if (result != XY_HAL_OK)
        return result;
    if (reg > 0xFFU) {
        /* The same encoding STM32's I2C_MEMADD_SIZE_16BIT produces: high byte first.
         * Refusing the address (or truncating it) made one public parameter mean three
         * different things across the backends. */
        I2C_SendData(instance, (uint8_t)(reg >> 8));
        result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
        if (result != XY_HAL_OK)
            return result;
    }
    I2C_SendData(instance, (uint8_t)(reg & 0xFFU));
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
    if (result != XY_HAL_OK)
        return result;
    for (i = 0U; i < len; ++i) {
        I2C_SendData(instance, data[i]);
        result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
        if (result != XY_HAL_OK)
            return result;
    }
    I2C_GenerateSTOP(instance, ENABLE);
    return i2c_wait_idle(instance, timeout);
}

xy_hal_error_t xy_hal_i2c_mem_read(void* instance, uint16_t address, uint16_t reg, uint8_t* data,
                                   size_t len, uint32_t timeout) {
    size_t i;
    xy_hal_error_t result;
    int index = i2c_index(instance);
    if (index < 0 || !i2c_address_valid(address) || len == 0U || data == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    result = i2c_wait_idle(instance, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_AcknowledgeConfig(instance, ENABLE);
    I2C_GenerateSTART(instance, ENABLE);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_Send7bitAddress(instance, i2c_sdk_address(address), I2C_Direction_Transmitter);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout);
    if (result != XY_HAL_OK)
        return result;
    if (reg > 0xFFU) {
        /* The same encoding STM32's I2C_MEMADD_SIZE_16BIT produces: high byte first.
         * Refusing the address (or truncating it) made one public parameter mean three
         * different things across the backends. */
        I2C_SendData(instance, (uint8_t)(reg >> 8));
        result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
        if (result != XY_HAL_OK)
            return result;
    }
    I2C_SendData(instance, (uint8_t)(reg & 0xFFU));
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_GenerateSTART(instance, ENABLE);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (result != XY_HAL_OK)
        return result;
    I2C_Send7bitAddress(instance, i2c_sdk_address(address), I2C_Direction_Receiver);
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, timeout);
    if (result != XY_HAL_OK)
        return result;
    for (i = 0U; i < len; ++i) {
        if (i + 1U == len) {
            I2C_AcknowledgeConfig(instance, DISABLE);
        }
        result = i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_RECEIVED, timeout);
        if (result != XY_HAL_OK)
            return result;
        data[i] = I2C_ReceiveData(instance);
    }
    I2C_GenerateSTOP(instance, ENABLE);
    I2C_AcknowledgeConfig(instance, ENABLE);
    return i2c_wait_idle(instance, timeout);
}

xy_hal_error_t xy_hal_i2c_master_transmit_dma(void* instance, uint16_t address, const uint8_t* data,
                                              size_t len) {
    int index = i2c_index(instance);
    if (index < 0 || !i2c_address_valid(address) || data == NULL || len == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2c_master_receive_dma(void* instance, uint16_t address, uint8_t* data,
                                             size_t len) {
    int index = i2c_index(instance);
    if (index < 0 || !i2c_address_valid(address) || data == NULL || len == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2c_register_callback(void* instance, xy_hal_i2c_callback_t callback,
                                            void* arg) {
    int index = i2c_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    i2c_callbacks[index] = callback;
    i2c_callback_args[index] = arg;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_i2c_is_device_ready(void* instance, uint16_t address, uint32_t trials,
                                          uint32_t timeout) {
    uint32_t trial;
    uint32_t start;
    xy_hal_error_t result = XY_HAL_ERROR_NOT_FOUND;
    xy_hal_error_t cleanup_result;
    int index = i2c_index(instance);
    if (index < 0 || !i2c_address_valid(address) || trials == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    for (trial = 0U; trial < trials; ++trial) {
        result = i2c_wait_idle(instance, timeout);
        if (result != XY_HAL_OK)
            return result;
        I2C_AcknowledgeConfig(instance, ENABLE);
        I2C_GenerateSTART(instance, ENABLE);
        result = i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT, timeout);
        if (result != XY_HAL_OK)
            continue;
        I2C_Send7bitAddress(instance, i2c_sdk_address(address), I2C_Direction_Transmitter);
        start = i2c_tick_ms();
        for (;;) {
            if (I2C_GetFlagStatus(instance, I2C_FLAG_AF) == SET) {
                i2c_abort(instance);
                result = XY_HAL_ERROR_NOT_FOUND;
                break;
            }
            if (I2C_GetFlagStatus(instance, I2C_FLAG_BERR) == SET ||
                I2C_GetFlagStatus(instance, I2C_FLAG_ARLO) == SET ||
                I2C_GetFlagStatus(instance, I2C_FLAG_OVR) == SET) {
                i2c_abort(instance);
                result = XY_HAL_ERROR_IO;
                break;
            }
            if (I2C_GetFlagStatus(instance, I2C_FLAG_ADDR) == SET) {
                (void)((I2C_TypeDef*)instance)->STAR2;
                result = XY_HAL_OK;
                break;
            }
            if (timeout != 0U && (i2c_tick_ms() - start) >= timeout) {
                i2c_abort(instance);
                result = XY_HAL_ERROR_TIMEOUT;
                break;
            }
        }
        if (result == XY_HAL_OK) {
            I2C_GenerateSTOP(instance, ENABLE);
            return i2c_wait_idle(instance, timeout);
        }
        cleanup_result = i2c_wait_idle(instance, timeout);
        if (cleanup_result != XY_HAL_OK)
            return cleanup_result;
    }
    return result;
}
xy_hal_error_t xy_hal_i2c_error(void* instance) {
    int index = i2c_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (i2c_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    if (i2c_callbacks[index] != NULL) {
        i2c_callbacks[index](instance, XY_HAL_I2C_EVENT_ERROR, i2c_callback_args[index]);
    }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_i2c_set_error_cb(void* instance, xy_hal_i2c_callback_t callback, void* arg) {
    return xy_hal_i2c_register_callback(instance, callback, arg);
}

#else
#error "WCH I2C backend requires MCU_CH32"
#endif
