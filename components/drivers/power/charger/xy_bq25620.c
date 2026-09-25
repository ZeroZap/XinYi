/**
 * @file xy_bq25620.c
 * @brief TI BQ25620 Standalone I2C 1-Cell Li-Ion Battery Charger Driver Implementation
 * @version 1.0.0
 * @date 2026-03-17
 * 
 * @note BQ25620 驱动实现 - 基于 I2C 接口
 */

#include "xy_bq25620.h"
#include "xy_charger_device.h"
#include "xy_hal_i2c.h"
#include <string.h>

#define BQ25620_OWNER_COOKIE 0x42513230UL

/* ==================== Private Functions ==================== */

static bool bq25620_transport_ready(const xy_bq25620_t *dev)
{
    return dev != NULL && dev->i2c_handle != NULL;
}

static bool bq25620_ready(const xy_bq25620_t *dev)
{
    return bq25620_transport_ready(dev) && dev->initialized &&
           dev->base.base.initialized != 0U && dev->owner_cookie == BQ25620_OWNER_COOKIE;
}

static bool bq25620_register_valid(uint8_t reg)
{
    return reg <= BQ25620_REG_DEVICE_ID;
}

static int bq25620_from_hal(xy_hal_error_t error)
{
    switch (error) {
        case XY_HAL_OK: return XY_DEVICE_OK;
        case XY_HAL_ERROR_INVALID_PARAM: return XY_DEVICE_INVALID_PARAM;
        case XY_HAL_ERROR_TIMEOUT: return XY_DEVICE_TIMEOUT;
        case XY_HAL_ERROR_BUSY: return XY_DEVICE_BUSY;
        case XY_HAL_ERROR_NOT_SUPPORTED: return XY_DEVICE_NOT_SUPPORT;
        case XY_HAL_ERROR_NOT_FOUND: return XY_DEVICE_NOT_FOUND;
        case XY_HAL_ERROR_NO_MEMORY: return XY_DEVICE_NO_MEM;
        case XY_HAL_ERROR_NO_RESOURCE: return XY_DEVICE_NO_RESOURCE;
        case XY_HAL_ERROR_IO: return XY_DEVICE_IO_ERROR;
        case XY_HAL_ERROR_NOT_INIT: return XY_DEVICE_NOT_INIT;
        case XY_HAL_ERROR_ALREADY_INIT: return XY_DEVICE_ALREADY_INIT;
        default: return XY_DEVICE_ERROR;
    }
}

/**
 * @brief I2C 读取寄存器
 */
static int bq25620_i2c_read(xy_bq25620_t *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
    uint8_t next;

    if (!bq25620_transport_ready(dev) || !data || len != 1U) {
        return XY_DEVICE_INVALID_PARAM;
    }

    void *i2c = dev->i2c_handle;

    /* 写入寄存器地址 */
    xy_hal_error_t ret = xy_hal_i2c_master_transmit(i2c, dev->i2c_addr, &reg, 1, 100);
    if (ret != XY_HAL_OK) {
        return bq25620_from_hal(ret);
    }

    /* 先读入局部变量；失败时不发布部分或污染数据。 */
    ret = xy_hal_i2c_master_receive(i2c, dev->i2c_addr, &next, 1U, 100);
    if (ret != XY_HAL_OK) {
        return bq25620_from_hal(ret);
    }

    *data = next;
    return XY_DEVICE_OK;
}

/**
 * @brief I2C 写入寄存器
 */
