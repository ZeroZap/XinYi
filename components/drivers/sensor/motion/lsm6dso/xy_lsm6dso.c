#include "xy_lsm6dso.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"

#include <string.h>

static int transport_ready(const xy_lsm6dso_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int ready(const xy_lsm6dso_t *dev)
{
    return transport_ready(dev) && dev->initialized;
}

static xy_error_t read_reg(xy_lsm6dso_t *dev, uint8_t reg, uint8_t *data, size_t length)
{
    if (!transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
}

static xy_error_t write_reg(xy_lsm6dso_t *dev, uint8_t reg, uint8_t value)
{
    if (!transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_lsm6dso_init(xy_lsm6dso_t *dev, void *i2c_handle)
{
    uint8_t id;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_LSM6DSO_ADDR, 1000U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    result = read_reg(dev, XY_LSM6DSO_REG_WHOAMI, &id, 1U);
    if (result == XY_DEVICE_OK && id != XY_LSM6DSO_WHOAMI) {
        result = XY_DEVICE_NOT_FOUND;
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_LSM6DSO_REG_CTRL3_C, 1U);
    }
    if (result == XY_DEVICE_OK) {
        xy_device_delay_ms(10U);
        result = write_reg(dev, XY_LSM6DSO_REG_CTRL4_C, 0U);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_LSM6DSO_REG_CTRL1_XL, XY_LSM6DSO_CTRL1_XL_104HZ_2G);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_LSM6DSO_REG_CTRL2_G, XY_LSM6DSO_CTRL2_G_104HZ_250DPS);
    }
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_lsm6dso_deinit(xy_lsm6dso_t *dev)
{
    xy_error_t result;

    if (!ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = write_reg(dev, XY_LSM6DSO_REG_CTRL1_XL, 0U);
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_LSM6DSO_REG_CTRL2_G, 0U);
    }
    if (result != XY_DEVICE_OK) {
        return result;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_lsm6dso_read(xy_lsm6dso_t *dev, xy_lsm6dso_sample_t *sample)
{
    uint8_t accel[6];
    uint8_t gyro[6];
    xy_lsm6dso_sample_t next;
    xy_error_t result;

    if (!ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = read_reg(dev, XY_LSM6DSO_REG_OUTX_L_XL, accel, sizeof(accel));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    result = read_reg(dev, XY_LSM6DSO_REG_OUTX_L_G, gyro, sizeof(gyro));
    if (result != XY_DEVICE_OK) {
        return result;
    }

    next.accel_x = (int16_t)((uint16_t)accel[0] | ((uint16_t)accel[1] << 8));
    next.accel_y = (int16_t)((uint16_t)accel[2] | ((uint16_t)accel[3] << 8));
    next.accel_z = (int16_t)((uint16_t)accel[4] | ((uint16_t)accel[5] << 8));
    next.gyro_x = (int16_t)((uint16_t)gyro[0] | ((uint16_t)gyro[1] << 8));
    next.gyro_y = (int16_t)((uint16_t)gyro[2] | ((uint16_t)gyro[3] << 8));
    next.gyro_z = (int16_t)((uint16_t)gyro[4] | ((uint16_t)gyro[5] << 8));
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
