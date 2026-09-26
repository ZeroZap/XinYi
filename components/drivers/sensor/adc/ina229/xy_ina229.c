#include "xy_ina229.h"

#include <string.h>

#define INA229_SPI_SPEED_HZ 10000000U
#define INA229_SPI_MODE 1U

static int ina229_transport_ready(const xy_ina229_t *dev)
{
    return dev != NULL && dev->spi_dev.base.initialized && dev->spi_dev.spi_handle != NULL &&
           dev->spi_dev.cs_pin != NULL;
}

static int ina229_ready(const xy_ina229_t *dev)
{
    return ina229_transport_ready(dev) && dev->initialized != 0U && dev->core.initialized != 0U;
}

static int ina229_transfer(xy_ina229_t *dev, const uint8_t *tx, uint8_t *rx, uint8_t len)
{
    int result;
    if (!ina229_transport_ready(dev) || tx == NULL || rx == NULL || len == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_spi_device_transfer(&dev->spi_dev, tx, rx, len);
    if (result == (int)len) return XY_DEVICE_OK;
    if (result < 0) return result;
    return XY_DEVICE_IO_ERROR;
}

static int ina229_read(void *context, uint8_t reg, uint8_t *data, uint8_t len)
{
    xy_ina229_t *dev = context;
    uint8_t tx[6] = {0};
    uint8_t rx[6] = {0};
    int result;
    if (data == NULL || len == 0U || len > 5U) return XY_DEVICE_INVALID_PARAM;
    tx[0] = (uint8_t)((reg << 2U) | 0x01U);
    result = ina229_transfer(dev, tx, rx, (uint8_t)(len + 1U));
    if (result == XY_DEVICE_OK) memcpy(data, &rx[1], len);
    return result;
}

static int ina229_write16(void *context, uint8_t reg, uint16_t value)
{
    xy_ina229_t *dev = context;
    uint8_t tx[3] = {(uint8_t)(reg << 2U), (uint8_t)(value >> 8), (uint8_t)value};
    uint8_t rx[3] = {0};
    return ina229_transfer(dev, tx, rx, sizeof(tx));
}

static int ina229_read16(xy_ina229_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int result = ina229_read(dev, reg, data, sizeof(data));
    if (result == XY_DEVICE_OK) *value = ((uint16_t)data[0] << 8) | data[1];
    return result;
}

int xy_ina229_init(xy_ina229_t *dev, void *spi_handle, void *cs_pin,
                   const xy_ina22x_config_t *config)
{
    xy_ina229_t next = {0};
    uint16_t manufacturer;
    uint16_t device_id;
    int result;

    if (dev == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    if (spi_handle == NULL || cs_pin == NULL ||
        xy_ina22x_core_config_valid(config, &next.core.shunt_cal) != XY_DEVICE_OK) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_spi_device_init(&next.spi_dev, spi_handle, cs_pin,
                                INA229_SPI_SPEED_HZ, INA229_SPI_MODE);
    if (result != XY_DEVICE_OK || !ina229_transport_ready(&next)) return result != XY_DEVICE_OK ? result : XY_DEVICE_NOT_INIT;
    next.core.config = *config;
    next.core.transport.read = ina229_read;
    next.core.transport.write16 = ina229_write16;
    next.core.transport.context = &next;
    result = ina229_read16(&next, XY_INA22X_REG_MANUFACTURER, &manufacturer);
    if (result != XY_DEVICE_OK) return result;
    result = ina229_read16(&next, XY_INA22X_REG_DEVICE_ID, &device_id);
    if (result != XY_DEVICE_OK) return result;
    if (manufacturer != XY_INA22X_MANUFACTURER_ID || (device_id >> 4U) != XY_INA229_DIE_ID) {
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

int xy_ina229_deinit(xy_ina229_t *dev)
{
    int result;
    if (!ina229_ready(dev)) return XY_DEVICE_INVALID_PARAM;
    result = xy_ina22x_core_shutdown(&dev->core);
    if (result != XY_DEVICE_OK) return result;
    memset(dev, 0, sizeof(*dev));
    return XY_DEVICE_OK;
}

int xy_ina229_read(xy_ina229_t *dev, xy_ina22x_sample_t *sample)
{
    int result;
    if (!ina229_ready(dev) || sample == NULL) return XY_DEVICE_INVALID_PARAM;
    result = xy_ina22x_core_read(&dev->core);
    if (result == XY_DEVICE_OK) *sample = dev->core.sample;
    return result;
}
