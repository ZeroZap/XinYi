#ifndef XY_LSM6DSO_H
#define XY_LSM6DSO_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_LSM6DSO_ADDR 0x6AU
#define XY_LSM6DSO_REG_WHOAMI 0x0FU
#define XY_LSM6DSO_REG_CTRL1_XL 0x10U
#define XY_LSM6DSO_REG_CTRL2_G 0x11U
#define XY_LSM6DSO_REG_CTRL3_C 0x12U
#define XY_LSM6DSO_REG_CTRL4_C 0x13U
#define XY_LSM6DSO_REG_OUTX_L_G 0x22U
#define XY_LSM6DSO_REG_OUTX_L_XL 0x28U
#define XY_LSM6DSO_WHOAMI 0x6CU
#define XY_LSM6DSO_CTRL1_XL_104HZ_2G 0x40U
#define XY_LSM6DSO_CTRL2_G_104HZ_250DPS 0x40U
typedef struct { int16_t accel_x,accel_y,accel_z; int16_t gyro_x,gyro_y,gyro_z; uint32_t timestamp; } xy_lsm6dso_sample_t;
typedef struct { xy_i2c_device_t i2c_dev; xy_lsm6dso_sample_t sample; uint8_t initialized; } xy_lsm6dso_t;
xy_error_t xy_lsm6dso_init(xy_lsm6dso_t*,void*);
xy_error_t xy_lsm6dso_deinit(xy_lsm6dso_t*);
xy_error_t xy_lsm6dso_read(xy_lsm6dso_t*,xy_lsm6dso_sample_t*);
#endif
