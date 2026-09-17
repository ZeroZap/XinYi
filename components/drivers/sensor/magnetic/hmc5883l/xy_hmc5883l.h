#ifndef XY_HMC5883L_H
#define XY_HMC5883L_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_HMC5883L_ADDR 0x1EU

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} xy_hmc5883l_data_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_hmc5883l_data_t data;
    uint8_t initialized;
} xy_hmc5883l_t;

xy_error_t xy_hmc5883l_init(xy_hmc5883l_t *dev, void *i2c_handle);
xy_error_t xy_hmc5883l_deinit(xy_hmc5883l_t *dev);
xy_error_t xy_hmc5883l_data_ready(xy_hmc5883l_t *dev, uint8_t *ready);
xy_error_t xy_hmc5883l_read(xy_hmc5883l_t *dev, xy_hmc5883l_data_t *data);

#endif
