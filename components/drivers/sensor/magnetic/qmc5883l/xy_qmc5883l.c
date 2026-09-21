#include "xy_qmc5883l.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"

#include <string.h>

static int qmc5883l_ready(const xy_qmc5883l_t *dev)
{
    return dev != NULL && dev->initialized && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static xy_error_t qmc5883l_write_u8(xy_qmc5883l_t *dev, uint8_t reg, uint8_t value)
{
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_qmc5883l_init(xy_qmc5883l_t *dev, void *i2c_handle)
{
    uint8_t id;
    xy_error_t ret;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_QMC5883L_ADDR, 1000U);
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XY_QMC5883L_REG_CHIP_ID, &id, 1U);
    if (ret != XY_DEVICE_OK || id != XY_QMC5883L_CHIP_ID) {
        memset(dev, 0, sizeof(*dev));
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_NOT_FOUND;
    }
    ret = qmc5883l_write_u8(dev, XY_QMC5883L_REG_CONTROL2, 0x80U);
    if (ret == XY_DEVICE_OK) {
        xy_device_delay_ms(10U);
        ret = qmc5883l_write_u8(dev, XY_QMC5883L_REG_PERIOD, 0x01U);
    }
    if (ret == XY_DEVICE_OK) {
        ret = qmc5883l_write_u8(dev, XY_QMC5883L_REG_CONTROL1,
                                XY_QMC5883L_CONTROL1_2G_200HZ);
    }
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_qmc5883l_deinit(xy_qmc5883l_t *dev)
{
    xy_error_t ret;
    if (!qmc5883l_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = qmc5883l_write_u8(dev, XY_QMC5883L_REG_CONTROL1, 0x00U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_qmc5883l_read(xy_qmc5883l_t *dev, xy_qmc5883l_sample_t *sample)
{
    uint8_t status;
    uint8_t bytes[6];
    xy_qmc5883l_sample_t next;
    xy_error_t ret;

    if (!qmc5883l_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XY_QMC5883L_REG_STATUS, &status, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    if ((status & 0x01U) == 0U) {
        return XY_DEVICE_BUSY;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XY_QMC5883L_REG_DATA_X_LSB, bytes, 6U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next.raw_x = (int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
    next.raw_y = (int16_t)((uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8));
    next.raw_z = (int16_t)((uint16_t)bytes[4] | ((uint16_t)bytes[5] << 8));
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
