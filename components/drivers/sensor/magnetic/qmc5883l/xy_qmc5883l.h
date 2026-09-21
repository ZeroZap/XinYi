#ifndef XY_QMC5883L_H
#define XY_QMC5883L_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_QMC5883L_ADDR 0x0DU
#define XY_QMC5883L_REG_DATA_X_LSB 0x00U
#define XY_QMC5883L_REG_STATUS 0x06U
#define XY_QMC5883L_REG_CONTROL1 0x09U
#define XY_QMC5883L_REG_CONTROL2 0x0AU
#define XY_QMC5883L_REG_PERIOD 0x0BU
#define XY_QMC5883L_REG_CHIP_ID 0x0DU
#define XY_QMC5883L_CHIP_ID 0xFFU
#define XY_QMC5883L_CONTROL1_2G_200HZ 0x0DU

typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    uint32_t timestamp;
} xy_qmc5883l_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_qmc5883l_sample_t sample;
    uint8_t initialized;
} xy_qmc5883l_t;

xy_error_t xy_qmc5883l_init(xy_qmc5883l_t *dev, void *i2c_handle);
xy_error_t xy_qmc5883l_deinit(xy_qmc5883l_t *dev);
xy_error_t xy_qmc5883l_read(xy_qmc5883l_t *dev, xy_qmc5883l_sample_t *sample);

#endif
