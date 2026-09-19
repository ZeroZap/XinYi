#include "xy_aht30.h"
#include "xy_hal_delay.h"

#include <string.h>

static uint8_t crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0xFFU;

    while (length-- != 0U) {
        crc ^= *data++;
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U ? (uint8_t)((crc << 1) ^ 0x31U)
                                      : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

xy_error_t xy_aht30_init(xy_aht30_t *dev, void *i2c_handle)
{
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_AHT30_ADDR, 100U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    xy_hal_delay_ms(5U);
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_aht30_deinit(xy_aht30_t *dev)
{
    if (dev == NULL || dev->initialized == 0U || dev->i2c_dev.base.initialized == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    return XY_DEVICE_OK;
}

xy_error_t xy_aht30_read(xy_aht30_t *dev, xy_aht30_data_t *out)
{
    static const uint8_t command[] = {0xACU, 0x33U, 0x00U};
    uint8_t frame[7];
    xy_aht30_data_t value;
    xy_error_t result;
    uint32_t humidity_raw;
    uint32_t temperature_raw;

    if (dev == NULL || out == NULL || dev->initialized == 0U ||
        dev->i2c_dev.base.initialized == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = xy_i2c_device_write(&dev->i2c_dev, command, sizeof(command));
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
    if ((frame[0] & 0x18U) != 0x18U || crc8(frame, 6U) != frame[6]) {
        return XY_HAL_ERROR_CRC;
    }

    humidity_raw = ((uint32_t)frame[1] << 12) | ((uint32_t)frame[2] << 4) |
                   ((uint32_t)frame[3] >> 4);
    temperature_raw = ((uint32_t)(frame[3] & 0x0FU) << 16) |
                      ((uint32_t)frame[4] << 8) | frame[5];
    value.humidity_centi_pct = (uint32_t)(((uint64_t)humidity_raw * 10000U) >> 20);
    value.temperature_centi_c =
        (int32_t)(((uint64_t)temperature_raw * 20000U) >> 20) - 5000;

    dev->data = value;
    *out = value;
    return XY_DEVICE_OK;
}
