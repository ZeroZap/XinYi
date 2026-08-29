/**
 * @file xy_bmp280.c
 * @brief Canonical BMP280 pressure/temperature Device driver
 */

#include "xy_bmp280.h"
#include <string.h>

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
}

static int16_t read_i16_le(const uint8_t *data)
{
    return (int16_t)read_u16_le(data);
}

static void bmp280_parse_calibration(xy_bmp280_calibration_t *calibration,
                                     const uint8_t data[24])
{
    calibration->dig_t1 = read_u16_le(&data[0]);
    calibration->dig_t2 = read_i16_le(&data[2]);
    calibration->dig_t3 = read_i16_le(&data[4]);
    calibration->dig_p1 = read_u16_le(&data[6]);
    calibration->dig_p2 = read_i16_le(&data[8]);
    calibration->dig_p3 = read_i16_le(&data[10]);
    calibration->dig_p4 = read_i16_le(&data[12]);
    calibration->dig_p5 = read_i16_le(&data[14]);
    calibration->dig_p6 = read_i16_le(&data[16]);
    calibration->dig_p7 = read_i16_le(&data[18]);
    calibration->dig_p8 = read_i16_le(&data[20]);
    calibration->dig_p9 = read_i16_le(&data[22]);
}

static int32_t bmp280_compensate_temperature(const xy_bmp280_calibration_t *calibration,
                                             int32_t adc_temperature, int32_t *t_fine)
{
    int32_t var1;
    int32_t var2;

    var1 = ((((adc_temperature >> 3) - ((int32_t)calibration->dig_t1 << 1))) *
            (int32_t)calibration->dig_t2) >>
           11;
    var2 = (((((adc_temperature >> 4) - (int32_t)calibration->dig_t1) *
              ((adc_temperature >> 4) - (int32_t)calibration->dig_t1)) >>
             12) *
            (int32_t)calibration->dig_t3) >>
           14;
    *t_fine = var1 + var2;
    return (*t_fine * 5 + 128) >> 8;
}

static int bmp280_compensate_pressure(const xy_bmp280_calibration_t *calibration,
                                      int32_t adc_pressure, int32_t t_fine,
                                      uint32_t *pressure)
{
    int64_t var1;
    int64_t var2;
    int64_t value;

    var1 = (int64_t)t_fine - 128000;
    var2 = var1 * var1 * calibration->dig_p6;
    var2 += (var1 * calibration->dig_p5) << 17;
    var2 += (int64_t)calibration->dig_p4 << 35;
    var1 = ((var1 * var1 * calibration->dig_p3) >> 8) +
           ((var1 * calibration->dig_p2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * calibration->dig_p1) >> 33;
    if (var1 == 0) {
        return XY_DEVICE_IO_ERROR;
    }

    value = 1048576 - adc_pressure;
    value = (((value << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)calibration->dig_p9 * (value >> 13) * (value >> 13)) >> 25;
    var2 = ((int64_t)calibration->dig_p8 * value) >> 19;
    value = ((value + var1 + var2) >> 8) + ((int64_t)calibration->dig_p7 << 4);
    *pressure = (uint32_t)(value >> 8);
    return XY_DEVICE_OK;
}

int xy_bmp280_init_addr(xy_bmp280_t *bmp, void *i2c_handle, uint8_t addr)
{
    uint8_t id;
    uint8_t calibration_data[24];
    uint8_t value;
    int result;

    if (bmp == NULL || i2c_handle == NULL ||
        (addr != BMP280_ADDR_DEFAULT && addr != BMP280_ADDR_ALT)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(bmp, 0, sizeof(*bmp));
    result = xy_i2c_device_init(&bmp->i2c_dev, i2c_handle, addr, 1000U);
    if (result < 0) {
        return result;
    }
    bmp->addr = addr;

    result = xy_i2c_device_read_reg(&bmp->i2c_dev, BMP280_REG_ID, &id, 1U);
    if (result < 0) {
        return result;
    }
    if (id != BMP280_ID_VALUE) {
        return XY_DEVICE_NOT_FOUND;
    }

    value = 0xB6U;
    result = xy_i2c_device_write_reg(&bmp->i2c_dev, BMP280_REG_RESET, &value, 1U);
    if (result < 0) {
        return result;
    }

    result = xy_i2c_device_read_reg(&bmp->i2c_dev, BMP280_REG_CALIB, calibration_data,
                                    sizeof(calibration_data));
    if (result < 0) {
        return result;
    }
    bmp280_parse_calibration(&bmp->calibration, calibration_data);

    value = 0x00U;
    result = xy_i2c_device_write_reg(&bmp->i2c_dev, BMP280_REG_CONFIG, &value, 1U);
    if (result < 0) {
        return result;
    }
    value = 0x27U;
    result = xy_i2c_device_write_reg(&bmp->i2c_dev, BMP280_REG_CTRL_MEAS, &value, 1U);
    if (result < 0) {
        return result;
    }

    bmp->initialized = 1U;
    return XY_DEVICE_OK;
}

int xy_bmp280_deinit(xy_bmp280_t *bmp)
{
    uint8_t value = 0x00U;
    int result;

    if (bmp == NULL || bmp->initialized == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_i2c_device_write_reg(&bmp->i2c_dev, BMP280_REG_CTRL_MEAS, &value, 1U);
    if (result < 0) {
        return result;
    }
    bmp->initialized = 0U;
    return XY_DEVICE_OK;
}

int xy_bmp280_read(xy_bmp280_t *bmp)
{
    uint8_t data[6];
    int32_t adc_pressure;
    int32_t adc_temperature;
    int32_t temperature;
    int32_t t_fine;
    uint32_t pressure;
    int result;

    if (bmp == NULL || bmp->initialized == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = xy_i2c_device_read_reg(&bmp->i2c_dev, BMP280_REG_PRESS_DATA, data, sizeof(data));
    if (result < 0) {
        return result;
    }

    adc_pressure = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
    adc_temperature = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);
    temperature = bmp280_compensate_temperature(&bmp->calibration, adc_temperature, &t_fine);
    result = bmp280_compensate_pressure(&bmp->calibration, adc_pressure, t_fine, &pressure);
    if (result < 0) {
        return result;
    }

    bmp->temperature = temperature;
    bmp->pressure = pressure;
    return XY_DEVICE_OK;
}

int xy_bmp280_get_temperature(const xy_bmp280_t *bmp, int32_t *temperature)
{
    if (bmp == NULL || temperature == NULL || bmp->initialized == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    *temperature = bmp->temperature;
    return XY_DEVICE_OK;
}

int xy_bmp280_get_pressure(const xy_bmp280_t *bmp, uint32_t *pressure)
{
    if (bmp == NULL || pressure == NULL || bmp->initialized == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    *pressure = bmp->pressure;
    return XY_DEVICE_OK;
}

int32_t xy_bmp280_read_temperature(xy_bmp280_t *bmp)
{
    if (xy_bmp280_read(bmp) != XY_DEVICE_OK) {
        return bmp != NULL ? bmp->temperature : 0;
    }
    return bmp->temperature;
}

uint32_t xy_bmp280_read_pressure(xy_bmp280_t *bmp)
{
    if (xy_bmp280_read(bmp) != XY_DEVICE_OK) {
        return bmp != NULL ? bmp->pressure : 0U;
    }
    return bmp->pressure;
}
