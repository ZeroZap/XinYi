#ifndef XY_SC7A22H_H
#define XY_SC7A22H_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_SC7A22H_ADDR 0x18U
#define XY_SC7A22H_REG_WHO_AM_I 0x01U
#define XY_SC7A22H_REG_COM_CFG 0x05U
#define XY_SC7A22H_REG_INT_CFG1 0x06U
#define XY_SC7A22H_REG_INT_CFG2 0x07U
#define XY_SC7A22H_REG_HPF_LPF_CFG 0x08U
#define XY_SC7A22H_REG_DATA_SAT 0x0AU
#define XY_SC7A22H_REG_DATA_STAT 0x0BU
#define XY_SC7A22H_REG_OUT_X_H 0x0CU
#define XY_SC7A22H_REG_OUT_Y_H 0x0EU
#define XY_SC7A22H_REG_OUT_Z_H 0x10U
#define XY_SC7A22H_REG_ACC_CONF 0x40U
#define XY_SC7A22H_REG_ACC_RANGE 0x41U
#define XY_SC7A22H_REG_FIFO_DOWNS 0x45U
#define XY_SC7A22H_REG_SOFT_RST 0x4AU
#define XY_SC7A22H_REG_SELF_TEST 0x6DU
#define XY_SC7A22H_REG_PWR_CTRL 0x7DU
#define XY_SC7A22H_ACC_ENABLE 0x04U
#define XY_SC7A22H_DEMO_ACC_CONF 0x07U
#define XY_SC7A22H_DEMO_ACC_RANGE 0x01U
#define XY_SC7A22H_DEMO_COM_CFG 0x50U
#define XY_SC7A22H_DEMO_INT_CFG1 0x01U
#define XY_SC7A22H_DEMO_FILTER_CFG 0x05U
#define XY_SC7A22H_WHO_AM_I_VALUE 0x18U

typedef struct { int16_t x; int16_t y; int16_t z; } xy_sc7a22h_data_t;
typedef struct { int32_t x_mg; int32_t y_mg; int32_t z_mg; } xy_sc7a22h_accel_t;
typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_sc7a22h_data_t data;
    uint8_t com_cfg;
    uint8_t acc_conf;
    uint8_t acc_range;
    uint8_t data_status;
    uint8_t initialized;
} xy_sc7a22h_t;

xy_error_t xy_sc7a22h_init(xy_sc7a22h_t *dev, void *i2c_handle);
xy_error_t xy_sc7a22h_deinit(xy_sc7a22h_t *dev);
xy_error_t xy_sc7a22h_read_config(xy_sc7a22h_t *dev);
xy_error_t xy_sc7a22h_read_status(xy_sc7a22h_t *dev, uint8_t *status);
xy_error_t xy_sc7a22h_read(xy_sc7a22h_t *dev, xy_sc7a22h_data_t *data);
xy_error_t xy_sc7a22h_read_accel(xy_sc7a22h_t *dev, xy_sc7a22h_accel_t *accel);
xy_error_t xy_sc7a22h_set_acc_config(xy_sc7a22h_t *dev, uint8_t config);
xy_error_t xy_sc7a22h_set_acc_range(xy_sc7a22h_t *dev, uint8_t range);

#endif
