#ifndef XY_AS5600_H
#define XY_AS5600_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_AS5600_ADDR 0x36U
#define XY_AS5600_REG_ANGLE_H 0x0EU

typedef struct {
    uint16_t angle_raw;
    uint32_t timestamp;
} xy_as5600_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_as5600_sample_t sample;
    uint8_t initialized;
} xy_as5600_t;

xy_error_t xy_as5600_init(xy_as5600_t *dev, void *i2c_handle);
xy_error_t xy_as5600_deinit(xy_as5600_t *dev);
xy_error_t xy_as5600_read(xy_as5600_t *dev, xy_as5600_sample_t *sample);

#endif
