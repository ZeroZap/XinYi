#ifndef XY_SC7A20_H
#define XY_SC7A20_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_SC7A20_ADDR 0x18U
#define XY_SC7A20_ADDR_ALT 0x19U
#define XY_SC7A20_REG_WHOAMI 0x0FU
#define XY_SC7A20_REG_CTRL_REG1 0x20U
#define XY_SC7A20_REG_CTRL_REG2 0x21U
#define XY_SC7A20_REG_CTRL_REG3 0x22U
#define XY_SC7A20_REG_CTRL_REG4 0x23U
#define XY_SC7A20_REG_OUT_X_L 0x28U
#define XY_SC7A20_WHOAMI_VALUE 0x11U

#define XY_SC7A20_ODR_POWER_DOWN 0x00U
#define XY_SC7A20_ODR_1HZ 0x10U
#define XY_SC7A20_ODR_10HZ 0x20U
#define XY_SC7A20_ODR_25HZ 0x30U
#define XY_SC7A20_ODR_50HZ 0x40U
#define XY_SC7A20_ODR_100HZ 0x50U
#define XY_SC7A20_ODR_200HZ 0x60U
#define XY_SC7A20_ODR_400HZ 0x70U

#define XY_SC7A20_RANGE_2G 0x00U
#define XY_SC7A20_RANGE_4G 0x10U
#define XY_SC7A20_RANGE_8G 0x20U
#define XY_SC7A20_RANGE_16G 0x30U

#define XY_SC7A20_AXIS_ENABLE_MASK 0x07U
#define XY_SC7A20_BDU_MASK 0x08U
/* The SC7A20 output registers are 12-bit left-aligned inside 16 bits: four zero bits at the LSB end.
 * A full-scale reading is therefore 0x0FFF in the shifted value, which fixes the mg-per-LSB scale. */
#define XY_SC7A20_FULL_SCALE_LSB 2048

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    uint32_t timestamp;
} xy_sc7a20_data_t;

typedef struct {
    int32_t x_mg;
    int32_t y_mg;
    int32_t z_mg;
} xy_sc7a20_accel_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_sc7a20_data_t data;
    uint8_t range_g; /**< configured full scale in g: 2, 4, 8 or 16 */
    uint8_t odr_reg; /**< configured CTRL_REG1 value */
    uint8_t initialized;
} xy_sc7a20_t;

xy_error_t xy_sc7a20_init(xy_sc7a20_t *dev, void *i2c_handle);
xy_error_t xy_sc7a20_deinit(xy_sc7a20_t *dev);
xy_error_t xy_sc7a20_read(xy_sc7a20_t *dev, xy_sc7a20_data_t *data);
xy_error_t xy_sc7a20_read_accel(xy_sc7a20_t *dev, xy_sc7a20_accel_t *accel);
xy_error_t xy_sc7a20_set_range(xy_sc7a20_t *dev, uint8_t range_g);
xy_error_t xy_sc7a20_set_odr(xy_sc7a20_t *dev, uint8_t odr_reg);

#endif /* XY_SC7A20_H */
