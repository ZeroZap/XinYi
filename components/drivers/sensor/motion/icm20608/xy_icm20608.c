#include "xy_icm20608.h"
#include "xy_device_timing.h"

#include <string.h>

static int icm20608_ready(const xy_icm20608_t *dev)
{
    if (dev == NULL || dev->initialized == 0U || dev->configuration_synchronized == 0U) {
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

static xy_error_t icm20608_update_bits(xy_icm20608_t *dev, uint8_t reg, uint8_t mask,
                                       uint8_t value)
{
    uint8_t current;
    xy_error_t result = icm20608_read(dev, reg, &current, 1U);

    if (result != XY_DEVICE_OK) return result;
    current = (uint8_t)((current & (uint8_t)~mask) | (value & mask));
    return icm20608_write(dev, reg, current);
}

static int32_t accel_full_scale_mg(xy_icm20608_accel_range_t range)
{
    return 2000 << (uint8_t)range;
}

static int32_t gyro_full_scale_mdps(xy_icm20608_gyro_range_t range)
{
    return 250000 << (uint8_t)range;
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
        result = icm20608_write(dev, XY_ICM20608_REG_SMPLRT_DIV, 0x09U);
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
    xy_icm20608_t candidate;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL ||
        (address != XY_ICM20608_ADDR_DEFAULT && address != XY_ICM20608_ADDR_ALT)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&candidate, 0, sizeof(candidate));
    result = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, address, 100U);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    candidate.transport = XY_ICM20608_TRANSPORT_I2C;
    candidate.address = address;
    result = icm20608_configure(&candidate);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    candidate.accel_range = XY_ICM20608_ACCEL_RANGE_4G;
    candidate.gyro_range = XY_ICM20608_GYRO_RANGE_500DPS;
    candidate.gyro_dlpf = XY_ICM20608_DLPF_20HZ;
    candidate.accel_dlpf = XY_ICM20608_DLPF_20HZ;
    candidate.odr_hz = 100U;
    candidate.configuration_synchronized = 1U;
    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_init_spi(xy_icm20608_t *dev, void *context,
                                xy_icm20608_spi_read_t read_fn,
                                xy_icm20608_spi_write_t write_fn)
{
    xy_icm20608_t candidate;
    xy_error_t result;

    if (dev == NULL || context == NULL || read_fn == NULL || write_fn == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&candidate, 0, sizeof(candidate));
    candidate.transport = XY_ICM20608_TRANSPORT_SPI;
    candidate.spi_context = context;
    candidate.spi_read = read_fn;
    candidate.spi_write = write_fn;
    result = icm20608_configure(&candidate);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    candidate.accel_range = XY_ICM20608_ACCEL_RANGE_4G;
    candidate.gyro_range = XY_ICM20608_GYRO_RANGE_500DPS;
    candidate.gyro_dlpf = XY_ICM20608_DLPF_20HZ;
    candidate.accel_dlpf = XY_ICM20608_DLPF_20HZ;
    candidate.odr_hz = 100U;
    candidate.configuration_synchronized = 1U;
    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_set_accel_range(xy_icm20608_t *dev,
                                       xy_icm20608_accel_range_t range)
{
    xy_error_t result;

    if (!icm20608_ready(dev) || range > XY_ICM20608_ACCEL_RANGE_16G) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_update_bits(dev, XY_ICM20608_REG_ACCEL_CONFIG, 0x18U,
                                  (uint8_t)range << 3U);
    if (result == XY_DEVICE_OK) dev->accel_range = range;
    return result;
}

xy_error_t xy_icm20608_set_gyro_range(xy_icm20608_t *dev,
                                      xy_icm20608_gyro_range_t range)
{
    xy_error_t result;

    if (!icm20608_ready(dev) || range > XY_ICM20608_GYRO_RANGE_2000DPS) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_update_bits(dev, XY_ICM20608_REG_GYRO_CONFIG, 0x18U,
                                  (uint8_t)range << 3U);
    if (result == XY_DEVICE_OK) dev->gyro_range = range;
    return result;
}

xy_error_t xy_icm20608_set_odr(xy_icm20608_t *dev, uint16_t odr_hz)
{
    uint16_t divider;
    xy_error_t result;

    if (!icm20608_ready(dev) || odr_hz == 0U || odr_hz > 1000U ||
        (1000U % odr_hz) != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    divider = (uint16_t)(1000U / odr_hz) - 1U;
    if (divider > UINT8_MAX) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_write(dev, XY_ICM20608_REG_SMPLRT_DIV, (uint8_t)divider);
    if (result == XY_DEVICE_OK) dev->odr_hz = odr_hz;
    return result;
}

xy_error_t xy_icm20608_set_dlpf(xy_icm20608_t *dev, xy_icm20608_dlpf_t gyro_dlpf,
                                xy_icm20608_dlpf_t accel_dlpf)
{
    uint8_t gyro_current;
    uint8_t accel_current;
    xy_error_t result;

    if (!icm20608_ready(dev) || gyro_dlpf > XY_ICM20608_DLPF_5HZ ||
        accel_dlpf > XY_ICM20608_DLPF_5HZ) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_CONFIG, &gyro_current, 1U);
    if (result != XY_DEVICE_OK) return result;
    result = icm20608_read(dev, XY_ICM20608_REG_ACCEL_CONFIG2, &accel_current, 1U);
    if (result != XY_DEVICE_OK) return result;

    result = icm20608_write(dev, XY_ICM20608_REG_CONFIG,
                            (uint8_t)((gyro_current & 0xF8U) | (uint8_t)gyro_dlpf));
    if (result != XY_DEVICE_OK) return result;
    result = icm20608_write(dev, XY_ICM20608_REG_ACCEL_CONFIG2,
                            (uint8_t)((accel_current & 0xF8U) | (uint8_t)accel_dlpf));
    if (result != XY_DEVICE_OK) {
        if (icm20608_write(dev, XY_ICM20608_REG_CONFIG, gyro_current) != XY_DEVICE_OK) {
            dev->configuration_synchronized = 0U;
        }
        return result;
    }

    dev->gyro_dlpf = gyro_dlpf;
    dev->accel_dlpf = accel_dlpf;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_set_bias(xy_icm20608_t *dev,
                                const xy_icm20608_accel_t *accel_bias,
                                const xy_icm20608_gyro_t *gyro_bias)
{
    int32_t accel_limit;
    int32_t gyro_limit;

    if (!icm20608_ready(dev) || accel_bias == NULL || gyro_bias == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    accel_limit = accel_full_scale_mg(dev->accel_range);
    gyro_limit = gyro_full_scale_mdps(dev->gyro_range);
    if (accel_bias->x_mg < -accel_limit || accel_bias->x_mg > accel_limit ||
        accel_bias->y_mg < -accel_limit || accel_bias->y_mg > accel_limit ||
        accel_bias->z_mg < -accel_limit || accel_bias->z_mg > accel_limit ||
        gyro_bias->x_mdps < -gyro_limit || gyro_bias->x_mdps > gyro_limit ||
        gyro_bias->y_mdps < -gyro_limit || gyro_bias->y_mdps > gyro_limit ||
        gyro_bias->z_mdps < -gyro_limit || gyro_bias->z_mdps > gyro_limit) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->accel_bias = *accel_bias;
    dev->gyro_bias = *gyro_bias;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_set_sleep(xy_icm20608_t *dev, uint8_t sleep)
{
    xy_error_t result;

    if (!icm20608_ready(dev) || sleep > 1U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_update_bits(dev, XY_ICM20608_REG_PWR_MGMT_1, 0x40U,
                                  sleep != 0U ? 0x40U : 0x00U);
    if (result == XY_DEVICE_OK) {
        if (sleep == 0U && dev->sleeping != 0U) {
            (void)xy_device_delay_ms(35U);
        }
        dev->sleeping = sleep;
    }
    return result;
}

xy_error_t xy_icm20608_set_data_ready_interrupt(xy_icm20608_t *dev, uint8_t enable)
{
    xy_error_t result;

    if (!icm20608_ready(dev) || enable > 1U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_update_bits(dev, XY_ICM20608_REG_INT_ENABLE, 0x01U, enable);
    if (result == XY_DEVICE_OK) dev->data_ready_interrupt_enabled = enable;
    return result;
}

xy_error_t xy_icm20608_read_interrupt_status(xy_icm20608_t *dev, uint8_t *status)
{
    uint8_t next;
    xy_error_t result;

    if (!icm20608_ready(dev) || status == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_INT_STATUS, &next, 1U);
    if (result == XY_DEVICE_OK) *status = next;
    return result;
}

xy_error_t xy_icm20608_deinit(xy_icm20608_t *dev)
{
    xy_error_t result;

    if (!icm20608_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_update_bits(dev, XY_ICM20608_REG_PWR_MGMT_1, 0x40U, 0x40U);
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

xy_error_t xy_icm20608_read_sample(xy_icm20608_t *dev, xy_icm20608_sample_t *sample)
{
    uint8_t data[14];
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    int16_t temperature_raw;
    xy_icm20608_sample_t next;
    xy_error_t result;

    if (!icm20608_ready(dev) || sample == NULL || dev->sleeping != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_ACCEL_XOUT_H, data, sizeof(data));
    if (result != XY_DEVICE_OK) return result;

    accel_raw[0] = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    accel_raw[1] = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    accel_raw[2] = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    temperature_raw = (int16_t)(((uint16_t)data[6] << 8) | data[7]);
    gyro_raw[0] = (int16_t)(((uint16_t)data[8] << 8) | data[9]);
    gyro_raw[1] = (int16_t)(((uint16_t)data[10] << 8) | data[11]);
    gyro_raw[2] = (int16_t)(((uint16_t)data[12] << 8) | data[13]);

    next.accel.x_mg = (int32_t)accel_raw[0] * accel_full_scale_mg(dev->accel_range) / 32768;
    next.accel.y_mg = (int32_t)accel_raw[1] * accel_full_scale_mg(dev->accel_range) / 32768;
    next.accel.z_mg = (int32_t)accel_raw[2] * accel_full_scale_mg(dev->accel_range) / 32768;
    next.temperature_centi_c = ((int32_t)temperature_raw * 1000 / 3268) + 2500;
    next.gyro.x_mdps =
        (int32_t)((int64_t)gyro_raw[0] * gyro_full_scale_mdps(dev->gyro_range) / 32768);
    next.gyro.y_mdps =
        (int32_t)((int64_t)gyro_raw[1] * gyro_full_scale_mdps(dev->gyro_range) / 32768);
    next.gyro.z_mdps =
        (int32_t)((int64_t)gyro_raw[2] * gyro_full_scale_mdps(dev->gyro_range) / 32768);

    next.accel.x_mg -= dev->accel_bias.x_mg;
    next.accel.y_mg -= dev->accel_bias.y_mg;
    next.accel.z_mg -= dev->accel_bias.z_mg;
    next.gyro.x_mdps -= dev->gyro_bias.x_mdps;
    next.gyro.y_mdps -= dev->gyro_bias.y_mdps;
    next.gyro.z_mdps -= dev->gyro_bias.z_mdps;

    dev->accel = next.accel;
    dev->gyro = next.gyro;
    dev->temperature_centi_c = next.temperature_centi_c;
    *sample = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_icm20608_read_accel(xy_icm20608_t *dev, xy_icm20608_accel_t *accel)
{
    uint8_t data[6];
    int16_t raw[3];
    xy_icm20608_accel_t next;
    xy_error_t result;

    if (!icm20608_ready(dev) || accel == NULL || dev->sleeping != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_ACCEL_XOUT_H, data, sizeof(data));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    raw[0] = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    raw[1] = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    raw[2] = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    next.x_mg = (int32_t)raw[0] * accel_full_scale_mg(dev->accel_range) / 32768;
    next.y_mg = (int32_t)raw[1] * accel_full_scale_mg(dev->accel_range) / 32768;
    next.z_mg = (int32_t)raw[2] * accel_full_scale_mg(dev->accel_range) / 32768;
    next.x_mg -= dev->accel_bias.x_mg;
    next.y_mg -= dev->accel_bias.y_mg;
    next.z_mg -= dev->accel_bias.z_mg;
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

    if (!icm20608_ready(dev) || gyro == NULL || dev->sleeping != 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = icm20608_read(dev, XY_ICM20608_REG_GYRO_XOUT_H, data, sizeof(data));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    raw[0] = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    raw[1] = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    raw[2] = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    next.x_mdps = (int32_t)((int64_t)raw[0] * gyro_full_scale_mdps(dev->gyro_range) / 32768);
    next.y_mdps = (int32_t)((int64_t)raw[1] * gyro_full_scale_mdps(dev->gyro_range) / 32768);
    next.z_mdps = (int32_t)((int64_t)raw[2] * gyro_full_scale_mdps(dev->gyro_range) / 32768);
    next.x_mdps -= dev->gyro_bias.x_mdps;
    next.y_mdps -= dev->gyro_bias.y_mdps;
    next.z_mdps -= dev->gyro_bias.z_mdps;
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

    if (!icm20608_ready(dev) || temperature_centi_c == NULL || dev->sleeping != 0U) {
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
