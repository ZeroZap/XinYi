#include "xy_bme680.h"
#include "xy_hal_delay.h"
#include <string.h>

static BME68X_INTF_RET_TYPE bus_read(uint8_t reg, uint8_t *data, uint32_t length,
                                     void *context)
{
    xy_bme680_t *dev = context;

    dev->transport_error = xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
    return dev->transport_error == XY_DEVICE_OK ? BME68X_INTF_RET_SUCCESS : -1;
}

static BME68X_INTF_RET_TYPE bus_write(uint8_t reg, const uint8_t *data, uint32_t length,
                                      void *context)
{
    xy_bme680_t *dev = context;

    dev->transport_error = xy_i2c_device_write_reg(&dev->i2c_dev, reg, data, length);
    return dev->transport_error == XY_DEVICE_OK ? BME68X_INTF_RET_SUCCESS : -1;
}

static void delay_us(uint32_t us, void *context)
{
    (void)context;
    xy_hal_delay_ms((us + 999U) / 1000U);
}

static xy_error_t map_error(const xy_bme680_t *dev, int8_t result)
{
    if (result == BME68X_OK) {
        return XY_DEVICE_OK;
    }
    if (result == BME68X_E_COM_FAIL && dev->transport_error != XY_DEVICE_OK) {
        return dev->transport_error;
    }
    if (result == BME68X_E_DEV_NOT_FOUND) {
        return XY_DEVICE_NOT_FOUND;
    }
    if (result == BME68X_E_NULL_PTR || result == BME68X_E_INVALID_LENGTH) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return XY_DEVICE_IO_ERROR;
}

static xy_error_t init_fail(xy_bme680_t *dev, xy_error_t result)
{
    memset(dev, 0, sizeof(*dev));
    return result;
}

xy_error_t xy_bme680_init(xy_bme680_t *dev, void *i2c_handle, uint8_t addr)
{
    xy_error_t result;

    if (!dev || !i2c_handle || (addr != 0x76U && addr != 0x77U)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 100U);
    if (result != XY_DEVICE_OK) {
        return init_fail(dev, result);
    }

    dev->bosch.intf = BME68X_I2C_INTF;
    dev->bosch.intf_ptr = dev;
    dev->bosch.read = bus_read;
    dev->bosch.write = bus_write;
    dev->bosch.delay_us = delay_us;
    dev->bosch.amb_temp = 25;

    dev->transport_error = XY_DEVICE_OK;
    result = map_error(dev, bme68x_init(&dev->bosch));
    if (result != XY_DEVICE_OK) {
        return init_fail(dev, result);
    }

    dev->config.os_hum = BME68X_OS_2X;
    dev->config.os_pres = BME68X_OS_4X;
    dev->config.os_temp = BME68X_OS_8X;
    dev->config.filter = BME68X_FILTER_SIZE_3;
    dev->config.odr = BME68X_ODR_NONE;
    dev->transport_error = XY_DEVICE_OK;
    result = map_error(dev, bme68x_set_conf(&dev->config, &dev->bosch));
    if (result != XY_DEVICE_OK) {
        return init_fail(dev, result);
    }

    dev->heater.enable = BME68X_ENABLE;
    dev->heater.heatr_temp = 320U;
    dev->heater.heatr_dur = 150U;
    dev->transport_error = XY_DEVICE_OK;
    result = map_error(dev, bme68x_set_heatr_conf(BME68X_FORCED_MODE, &dev->heater, &dev->bosch));
    if (result != XY_DEVICE_OK) {
        return init_fail(dev, result);
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_bme680_deinit(xy_bme680_t *dev)
{
    xy_error_t result;

    if (!dev || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    dev->transport_error = XY_DEVICE_OK;
    result = map_error(dev, bme68x_set_op_mode(BME68X_SLEEP_MODE, &dev->bosch));
    if (result == XY_DEVICE_OK) {
        dev->initialized = 0U;
        dev->i2c_dev.i2c_handle = NULL;
        dev->i2c_dev.base.initialized = 0U;
    }
    return result;
}

xy_error_t xy_bme680_read(xy_bme680_t *dev, xy_bme680_data_t *output)
{
    struct bme68x_data bosch_data;
    xy_bme680_data_t next;
    uint8_t sample_count = 0U;
    uint32_t measurement_us;
    xy_error_t result;

    if (!dev || !output || !dev->initialized || !dev->i2c_dev.base.initialized ||
        !dev->i2c_dev.i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    dev->transport_error = XY_DEVICE_OK;
    result = map_error(dev, bme68x_set_op_mode(BME68X_FORCED_MODE, &dev->bosch));
    if (result != XY_DEVICE_OK) {
        return result;
    }

    measurement_us = bme68x_get_meas_dur(BME68X_FORCED_MODE, &dev->config, &dev->bosch) +
                     (uint32_t)dev->heater.heatr_dur * 1000U;
    delay_us(measurement_us + 5000U, dev);
    dev->transport_error = XY_DEVICE_OK;
    result = map_error(dev, bme68x_get_data(BME68X_FORCED_MODE, &bosch_data, &sample_count,
                                            &dev->bosch));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    if (sample_count == 0U) {
        return XY_DEVICE_BUSY;
    }
    if ((bosch_data.status & (BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK)) !=
        (BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK)) {
        return XY_DEVICE_BUSY;
    }

#ifdef BME68X_USE_FPU
    next.temperature_centi_c = (int32_t)(bosch_data.temperature * 100.0f);
    next.pressure_pa = (uint32_t)bosch_data.pressure;
    next.humidity_milli_pct = (uint32_t)(bosch_data.humidity * 1000.0f);
    next.gas_ohms = (uint32_t)bosch_data.gas_resistance;
#else
    next.temperature_centi_c = bosch_data.temperature;
    next.pressure_pa = bosch_data.pressure;
    next.humidity_milli_pct = bosch_data.humidity;
    next.gas_ohms = bosch_data.gas_resistance;
#endif
    next.status = bosch_data.status;
    dev->data = next;
    *output = next;
    return XY_DEVICE_OK;
}
