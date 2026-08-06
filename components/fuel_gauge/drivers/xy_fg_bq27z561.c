/**
 * @file xy_fg_bq27z561.c
 * @brief BQ27z561 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Reference: Zephyr BQ27z561 driver
 */

#include "xy_fg_bq27z561.h"
#include "xy_log.h"
#include "xy_sensor_device.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* BQ27z561 寄存器定义 */
#define BQ27Z561_REG_CTRL       0x00
#define BQ27Z561_REG_TEMP       0x06
#define BQ27Z561_REG_VOLT       0x08
#define BQ27Z561_REG_FLAGS      0x0A
#define BQ27Z561_REG_NOM_CAP    0x12
#define BQ27Z561_REG_REM_CAP    0x14
#define BQ27Z561_REG_SOC        0x2C
#define BQ27Z561_REG_CYCLE_CNT  0x2A
#define BQ27Z561_REG_CURR       0x58
#define BQ27Z561_REG_AVG_CURR   0x5A
#define BQ27Z561_REG_SOH        0x7A
#define BQ27Z561_IO_RETRIES     3

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
    xy_fuel_gauge_alert_t alert;
    int16_t average_current_ma;
} bq27z561_private_data_t;

static int bq27z561_read_reg16(bq27z561_private_data_t *priv, uint8_t reg, uint16_t *value)
{
    if (!priv || !value) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    for (int attempt = 0; attempt < BQ27Z561_IO_RETRIES; ++attempt) {
        if (xy_sensor_i2c_read_reg16(&priv->bus, reg, value) == 0) {
            return XY_FG_OK;
        }
    }

    return XY_FG_ERROR;
}

static int bq27z561_read_reg8(bq27z561_private_data_t *priv, uint8_t reg, uint8_t *value)
{
    if (!priv || !value) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    for (int attempt = 0; attempt < BQ27Z561_IO_RETRIES; ++attempt) {
        if (xy_sensor_i2c_read_reg(&priv->bus, reg, value) == 0) {
            return XY_FG_OK;
        }
    }

    return XY_FG_ERROR;
}

/**
 * @brief BQ27z561 初始化
 */
static int bq27z561_init(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->data) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    
    /* 读取设备 ID */
    uint16_t device_id;
    if (bq27z561_read_reg16(priv, 0x0002, &device_id) != XY_FG_OK) {
        priv->initialized = false;
        return XY_FG_ERROR;
    }
    
    xy_log_i("BQ27z561 device ID: 0x%04X\n", device_id);
    
    priv->initialized = true;
    xy_log_i("BQ27z561 initialized\n");
    return 0;
}

/**
 * @brief BQ27z561 数据获取
 */
