#include "xy_aht10.h"
#include "xy_hal_delay.h"

#include <string.h>

static const uint8_t g_aht10_init_command[] = {0xE1U, 0x08U, 0x00U};
static const uint8_t g_aht10_measure_command[] = {0xACU, 0x33U, 0x00U};

xy_error_t xy_aht10_init(xy_aht10_t *dev, void *i2c_handle, uint8_t address)
{
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL ||
        (address != 0U && address != XY_AHT10_DEFAULT_ADDRESS)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_AHT10_DEFAULT_ADDRESS, 100U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    result = xy_i2c_device_write(&dev->i2c_dev, g_aht10_init_command,
                                 sizeof(g_aht10_init_command));
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    xy_hal_delay_ms(10U);
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_aht10_deinit(xy_aht10_t *dev)
{
    if (dev == NULL || dev->initialized == 0U || dev->i2c_dev.base.initialized == 0U ||
        dev->i2c_dev.i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_aht10_read(xy_aht10_t *dev, xy_aht10_data_t *out)
{
    uint8_t frame[6];
    uint32_t humidity_raw;
    uint32_t temperature_raw;
    xy_aht10_data_t next;
    xy_error_t result;

    if (dev == NULL || out == NULL || dev->initialized == 0U ||
        dev->i2c_dev.base.initialized == 0U || dev->i2c_dev.i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = xy_i2c_device_write(&dev->i2c_dev, g_aht10_measure_command,
                                 sizeof(g_aht10_measure_command));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    xy_hal_delay_ms(80U);

    result = xy_i2c_device_read(&dev->i2c_dev, frame, sizeof(frame));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    if ((frame[0] & 0x80U) != 0U) {
        return XY_DEVICE_BUSY;
    }

    humidity_raw = ((uint32_t)frame[1] << 12) | ((uint32_t)frame[2] << 4) |
                   ((uint32_t)frame[3] >> 4);
    temperature_raw = ((uint32_t)(frame[3] & 0x0FU) << 16) |
                      ((uint32_t)frame[4] << 8) | frame[5];
    next.humidity_centi_pct = (uint32_t)(((uint64_t)humidity_raw * 10000U) >> 20);
    next.temperature_centi_c =
        (int32_t)(((uint64_t)temperature_raw * 20000U) >> 20) - 5000;

    dev->data = next;
    *out = next;
    return XY_DEVICE_OK;
}
