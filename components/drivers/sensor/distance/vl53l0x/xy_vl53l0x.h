#ifndef XY_VL53L0X_H
#define XY_VL53L0X_H

#include "xy_dev_i2c.h"
#include <stdint.h>

enum {
    XY_VL53L0X_ADDR = 0x29U,
    XY_VL53L0X_REG_MODEL_ID = 0xC0U,
    XY_VL53L0X_REG_SYSRANGE_START = 0x00U,
    XY_VL53L0X_REG_RANGE_STATUS = 0x14U,
    XY_VL53L0X_MODEL_ID = 0xEEU,
};

typedef struct {
    uint16_t distance_mm;
    uint32_t timestamp;
} xy_vl53l0x_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_vl53l0x_sample_t sample;
    uint8_t initialized;
} xy_vl53l0x_t;

xy_error_t xy_vl53l0x_init(xy_vl53l0x_t *dev, void *i2c_handle);
xy_error_t xy_vl53l0x_deinit(xy_vl53l0x_t *dev);
xy_error_t xy_vl53l0x_read(xy_vl53l0x_t *dev, xy_vl53l0x_sample_t *sample);

#endif
