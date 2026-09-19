#ifndef __SENSOR_BMP280_H__
#define __SENSOR_BMP280_H__

#include "sensor_core.h"
#include "xy_bmp280.h"

#define BMP280_REG_CHIP_ID BMP280_REG_ID
#define BMP280_REG_PRESS_MSB BMP280_REG_PRESS_DATA
#define BMP280_REG_CALIB00 BMP280_REG_CALIB
#define BMP280_CHIP_ID BMP280_ID_VALUE

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    xy_bmp280_t device;
} bmp280_priv_t;

/* 创建传感器设备 */
sensor_device_t *bmp280_create_pressure(const char *name, void *i2c_bus);
sensor_device_t *bmp280_create_temperature(const char *name, void *i2c_bus);

#endif /* __SENSOR_BMP280_H__ */