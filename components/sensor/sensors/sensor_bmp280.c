#include "sensor_bmp280.h"

#include <string.h>

static sensor_err_t bmp280_map_error(int result)
{
    if (result == XY_DEVICE_OK) return SENSOR_EOK;
    if (result == XY_DEVICE_INVALID_PARAM) return SENSOR_EINVAL;
    if (result == XY_DEVICE_BUSY) return SENSOR_EBUSY;
    if (result == XY_DEVICE_TIMEOUT) return SENSOR_ETIMEOUT;
    if (result == XY_DEVICE_NO_MEM) return SENSOR_ENOMEM;
    if (result == XY_DEVICE_NOT_FOUND) return SENSOR_ENODEV;
    return SENSOR_EIO;
}

static sensor_err_t bmp280_init(sensor_device_t *sensor)
{
    bmp280_priv_t *priv;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (bmp280_priv_t *)sensor->priv_data;
    int result = xy_bmp280_init_addr(&priv->device, sensor->bus, priv->i2c_addr);
    if (result == XY_DEVICE_OK) SENSOR_DELAY_MS(10U);
    return bmp280_map_error(result);
}

static sensor_err_t bmp280_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }
    return bmp280_map_error(xy_bmp280_deinit(&((bmp280_priv_t *)sensor->priv_data)->device));
}

static sensor_err_t bmp280_pressure_read(sensor_device_t *sensor, sensor_data_t *data)
{
    bmp280_priv_t *priv;
    sensor_data_t next;
    uint32_t pressure;
    int result;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (bmp280_priv_t *)sensor->priv_data;
    result = xy_bmp280_read(&priv->device);
    if (result == XY_DEVICE_OK) result = xy_bmp280_get_pressure(&priv->device, &pressure);
    if (result != XY_DEVICE_OK) return bmp280_map_error(result);
    memset(&next, 0, sizeof(next));
    next.type = SENSOR_TYPE_PRESSURE;
    next.unit = SENSOR_UNIT_PASCAL;
    next.value.val_uint32 = pressure;
    next.timestamp = SENSOR_GET_TICK();
    next.accuracy = 98U;
    *data = next;
    return SENSOR_EOK;
}

static sensor_err_t bmp280_temperature_read(sensor_device_t *sensor, sensor_data_t *data)
{
    bmp280_priv_t *priv;
    sensor_data_t next;
    int32_t temperature;
    int result;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (bmp280_priv_t *)sensor->priv_data;
    result = xy_bmp280_read(&priv->device);
    if (result == XY_DEVICE_OK) result = xy_bmp280_get_temperature(&priv->device, &temperature);
    if (result != XY_DEVICE_OK) return bmp280_map_error(result);
    memset(&next, 0, sizeof(next));
    next.type = SENSOR_TYPE_TEMPERATURE;
    next.unit = SENSOR_UNIT_CELSIUS;
#if SENSOR_USE_FLOAT
    next.value.val_float = (float)temperature / 100.0F;
#else
    next.value.val_int32 = temperature;
#endif
    next.timestamp = SENSOR_GET_TICK();
    next.accuracy = 98U;
    *data = next;
    return SENSOR_EOK;
}

static const sensor_ops_t bmp280_pressure_ops = {
    .init = bmp280_init,
    .deinit = bmp280_deinit,
    .read = bmp280_pressure_read,
};
static const sensor_ops_t bmp280_temperature_ops = {
    .init = bmp280_init,
    .deinit = bmp280_deinit,
    .read = bmp280_temperature_read,
};

static sensor_device_t *bmp280_create(const char *name, void *i2c_bus,
                                      const sensor_ops_t *ops, sensor_type_t type,
                                      sensor_unit_t unit, int32_t range_min,
                                      int32_t range_max, uint8_t resolution)
{
    sensor_device_t *sensor;
    bmp280_priv_t *priv;

    if (name == NULL || i2c_bus == NULL) return NULL;
    sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(*sensor));
    priv = (bmp280_priv_t *)SENSOR_MALLOC(sizeof(*priv));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }
    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->i2c_addr = BMP280_ADDR_DEFAULT;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "Bosch";
    sensor->info.model = "BMP280";
    sensor->info.version = 0x0100U;
    sensor->info.type = type;
    sensor->info.unit = unit;
    sensor->info.range_min = range_min;
    sensor->info.range_max = range_max;
    sensor->info.resolution = resolution;
    sensor->info.max_odr = 157U;
    sensor->info.flags = SENSOR_FLAG_HIGH_PRECISION;
    sensor->ops = ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 26U;
    return sensor;
}

sensor_device_t *bmp280_create_pressure(const char *name, void *i2c_bus)
{
    return bmp280_create(name, i2c_bus, &bmp280_pressure_ops, SENSOR_TYPE_PRESSURE,
                         SENSOR_UNIT_PASCAL, 30000, 110000, 18U);
}

sensor_device_t *bmp280_create_temperature(const char *name, void *i2c_bus)
{
    return bmp280_create(name, i2c_bus, &bmp280_temperature_ops, SENSOR_TYPE_TEMPERATURE,
                         SENSOR_UNIT_CELSIUS, -40, 85, 16U);
}
