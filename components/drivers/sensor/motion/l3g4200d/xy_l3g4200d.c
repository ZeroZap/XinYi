#include "xy_l3g4200d.h"

#include <string.h>

#define L3G4200D_REG_WHO_AM_I 0x0FU
#define L3G4200D_REG_CTRL1 0x20U
#define L3G4200D_REG_CTRL4 0x23U
#define L3G4200D_REG_STATUS 0x27U
#define L3G4200D_REG_OUT_X_L_AUTO 0xA8U

static bool l3g4200d_ready(const xy_l3g4200d_t *dev);

static xy_error_t l3g4200d_read_reg(xy_l3g4200d_t *dev, uint8_t reg, uint8_t *data,
                                    size_t length)
{
    if (dev == NULL || dev->i2c_dev.base.initialized == 0U ||
        dev->i2c_dev.i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
}

static xy_error_t l3g4200d_write_reg(xy_l3g4200d_t *dev, uint8_t reg, uint8_t value)
{
    if (dev == NULL || dev->i2c_dev.base.initialized == 0U ||
        dev->i2c_dev.i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

static bool l3g4200d_ready(const xy_l3g4200d_t *dev)
{
    return dev != NULL && dev->initialized != 0U && dev->i2c_dev.base.initialized != 0U &&
           dev->i2c_dev.i2c_handle != NULL;
}

xy_error_t xy_l3g4200d_init(xy_l3g4200d_t *dev, void *i2c, uint8_t address)
{
    uint8_t id;
    xy_error_t result;

    if (dev == NULL || i2c == NULL || (address != 0x68U && address != 0x69U)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c, address, 100U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    result = l3g4200d_read_reg(dev, L3G4200D_REG_WHO_AM_I, &id, 1U);
    if (result != XY_DEVICE_OK || id != 0xD3U) {
        memset(dev, 0, sizeof(*dev));
        return result == XY_DEVICE_OK ? XY_DEVICE_NOT_FOUND : result;
    }

    result = l3g4200d_write_reg(dev, L3G4200D_REG_CTRL4, 0x80U);
    if (result == XY_DEVICE_OK) {
        result = l3g4200d_write_reg(dev, L3G4200D_REG_CTRL1, 0x1FU);
    }
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    dev->range_dps = 250U;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_l3g4200d_deinit(xy_l3g4200d_t *dev)
{
    xy_error_t result;

    if (!l3g4200d_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = l3g4200d_write_reg(dev, L3G4200D_REG_CTRL1, 0x07U);
    if (result == XY_DEVICE_OK) {
        dev->initialized = 0U;
        dev->i2c_dev.i2c_handle = NULL;
        dev->i2c_dev.base.initialized = 0U;
    }
    return result;
}

xy_error_t xy_l3g4200d_data_ready(xy_l3g4200d_t *dev, uint8_t *ready)
{
    uint8_t status;
    xy_error_t result;

    if (!l3g4200d_ready(dev) || ready == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = l3g4200d_read_reg(dev, L3G4200D_REG_STATUS, &status, 1U);
    if (result == XY_DEVICE_OK) {
        *ready = (status >> 3) & 1U;
    }
    return result;
}

xy_error_t xy_l3g4200d_set_range(xy_l3g4200d_t *dev, uint16_t range_dps)
{
    uint8_t value;
    xy_error_t result;

    if (!l3g4200d_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    switch (range_dps) {
    case 250U:
        value = 0x80U;
        break;
    case 500U:
        value = 0x90U;
        break;
    case 2000U:
        value = 0xA0U;
        break;
    default:
        return XY_DEVICE_INVALID_PARAM;
    }

    result = l3g4200d_write_reg(dev, L3G4200D_REG_CTRL4, value);
    if (result == XY_DEVICE_OK) {
        dev->range_dps = range_dps;
    }
    return result;
}

xy_error_t xy_l3g4200d_read(xy_l3g4200d_t *dev, xy_l3g4200d_data_t *data)
{
    uint8_t raw[6];
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    int32_t sensitivity;
    xy_l3g4200d_data_t sample;
    xy_error_t result;

    if (!l3g4200d_ready(dev) || data == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = l3g4200d_read_reg(dev, L3G4200D_REG_OUT_X_L_AUTO, raw, sizeof(raw));
    if (result != XY_DEVICE_OK) {
        return result;
    }

    raw_x = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
    raw_y = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
    raw_z = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);
    sensitivity = dev->range_dps == 250U ? 875 : dev->range_dps == 500U ? 1750 : 7000;
    sample.x_mdps = (int32_t)raw_x * sensitivity / 100;
    sample.y_mdps = (int32_t)raw_y * sensitivity / 100;
    sample.z_mdps = (int32_t)raw_z * sensitivity / 100;
    dev->data = sample;
    *data = sample;
    return XY_DEVICE_OK;
}
