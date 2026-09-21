#ifndef XY_LIS2DW12_H
#define XY_LIS2DW12_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_LIS2DW12_ADDR 0x18U
#define XY_LIS2DW12_REG_WHO_AM_I 0x0FU
#define XY_LIS2DW12_REG_CTRL1 0x20U
#define XY_LIS2DW12_REG_CTRL2 0x21U
#define XY_LIS2DW12_REG_OUT_X_L 0x28U
#define XY_LIS2DW12_WHO_AM_I 0x44U
#define XY_LIS2DW12_CTRL2_SOFT_RESET 0x40U
#define XY_LIS2DW12_CTRL2_IF_ADD_INC 0x04U
#define XY_LIS2DW12_CTRL1_100HZ_LP1 0x51U
typedef struct {int16_t raw_x,raw_y,raw_z;int32_t x_mg,y_mg,z_mg;uint32_t timestamp;} xy_lis2dw12_sample_t;
typedef struct {xy_i2c_device_t i2c_dev;xy_lis2dw12_sample_t sample;uint8_t initialized;} xy_lis2dw12_t;
xy_error_t xy_lis2dw12_init(xy_lis2dw12_t*,void*);
xy_error_t xy_lis2dw12_deinit(xy_lis2dw12_t*);
xy_error_t xy_lis2dw12_read(xy_lis2dw12_t*,xy_lis2dw12_sample_t*);
#endif
