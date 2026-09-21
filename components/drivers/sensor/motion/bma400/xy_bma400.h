#ifndef XY_BMA400_H
#define XY_BMA400_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_BMA400_ADDR 0x14U
#define XY_BMA400_REG_CHIP_ID 0x00U
#define XY_BMA400_REG_ACC_X_LSB 0x04U
#define XY_BMA400_REG_ACC_CONFIG0 0x19U
#define XY_BMA400_REG_ACC_CONFIG1 0x1AU
#define XY_BMA400_REG_ACC_CONFIG2 0x1BU
#define XY_BMA400_REG_CMD 0x7EU
#define XY_BMA400_CHIP_ID 0x90U
#define XY_BMA400_CONFIG0_LOW_POWER 0x01U
#define XY_BMA400_CONFIG1_2G 0x00U
#define XY_BMA400_CONFIG2_25HZ 0x06U
typedef struct { int16_t raw_x; int16_t raw_y; int16_t raw_z; uint32_t timestamp; } xy_bma400_sample_t;
typedef struct { xy_i2c_device_t i2c_dev; xy_bma400_sample_t sample; uint8_t initialized; } xy_bma400_t;
xy_error_t xy_bma400_init(xy_bma400_t *dev, void *i2c_handle);
xy_error_t xy_bma400_deinit(xy_bma400_t *dev);
xy_error_t xy_bma400_read(xy_bma400_t *dev, xy_bma400_sample_t *sample);
#endif
