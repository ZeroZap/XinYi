#ifndef XY_LIS2DH12_H
#define XY_LIS2DH12_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_LIS2DH12_ADDR 0x18U
#define XY_LIS2DH12_REG_WHO_AM_I 0x0FU
#define XY_LIS2DH12_REG_TEMP_CFG 0x1FU
#define XY_LIS2DH12_REG_CTRL1 0x20U
#define XY_LIS2DH12_REG_CTRL4 0x23U
#define XY_LIS2DH12_REG_OUT_X_L 0x28U
#define XY_LIS2DH12_AUTO_INCREMENT 0x80U
#define XY_LIS2DH12_WHO_AM_I 0x33U
#define XY_LIS2DH12_CTRL1_10HZ_XYZ 0x27U
#define XY_LIS2DH12_CTRL4_HR_2G 0x08U
#define XY_LIS2DH12_TEMP_ENABLE 0xC0U
typedef struct { int16_t raw_x,raw_y,raw_z; int32_t x_mg,y_mg,z_mg; uint32_t timestamp; } xy_lis2dh12_sample_t;
typedef struct { xy_i2c_device_t i2c_dev; xy_lis2dh12_sample_t sample; uint8_t initialized; } xy_lis2dh12_t;
xy_error_t xy_lis2dh12_init(xy_lis2dh12_t*,void*);
xy_error_t xy_lis2dh12_deinit(xy_lis2dh12_t*);
xy_error_t xy_lis2dh12_read(xy_lis2dh12_t*,xy_lis2dh12_sample_t*);
#endif