static int bq25620_i2c_write(xy_bq25620_t *dev, uint8_t reg, uint8_t data)
{
    if (!bq25620_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    void *i2c = dev->i2c_handle;
    uint8_t tx_buf[2] = {reg, data};
    
    return bq25620_from_hal(
        xy_hal_i2c_master_transmit(i2c, dev->i2c_addr, tx_buf, 2, 100));
}

/**
 * @brief 更新寄存器位
 */
static int bq25620_i2c_update_bits(xy_bq25620_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t reg_value;
    int ret = bq25620_i2c_read(dev, reg, &reg_value, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    reg_value = (reg_value & ~mask) | (value & mask);
    
    return bq25620_i2c_write(dev, reg, reg_value);
}

/**
 * @brief 电流转寄存器值
 */
static uint8_t current_to_reg(uint32_t current_mA, uint8_t step, uint8_t min_mA)
{
    if (current_mA < min_mA) {
        return 0;
    }
    
    uint8_t reg_value = (current_mA - min_mA) / step;
    return reg_value;
}

/**
 * @brief 寄存器值转电流
 */
static uint32_t reg_to_current(uint8_t reg_value, uint8_t step, uint8_t min_mA)
{
    return min_mA + ((uint32_t)reg_value * step);
}

/**
 * @brief 电压转寄存器值
 */
static uint8_t voltage_to_reg(uint32_t voltage_mV, uint8_t step, uint16_t min_mV)
{
    if (voltage_mV < min_mV) {
        return 0;
    }
    
    uint8_t reg_value = (voltage_mV - min_mV) / step;
    return reg_value;
}

/**
 * @brief 寄存器值转电压
 */
static uint32_t reg_to_voltage(uint8_t reg_value, uint8_t step, uint16_t min_mV)
{
    return min_mV + ((uint32_t)reg_value * step);
}

static bool bq25620_config_valid(const xy_charger_device_config_t *config)
{
    return config != NULL &&
           config->input_current_limit >= BQ25620_ILIM_MIN_mA &&
           config->input_current_limit <= BQ25620_ILIM_MAX_mA &&
           (config->input_current_limit - BQ25620_ILIM_MIN_mA) % BQ25620_ILIM_STEP_mA == 0U &&
           config->charge_current >= BQ25620_ICHG_MIN_mA &&
           config->charge_current <= BQ25620_ICHG_MAX_mA &&
           (config->charge_current - BQ25620_ICHG_MIN_mA) % BQ25620_ICHG_STEP_mA == 0U &&
           config->charge_voltage >= BQ25620_VREG_MIN_mV &&
           config->charge_voltage <= BQ25620_VREG_MAX_mV &&
           (config->charge_voltage - BQ25620_VREG_MIN_mV) % BQ25620_VREG_STEP_mV == 0U &&
           config->precharge_current >= 64U && config->precharge_current <= 960U &&
           (config->precharge_current - 64U) % 64U == 0U &&
           config->termination_current >= 64U && config->termination_current <= 960U &&
           (config->termination_current - 64U) % 64U == 0U &&
           config->recharge_threshold >= 100U && config->recharge_threshold <= 300U &&
           config->recharge_threshold % 100U == 0U;
}

/* ==================== Hardware Operations ==================== */

static int bq25620_hw_init(void *hw_data)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 读取设备 ID 验证 */
    uint8_t dev_id;
    int ret = bq25620_i2c_read(dev, BQ25620_REG_DEVICE_ID, &dev_id, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 验证型号 */
    if ((dev_id & BQ25620_PART_NUMBER_MASK) != BQ25620_PART_NUMBER) {
        return XY_DEVICE_NOT_SUPPORT;
    }
    
    dev->initialized = true;
    dev->base.base.initialized = 1U;
    dev->owner_cookie = BQ25620_OWNER_COOKIE;
    return XY_DEVICE_OK;
}

static int bq25620_hw_read_status(void *hw_data, xy_charger_device_status_t *status)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    xy_charger_device_status_t next = {0};
    uint8_t stat0;
    uint8_t stat1;
    uint8_t reg_value;
    int ret;

    if (!bq25620_ready(dev) || !status) {
        return XY_DEVICE_INVALID_PARAM;
    }

    ret = bq25620_i2c_read(dev, BQ25620_REG_CHG_STAT_0, &stat0, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = bq25620_i2c_read(dev, BQ25620_REG_CHG_STAT_1, &stat1, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = bq25620_i2c_read(dev, BQ25620_REG_CHG_CTRL_1, &reg_value, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next.configured_charge_current = reg_to_current(reg_value & BQ25620_ICHG_MASK,
                                                    BQ25620_ICHG_STEP_mA,
                                                    BQ25620_ICHG_MIN_mA);
    ret = bq25620_i2c_read(dev, BQ25620_REG_CHG_CTRL_3, &reg_value, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next.configured_charge_voltage = reg_to_voltage(reg_value & BQ25620_VREG_MASK,
                                                    BQ25620_VREG_STEP_mV,
                                                    BQ25620_VREG_MIN_mV);
    ret = bq25620_i2c_read(dev, BQ25620_REG_CHG_CTRL_4, &reg_value, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next.configured_input_current_limit =
        reg_to_current(reg_value & BQ25620_ILIM_MASK,
                       BQ25620_ILIM_STEP_mA, BQ25620_ILIM_MIN_mA);

    switch (stat0 & BQ25620_STAT_CHG_MASK) {
        case BQ25620_STAT_CHG_IDLE: next.state = XY_CHARGER_DEVICE_STATE_IDLE; break;
        case BQ25620_STAT_CHG_PRECHG: next.state = XY_CHARGER_DEVICE_STATE_PRE_CHARGE; break;
        case BQ25620_STAT_CHG_FAST: next.state = XY_CHARGER_DEVICE_STATE_FAST_CHARGE; break;
        case BQ25620_STAT_CHG_DONE: next.state = XY_CHARGER_DEVICE_STATE_CHARGE_DONE; break;
        default: next.state = XY_CHARGER_DEVICE_STATE_FAULT; break;
    }

    switch (stat1 & BQ25620_FAULT_MASK) {
        case BQ25620_FAULT_NORMAL: next.fault = XY_CHARGER_DEVICE_FAULT_NONE; break;
        case BQ25620_FAULT_INPUT_OVP: next.fault = XY_CHARGER_DEVICE_FAULT_INPUT_OVP; break;
        case BQ25620_FAULT_THERMAL: next.fault = XY_CHARGER_DEVICE_FAULT_THERMAL; break;
        case BQ25620_FAULT_CHG_TIMEOUT: next.fault = XY_CHARGER_DEVICE_FAULT_CHARGE_TIMEOUT; break;
        case BQ25620_FAULT_BAT_OVP: next.fault = XY_CHARGER_DEVICE_FAULT_BAT_OVP; break;
        default: next.fault = XY_CHARGER_DEVICE_FAULT_UNKNOWN; break;
    }

    if (next.fault != XY_CHARGER_DEVICE_FAULT_NONE) {
        next.state = XY_CHARGER_DEVICE_STATE_FAULT;
    }

    next.power_good = (stat0 & BQ25620_STAT_PG) != 0U;
    next.charging = next.state == XY_CHARGER_DEVICE_STATE_PRE_CHARGE ||
                    next.state == XY_CHARGER_DEVICE_STATE_FAST_CHARGE ||
                    next.state == XY_CHARGER_DEVICE_STATE_CONSTANT_VOLT;
    next.done = next.state == XY_CHARGER_DEVICE_STATE_CHARGE_DONE;
    *status = next;
    return XY_DEVICE_OK;
}

static int bq25620_hw_set_config(void *hw_data, const xy_charger_device_config_t *config)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    uint8_t reg_value;
    int ret;

    if (!bq25620_ready(dev) || !bq25620_config_valid(config)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    reg_value = current_to_reg(config->charge_current, BQ25620_ICHG_STEP_mA,
                               BQ25620_ICHG_MIN_mA) & BQ25620_ICHG_MASK;
    ret = bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_1,
                                  BQ25620_ICHG_MASK, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = voltage_to_reg(config->charge_voltage, BQ25620_VREG_STEP_mV,
                               BQ25620_VREG_MIN_mV) & BQ25620_VREG_MASK;
    ret = bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_3,
                                  BQ25620_VREG_MASK, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = (current_to_reg(config->input_current_limit, BQ25620_ILIM_STEP_mA,
                                BQ25620_ILIM_MIN_mA) & BQ25620_ILIM_MASK) |
                BQ25620_EN_ILIM;
    ret = bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_4,
                                  BQ25620_ILIM_MASK | BQ25620_EN_ILIM, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = ((current_to_reg(config->termination_current, 64, 64) & 0x0FU) << 4) |
                (current_to_reg(config->precharge_current, 64, 64) & 0x0FU);
    ret = bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_2, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = config->auto_recharge ? BQ25620_AUTO_RECHG : 0U;
    reg_value |= ((config->recharge_threshold / 100U) & 0x03U) << 6;
    return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_5,
                                   BQ25620_VRECHG_MASK | BQ25620_AUTO_RECHG, reg_value);
}

static int bq25620_hw_enable(void *hw_data, bool enable)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!bq25620_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (enable) {
        return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_0,
                                        BQ25620_EN_CHG, BQ25620_EN_CHG);
    } else {
        return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_0,
                                        BQ25620_EN_CHG, 0);
    }
}

static int bq25620_hw_read_reg(void *hw_data, uint8_t reg, uint8_t *value)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!bq25620_ready(dev) || !value || !bq25620_register_valid(reg)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_read(dev, reg, value, 1);
}

static int bq25620_hw_write_reg(void *hw_data, uint8_t reg, uint8_t value)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!bq25620_ready(dev) || !bq25620_register_valid(reg)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_write(dev, reg, value);
}

