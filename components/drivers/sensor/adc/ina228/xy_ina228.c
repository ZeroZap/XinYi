#include "xy_ina228.h"

#include <string.h>

static int ina228_transport_ready(const xy_ina228_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int ina228_ready(const xy_ina228_t *dev)
{
    return ina228_transport_ready(dev) && dev->initialized != 0U && dev->core.initialized != 0U;
}

static int ina228_read(void *context, uint8_t reg, uint8_t *data, uint8_t len)
{
    xy_ina228_t *dev = context;
    if (!ina228_transport_ready(dev) || data == NULL || len == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
}

static int ina228_write16(void *context, uint8_t reg, uint16_t value)
{
    xy_ina228_t *dev = context;
    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    if (!ina228_transport_ready(dev)) return XY_DEVICE_INVALID_PARAM;
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, data, sizeof(data));
}

static int ina228_read16(xy_ina228_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int result = ina228_read(dev, reg, data, sizeof(data));
    if (result == XY_DEVICE_OK) *value = ((uint16_t)data[0] << 8) | data[1];
    return result;
}

int xy_ina228_init(xy_ina228_t *dev, void *i2c_handle, uint8_t address,
                   const xy_ina22x_config_t *config)
{
    xy_ina228_t next = {0};
    uint16_t manufacturer;
    uint16_t device_id;
    int result;

    if (dev == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    if (i2c_handle == NULL || address < XY_INA228_ADDR_MIN ||
        address > XY_INA228_ADDR_MAX ||
        xy_ina22x_core_config_valid(config, &next.core.shunt_cal) != XY_DEVICE_OK) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_i2c_device_init(&next.i2c_dev, i2c_handle, address, 1000U);
    if (result != XY_DEVICE_OK || !ina228_transport_ready(&next)) return result != XY_DEVICE_OK ? result : XY_DEVICE_NOT_INIT;
    next.address = address;
    next.core.config = *config;
    next.core.transport.read = ina228_read;
    next.core.transport.write16 = ina228_write16;
    next.core.transport.context = &next;
    result = ina228_read16(&next, XY_INA22X_REG_MANUFACTURER, &manufacturer);
    if (result != XY_DEVICE_OK) return result;
    result = ina228_read16(&next, XY_INA22X_REG_DEVICE_ID, &device_id);
    if (result != XY_DEVICE_OK) return result;
    if (manufacturer != XY_INA22X_MANUFACTURER_ID || (device_id >> 4U) != XY_INA228_DIE_ID) {
        return XY_DEVICE_NOT_FOUND;
    }
    result = xy_ina22x_core_configure(&next.core);
    if (result != XY_DEVICE_OK) return result;
    next.core.initialized = 1U;
    next.initialized = 1U;
    *dev = next;
    dev->core.transport.context = dev;
    return XY_DEVICE_OK;
}

int xy_ina228_deinit(xy_ina228_t *dev)
{
    int result;
    if (!ina228_ready(dev)) return XY_DEVICE_INVALID_PARAM;
    result = xy_ina22x_core_shutdown(&dev->core);
    if (result != XY_DEVICE_OK) return result;
    memset(dev, 0, sizeof(*dev));
    return XY_DEVICE_OK;
}

int xy_ina228_read(xy_ina228_t *dev, xy_ina22x_sample_t *sample)
{
    int result;
    if (!ina228_ready(dev) || sample == NULL) return XY_DEVICE_INVALID_PARAM;
    result = xy_ina22x_core_read(&dev->core);
    if (result == XY_DEVICE_OK) *sample = dev->core.sample;
    return result;
}
