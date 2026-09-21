#ifndef XY_KX023_H
#define XY_KX023_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_KX023_ADDR 0x1EU
#define XY_KX023_REG_WHO_AM_I 0x0FU
#define XY_KX023_REG_XOUT_L 0x06U
#define XY_KX023_REG_CNTL1 0x18U
#define XY_KX023_REG_ODCNTL 0x1BU
#define XY_KX023_REG_SOFT_RESET 0x7FU
#define XY_KX023_WHO_AM_I 0x15U
#define XY_KX023_ODR_12_5HZ 0x00U
#define XY_KX023_MODE_STANDBY 0x00U
#define XY_KX023_MODE_LOW_POWER 0x01U
typedef struct { int16_t raw_x; int16_t raw_y; int16_t raw_z; uint32_t timestamp; } xy_kx023_sample_t;
typedef struct { xy_i2c_device_t i2c_dev; xy_kx023_sample_t sample; uint8_t initialized; } xy_kx023_t;
xy_error_t xy_kx023_init(xy_kx023_t *dev, void *i2c_handle);
xy_error_t xy_kx023_deinit(xy_kx023_t *dev);
xy_error_t xy_kx023_read(xy_kx023_t *dev, xy_kx023_sample_t *sample);
#endif
