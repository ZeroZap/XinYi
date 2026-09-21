#ifndef XY_IST8310_H
#define XY_IST8310_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_IST8310_ADDR 0x0CU
#define XY_IST8310_REG_WHOAMI 0x00U
#define XY_IST8310_REG_CTRL1 0x01U
#define XY_IST8310_REG_CTRL2 0x02U
#define XY_IST8310_REG_DATA 0x03U
#define XY_IST8310_WHOAMI 0x10U
#define XY_IST8310_CTRL1_CONTINUOUS_100HZ 0x1AU
#define XY_IST8310_CTRL2_ENABLE 0x40U
typedef struct { int16_t raw_x; int16_t raw_y; int16_t raw_z; uint32_t timestamp; } xy_ist8310_sample_t;
typedef struct { xy_i2c_device_t i2c_dev; xy_ist8310_sample_t sample; uint8_t initialized; } xy_ist8310_t;
xy_error_t xy_ist8310_init(xy_ist8310_t *dev, void *i2c_handle);
xy_error_t xy_ist8310_deinit(xy_ist8310_t *dev);
xy_error_t xy_ist8310_read(xy_ist8310_t *dev, xy_ist8310_sample_t *sample);
#endif
