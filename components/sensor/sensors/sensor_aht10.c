/**
 * @file sensor_aht10.c
 * @brief AHT10 legacy Sensor compatibility wrapper
 */
#include "sensor_aht10.h"

#include <string.h>

static sensor_err_t aht10_map_error(xy_error_t result)
{
    if (result == XY_DEVICE_OK) {
        return SENSOR_EOK;
    }
    if (result == XY_DEVICE_INVALID_PARAM) {
        return SENSOR_EINVAL;
    }
    if (result == XY_DEVICE_BUSY) {
        return SENSOR_EBUSY;
    }
    if (result == XY_DEVICE_TIMEOUT) {
        return SENSOR_ETIMEOUT;
    }
    if (result == XY_DEVICE_NO_MEM) {
        return SENSOR_ENOMEM;
    }
    return SENSOR_EIO;
}

static sensor_err_t aht10_init(sensor_device_t *sensor)
{
    aht10_priv_t *priv;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (aht10_priv_t *)sensor->priv_data;
    return aht10_map_error(xy_aht10_init(&priv->device, sensor->bus, priv->i2c_addr));
}

static sensor_err_t aht10_read(sensor_device_t *sensor, sensor_data_t *data)
{
    aht10_priv_t *priv;
    xy_aht10_data_t sample;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (aht10_priv_t *)sensor->priv_data;
    result = xy_aht10_read(&priv->device, &sample);
    if (result != XY_DEVICE_OK) {
        return aht10_map_error(result);
    }

    data->type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    data->unit = SENSOR_UNIT_PERCENT;
    data->value.val_float = (float)sample.humidity_centi_pct / 100.0f;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 90U;
    return SENSOR_EOK;
}

static sensor_err_t aht10_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }

    return aht10_map_error(xy_aht10_deinit(&((aht10_priv_t *)sensor->priv_data)->device));
}

static const sensor_ops_t aht10_ops = {
    .init = aht10_init, .deinit = aht10_deinit, .read = aht10_read,
};

sensor_device_t *aht10_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor;
    aht10_priv_t *priv;

    if (name == NULL || i2c_bus == NULL ||
        (addr != 0U && addr != AHT10_ADDR_DEFAULT)) {
        return NULL;
    }
    sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    priv = (aht10_priv_t *)SENSOR_MALLOC(sizeof(aht10_priv_t));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }

    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->i2c_addr = AHT10_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.name[SENSOR_NAME_MAX_LEN - 1U] = '\0';
    sensor->info.vendor = "Aosong";
    sensor->info.model = "AHT10";
    sensor->info.type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    sensor->info.max_odr = 10U;

    sensor->ops = &aht10_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 10U;
    return sensor;
}