/* ==================== Public API Implementation ==================== */

int xy_bq25620_init(xy_bq25620_t *dev, void *i2c_handle, uint8_t i2c_addr)
{
    xy_bq25620_t next = {0};
    int ret;

    if (!dev || !i2c_handle || i2c_addr != 0x6AU) {
        return XY_DEVICE_INVALID_PARAM;
    }

    next.i2c_handle = i2c_handle;
    next.i2c_addr = i2c_addr;
    
    /* 设置硬件操作接口 */
    next.base.hw_init = bq25620_hw_init;
    next.base.hw_read_status = bq25620_hw_read_status;
    next.base.hw_set_config = bq25620_hw_set_config;
    next.base.hw_enable = bq25620_hw_enable;
    next.base.hw_read_reg = bq25620_hw_read_reg;
    next.base.hw_write_reg = bq25620_hw_write_reg;
    next.base.hw_data = &next;
    
    /* 初始化硬件 */
    ret = bq25620_hw_init(&next);
    if (ret == XY_DEVICE_OK) {
        *dev = next;
        dev->base.hw_data = dev;
    } else if (dev->owner_cookie != BQ25620_OWNER_COOKIE) {
        memset(dev, 0, sizeof(*dev));
    }
    return ret;
}

int xy_bq25620_deinit(xy_bq25620_t *dev)
{
    if (!bq25620_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    int ret = xy_bq25620_stop_charge(dev);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    memset(dev, 0, sizeof(*dev));
    return XY_DEVICE_OK;
}

int xy_bq25620_read_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t *value)
{
    if (!bq25620_ready(dev) || !value || !bq25620_register_valid(reg)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_read(dev, reg, value, 1);
}

int xy_bq25620_write_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t value)
{
    if (!bq25620_ready(dev) || !bq25620_register_valid(reg)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_write(dev, reg, value);
}

int xy_bq25620_get_device_id(xy_bq25620_t *dev, uint8_t *id)
{
    if (!bq25620_ready(dev) || !id) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_read(dev, BQ25620_REG_DEVICE_ID, id, 1);
}

int xy_bq25620_get_status(xy_bq25620_t *dev, xy_charger_device_status_t *status)
{
    if (!bq25620_ready(dev) || !status) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_hw_read_status(dev, status);
}

int xy_bq25620_configure(xy_bq25620_t *dev, const xy_charger_device_config_t *config)
{
    if (!bq25620_ready(dev) || !config) {
        return XY_DEVICE_INVALID_PARAM;
    }

    return bq25620_hw_set_config(dev, config);
}

int xy_bq25620_set_charge_current(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!bq25620_ready(dev) || current_mA < BQ25620_ICHG_MIN_mA ||
        current_mA > BQ25620_ICHG_MAX_mA ||
        (current_mA - BQ25620_ICHG_MIN_mA) % BQ25620_ICHG_STEP_mA != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t ichg_reg = current_to_reg(current_mA, BQ25620_ICHG_STEP_mA, BQ25620_ICHG_MIN_mA);
    return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_1, BQ25620_ICHG_MASK,
                                   ichg_reg);
}

