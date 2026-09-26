#ifndef XY_QMA6100P_H
#define XY_QMA6100P_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_QMA6100P_ADDR_LOW 0x12U
#define XY_QMA6100P_ADDR_HIGH 0x13U
#define XY_QMA6100P_CHIP_ID 0x90U

#define XY_QMA6100P_REG_CHIP_ID 0x00U
#define XY_QMA6100P_REG_X_LSB 0x01U
#define XY_QMA6100P_REG_INT_STATUS2 0x0BU
#define XY_QMA6100P_REG_RANGE 0x0FU
#define XY_QMA6100P_REG_BW 0x10U
#define XY_QMA6100P_REG_POWER 0x11U
#define XY_QMA6100P_REG_INT_ENABLE1 0x17U
#define XY_QMA6100P_REG_INT_MAP1 0x1AU
#define XY_QMA6100P_REG_INT_MAP3 0x1CU
#define XY_QMA6100P_REG_INT_PIN_CONFIG 0x20U
#define XY_QMA6100P_REG_INT_CONFIG 0x21U

#define XY_QMA6100P_RANGE_2G 0x01U
#define XY_QMA6100P_RANGE_4G 0x02U
#define XY_QMA6100P_RANGE_8G 0x04U
#define XY_QMA6100P_RANGE_16G 0x08U
#define XY_QMA6100P_RANGE_32G 0x0FU
#define XY_QMA6100P_BW_100HZ 0xE6U
#define XY_QMA6100P_POWER_ACTIVE 0x80U
#define XY_QMA6100P_DATA_READY_BIT 0x10U

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} xy_qma6100p_raw_t;

typedef struct {
    int32_t x_mg;
    int32_t y_mg;
    int32_t z_mg;
} xy_qma6100p_accel_t;

typedef struct {
    uint8_t enable1;
    uint8_t map_int1;
    uint8_t map_int2;
    uint8_t pin_config;
    uint8_t interrupt_config;
} xy_qma6100p_interrupt_config_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_qma6100p_raw_t raw;
    uint8_t address;
    uint8_t range;
    uint8_t bandwidth;
    uint8_t initialized;
} xy_qma6100p_t;

xy_error_t xy_qma6100p_init(xy_qma6100p_t *dev, void *i2c_handle, uint8_t address);
xy_error_t xy_qma6100p_deinit(xy_qma6100p_t *dev);
xy_error_t xy_qma6100p_configure_data_ready_interrupts(xy_qma6100p_t *dev,
                                                       uint8_t int1_enable,
                                                       uint8_t int2_enable);
xy_error_t xy_qma6100p_read_interrupt_status(xy_qma6100p_t *dev, uint8_t *status);
xy_error_t xy_qma6100p_read_interrupt_config(xy_qma6100p_t *dev,
                                             xy_qma6100p_interrupt_config_t *config);
xy_error_t xy_qma6100p_read_raw(xy_qma6100p_t *dev, xy_qma6100p_raw_t *raw);
xy_error_t xy_qma6100p_read_accel(xy_qma6100p_t *dev, xy_qma6100p_accel_t *accel);

#endif
