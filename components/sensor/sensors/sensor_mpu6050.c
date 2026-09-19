#include "sensor_mpu6050.h"

#include <string.h>

static sensor_err_t mpu6050_map_error(int result)
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
    if (result == XY_DEVICE_NOT_FOUND) {
        return SENSOR_ENODEV;
    }
    return SENSOR_EIO;
}

static sensor_err_t mpu6050_init(sensor_device_t *sensor)
{
    mpu6050_priv_t *priv;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }
    priv = (mpu6050_priv_t *)sensor->priv_data;
    return mpu6050_map_error(
        xy_mpu6050_init_addr(&priv->device, sensor->bus, priv->i2c_addr));
}

static sensor_err_t mpu6050_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL) {
        return SENSOR_EINVAL;
    }
    return mpu6050_map_error(
        xy_mpu6050_deinit(&((mpu6050_priv_t *)sensor->priv_data)->device));
}

static sensor_err_t mpu6050_accel_read(sensor_device_t *sensor, sensor_data_t *data)
{
    sensor_data_t next;
    float x;
    float y;
    float z;
    int result;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_mpu6050_read_accel(&((mpu6050_priv_t *)sensor->priv_data)->device, &x, &y,
                                   &z);
    if (result != XY_MPU6050_OK) {
        return mpu6050_map_error(result);
    }
    memset(&next, 0, sizeof(next));
    next.type = SENSOR_TYPE_ACCELEROMETER;
    next.unit = SENSOR_UNIT_MILLI_G;
    next.value.val_3axis.x = (int32_t)(x * 1000.0F);
    next.value.val_3axis.y = (int32_t)(y * 1000.0F);
    next.value.val_3axis.z = (int32_t)(z * 1000.0F);
    next.timestamp = SENSOR_GET_TICK();
    next.accuracy = 95U;
    *data = next;
    return SENSOR_EOK;
}

static sensor_err_t mpu6050_gyro_read(sensor_device_t *sensor, sensor_data_t *data)
{
    sensor_data_t next;
    float x;
    float y;
    float z;
    int result;

    if (sensor == NULL || sensor->priv_data == NULL || sensor->bus == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    result = xy_mpu6050_read_gyro(&((mpu6050_priv_t *)sensor->priv_data)->device, &x, &y, &z);
    if (result != XY_MPU6050_OK) {
        return mpu6050_map_error(result);
    }
    memset(&next, 0, sizeof(next));
    next.type = SENSOR_TYPE_GYROSCOPE;
    next.unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    next.value.val_3axis.x = (int32_t)x;
    next.value.val_3axis.y = (int32_t)y;
    next.value.val_3axis.z = (int32_t)z;
    next.timestamp = SENSOR_GET_TICK();
    next.accuracy = 95U;
    *data = next;
    return SENSOR_EOK;
}

static const sensor_ops_t mpu6050_accel_ops = {
    .init = mpu6050_init,
    .deinit = mpu6050_deinit,
    .read = mpu6050_accel_read,
};

static const sensor_ops_t mpu6050_gyro_ops = {
    .init = mpu6050_init,
    .deinit = mpu6050_deinit,
    .read = mpu6050_gyro_read,
};

static sensor_device_t *mpu6050_create(const char *name, void *i2c_bus,
                                       const sensor_ops_t *ops, sensor_type_t type,
                                       sensor_unit_t unit, int32_t range_min,
                                       int32_t range_max, uint32_t max_odr)
{
    sensor_device_t *sensor;
    mpu6050_priv_t *priv;

    if (name == NULL || i2c_bus == NULL) {
        return NULL;
    }
    sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(*sensor));
    priv = (mpu6050_priv_t *)SENSOR_MALLOC(sizeof(*priv));
    if (sensor == NULL || priv == NULL) {
        SENSOR_FREE(sensor);
        SENSOR_FREE(priv);
        return NULL;
    }
    memset(sensor, 0, sizeof(*sensor));
    memset(priv, 0, sizeof(*priv));
    priv->i2c_addr = MPU6050_ADDR_DEFAULT;
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1U);
    sensor->info.vendor = "InvenSense";
    sensor->info.model = "MPU6050";
    sensor->info.version = 0x0100U;
    sensor->info.type = type;
    sensor->info.unit = unit;
    sensor->info.range_max = range_max;
    sensor->info.range_min = range_min;
    sensor->info.resolution = 16U;
    sensor->info.max_odr = max_odr;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;
    sensor->ops = ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100U;
    return sensor;
}

sensor_device_t *mpu6050_create_accel(const char *name, void *i2c_bus)
{
    return mpu6050_create(name, i2c_bus, &mpu6050_accel_ops, SENSOR_TYPE_ACCELEROMETER,
                          SENSOR_UNIT_MILLI_G, -2000, 2000, 1000U);
}

sensor_device_t *mpu6050_create_gyro(const char *name, void *i2c_bus)
{
    return mpu6050_create(name, i2c_bus, &mpu6050_gyro_ops, SENSOR_TYPE_GYROSCOPE,
                          SENSOR_UNIT_DEGREE_PER_SECOND, -250, 250, 8000U);
}
