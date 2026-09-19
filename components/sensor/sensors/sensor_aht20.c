#include "sensor_aht20.h"

#include <string.h>

static sensor_err_t aht20_map_error(int result)
{
    if (result == XY_AHT20_OK) {
        return SENSOR_EOK;
    }
    if (result == XY_AHT20_INVALID_PARAM || result == XY_DEVICE_INVALID_PARAM) {
        return SENSOR_EINVAL;
    }
    if (result == XY_AHT20_BUSY || result == XY_DEVICE_BUSY) {
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

static sensor_err_t aht20_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }

    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;
    return aht20_map_error(xy_aht20_init(&priv->device, sensor->bus));
}

static sensor_err_t aht20_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }

    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;
    return aht20_map_error(xy_aht20_deinit(&priv->device));
}

static sensor_err_t aht20_temperature_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;
    int result = xy_aht20_read(&priv->device);
    if (result != XY_AHT20_OK) {
        return aht20_map_error(result);
    }

    sensor_data_t measurement = {.type = SENSOR_TYPE_TEMPERATURE,
                                 .unit = SENSOR_UNIT_CELSIUS,
                                 .timestamp = priv->device.data.timestamp,
                                 .accuracy = 98U};
#if SENSOR_USE_FLOAT
    measurement.value.val_float = (float)priv->device.data.temperature / 100.0f;
#else
    measurement.value.val_int32 = priv->device.data.temperature;
#endif
    *data = measurement;
    return SENSOR_EOK;
}

static sensor_err_t aht20_humidity_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;
    int result = xy_aht20_read(&priv->device);
    if (result != XY_AHT20_OK) {
        return aht20_map_error(result);
    }

    sensor_data_t measurement = {.type = SENSOR_TYPE_HUMIDITY,
                                 .unit = SENSOR_UNIT_PERCENT,
                                 .timestamp = priv->device.data.timestamp,
                                 .accuracy = 98U};
#if SENSOR_USE_FLOAT
    measurement.value.val_float = (float)priv->device.data.humidity / 100.0f;
#else
    measurement.value.val_int32 = priv->device.data.humidity;
#endif
    *data = measurement;
    return SENSOR_EOK;
}

static const sensor_ops_t aht20_temperature_ops = {
    .init = aht20_init, .deinit = aht20_deinit, .read = aht20_temperature_read};
static const sensor_ops_t aht20_humidity_ops = {
    .init = aht20_init, .deinit = aht20_deinit, .read = aht20_humidity_read};

static sensor_device_t *aht20_create(const char *name, void *i2c_bus,
                                     const sensor_ops_t *ops, sensor_type_t type,
                                     sensor_unit_t unit, int32_t range_min, int32_t range_max)
{
    if (name == NULL || i2c_bus == NULL) {
        return NULL;
    }

    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(*sensor));
    aht20_priv_t *priv = (aht20_priv_t *)SENSOR_MALLOC(sizeof(*priv));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }

    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->i2c_addr = AHT20_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "ASAIR";
    sensor->info.model = "AHT20";
    sensor->info.version = 0x0100;
    sensor->info.type = type;
    sensor->info.unit = unit;
    sensor->info.range_min = range_min;
    sensor->info.range_max = range_max;
    sensor->info.resolution = 16U;
    sensor->info.max_odr = 10U;
    sensor->ops = ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 1U;
    return sensor;
}

sensor_device_t *aht20_create_temperature(const char *name, void *i2c_bus)
{
    return aht20_create(name, i2c_bus, &aht20_temperature_ops, SENSOR_TYPE_TEMPERATURE,
                        SENSOR_UNIT_CELSIUS, -40, 85);
}

sensor_device_t *aht20_create_humidity(const char *name, void *i2c_bus)
{
    return aht20_create(name, i2c_bus, &aht20_humidity_ops, SENSOR_TYPE_HUMIDITY,
                        SENSOR_UNIT_PERCENT, 0, 100);
}
