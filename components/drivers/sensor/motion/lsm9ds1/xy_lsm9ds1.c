#include "xy_lsm9ds1.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"

#include <string.h>

static int transport_ready(const xy_i2c_device_t *transport)
{
    return transport != NULL && transport->base.initialized && transport->i2c_handle != NULL;
}

static int ready(const xy_lsm9ds1_t *dev)
{
    return dev != NULL && dev->initialized && transport_ready(&dev->imu) &&
           transport_ready(&dev->mag);
}

static xy_error_t read_reg(xy_i2c_device_t *transport, uint8_t reg, uint8_t *data, size_t length)
{
    if (!transport_ready(transport)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(transport, reg, data, length);
}

static xy_error_t write_reg(xy_i2c_device_t *transport, uint8_t reg, uint8_t value)
{
    if (!transport_ready(transport)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(transport, reg, &value, 1U);
}

xy_error_t xy_lsm9ds1_init(xy_lsm9ds1_t *dev, void *i2c_handle)
{
    uint8_t imu_id;
    uint8_t mag_id;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->imu, i2c_handle, XY_LSM9DS1_IMU_ADDR, 1000U);
    if (result == XY_DEVICE_OK) {
        result = xy_i2c_device_init(&dev->mag, i2c_handle, XY_LSM9DS1_MAG_ADDR, 1000U);
    }
    if (result == XY_DEVICE_OK) {
        result = read_reg(&dev->imu, XY_LSM9DS1_REG_WHOAMI_IMU, &imu_id, 1U);
    }
    if (result == XY_DEVICE_OK && imu_id != XY_LSM9DS1_IMU_WHOAMI) {
        result = XY_DEVICE_NOT_FOUND;
    }
    if (result == XY_DEVICE_OK) {
        result = read_reg(&dev->mag, XY_LSM9DS1_REG_WHOAMI_IMU, &mag_id, 1U);
    }
    if (result == XY_DEVICE_OK && mag_id != XY_LSM9DS1_MAG_WHOAMI) {
        result = XY_DEVICE_NOT_FOUND;
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(&dev->imu, XY_LSM9DS1_REG_CTRL3_C, 1U);
    }
    if (result == XY_DEVICE_OK) {
        xy_device_delay_ms(10U);
        result = write_reg(&dev->imu, XY_LSM9DS1_REG_CTRL1_XL,
                           XY_LSM9DS1_CTRL1_XL_104HZ_2G);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(&dev->imu, XY_LSM9DS1_REG_CTRL2_G,
                           XY_LSM9DS1_CTRL2_G_104HZ_250DPS);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(&dev->mag, XY_LSM9DS1_REG_CTRL_REG1_M,
                           XY_LSM9DS1_CTRL1_M_10HZ_HIGH_POWER);
    }
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_lsm9ds1_deinit(xy_lsm9ds1_t *dev)
{
    xy_error_t result;

    if (!ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = write_reg(&dev->imu, XY_LSM9DS1_REG_CTRL1_XL, 0U);
    if (result == XY_DEVICE_OK) {
        result = write_reg(&dev->imu, XY_LSM9DS1_REG_CTRL2_G, 0U);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(&dev->mag, XY_LSM9DS1_REG_CTRL_REG1_M, 0U);
    }
    if (result != XY_DEVICE_OK) {
        return result;
    }

    dev->initialized = 0U;
    dev->imu.base.initialized = 0U;
    dev->mag.base.initialized = 0U;
    dev->imu.i2c_handle = NULL;
    dev->mag.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_lsm9ds1_read(xy_lsm9ds1_t *dev, xy_lsm9ds1_sample_t *sample)
{
    uint8_t accel[6];
    uint8_t gyro[6];
    uint8_t mag[6];
    xy_lsm9ds1_sample_t next;
    xy_error_t result;

    if (!ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = read_reg(&dev->imu, XY_LSM9DS1_REG_OUTX_L_XL, accel, sizeof(accel));
    if (result == XY_DEVICE_OK) {
        result = read_reg(&dev->imu, XY_LSM9DS1_REG_OUTX_L_G, gyro, sizeof(gyro));
    }
    if (result == XY_DEVICE_OK) {
        result = read_reg(&dev->mag, XY_LSM9DS1_REG_OUTX_L_XL, mag, sizeof(mag));
    }
    if (result != XY_DEVICE_OK) {
        return result;
    }

    next.accel_x = (int16_t)((uint16_t)accel[0] | ((uint16_t)accel[1] << 8));
    next.accel_y = (int16_t)((uint16_t)accel[2] | ((uint16_t)accel[3] << 8));
    next.accel_z = (int16_t)((uint16_t)accel[4] | ((uint16_t)accel[5] << 8));
    next.gyro_x = (int16_t)((uint16_t)gyro[0] | ((uint16_t)gyro[1] << 8));
    next.gyro_y = (int16_t)((uint16_t)gyro[2] | ((uint16_t)gyro[3] << 8));
    next.gyro_z = (int16_t)((uint16_t)gyro[4] | ((uint16_t)gyro[5] << 8));
    next.mag_x = (int16_t)((uint16_t)mag[0] | ((uint16_t)mag[1] << 8));
    next.mag_y = (int16_t)((uint16_t)mag[2] | ((uint16_t)mag[3] << 8));
    next.mag_z = (int16_t)((uint16_t)mag[4] | ((uint16_t)mag[5] << 8));
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
