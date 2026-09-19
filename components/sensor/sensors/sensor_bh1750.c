#include "sensor_bh1750.h"
#include "xy_bh1750.h"

#include <string.h>

static sensor_err_t bh1750_map_error(int result)
{
    if (result == XY_DEVICE_OK) return SENSOR_EOK;
    if (result == XY_DEVICE_INVALID_PARAM) return SENSOR_EINVAL;
    if (result == XY_DEVICE_BUSY) return SENSOR_EBUSY;
    if (result == XY_DEVICE_TIMEOUT) return SENSOR_ETIMEOUT;
    if (result == XY_DEVICE_NO_MEM) return SENSOR_ENOMEM;
    return SENSOR_EIO;
}

static sensor_err_t bh1750_init(sensor_device_t *sensor)
{
    bh1750_priv_t *priv;
    int result;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (bh1750_priv_t *)sensor->priv_data;
    result = xy_bh1750_init(&priv->device, sensor->bus, priv->i2c_addr);
    if (result != XY_BH1750_OK) {
        return bh1750_map_error(result);
    }
    result = xy_bh1750_set_resolution(&priv->device, XY_BH1750_HIGH_RES);
    if (result == XY_BH1750_OK) {
        result = xy_bh1750_set_mode(&priv->device, XY_BH1750_ONE_TIME);
    }
    if (result != XY_BH1750_OK) {
        (void)xy_bh1750_deinit(&priv->device);
    }
    return bh1750_map_error(result);
}

static sensor_err_t bh1750_read(sensor_device_t *sensor, sensor_data_t *data)
{
    bh1750_priv_t *priv;
    sensor_data_t next;
    float illuminance;
    int result;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (bh1750_priv_t *)sensor->priv_data;
    result = xy_bh1750_get_illuminance(&priv->device, &illuminance);
    if (result != XY_BH1750_OK) {
        return bh1750_map_error(result);
    }
    memset(&next, 0, sizeof(next));
    next.type = SENSOR_TYPE_LIGHT;
    next.unit = SENSOR_UNIT_LUX;
    /* Preserve the legacy public conversion while the canonical owner keeps its typed API. */
    next.value.val_float = illuminance / 1.2f;
    next.timestamp = SENSOR_GET_TICK();
    *data = next;
    return SENSOR_EOK;
}

static sensor_err_t bh1750_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }
    return bh1750_map_error(xy_bh1750_deinit(&((bh1750_priv_t *)sensor->priv_data)->device));
}

static const sensor_ops_t bh1750_ops = {
    .init = bh1750_init,
    .deinit = bh1750_deinit,
    .read = bh1750_read,
};

sensor_device_t *bh1750_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor;
    bh1750_priv_t *priv;

    if (name == NULL || i2c_bus == NULL) {
        return NULL;
    }
    sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(*sensor));
    priv = (bh1750_priv_t *)SENSOR_MALLOC(sizeof(*priv));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }
    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->i2c_addr = BH1750_ADDR;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "ROHM";
    sensor->info.model = "BH1750";
    sensor->info.type = SENSOR_TYPE_LIGHT;
    sensor->ops = &bh1750_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    return sensor;
}
