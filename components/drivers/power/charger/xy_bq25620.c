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
    return bq25620_transport_ready(dev) && dev->base.base.initialized != 0U &&
           dev->owner_cookie == BQ25620_OWNER_COOKIE;
}

static bool bq25620_register_valid(uint8_t reg)
{
    return reg >= BQ25620_REG_CHG_CTRL_1 && reg <= BQ25620_REG_DEVICE_ID;
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
    uint8_t next[2];

    if (!bq25620_transport_ready(dev) || !data || len == 0U || len > sizeof(next)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    void *i2c = dev->i2c_handle;

    /* 写入寄存器地址 */
    xy_hal_error_t ret = xy_hal_i2c_master_transmit(i2c, dev->i2c_addr, &reg, 1, 100);
    if (ret != XY_HAL_OK) {
        return bq25620_from_hal(ret);
    }

    /* 先读入局部变量；失败时不发布部分或污染数据。 */
    ret = xy_hal_i2c_master_receive(i2c, dev->i2c_addr, next, len, 100);
    if (ret != XY_HAL_OK) {
        return bq25620_from_hal(ret);
    }

    memcpy(data, next, len);
    return XY_DEVICE_OK;
}

/**
 * @brief I2C 写入寄存器
 */
static int bq25620_i2c_write(xy_bq25620_t *dev, uint8_t reg,
                             const uint8_t *data, uint8_t len)
{
    uint8_t tx_buf[3];

    if (!bq25620_transport_ready(dev) || data == NULL || len == 0U || len > 2U) {
        return XY_DEVICE_INVALID_PARAM;
    }

    tx_buf[0] = reg;
    memcpy(&tx_buf[1], data, len);
    return bq25620_from_hal(
        xy_hal_i2c_master_transmit(dev->i2c_handle, dev->i2c_addr, tx_buf, len + 1U, 100));
}

static int bq25620_read_u16(xy_bq25620_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int ret = bq25620_i2c_read(dev, reg, data, sizeof(data));
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    *value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    return XY_DEVICE_OK;
}

static int bq25620_write_u16(xy_bq25620_t *dev, uint8_t reg, uint16_t value)
{
    const uint8_t data[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
    return bq25620_i2c_write(dev, reg, data, sizeof(data));
}

static int bq25620_update_u8(xy_bq25620_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t reg_value;
    int ret = bq25620_i2c_read(dev, reg, &reg_value, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    reg_value = (uint8_t)((reg_value & (uint8_t)~mask) | (value & mask));
    return bq25620_i2c_write(dev, reg, &reg_value, 1U);
}

static int bq25620_update_u16(xy_bq25620_t *dev, uint8_t reg, uint16_t mask, uint16_t value)
{
    uint16_t reg_value;
    int ret = bq25620_read_u16(dev, reg, &reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    reg_value = (uint16_t)((reg_value & (uint16_t)~mask) | (value & mask));
    return bq25620_write_u16(dev, reg, reg_value);
}

/**
 * @brief 电流转寄存器值
 */
static uint16_t current_to_reg(uint32_t current_mA, uint16_t step)
{
    return (uint16_t)(current_mA / step);
}

/**
 * @brief 寄存器值转电流
 */
static uint32_t reg_to_current(uint16_t reg_value, uint16_t step)
{
    return (uint32_t)reg_value * step;
}

/**
 * @brief 电压转寄存器值
 */
static uint16_t voltage_to_reg(uint32_t voltage_mV, uint16_t step)
{
    return (uint16_t)(voltage_mV / step);
}

/**
 * @brief 寄存器值转电压
 */
static uint32_t reg_to_voltage(uint16_t reg_value, uint16_t step)
{
    return (uint32_t)reg_value * step;
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
           config->precharge_current >= 20U && config->precharge_current <= 620U &&
           config->precharge_current % 20U == 0U &&
           config->termination_current >= 10U && config->termination_current <= 620U &&
           config->termination_current % 10U == 0U &&
           (config->recharge_threshold == 100U || config->recharge_threshold == 200U) &&
           config->auto_recharge;
}

/* ==================== Hardware Operations ==================== */

static int bq25620_probe(xy_bq25620_t *dev)
{
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
    
    dev->base.base.initialized = 1U;
    dev->owner_cookie = BQ25620_OWNER_COOKIE;
    return XY_DEVICE_OK;
}

static int bq25620_read_status(xy_bq25620_t *dev, xy_charger_device_status_t *status)
{
    xy_charger_device_status_t next = {0};
    uint8_t stat0;
    uint8_t stat1;
    uint16_t reg_value;
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
    ret = bq25620_read_u16(dev, BQ25620_REG_CHG_CTRL_1, &reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    uint16_t setpoint = (reg_value & BQ25620_ICHG_MASK) >> 6U;
    if (setpoint < BQ25620_ICHG_MIN_mA / BQ25620_ICHG_STEP_mA ||
        setpoint > BQ25620_ICHG_MAX_mA / BQ25620_ICHG_STEP_mA) {
        return XY_DEVICE_ERROR;
    }
    next.configured_charge_current = reg_to_current(setpoint, BQ25620_ICHG_STEP_mA);
    ret = bq25620_read_u16(dev, BQ25620_REG_CHG_CTRL_3, &reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    setpoint = (reg_value & BQ25620_VREG_MASK) >> 3U;
    if (setpoint < BQ25620_VREG_MIN_mV / BQ25620_VREG_STEP_mV ||
        setpoint > BQ25620_VREG_MAX_mV / BQ25620_VREG_STEP_mV) {
        return XY_DEVICE_ERROR;
    }
    next.configured_charge_voltage = reg_to_voltage(setpoint, BQ25620_VREG_STEP_mV);
    ret = bq25620_read_u16(dev, BQ25620_REG_CHG_CTRL_4, &reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    setpoint = (reg_value & BQ25620_ILIM_MASK) >> 4U;
    if (setpoint < BQ25620_ILIM_MIN_mA / BQ25620_ILIM_STEP_mA ||
        setpoint > BQ25620_ILIM_MAX_mA / BQ25620_ILIM_STEP_mA) {
        return XY_DEVICE_ERROR;
    }
    next.configured_input_current_limit = reg_to_current(setpoint, BQ25620_ILIM_STEP_mA);

    switch (stat0 & BQ25620_STAT_CHG_MASK) {
        case BQ25620_STAT_CHG_IDLE: next.state = XY_CHARGER_DEVICE_STATE_IDLE; break;
        case BQ25620_STAT_CHG_FAST: next.state = XY_CHARGER_DEVICE_STATE_FAST_CHARGE; break;
        case BQ25620_STAT_CHG_CV: next.state = XY_CHARGER_DEVICE_STATE_CONSTANT_VOLT; break;
        case BQ25620_STAT_CHG_TOPOFF: next.state = XY_CHARGER_DEVICE_STATE_CHARGE_DONE; break;
    }

    if ((stat1 & BQ25620_FAULT_INPUT_OVP) != 0U) {
        next.fault = XY_CHARGER_DEVICE_FAULT_INPUT_OVP;
    } else if ((stat1 & BQ25620_FAULT_BAT_OVP) != 0U) {
        next.fault = XY_CHARGER_DEVICE_FAULT_BAT_OVP;
    } else if ((stat1 & BQ25620_FAULT_THERMAL) != 0U) {
        next.fault = XY_CHARGER_DEVICE_FAULT_THERMAL;
    } else if ((stat1 & BQ25620_FAULT_TS_MASK) == 1U) {
        next.fault = XY_CHARGER_DEVICE_FAULT_COLD;
    } else if ((stat1 & BQ25620_FAULT_TS_MASK) == 2U) {
        next.fault = XY_CHARGER_DEVICE_FAULT_HOT;
    } else if ((stat1 & (BQ25620_FAULT_SYS | BQ25620_FAULT_OTG)) != 0U ||
               (stat1 & BQ25620_FAULT_TS_MASK) == 7U) {
        next.fault = XY_CHARGER_DEVICE_FAULT_UNKNOWN;
    } else {
        next.fault = XY_CHARGER_DEVICE_FAULT_NONE;
    }

    if (next.fault != XY_CHARGER_DEVICE_FAULT_NONE) {
        next.state = XY_CHARGER_DEVICE_STATE_FAULT;
    }

    next.power_good = (stat0 & BQ25620_STAT_VBUS_MASK) != 0U &&
                      (stat1 & BQ25620_FAULT_INPUT_OVP) == 0U;
    next.charging = next.state == XY_CHARGER_DEVICE_STATE_PRE_CHARGE ||
                    next.state == XY_CHARGER_DEVICE_STATE_FAST_CHARGE ||
                    next.state == XY_CHARGER_DEVICE_STATE_CONSTANT_VOLT;
    next.done = next.state == XY_CHARGER_DEVICE_STATE_CHARGE_DONE;
    *status = next;
    return XY_DEVICE_OK;
}

static int bq25620_set_config(xy_bq25620_t *dev,
                              const xy_charger_device_config_t *config)
{
    uint16_t reg_value;
    int ret;

    if (!bq25620_ready(dev) || !bq25620_config_valid(config)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    reg_value = current_to_reg(config->charge_current, BQ25620_ICHG_STEP_mA) << 6U;
    ret = bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_1,
                             BQ25620_ICHG_MASK, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = voltage_to_reg(config->charge_voltage, BQ25620_VREG_STEP_mV) << 3U;
    ret = bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_3,
                             BQ25620_VREG_MASK, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = current_to_reg(config->input_current_limit, BQ25620_ILIM_STEP_mA) << 4U;
    ret = bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_4,
                             BQ25620_ILIM_MASK, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = current_to_reg(config->precharge_current, 20U) << 4U;
    ret = bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_2,
                             BQ25620_IPRECHG_MASK, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    reg_value = current_to_reg(config->termination_current, 10U) << 3U;
    ret = bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_5,
                             BQ25620_ITERM_MASK, reg_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    return bq25620_update_u8(dev, BQ25620_REG_CHG_CTRL_0, BQ25620_VRECHG,
                             config->recharge_threshold == 200U ? BQ25620_VRECHG : 0U);
}

static int bq25620_enable(xy_bq25620_t *dev, bool enable)
{
    if (!bq25620_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (enable) {
        return bq25620_update_u8(dev, BQ25620_REG_CHG_CTRL_6,
                                 BQ25620_EN_CHG, BQ25620_EN_CHG);
    } else {
        return bq25620_update_u8(dev, BQ25620_REG_CHG_CTRL_6, BQ25620_EN_CHG, 0U);
    }
}

/* ==================== Public API Implementation ==================== */

int xy_bq25620_init(xy_bq25620_t *dev, void *i2c_handle, uint8_t i2c_addr)
{
    xy_bq25620_t next = {0};
    int ret;

    if (!dev || !i2c_handle || i2c_addr != BQ25620_I2C_ADDR) {
        return XY_DEVICE_INVALID_PARAM;
    }

    next.i2c_handle = i2c_handle;
    next.i2c_addr = i2c_addr;
    
    /* 初始化硬件 */
    ret = bq25620_probe(&next);
    if (ret == XY_DEVICE_OK) {
        *dev = next;
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
    
    return bq25620_read_status(dev, status);
}

int xy_bq25620_configure(xy_bq25620_t *dev, const xy_charger_device_config_t *config)
{
    if (!bq25620_ready(dev) || !config) {
        return XY_DEVICE_INVALID_PARAM;
    }

    return bq25620_set_config(dev, config);
}

int xy_bq25620_set_charge_current(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!bq25620_ready(dev) || current_mA < BQ25620_ICHG_MIN_mA ||
        current_mA > BQ25620_ICHG_MAX_mA ||
        (current_mA - BQ25620_ICHG_MIN_mA) % BQ25620_ICHG_STEP_mA != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint16_t ichg_reg = current_to_reg(current_mA, BQ25620_ICHG_STEP_mA) << 6U;
    return bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_1, BQ25620_ICHG_MASK,
                              ichg_reg);
}

int xy_bq25620_set_charge_voltage(xy_bq25620_t *dev, uint32_t voltage_mV)
{
    if (!bq25620_ready(dev) || voltage_mV < BQ25620_VREG_MIN_mV ||
        voltage_mV > BQ25620_VREG_MAX_mV ||
        (voltage_mV - BQ25620_VREG_MIN_mV) % BQ25620_VREG_STEP_mV != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint16_t vreg_reg = voltage_to_reg(voltage_mV, BQ25620_VREG_STEP_mV) << 3U;
    return bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_3, BQ25620_VREG_MASK,
                              vreg_reg);
}

int xy_bq25620_set_input_limit(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!bq25620_ready(dev) || current_mA < BQ25620_ILIM_MIN_mA ||
        current_mA > BQ25620_ILIM_MAX_mA ||
        (current_mA - BQ25620_ILIM_MIN_mA) % BQ25620_ILIM_STEP_mA != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint16_t ilim_reg = current_to_reg(current_mA, BQ25620_ILIM_STEP_mA) << 4U;
    return bq25620_update_u16(dev, BQ25620_REG_CHG_CTRL_4, BQ25620_ILIM_MASK,
                              ilim_reg);
}

int xy_bq25620_start_charge(xy_bq25620_t *dev)
{
    if (!bq25620_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_enable(dev, true);
}

int xy_bq25620_stop_charge(xy_bq25620_t *dev)
{
    if (!bq25620_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_enable(dev, false);
}

/* ==================== End of File ==================== */
