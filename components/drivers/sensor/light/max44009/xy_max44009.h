#ifndef XY_MAX44009_H
#define XY_MAX44009_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_MAX44009_ADDR_LOW 0x4AU
#define XY_MAX44009_ADDR_HIGH 0x4BU
#define XY_MAX44009_REG_LUX_HIGH 0x03U
#define XY_MAX44009_REG_LUX_LOW 0x04U

typedef struct {
    uint32_t illuminance_mlux;
    uint32_t timestamp;
} xy_max44009_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_max44009_sample_t sample;
    uint8_t initialized;
} xy_max44009_t;

xy_error_t xy_max44009_init(xy_max44009_t *dev, void *i2c_handle, uint8_t addr);
xy_error_t xy_max44009_deinit(xy_max44009_t *dev);
xy_error_t xy_max44009_read(xy_max44009_t *dev, xy_max44009_sample_t *sample);

#endif
