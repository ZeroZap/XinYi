#ifndef XY_HMC5883L_H
#define XY_HMC5883L_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_HMC5883L_ADDR 0x1EU

typedef enum {
    XY_HMC5883L_GAIN_0_88_GA = 0x00U,
    XY_HMC5883L_GAIN_1_30_GA = 0x20U,
    XY_HMC5883L_GAIN_8_10_GA = 0xE0U,
} xy_hmc5883l_gain_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} xy_hmc5883l_data_t;

typedef struct {
    int32_t x_mgauss;
    int32_t y_mgauss;
    int32_t z_mgauss;
} xy_hmc5883l_field_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_hmc5883l_data_t data;
    xy_hmc5883l_gain_t gain;
    uint8_t initialized;
} xy_hmc5883l_t;

xy_error_t xy_hmc5883l_init(xy_hmc5883l_t *dev, void *i2c_handle);
xy_error_t xy_hmc5883l_deinit(xy_hmc5883l_t *dev);
xy_error_t xy_hmc5883l_data_ready(xy_hmc5883l_t *dev, uint8_t *ready);
xy_error_t xy_hmc5883l_set_gain(xy_hmc5883l_t *dev, xy_hmc5883l_gain_t gain);
xy_error_t xy_hmc5883l_get_gain(xy_hmc5883l_t *dev, xy_hmc5883l_gain_t *gain);
xy_error_t xy_hmc5883l_read(xy_hmc5883l_t *dev, xy_hmc5883l_data_t *data);
xy_error_t xy_hmc5883l_read_field(xy_hmc5883l_t *dev, xy_hmc5883l_field_t *field);

#endif