static int bq27z561_fetch(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->data) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    if (!fg->initialized || !priv->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }
    
    /* 读取电压 (mV) */
    uint16_t voltage;
    if (bq27z561_read_reg16(priv, BQ27Z561_REG_VOLT, &voltage) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    
    /* 读取电流 (mA) */
    int16_t current;
    if (bq27z561_read_reg16(priv, BQ27Z561_REG_CURR, (uint16_t*)&current) != XY_FG_OK) {
        return XY_FG_ERROR;
    }

    /* 读取平均电流 (mA)，公共 snapshot 没有独立字段，驱动私有缓存该值。 */
    int16_t average_current;
    if (bq27z561_read_reg16(priv, BQ27Z561_REG_AVG_CURR,
                            (uint16_t *)&average_current) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    
    /* 读取 SOC (%) */
    uint8_t soc;
    if (bq27z561_read_reg8(priv, BQ27Z561_REG_SOC, &soc) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    
    /* 读取 SOH (%) */
    uint8_t soh;
    if (bq27z561_read_reg8(priv, BQ27Z561_REG_SOH, &soh) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    
    /* 读取温度 (0.1K) */
    uint16_t temp;
    if (bq27z561_read_reg16(priv, BQ27Z561_REG_TEMP, &temp) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    int16_t temp_c = (int16_t)((int32_t)temp - 2731);  /* 转换为 0.1°C */
    
    /* 读取满充容量 (mAh) */
    uint16_t full_cap;
    if (bq27z561_read_reg16(priv, BQ27Z561_REG_NOM_CAP, &full_cap) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    
    /* 读取剩余容量 (mAh) */
    uint16_t rem_cap;
    if (bq27z561_read_reg16(priv, BQ27Z561_REG_REM_CAP, &rem_cap) != XY_FG_OK) {
        return XY_FG_ERROR;
    }

    /* 读取循环次数 */
    uint16_t cycle_count;
    if (bq27z561_read_reg16(priv, BQ27Z561_REG_CYCLE_CNT, &cycle_count) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    
    /* 存储数据 */
    priv->data.voltage_mv = voltage;
    priv->data.current_ma = current;
    priv->data.soc = soc;
    priv->data.soh = soh;
    priv->data.temperature_c = temp_c;
    priv->data.full_capacity_mah = full_cap;
    priv->data.remain_capacity_mah = rem_cap;
    priv->data.cycle_count = cycle_count;
    priv->average_current_ma = average_current;
    fg->latest = priv->data;
    
    return 0;
}

/**
 * @brief BQ27z561 通道数据获取
 */
static int bq27z561_channel_get(xy_fuel_gauge_t *fg,
                                xy_fuel_gauge_data_type_t channel,
                                int32_t *val)
{
    if (!fg || !fg->data || !val) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    if (!fg->initialized || !priv->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }

    switch (channel) {
        case XY_FG_DATA_VOLTAGE:
            *val = priv->data.voltage_mv;
            break;
        case XY_FG_DATA_CURRENT:
            *val = priv->data.current_ma;
            break;
        case XY_FG_DATA_AVERAGE_CURRENT:
            *val = priv->average_current_ma;
            break;
        case XY_FG_DATA_SOC:
            *val = priv->data.soc;
            break;
        case XY_FG_DATA_SOH:
            *val = priv->data.soh;
            break;
        case XY_FG_DATA_TEMPERATURE:
            *val = priv->data.temperature_c;
            break;
        case XY_FG_DATA_FULL_CAPACITY:
            *val = priv->data.full_capacity_mah;
            break;
        case XY_FG_DATA_REMAIN_CAPACITY:
            *val = priv->data.remain_capacity_mah;
            break;
        case XY_FG_DATA_CYCLE_COUNT:
            *val = priv->data.cycle_count;
            break;
        default:
            return XY_FG_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

static int bq27z561_alert_set(xy_fuel_gauge_t *fg,
                              const xy_fuel_gauge_alert_t *alert)
{
    if (!fg || !fg->data || !alert) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    if (!fg->initialized || !priv->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }

    priv->alert = *alert;
    return XY_FG_OK;
}

static int bq27z561_alert_get(xy_fuel_gauge_t *fg,
                              xy_fuel_gauge_alert_t *alert)
{
    if (!fg || !fg->data || !alert) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    if (!fg->initialized || !priv->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }

    *alert = priv->alert;
    return XY_FG_OK;
}

/**
 * @brief BQ27z561 驱动 API
 */
static const xy_fuel_gauge_api_t bq27z561_driver_api = {
    .init = bq27z561_init,
    .fetch = bq27z561_fetch,
    .channel_get = bq27z561_channel_get,
    .alert_set = bq27z561_alert_set,
    .alert_get = bq27z561_alert_get,
};

/* 设备实例 */
static bq27z561_private_data_t bq27z561_priv;
static xy_fuel_gauge_t bq27z561_device = {
    .name = "BQ27z561",
    .api = &bq27z561_driver_api,
    .data = &bq27z561_priv,
};

/**
 * @brief 注册 BQ27z561 电量计
 */
int xy_fuel_gauge_bq27z561_register(void *i2c_handle, uint8_t addr)
{
    if (!i2c_handle) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    if (xy_fuel_gauge_device_get(bq27z561_device.name)) {
        return XY_FG_ERROR;
    }

    xy_sensor_bus_config_i2c(&bq27z561_priv.bus, i2c_handle, addr ? addr : BQ27Z561_ADDR);
    return xy_fuel_gauge_device_register(&bq27z561_device);
}
