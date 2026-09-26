#include "xy_qma6100p.h"
#include "xy_device_timing.h"
#include <string.h>

static int qma_ready(const xy_qma6100p_t *dev)
{
    return dev != NULL && dev->initialized != 0U && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static xy_error_t qma_read(xy_qma6100p_t *dev, uint8_t reg, uint8_t *data, size_t length)
{
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
}

static xy_error_t qma_write(xy_qma6100p_t *dev, uint8_t reg, uint8_t value)
{
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

static int16_t decode_axis(uint8_t lsb, uint8_t msb)
{
    uint16_t packed = (uint16_t)(((uint16_t)msb << 8) | lsb);
    return (int16_t)packed / 4;
}

xy_error_t xy_qma6100p_init(xy_qma6100p_t *dev, void *i2c_handle, uint8_t address)
{
    uint8_t id;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL ||
        (address != XY_QMA6100P_ADDR_LOW && address != XY_QMA6100P_ADDR_HIGH)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, address, 100U);
    if (result != XY_DEVICE_OK || !dev->i2c_dev.base.initialized ||
        dev->i2c_dev.i2c_handle == NULL) {
        memset(dev, 0, sizeof(*dev));
        return result != XY_DEVICE_OK ? result : XY_DEVICE_INVALID_PARAM;
    }
    result = qma_read(dev, XY_QMA6100P_REG_CHIP_ID, &id, 1U);
    if (result != XY_DEVICE_OK || id != XY_QMA6100P_CHIP_ID) {
        memset(dev, 0, sizeof(*dev));
        return result != XY_DEVICE_OK ? result : XY_DEVICE_NOT_FOUND;
    }
    result = qma_write(dev, XY_QMA6100P_REG_RANGE, XY_QMA6100P_RANGE_2G);
    if (result == XY_DEVICE_OK)
        result = qma_write(dev, XY_QMA6100P_REG_BW, XY_QMA6100P_BW_100HZ);
    if (result == XY_DEVICE_OK)
        result = qma_write(dev, XY_QMA6100P_REG_POWER, XY_QMA6100P_POWER_ACTIVE);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    xy_device_delay_ms(2U);
    dev->address = address;
    dev->range = XY_QMA6100P_RANGE_2G;
    dev->bandwidth = XY_QMA6100P_BW_100HZ;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_qma6100p_deinit(xy_qma6100p_t *dev)
{
    xy_error_t result;

    if (!qma_ready(dev)) return XY_DEVICE_INVALID_PARAM;
    result = qma_write(dev, XY_QMA6100P_REG_POWER, 0U);
    if (result != XY_DEVICE_OK) return result;
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_qma6100p_configure_data_ready_interrupts(xy_qma6100p_t *dev,
                                                       uint8_t int1_enable,
                                                       uint8_t int2_enable)
{
    xy_error_t result;

    if (!qma_ready(dev) || int1_enable > 1U || int2_enable > 1U ||
        (int1_enable | int2_enable) == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = qma_write(dev, XY_QMA6100P_REG_INT_PIN_CONFIG, 0x05U);
    if (result == XY_DEVICE_OK) result = qma_write(dev, XY_QMA6100P_REG_INT_CONFIG, 0x0CU);
    if (result == XY_DEVICE_OK)
        result = qma_write(dev, XY_QMA6100P_REG_INT_MAP1,
                           int1_enable != 0U ? XY_QMA6100P_DATA_READY_BIT : 0U);
    if (result == XY_DEVICE_OK)
        result = qma_write(dev, XY_QMA6100P_REG_INT_MAP3,
                           int2_enable != 0U ? XY_QMA6100P_DATA_READY_BIT : 0U);
    if (result == XY_DEVICE_OK)
        result = qma_write(dev, XY_QMA6100P_REG_INT_ENABLE1, XY_QMA6100P_DATA_READY_BIT);
    return result;
}

xy_error_t xy_qma6100p_read_interrupt_status(xy_qma6100p_t *dev, uint8_t *status)
{
    uint8_t next;
    xy_error_t result;

    if (!qma_ready(dev) || status == NULL) return XY_DEVICE_INVALID_PARAM;
    result = qma_read(dev, XY_QMA6100P_REG_INT_STATUS2, &next, 1U);
    if (result == XY_DEVICE_OK) *status = next;
    return result;
}

xy_error_t xy_qma6100p_read_raw(xy_qma6100p_t *dev, xy_qma6100p_raw_t *raw)
{
    uint8_t data[6];
    xy_qma6100p_raw_t next;
    xy_error_t result;

    if (!qma_ready(dev) || raw == NULL) return XY_DEVICE_INVALID_PARAM;
    result = qma_read(dev, XY_QMA6100P_REG_X_LSB, data, sizeof(data));
    if (result != XY_DEVICE_OK) return result;
    next.x = decode_axis(data[0], data[1]);
    next.y = decode_axis(data[2], data[3]);
    next.z = decode_axis(data[4], data[5]);
    dev->raw = next;
    *raw = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_qma6100p_read_accel(xy_qma6100p_t *dev, xy_qma6100p_accel_t *accel)
{
    xy_qma6100p_raw_t raw;
    xy_qma6100p_accel_t next;
    xy_error_t result;

    if (!qma_ready(dev) || accel == NULL) return XY_DEVICE_INVALID_PARAM;
    result = xy_qma6100p_read_raw(dev, &raw);
    if (result != XY_DEVICE_OK) return result;
    next.x_mg = ((int32_t)raw.x * 1000) / 4096;
    next.y_mg = ((int32_t)raw.y * 1000) / 4096;
    next.z_mg = ((int32_t)raw.z * 1000) / 4096;
    *accel = next;
    return XY_DEVICE_OK;
}
