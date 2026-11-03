#include "xy_sc7a20.h"
#include "xy_hal_sys.h"

#include <string.h>

static int sc7a20_transport_ready(const xy_sc7a20_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int sc7a20_ready(const xy_sc7a20_t *dev)
{
    return sc7a20_transport_ready(dev) && dev->initialized;
}

static xy_error_t sc7a20_read_reg(xy_sc7a20_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    if (!sc7a20_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
}

static xy_error_t sc7a20_write_u8(xy_sc7a20_t *dev, uint8_t reg, uint8_t value)
{
    if (!sc7a20_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

/* CTRL_REG4 carries the range in bits [5:4] and the block-update flag in bit 3. */
static uint8_t sc7a20_range_reg(uint8_t range_g)
{
    switch (range_g) {
    case 4:
        return XY_SC7A20_RANGE_4G;
    case 8:
        return XY_SC7A20_RANGE_8G;
    case 16:
        return XY_SC7A20_RANGE_16G;
    case 2:
    default:
        return XY_SC7A20_RANGE_2G;
    }
}

xy_error_t xy_sc7a20_init(xy_sc7a20_t *dev, void *i2c_handle)
{
    xy_sc7a20_t candidate;
    const int preserve_live_owner = sc7a20_ready(dev) && dev->i2c_dev.dev_addr == XY_SC7A20_ADDR;
    uint8_t whoami;
    xy_error_t ret;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&candidate, 0, sizeof(candidate));
    ret = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, XY_SC7A20_ADDR, 1000U);
    if (ret != XY_DEVICE_OK || !sc7a20_transport_ready(&candidate)) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }

    ret = sc7a20_read_reg(&candidate, XY_SC7A20_REG_WHOAMI, &whoami, 1U);
    if (ret != XY_DEVICE_OK || whoami != XY_SC7A20_WHOAMI_VALUE) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_NOT_FOUND;
    }

    /* CTRL_REG1: 100 Hz output data rate, XYZ enabled. CTRL_REG4: +/-2 g with block update. */
    candidate.odr_reg = (uint8_t)(XY_SC7A20_ODR_100HZ | XY_SC7A20_AXIS_ENABLE_MASK);
    ret = sc7a20_write_u8(&candidate, XY_SC7A20_REG_CTRL_REG1, candidate.odr_reg);
    if (ret == XY_DEVICE_OK) {
        ret = sc7a20_write_u8(&candidate, XY_SC7A20_REG_CTRL_REG4,
                              (uint8_t)(XY_SC7A20_RANGE_2G | XY_SC7A20_BDU_MASK));
    }
    if (ret != XY_DEVICE_OK) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret;
    }

    candidate.range_g = 2U;
    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a20_deinit(xy_sc7a20_t *dev)
{
    xy_error_t ret;

    if (!sc7a20_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = sc7a20_write_u8(dev, XY_SC7A20_REG_CTRL_REG1, XY_SC7A20_ODR_POWER_DOWN);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    dev->initialized = 0U;
    dev->odr_reg = XY_SC7A20_ODR_POWER_DOWN;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a20_read(xy_sc7a20_t *dev, xy_sc7a20_data_t *data)
{
    uint8_t bytes[6];
    xy_sc7a20_data_t next;
    xy_error_t ret;

    if (!sc7a20_ready(dev) || data == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = sc7a20_read_reg(dev, XY_SC7A20_REG_OUT_X_L, bytes, sizeof(bytes));
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    /* 12-bit left-aligned: shift each little-endian 16-bit word right by 4 to get the signed value. */
    next.x = (int16_t)((int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8)) >> 4);
    next.y = (int16_t)((int16_t)((uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8)) >> 4);
    next.z = (int16_t)((int16_t)((uint16_t)bytes[4] | ((uint16_t)bytes[5] << 8)) >> 4);
    next.timestamp = xy_hal_sys_get_tick_count();
    *data = next;
    dev->data = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a20_read_accel(xy_sc7a20_t *dev, xy_sc7a20_accel_t *accel)
{
    xy_sc7a20_data_t raw;
    xy_error_t ret;

    if (!sc7a20_ready(dev) || accel == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = xy_sc7a20_read(dev, &raw);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    /* Full scale is range_g g across 2048 counts, so one count is range_g * 1000 / 2048 milli-g.
     * The multiply happens before the divide: computing the per-LSB sensitivity first truncates to
     * zero at +/-2 g (2000 / 2048 == 0), which is exactly what the legacy owner did - every reading
     * it produced was 0 mg. Scaling the count directly keeps the fractional part. */
    const int32_t scale_mg = ((int32_t)dev->range_g * 1000);
    accel->x_mg = ((int32_t)raw.x * scale_mg) / XY_SC7A20_FULL_SCALE_LSB;
    accel->y_mg = ((int32_t)raw.y * scale_mg) / XY_SC7A20_FULL_SCALE_LSB;
    accel->z_mg = ((int32_t)raw.z * scale_mg) / XY_SC7A20_FULL_SCALE_LSB;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a20_set_range(xy_sc7a20_t *dev, uint8_t range_g)
{
    xy_error_t ret;

    if (!sc7a20_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    if (range_g != 2U && range_g != 4U && range_g != 8U && range_g != 16U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = sc7a20_write_u8(dev, XY_SC7A20_REG_CTRL_REG4,
                          (uint8_t)(sc7a20_range_reg(range_g) | XY_SC7A20_BDU_MASK));
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    dev->range_g = range_g;
    return XY_DEVICE_OK;
}

xy_error_t xy_sc7a20_set_odr(xy_sc7a20_t *dev, uint8_t odr_reg)
{
    uint8_t value;
    xy_error_t ret;

    if (!sc7a20_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    switch (odr_reg) {
    case XY_SC7A20_ODR_1HZ:
    case XY_SC7A20_ODR_10HZ:
    case XY_SC7A20_ODR_25HZ:
    case XY_SC7A20_ODR_50HZ:
    case XY_SC7A20_ODR_100HZ:
    case XY_SC7A20_ODR_200HZ:
    case XY_SC7A20_ODR_400HZ:
        break;
    default:
        return XY_DEVICE_INVALID_PARAM;
    }
    value = (uint8_t)(odr_reg | XY_SC7A20_AXIS_ENABLE_MASK);
    ret = sc7a20_write_u8(dev, XY_SC7A20_REG_CTRL_REG1, value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    dev->odr_reg = value;
    return XY_DEVICE_OK;
}
