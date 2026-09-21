#ifndef XY_VCNL4040_H
#define XY_VCNL4040_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_VCNL4040_ADDR 0x60U
#define XY_VCNL4040_REG_PS_DATA_L 0x08U

typedef struct {
    uint16_t proximity_raw;
    uint32_t timestamp;
} xy_vcnl4040_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_vcnl4040_sample_t sample;
    uint8_t initialized;
} xy_vcnl4040_t;

xy_error_t xy_vcnl4040_init(xy_vcnl4040_t *dev, void *i2c_handle);
xy_error_t xy_vcnl4040_deinit(xy_vcnl4040_t *dev);
xy_error_t xy_vcnl4040_read(xy_vcnl4040_t *dev, xy_vcnl4040_sample_t *sample);

#endif
