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
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_hmc5883l_deinit(xy_hmc5883l_t *dev)
{
    xy_error_t result;
    if (!dev || !dev->initialized || !dev->i2c_dev.base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = write_reg(dev, HMC5883L_REG_MODE, 0x03U);
    if (result == XY_DEVICE_OK) {
        dev->initialized = 0U;
        dev->i2c_dev.base.initialized = false;
    }
    return result;
}

xy_error_t xy_hmc5883l_data_ready(xy_hmc5883l_t *dev, uint8_t *ready)
{
    uint8_t status;
    xy_error_t result;
    if (!dev || !ready || !dev->initialized || !dev->i2c_dev.base.initialized) {
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
    if (!dev || !data || !dev->initialized || !dev->i2c_dev.base.initialized) {
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
