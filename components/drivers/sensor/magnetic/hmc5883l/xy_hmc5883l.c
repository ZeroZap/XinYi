#include "xy_hmc5883l.h"
#include <string.h>

#define HMC5883L_REG_CONFIG_A 0x00U
#define HMC5883L_REG_CONFIG_B 0x01U
#define HMC5883L_REG_MODE 0x02U
#define HMC5883L_REG_DATA 0x03U
#define HMC5883L_REG_STATUS 0x09U
#define HMC5883L_REG_ID_A 0x0AU

static xy_error_t read_reg(xy_hmc5883l_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
}

static xy_error_t write_reg(xy_hmc5883l_t *dev, uint8_t reg, uint8_t value)
{
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_hmc5883l_set_gain(xy_hmc5883l_t *dev, xy_hmc5883l_gain_t gain)
{
    if (!dev || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle ||
        (gain != XY_HMC5883L_GAIN_0_88_GA && gain != XY_HMC5883L_GAIN_1_30_GA &&
         gain != XY_HMC5883L_GAIN_8_10_GA)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    xy_error_t result = write_reg(dev, HMC5883L_REG_CONFIG_B, (uint8_t)gain);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    dev->gain = gain;
    return XY_DEVICE_OK;
}

xy_error_t xy_hmc5883l_get_gain(xy_hmc5883l_t *dev, xy_hmc5883l_gain_t *gain)
{
    uint8_t value;
    xy_error_t result;
    if (!dev || !gain || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = read_reg(dev, HMC5883L_REG_CONFIG_B, &value, 1U);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    value &= 0xE0U;
    if (value != XY_HMC5883L_GAIN_0_88_GA && value != XY_HMC5883L_GAIN_1_30_GA &&
        value != XY_HMC5883L_GAIN_8_10_GA) {
        return XY_ERROR_FAIL;
    }
    *gain = (xy_hmc5883l_gain_t)value;
    dev->gain = *gain;
    return XY_DEVICE_OK;
}

xy_error_t xy_hmc5883l_init(xy_hmc5883l_t *dev, void *i2c_handle)
{
    uint8_t id[3];
    xy_error_t result;
    if (!dev || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_HMC5883L_ADDR, 100U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    result = read_reg(dev, HMC5883L_REG_ID_A, id, sizeof(id));
    if (result != XY_DEVICE_OK || id[0] != 'H' || id[1] != '4' || id[2] != '3') {
        memset(dev, 0, sizeof(*dev));
        return result == XY_DEVICE_OK ? XY_DEVICE_NOT_FOUND : result;
    }
    result = write_reg(dev, HMC5883L_REG_CONFIG_A, 0x70U);
    if (result == XY_DEVICE_OK) result = write_reg(dev, HMC5883L_REG_CONFIG_B, 0x20U);
    if (result == XY_DEVICE_OK) result = write_reg(dev, HMC5883L_REG_MODE, 0x00U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    dev->gain = XY_HMC5883L_GAIN_1_30_GA;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_hmc5883l_deinit(xy_hmc5883l_t *dev)
{
    xy_error_t result;
    if (!dev || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = write_reg(dev, HMC5883L_REG_MODE, 0x03U);
    if (result == XY_DEVICE_OK) {
        dev->initialized = 0U;
        dev->i2c_dev.base.initialized = false;
        dev->i2c_dev.i2c_handle = NULL;
    }
    return result;
}

xy_error_t xy_hmc5883l_data_ready(xy_hmc5883l_t *dev, uint8_t *ready)
{
    uint8_t status;
    xy_error_t result;
    if (!dev || !ready || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = read_reg(dev, HMC5883L_REG_STATUS, &status, 1U);
    if (result == XY_DEVICE_OK) {
        *ready = status & 0x01U;
    }
    return result;
}

xy_error_t xy_hmc5883l_read(xy_hmc5883l_t *dev, xy_hmc5883l_data_t *data)
{
    uint8_t raw[6];
    xy_hmc5883l_data_t next;
    xy_error_t result;
    if (!dev || !data || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = read_reg(dev, HMC5883L_REG_DATA, raw, sizeof(raw));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    next.x = (int16_t)(((uint16_t)raw[0] << 8) | raw[1]);
    next.z = (int16_t)(((uint16_t)raw[2] << 8) | raw[3]);
    next.y = (int16_t)(((uint16_t)raw[4] << 8) | raw[5]);
    if (next.x == -4096 || next.y == -4096 || next.z == -4096) {
        return XY_ERROR_OVERFLOW;
    }
    dev->data = next;
    *data = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_hmc5883l_read_field(xy_hmc5883l_t *dev, xy_hmc5883l_field_t *field)
{
    uint8_t bytes[6];
    uint8_t gain_value;
    xy_hmc5883l_data_t raw;
    xy_hmc5883l_field_t next;
    xy_hmc5883l_gain_t gain;
    int32_t sensitivity;
    xy_error_t result;

    if (!dev || !field || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = read_reg(dev, HMC5883L_REG_DATA, bytes, sizeof(bytes));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    raw.x = (int16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    raw.z = (int16_t)(((uint16_t)bytes[2] << 8) | bytes[3]);
    raw.y = (int16_t)(((uint16_t)bytes[4] << 8) | bytes[5]);
    if (raw.x == -4096 || raw.y == -4096 || raw.z == -4096) {
        return XY_ERROR_OVERFLOW;
    }
    result = read_reg(dev, HMC5883L_REG_CONFIG_B, &gain_value, 1U);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    gain_value &= 0xE0U;
    if (gain_value != XY_HMC5883L_GAIN_0_88_GA &&
        gain_value != XY_HMC5883L_GAIN_1_30_GA &&
        gain_value != XY_HMC5883L_GAIN_8_10_GA) {
        return XY_ERROR_FAIL;
    }
    gain = (xy_hmc5883l_gain_t)gain_value;
    switch (gain) {
    case XY_HMC5883L_GAIN_0_88_GA:
        sensitivity = 730;
        break;
    case XY_HMC5883L_GAIN_1_30_GA:
        sensitivity = 1090;
        break;
    case XY_HMC5883L_GAIN_8_10_GA:
        sensitivity = 2560;
        break;
    default:
        return XY_ERROR_FAIL;
    }
    next.x_mgauss = ((int32_t)raw.x * 1000) / sensitivity;
    next.y_mgauss = ((int32_t)raw.y * 1000) / sensitivity;
    next.z_mgauss = ((int32_t)raw.z * 1000) / sensitivity;
    dev->data = raw;
    dev->gain = gain;
    *field = next;
    return XY_DEVICE_OK;
}
