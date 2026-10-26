/**
 * @file xy_fg_max17043.c
 * @brief MAX17043 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Reference: Zephyr MAX17043 driver
 */

#include "xy_fg_max17043.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* MAX17043 寄存器定义 */
#define MAX17043_REG_VCELL      0x02
#define MAX17043_REG_SOC        0x04
#define MAX17043_REG_MODE       0x06
#define MAX17043_REG_VER        0x08
#define MAX17043_REG_HIBRT      0x0A
#define MAX17043_REG_CONFIG     0x0C
#define MAX17043_REG_VALRT      0x14
#define MAX17043_REG_CRATE      0x16
#define MAX17043_REG_VRESET     0x18
#define MAX17043_REG_STATUS     0x1A
#define MAX17043_REG_UNLOCK     0x3E
#define MAX17043_REG_COMMAND    0xFF

#define MAX17043_IO_RETRIES     3

static int max17043_read_reg16(xy_fg_max17043_data_t *priv, uint8_t reg, uint16_t *value)
{
    int ret = -1;

    if (!priv || !value) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    for (int attempt = 0; attempt < MAX17043_IO_RETRIES; ++attempt) {
        ret = xy_sensor_i2c_read_reg16(&priv->bus, reg, value);
        if (ret == 0) {
            return XY_FG_OK;
        }
    }

    return XY_FG_ERROR;
}

static int max17043_write_reg16(xy_fg_max17043_data_t *priv, uint8_t reg, uint16_t value)
{
    int ret = -1;

    if (!priv) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    for (int attempt = 0; attempt < MAX17043_IO_RETRIES; ++attempt) {
        ret = xy_sensor_i2c_write_reg16(&priv->bus, reg, value);
        if (ret == 0) {
            return XY_FG_OK;
        }
    }

    return XY_FG_ERROR;
}

/**
 * @brief MAX17043 初始化
 */
static int max17043_init(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->data) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    xy_fg_max17043_data_t *priv = (xy_fg_max17043_data_t *)fg->data;
    
    /* 读取版本 */
    uint16_t version;
    if (max17043_read_reg16(priv, MAX17043_REG_VER, &version) != XY_FG_OK) {
        priv->initialized = false;
        return XY_FG_ERROR;
    }
    
    xy_log_i("MAX17043 version: 0x%04X\n", version);
    
    /* 配置：默认配置 */
    if (max17043_write_reg16(priv, MAX17043_REG_CONFIG, 0x0000) != XY_FG_OK) {
        priv->initialized = false;
        return XY_FG_ERROR;
    }
    
    priv->initialized = true;
    xy_log_i("MAX17043 initialized\n");
    return 0;
}

/**
 * @brief MAX17043 数据获取
 */
static int max17043_fetch(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->data) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    xy_fg_max17043_data_t *priv = (xy_fg_max17043_data_t *)fg->data;
    if (!fg->initialized || !priv->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }

    xy_fuel_gauge_data_t snapshot = priv->data;

    /* 读取电压 (mV), VCELL LSB = 1.25mV */
    uint16_t vcell;
    if (max17043_read_reg16(priv, MAX17043_REG_VCELL, &vcell) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    snapshot.voltage_mv = (uint16_t)((uint32_t)(vcell >> 4) * 125U / 100U);

    /* 读取 SOC (%) */
    uint16_t soc_reg;
    if (max17043_read_reg16(priv, MAX17043_REG_SOC, &soc_reg) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    snapshot.soc = (uint8_t)(soc_reg >> 8);

    /* 读取充放电率 */
    uint16_t crate_reg;
    if (max17043_read_reg16(priv, MAX17043_REG_CRATE, &crate_reg) != XY_FG_OK) {
        return XY_FG_ERROR;
    }
    snapshot.current_ma = (int16_t)((int32_t)(int16_t)crate_reg * 208 / 1000);
    snapshot.temperature_c = 0;  /* MAX17043 不支持温度 */

    priv->data = snapshot;
    fg->latest = snapshot;

    return 0;
}

/**
 * @brief MAX17043 通道数据获取
 */
static int max17043_channel_get(xy_fuel_gauge_t *fg,
                                xy_fuel_gauge_data_type_t channel,
                                int32_t *val)
{
    if (!fg || !fg->data || !val) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    xy_fg_max17043_data_t *priv = (xy_fg_max17043_data_t *)fg->data;
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
        case XY_FG_DATA_SOC:
            *val = priv->data.soc;
            break;
        default:
            return XY_FG_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

static int max17043_alert_set(xy_fuel_gauge_t *fg,
                              const xy_fuel_gauge_alert_t *alert)
{
    if (!fg || !fg->data || !alert) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    xy_fg_max17043_data_t *priv = (xy_fg_max17043_data_t *)fg->data;
    if (!fg->initialized || !priv->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }

    priv->alert = *alert;
    return XY_FG_OK;
}

static int max17043_alert_get(xy_fuel_gauge_t *fg,
                              xy_fuel_gauge_alert_t *alert)
{
    if (!fg || !fg->data || !alert) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    xy_fg_max17043_data_t *priv = (xy_fg_max17043_data_t *)fg->data;
    if (!fg->initialized || !priv->initialized) {
        return XY_FG_ERROR_NOT_INITIALIZED;
    }

    *alert = priv->alert;
    return XY_FG_OK;
}

/**
 * @brief MAX17043 驱动 API
 */
static const xy_fuel_gauge_api_t max17043_driver_api = {
    .init = max17043_init,
    .fetch = max17043_fetch,
    .channel_get = max17043_channel_get,
    .alert_set = max17043_alert_set,
    .alert_get = max17043_alert_get,
};

/* 设备实例 */
static xy_fg_max17043_data_t max17043_priv;
static xy_fuel_gauge_t max17043_device = {
    .name = "MAX17043",
    .api = &max17043_driver_api,
    .data = &max17043_priv,
};

/**
 * @brief 注册调用方自有的 MAX17043 实例
 */
int xy_fuel_gauge_max17043_register_instance(xy_fuel_gauge_t *fg,
                                             xy_fg_max17043_data_t *priv,
                                             void *i2c_handle, uint8_t addr)
{
    if (!fg || !fg->name || !priv || !i2c_handle) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    if (xy_fuel_gauge_device_get(fg->name)) {
        return XY_FG_ERROR;
    }

    memset(priv, 0, sizeof(*priv));
    xy_sensor_bus_config_i2c(&priv->bus, i2c_handle, addr ? addr : MAX17043_ADDR);

    fg->api = &max17043_driver_api;
    fg->data = priv;
    fg->initialized = false;
    fg->next = NULL;

    return xy_fuel_gauge_device_register(fg);
}

/**
 * @brief 注册 MAX17043 电量计（单例兼容包装）
 */
int xy_fuel_gauge_max17043_register(void *i2c_handle, uint8_t addr)
{
    return xy_fuel_gauge_max17043_register_instance(&max17043_device,
                                                    &max17043_priv,
                                                    i2c_handle, addr);
}
