#ifndef XY_AK09918_H
#define XY_AK09918_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_AK09918_ADDR 0x0CU
#define XY_AK09918_REG_WIA1 0x00U
#define XY_AK09918_REG_WIA2 0x01U
#define XY_AK09918_REG_ST1 0x10U
#define XY_AK09918_REG_HXL 0x11U
#define XY_AK09918_REG_ST2 0x18U
#define XY_AK09918_REG_CNTL2 0x31U
#define XY_AK09918_REG_CNTL3 0x32U
#define XY_AK09918_WIA1 0x48U
#define XY_AK09918_WIA2 0x09U
#define XY_AK09918_MODE_CONTINUOUS_100HZ 0x08U
#define XY_AK09918_MODE_POWER_DOWN 0x00U
#define XY_AK09918_RESET 0x01U

typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    uint32_t timestamp;
} xy_ak09918_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_ak09918_sample_t sample;
    uint8_t initialized;
} xy_ak09918_t;

xy_error_t xy_ak09918_init(xy_ak09918_t *dev, void *i2c_handle);
xy_error_t xy_ak09918_deinit(xy_ak09918_t *dev);
xy_error_t xy_ak09918_read(xy_ak09918_t *dev, xy_ak09918_sample_t *sample);

#endif
