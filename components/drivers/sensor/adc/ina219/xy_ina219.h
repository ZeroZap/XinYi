#ifndef XY_INA219_H
#define XY_INA219_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_INA219_ADDR_MIN 0x40U
#define XY_INA219_ADDR_MAX 0x4FU
#define XY_INA219_ADDR_DEFAULT 0x40U

#define XY_INA219_REG_CONFIG 0x00U
#define XY_INA219_REG_SHUNT_VOLTAGE 0x01U
#define XY_INA219_REG_BUS_VOLTAGE 0x02U
#define XY_INA219_REG_POWER 0x03U
#define XY_INA219_REG_CURRENT 0x04U
#define XY_INA219_REG_CALIBRATION 0x05U

#define XY_INA219_CONFIG_RESET 0x8000U
#define XY_INA219_CONFIG_DEFAULT 0x399FU

typedef struct {
    uint32_t shunt_resistance_uohm;
    uint32_t current_lsb_ua;
    uint16_t config_register;
} xy_ina219_config_t;

typedef struct {
    int32_t shunt_voltage_uv;
    uint32_t bus_voltage_mv;
    int32_t current_ua;
    uint32_t power_uw;
} xy_ina219_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_ina219_config_t config;
    xy_ina219_sample_t sample;
    uint16_t calibration_register;
    uint8_t initialized;
} xy_ina219_t;

xy_error_t xy_ina219_init(xy_ina219_t *dev, void *i2c_handle, uint8_t addr,
                          const xy_ina219_config_t *config);
xy_error_t xy_ina219_deinit(xy_ina219_t *dev);
xy_error_t xy_ina219_read_shunt_voltage(xy_ina219_t *dev, int32_t *voltage_uv);
xy_error_t xy_ina219_read_bus_voltage(xy_ina219_t *dev, uint32_t *voltage_mv);
xy_error_t xy_ina219_read_current(xy_ina219_t *dev, int32_t *current_ua);
xy_error_t xy_ina219_read_power(xy_ina219_t *dev, uint32_t *power_uw);
xy_error_t xy_ina219_read_sample(xy_ina219_t *dev, xy_ina219_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif
