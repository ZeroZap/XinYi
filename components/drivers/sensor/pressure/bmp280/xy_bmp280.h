/**
 * @file xy_bmp280.h
 * @brief Canonical BMP280 pressure/temperature Device driver
 */

#ifndef XY_BMP280_H
#define XY_BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>

#define BMP280_ADDR_DEFAULT 0x76U
#define BMP280_ADDR_ALT 0x77U

#define BMP280_REG_CALIB 0x88U
#define BMP280_REG_ID 0xD0U
#define BMP280_REG_RESET 0xE0U
#define BMP280_REG_CTRL_MEAS 0xF4U
#define BMP280_REG_CONFIG 0xF5U
#define BMP280_REG_PRESS_DATA 0xF7U
#define BMP280_ID_VALUE 0x58U

typedef struct {
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
} xy_bmp280_calibration_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    int32_t temperature; /* 0.01 degrees Celsius */
    uint32_t pressure;   /* Pa */
    xy_bmp280_calibration_t calibration;
    uint8_t addr;
    uint8_t initialized;
} xy_bmp280_t;

int xy_bmp280_init_addr(xy_bmp280_t *bmp, void *i2c_handle, uint8_t addr);

static inline int xy_bmp280_init(xy_bmp280_t *bmp, void *i2c_handle)
{
    return xy_bmp280_init_addr(bmp, i2c_handle, BMP280_ADDR_DEFAULT);
}

int xy_bmp280_deinit(xy_bmp280_t *bmp);
int xy_bmp280_read(xy_bmp280_t *bmp);
int xy_bmp280_get_temperature(const xy_bmp280_t *bmp, int32_t *temperature);
int xy_bmp280_get_pressure(const xy_bmp280_t *bmp, uint32_t *pressure);

/* Compatibility value getters. Prefer the status-returning getters above. */
int32_t xy_bmp280_read_temperature(xy_bmp280_t *bmp);
uint32_t xy_bmp280_read_pressure(xy_bmp280_t *bmp);

#ifdef __cplusplus
}
#endif

#endif /* XY_BMP280_H */
