/**
 * @file sensor_sht30.c
 * @brief Legacy Sensor lifecycle compatibility wrapper for the canonical SHT30 Device driver
 */
#include "sensor_sht30.h"
#include "xy_sht30.h"

#include <string.h>

static sensor_err_t sht30_map_error(int result)
{
    if (result == XY_DEVICE_OK) {
        return SENSOR_EOK;
    }
    if (result == XY_DEVICE_INVALID_PARAM) {
        return SENSOR_EINVAL;
    }
    return SENSOR_EIO;
}

static sensor_err_t sht30_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }

    sht30_priv_t *priv = (sht30_priv_t *)sensor->priv_data;
    return sht30_map_error(xy_sht30_init_addr(&priv->device, sensor->bus, priv->i2c_addr));
}

static sensor_err_t sht30_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    sht30_priv_t *priv = (sht30_priv_t *)sensor->priv_data;
    int result = xy_sht30_read(&priv->device);
    if (result != XY_DEVICE_OK) {
        return sht30_map_error(result);
    }

    sensor_data_t measurement = {
        .type = SENSOR_TYPE_RELATIVE_HUMIDITY,
        .unit = SENSOR_UNIT_PERCENT,
        .value.val_float = (float)priv->device.humidity / 100.0f,
        .timestamp = SENSOR_GET_TICK(),
        .accuracy = 95U,
    };
    *data = measurement;
    return SENSOR_EOK;
}

static const sensor_ops_t sht30_ops = {
    .init = sht30_init, .deinit = NULL, .read = sht30_read,
};

sensor_device_t *sht30_create(const char *name, void *i2c_bus, uint8_t addr)
{
    if (name == NULL || i2c_bus == NULL) {
        return NULL;
    }

    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    sht30_priv_t *priv = (sht30_priv_t *)SENSOR_MALLOC(sizeof(sht30_priv_t));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(*priv));
    priv->i2c_addr = addr != 0U ? addr : SHT30_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "Sensirion";
    sensor->info.model = "SHT30";
    sensor->info.type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    sensor->info.max_odr = 10U;

    sensor->ops = &sht30_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 10U;
    return sensor;
}
