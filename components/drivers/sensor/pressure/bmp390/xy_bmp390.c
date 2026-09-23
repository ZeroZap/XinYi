#include "xy_bmp390.h"

#include "xy_hal_delay.h"
#include <string.h>

static int bmp390_transport_ready(const xy_bmp390_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized != 0U &&
           dev->i2c_dev.i2c_handle != NULL;
}

static BMP3_INTF_RET_TYPE bmp390_bus_read(uint8_t reg, uint8_t *data, uint32_t length,
                                          void *context)
{
    xy_bmp390_t *dev = context;

    if (!bmp390_transport_ready(dev)) {
        return -1;
    }

    dev->transport_error = xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
    return dev->transport_error == XY_DEVICE_OK ? BMP3_INTF_RET_SUCCESS : -1;
}

static BMP3_INTF_RET_TYPE bmp390_bus_write(uint8_t reg, const uint8_t *data, uint32_t length,
                                           void *context)
{
    xy_bmp390_t *dev = context;

    if (!bmp390_transport_ready(dev)) {
        return -1;
    }

    dev->transport_error = xy_i2c_device_write_reg(&dev->i2c_dev, reg, data, length);
    return dev->transport_error == XY_DEVICE_OK ? BMP3_INTF_RET_SUCCESS : -1;
}

static void bmp390_delay_us(uint32_t us, void *context)
{
    (void)context;
    xy_hal_delay_ms((us + 999U) / 1000U);
}

static xy_error_t bmp390_map_error(const xy_bmp390_t *dev, int8_t result)
{
    if (result == BMP3_OK) {
        return XY_DEVICE_OK;
    }
    if (result == BMP3_E_COMM_FAIL && dev->transport_error != XY_DEVICE_OK) {
        return dev->transport_error;
    }
    if (result == BMP3_E_DEV_NOT_FOUND) {
        return XY_DEVICE_NOT_FOUND;
    }
    if (result == BMP3_E_NULL_PTR || result == BMP3_E_INVALID_LEN) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return XY_DEVICE_IO_ERROR;
}

static xy_error_t bmp390_fail(xy_bmp390_t *dev, xy_error_t result)
{
    memset(dev, 0, sizeof(*dev));
    return result;
}

static int bmp390_ready(const xy_bmp390_t *dev)
{
    return dev != NULL && dev->initialized != 0U && dev->i2c_dev.base.initialized != 0U &&
           dev->i2c_dev.i2c_handle != NULL;
}

xy_error_t xy_bmp390_init(xy_bmp390_t *dev, void *i2c_handle, uint8_t addr)
{
    uint32_t settings_select;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL ||
        (addr != XY_BMP390_ADDR_PRIMARY && addr != XY_BMP390_ADDR_SECONDARY)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000U);
    if (result != XY_DEVICE_OK) {
        return bmp390_fail(dev, result);
    }

    dev->bosch.intf = BMP3_I2C_INTF;
    dev->bosch.intf_ptr = dev;
    dev->bosch.read = bmp390_bus_read;
    dev->bosch.write = bmp390_bus_write;
    dev->bosch.delay_us = bmp390_delay_us;

    dev->transport_error = XY_DEVICE_OK;
    result = bmp390_map_error(dev, bmp3_init(&dev->bosch));
    if (result != XY_DEVICE_OK) {
        return bmp390_fail(dev, result);
    }
    if (dev->bosch.chip_id != BMP390_CHIP_ID) {
        return bmp390_fail(dev, XY_DEVICE_NOT_FOUND);
    }

    dev->settings.press_en = BMP3_ENABLE;
    dev->settings.temp_en = BMP3_ENABLE;
    dev->settings.odr_filter.press_os = BMP3_OVERSAMPLING_4X;
    dev->settings.odr_filter.temp_os = BMP3_OVERSAMPLING_2X;
    dev->settings.odr_filter.odr = BMP3_ODR_25_HZ;
    dev->settings.odr_filter.iir_filter = BMP3_IIR_FILTER_COEFF_3;
    settings_select = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS |
                      BMP3_SEL_TEMP_OS | BMP3_SEL_ODR | BMP3_SEL_IIR_FILTER;
    dev->transport_error = XY_DEVICE_OK;
    result = bmp390_map_error(
        dev, bmp3_set_sensor_settings(settings_select, &dev->settings, &dev->bosch));
    if (result != XY_DEVICE_OK) {
        return bmp390_fail(dev, result);
    }

    dev->settings.op_mode = BMP3_MODE_NORMAL;
    dev->transport_error = XY_DEVICE_OK;
    result = bmp390_map_error(dev, bmp3_set_op_mode(&dev->settings, &dev->bosch));
    if (result != XY_DEVICE_OK) {
        return bmp390_fail(dev, result);
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_bmp390_deinit(xy_bmp390_t *dev)
{
    struct bmp3_settings next_settings;
    xy_error_t result;

    if (!bmp390_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    next_settings = dev->settings;
    next_settings.op_mode = BMP3_MODE_SLEEP;
    dev->transport_error = XY_DEVICE_OK;
    result = bmp390_map_error(dev, bmp3_set_op_mode(&next_settings, &dev->bosch));
    if (result == XY_DEVICE_OK) {
        dev->settings = next_settings;
        dev->initialized = 0U;
        dev->i2c_dev.base.initialized = 0U;
        dev->i2c_dev.i2c_handle = NULL;
    }
    return result;
}

xy_error_t xy_bmp390_read(xy_bmp390_t *dev, xy_bmp390_data_t *output)
{
    struct bmp3_data bosch_data = {0};
    xy_bmp390_data_t next;
    xy_error_t result;

    if (!bmp390_ready(dev) || output == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    dev->transport_error = XY_DEVICE_OK;
    result = bmp390_map_error(dev, bmp3_get_sensor_data(BMP3_PRESS_TEMP, &bosch_data, &dev->bosch));
    if (result != XY_DEVICE_OK) {
        return result;
    }

#ifdef BMP3_FLOAT_COMPENSATION
    next.temperature_centi_c = (int64_t)(bosch_data.temperature * 100.0);
    next.pressure_centi_pa = (uint64_t)(bosch_data.pressure * 100.0);
#else
    next.temperature_centi_c = bosch_data.temperature;
    next.pressure_centi_pa = bosch_data.pressure;
#endif
    dev->data = next;
    *output = next;
    return XY_DEVICE_OK;
}