int xy_bq25620_set_charge_voltage(xy_bq25620_t *dev, uint32_t voltage_mV)
{
    if (!bq25620_ready(dev) || voltage_mV < BQ25620_VREG_MIN_mV ||
        voltage_mV > BQ25620_VREG_MAX_mV ||
        (voltage_mV - BQ25620_VREG_MIN_mV) % BQ25620_VREG_STEP_mV != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t vreg_reg = voltage_to_reg(voltage_mV, BQ25620_VREG_STEP_mV, BQ25620_VREG_MIN_mV);
    return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_3, BQ25620_VREG_MASK,
                                   vreg_reg);
}

int xy_bq25620_set_input_limit(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!bq25620_ready(dev) || current_mA < BQ25620_ILIM_MIN_mA ||
        current_mA > BQ25620_ILIM_MAX_mA ||
        (current_mA - BQ25620_ILIM_MIN_mA) % BQ25620_ILIM_STEP_mA != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t ilim_reg = current_to_reg(current_mA, BQ25620_ILIM_STEP_mA, BQ25620_ILIM_MIN_mA);
    uint8_t ilim_value = (ilim_reg & BQ25620_ILIM_MASK) | BQ25620_EN_ILIM;
    return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_4,
                                   BQ25620_ILIM_MASK | BQ25620_EN_ILIM, ilim_value);
}

int xy_bq25620_start_charge(xy_bq25620_t *dev)
{
    if (!bq25620_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_hw_enable(dev, true);
}

int xy_bq25620_stop_charge(xy_bq25620_t *dev)
{
    if (!bq25620_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_hw_enable(dev, false);
}

/* ==================== End of File ==================== */
