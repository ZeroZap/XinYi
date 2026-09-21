#ifndef XY_LSM9DS1_H
#define XY_LSM9DS1_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_LSM9DS1_IMU_ADDR 0x6AU
#define XY_LSM9DS1_MAG_ADDR 0x1CU
#define XY_LSM9DS1_REG_WHOAMI_IMU 0x0FU
#define XY_LSM9DS1_REG_WHOAMI_MAG 0x0FU
#define XY_LSM9DS1_REG_CTRL1_XL 0x10U
#define XY_LSM9DS1_REG_CTRL2_G 0x11U
#define XY_LSM9DS1_REG_CTRL3_C 0x12U
#define XY_LSM9DS1_REG_CTRL_REG1_M 0x20U
#define XY_LSM9DS1_REG_OUTX_L_G 0x22U
#define XY_LSM9DS1_REG_OUTX_L_XL 0x28U
#define XY_LSM9DS1_REG_OUTX_L_M 0x28U
#define XY_LSM9DS1_IMU_WHOAMI 0x68U
#define XY_LSM9DS1_MAG_WHOAMI 0x3DU
#define XY_LSM9DS1_CTRL1_XL_104HZ_2G 0x40U
#define XY_LSM9DS1_CTRL2_G_104HZ_250DPS 0x40U
#define XY_LSM9DS1_CTRL1_M_10HZ_HIGH_POWER 0x70U
typedef struct { int16_t accel_x,accel_y,accel_z; int16_t gyro_x,gyro_y,gyro_z; int16_t mag_x,mag_y,mag_z; uint32_t timestamp; } xy_lsm9ds1_sample_t;
typedef struct { xy_i2c_device_t imu; xy_i2c_device_t mag; xy_lsm9ds1_sample_t sample; uint8_t initialized; } xy_lsm9ds1_t;
xy_error_t xy_lsm9ds1_init(xy_lsm9ds1_t *, void *);
xy_error_t xy_lsm9ds1_deinit(xy_lsm9ds1_t *);
xy_error_t xy_lsm9ds1_read(xy_lsm9ds1_t *, xy_lsm9ds1_sample_t *);
#endif
