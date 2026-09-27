#include "sensor_qma6100p.h"

#include <string.h>

static sensor_err_t map_error(xy_error_t result)
{
    if (result == XY_DEVICE_OK) return SENSOR_EOK;
    if (result == XY_DEVICE_INVALID_PARAM) return SENSOR_EINVAL;
    if (result == XY_DEVICE_TIMEOUT) return SENSOR_ETIMEOUT;
    if (result == XY_DEVICE_BUSY) return SENSOR_EBUSY;
    return SENSOR_EIO;
}

static sensor_err_t qma6100p_init(sensor_device_t *sensor)
{
    qma6100p_priv_t *priv;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (qma6100p_priv_t *)sensor->priv_data;
    return map_error(xy_qma6100p_init(&priv->device, sensor->bus, priv->address));
}

static sensor_err_t qma6100p_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL) return SENSOR_EINVAL;
    return map_error(xy_qma6100p_deinit(&((qma6100p_priv_t *)sensor->priv_data)->device));
}

static sensor_err_t qma6100p_read(sensor_device_t *sensor, sensor_data_t *data)
{
    xy_qma6100p_accel_t accel;
    xy_error_t result;

    if (sensor == NULL || sensor->bus == NULL || sensor->priv_data == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_qma6100p_read_accel(&((qma6100p_priv_t *)sensor->priv_data)->device, &accel);
    if (result != XY_DEVICE_OK) return map_error(result);
    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = accel.x_mg;
    data->value.val_3axis.y = accel.y_mg;
    data->value.val_3axis.z = accel.z_mg;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95U;
    return SENSOR_EOK;
}

static const sensor_ops_t qma6100p_ops = {
    .init = qma6100p_init,
    .deinit = qma6100p_deinit,
    .read = qma6100p_read,
};

sensor_device_t *qma6100p_create_accel(const char *name, void *bus, uint8_t address)
{
    sensor_device_t *sensor;
    qma6100p_priv_t *priv;

    if (name == NULL || bus == NULL ||
        (address != XY_QMA6100P_ADDR_LOW && address != XY_QMA6100P_ADDR_HIGH)) {
        return NULL;
    }
    sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(*sensor));
    priv = (qma6100p_priv_t *)SENSOR_MALLOC(sizeof(*priv));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }
    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->address = address;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.name[SENSOR_NAME_MAX_LEN - 1U] = '\0';
    sensor->info.vendor = "QST";
    sensor->info.model = "QMA6100P";
    sensor->info.version = 0x0100U;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_min = -2000;
    sensor->info.range_max = 2000;
    sensor->info.resolution = 14U;
    sensor->info.max_odr = 100U;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT;
    sensor->odr = 100U;
    sensor->ops = &qma6100p_ops;
    sensor->bus = bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    return sensor;
}
