#ifndef XY_AS5048B_H
#define XY_AS5048B_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_AS5048B_ADDR 0x40U
#define XY_AS5048B_REG_ANGLE_MSB 0xFEU
#define XY_AS5048B_REG_ANGLE_LSB 0xFFU

typedef struct {
    uint16_t angle_raw;
    uint32_t timestamp;
} xy_as5048b_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_as5048b_sample_t sample;
    uint8_t initialized;
} xy_as5048b_t;

xy_error_t xy_as5048b_init(xy_as5048b_t *dev, void *i2c_handle);
xy_error_t xy_as5048b_deinit(xy_as5048b_t *dev);
xy_error_t xy_as5048b_read(xy_as5048b_t *dev, xy_as5048b_sample_t *sample);

#endif
