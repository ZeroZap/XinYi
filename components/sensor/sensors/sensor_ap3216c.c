/**
 * @file sensor_ap3216c.c
 * @brief AP3216C legacy Sensor compatibility wrapper
 */
#include "sensor_ap3216c.h"

#include <string.h>

static sensor_err_t ap3216c_map_error(xy_error_t result)
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

static sensor_err_t ap3216c_init(sensor_device_t *sensor)
{
    ap3216c_priv_t *priv;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (ap3216c_priv_t *)sensor->priv_data;
    return ap3216c_map_error(xy_ap3216c_init(&priv->device, sensor->bus, priv->i2c_addr,
                                             priv->mode));
}

static sensor_err_t ap3216c_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }
    return ap3216c_map_error(
        xy_ap3216c_deinit(&((ap3216c_priv_t *)sensor->priv_data)->device));
}

static sensor_err_t ap3216c_light_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint32_t millilux;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_ap3216c_read_light(&((ap3216c_priv_t *)sensor->priv_data)->device, &millilux);
    if (result != XY_DEVICE_OK) {
        return ap3216c_map_error(result);
    }
    data->type = SENSOR_TYPE_AMBIENT_LIGHT;
    data->unit = SENSOR_UNIT_LUX;
    data->value.val_uint32 = millilux / 1000U;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 90U;
    return SENSOR_EOK;
}

static sensor_err_t ap3216c_proximity_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint16_t raw;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_ap3216c_read_proximity(&((ap3216c_priv_t *)sensor->priv_data)->device, &raw);
    if (result != XY_DEVICE_OK) {
        return ap3216c_map_error(result);
    }
    data->type = SENSOR_TYPE_PROXIMITY;
    data->unit = SENSOR_UNIT_NONE;
    data->value.val_int32 = raw;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 85U;
    return SENSOR_EOK;
}

static sensor_err_t ap3216c_ir_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint16_t raw;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_ap3216c_read_ir(&((ap3216c_priv_t *)sensor->priv_data)->device, &raw);
    if (result != XY_DEVICE_OK) {
        return ap3216c_map_error(result);
    }
    data->type = SENSOR_TYPE_IR;
    data->unit = SENSOR_UNIT_NONE;
    data->value.val_uint32 = raw;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 85U;
    return SENSOR_EOK;
}

static const sensor_ops_t ap3216c_light_ops = {
    .init = ap3216c_init, .deinit = ap3216c_deinit, .read = ap3216c_light_read,
};
static const sensor_ops_t ap3216c_proximity_ops = {
    .init = ap3216c_init, .deinit = ap3216c_deinit, .read = ap3216c_proximity_read,
};
static const sensor_ops_t ap3216c_ir_ops = {
    .init = ap3216c_init, .deinit = ap3216c_deinit, .read = ap3216c_ir_read,
};

static sensor_device_t *ap3216c_create(const char *name, void *i2c_bus, sensor_type_t type,
                                       const sensor_ops_t *ops)
{
    sensor_device_t *sensor;
    ap3216c_priv_t *priv;

    if (name == NULL || i2c_bus == NULL) {
        return NULL;
    }
    sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(*sensor));
    priv = (ap3216c_priv_t *)SENSOR_MALLOC(sizeof(*priv));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }
    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->i2c_addr = AP3216C_ADDR_DEFAULT;
    priv->mode = AP3216C_MODE_ALS_PS;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.name[SENSOR_NAME_MAX_LEN - 1U] = '\0';
    sensor->info.vendor = "Liteon";
    sensor->info.model = "AP3216C";
    sensor->info.version = 0x0100U;
    sensor->info.type = type;
    sensor->info.unit = type == SENSOR_TYPE_AMBIENT_LIGHT ? SENSOR_UNIT_LUX : SENSOR_UNIT_NONE;
    sensor->info.range_max = type == SENSOR_TYPE_AMBIENT_LIGHT ? 20000 : 1023;
    sensor->info.resolution = type == SENSOR_TYPE_AMBIENT_LIGHT ? 16U : 10U;
    sensor->info.max_odr = 10U;
    sensor->ops = ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 1U;
    return sensor;
}

sensor_device_t *ap3216c_create_light(const char *name, void *i2c_bus)
{
    return ap3216c_create(name, i2c_bus, SENSOR_TYPE_AMBIENT_LIGHT, &ap3216c_light_ops);
}

sensor_device_t *ap3216c_create_proximity(const char *name, void *i2c_bus)
{
    return ap3216c_create(name, i2c_bus, SENSOR_TYPE_PROXIMITY, &ap3216c_proximity_ops);
}

sensor_device_t *ap3216c_create_ir(const char *name, void *i2c_bus)
{
    return ap3216c_create(name, i2c_bus, SENSOR_TYPE_IR, &ap3216c_ir_ops);
}