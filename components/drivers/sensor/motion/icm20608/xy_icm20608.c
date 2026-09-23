#include "xy_icm20608.h"
#include "xy_device_timing.h"

#include <string.h>

static int icm20608_ready(const xy_icm20608_t *dev)
{
    if (dev == NULL || dev->initialized == 0U) {
        return 0;
    }
    if (dev->transport == XY_ICM20608_TRANSPORT_I2C) {
        return dev->i2c_dev.base.initialized != 0U && dev->i2c_dev.i2c_handle != NULL;
    }
    return dev->spi_context != NULL && dev->spi_read != NULL && dev->spi_write != NULL;
}

static xy_error_t icm20608_read(xy_icm20608_t *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (dev == NULL || data == NULL || len == 0U ||
        (dev->transport == XY_ICM20608_TRANSPORT_I2C
             ? (dev->i2c_dev.base.initialized == 0U || dev->i2c_dev.i2c_handle == NULL)
             : (dev->spi_context == NULL || dev->spi_read == NULL || dev->spi_write == NULL))) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (dev->transport == XY_ICM20608_TRANSPORT_I2C) {
        return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
    }
    return dev->spi_read(dev->spi_context, reg, data, len);
}

static xy_error_t icm20608_write(xy_icm20608_t *dev, uint8_t reg, uint8_t value)
{
    if (dev == NULL ||
        (dev->transport == XY_ICM20608_TRANSPORT_I2C
             ? (dev->i2c_dev.base.initialized == 0U || dev->i2c_dev.i2c_handle == NULL)
             : (dev->spi_context == NULL || dev->spi_read == NULL || dev->spi_write == NULL))) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (dev->transport == XY_ICM20608_TRANSPORT_I2C) {
        return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
    }
    return dev->spi_write(dev->spi_context, reg, &value, 1U);
}

static xy_error_t icm20608_configure(xy_icm20608_t *dev)
{
    uint8_t identity;
    xy_error_t result;

    result = icm20608_read(dev, XY_ICM20608_REG_WHO_AM_I, &identity, 1U);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    if (identity != XY_ICM20608_WHO_AM_I) {
        return XY_DEVICE_NOT_FOUND;
    }

    result = icm20608_write(dev, XY_ICM20608_REG_PWR_MGMT_1, 0x80U);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    (void)xy_device_delay_ms(100U);
    result = icm20608_write(dev, XY_ICM20608_REG_PWR_MGMT_1, 0x01U);
    if (result == XY_DEVICE_OK) {
        result = icm20608_write(dev, XY_ICM20608_REG_PWR_MGMT_2, 0x00U);
    }
    if (result == XY_DEVICE_OK) {
        result = icm20608_write(dev, XY_ICM20608_REG_GYRO_CONFIG, 0x08U);
    }
    if (result == XY_DEVICE_OK) {
        result = icm20608_write(dev, XY_ICM20608_REG_ACCEL_CONFIG, 0x08U);
    }
    if (result == XY_DEVICE_OK) {
        result = icm20608_write(dev, XY_ICM20608_REG_CONFIG, 0x04U);
    }
    if (result == XY_DEVICE_OK) {
        result = icm20608_write(dev, XY_ICM20608_REG_ACCEL_CONFIG2, 0x04U);
    }
    return result;
}

xy_error_t xy_icm20608_init_i2c(xy_icm20608_t *dev, void *i2c_handle, uint8_t address)
{
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL ||
        (address != XY_ICM20608_ADDR_DEFAULT && address != XY_ICM20608_ADDR_ALT)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, address, 100U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    dev->transport = XY_ICM20608_TRANSPORT_I2C;
    dev->address = address;
    result = icm20608_configure(dev);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_init_spi(xy_icm20608_t *dev, void *context,
                                xy_icm20608_spi_read_t read_fn,
                                xy_icm20608_spi_write_t write_fn)
{
    xy_error_t result;

    if (dev == NULL || context == NULL || read_fn == NULL || write_fn == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    dev->transport = XY_ICM20608_TRANSPORT_SPI;
    dev->spi_context = context;
    dev->spi_read = read_fn;
    dev->spi_write = write_fn;
    result = icm20608_configure(dev);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_deinit(xy_icm20608_t *dev)
{
    xy_error_t result;

    if (!icm20608_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_write(dev, XY_ICM20608_REG_PWR_MGMT_1, 0x40U);
    if (result == XY_DEVICE_OK) {
        dev->initialized = 0U;
        if (dev->transport == XY_ICM20608_TRANSPORT_I2C) {
            dev->i2c_dev.base.initialized = 0U;
            dev->i2c_dev.i2c_handle = NULL;
        } else {
            dev->spi_context = NULL;
            dev->spi_read = NULL;
            dev->spi_write = NULL;
        }
    }
    return result;
}

xy_error_t xy_icm20608_read_accel(xy_icm20608_t *dev, xy_icm20608_accel_t *accel)
{
    uint8_t data[6];
    int16_t raw[3];
    xy_icm20608_accel_t next;
    xy_error_t result;

    if (!icm20608_ready(dev) || accel == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_ACCEL_XOUT_H, data, sizeof(data));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    raw[0] = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    raw[1] = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    raw[2] = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    next.x_mg = (int32_t)raw[0] * 4000 / 32768;
    next.y_mg = (int32_t)raw[1] * 4000 / 32768;
    next.z_mg = (int32_t)raw[2] * 4000 / 32768;
    dev->accel = next;
    *accel = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_read_gyro(xy_icm20608_t *dev, xy_icm20608_gyro_t *gyro)
{
    uint8_t data[6];
    int16_t raw[3];
    xy_icm20608_gyro_t next;
    xy_error_t result;

    if (!icm20608_ready(dev) || gyro == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_GYRO_XOUT_H, data, sizeof(data));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    raw[0] = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    raw[1] = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    raw[2] = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    next.x_mdps = (int32_t)raw[0] * 500000 / 32768;
    next.y_mdps = (int32_t)raw[1] * 500000 / 32768;
    next.z_mdps = (int32_t)raw[2] * 500000 / 32768;
    dev->gyro = next;
    *gyro = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_read_temperature(xy_icm20608_t *dev, int32_t *temperature_centi_c)
{
    uint8_t data[2];
    int16_t raw;
    int32_t next;
    xy_error_t result;

    if (!icm20608_ready(dev) || temperature_centi_c == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_TEMP_OUT_H, data, sizeof(data));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    next = ((int32_t)raw * 1000 / 3268) + 2500;
    dev->temperature_centi_c = next;
    *temperature_centi_c = next;
    return XY_DEVICE_OK;
}
