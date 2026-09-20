#ifndef XY_LTC2945_H
#define XY_LTC2945_H

#include "xy_dev_i2c.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_LTC2945_ADDR_MIN 0x67U
#define XY_LTC2945_ADDR_MAX 0x6FU
#define XY_LTC2945_ADDR_DEFAULT 0x6FU

#define XY_LTC2945_REG_CONTROL 0x00U
#define XY_LTC2945_REG_ALERT 0x01U
#define XY_LTC2945_REG_STATUS 0x02U
#define XY_LTC2945_REG_FAULT 0x03U
#define XY_LTC2945_REG_POWER_MSB 0x05U
#define XY_LTC2945_REG_SENSE_MSB 0x14U
#define XY_LTC2945_REG_VIN_MSB 0x1EU

#define XY_LTC2945_CONTROL_MULT_SELECT 0x01U
#define XY_LTC2945_CONTROL_CONTINUOUS_SENSE_PLUS 0x05U

#define XY_LTC2945_OK XY_DEVICE_OK
#define XY_LTC2945_INVALID_PARAM XY_DEVICE_INVALID_PARAM

typedef struct {
    uint32_t shunt_resistance_uohm;
    uint8_t control_register;
    uint8_t alert_register;
} xy_ltc2945_config_t;

typedef struct {
    uint32_t bus_voltage_mv;
    uint32_t shunt_voltage_uv;
    uint32_t current_ua;
    uint32_t power_uw;
    uint8_t status;
    uint8_t fault;
    uint32_t timestamp;
} xy_ltc2945_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_ltc2945_config_t config;
    xy_ltc2945_sample_t sample;
    bool initialized;
} xy_ltc2945_t;

int xy_ltc2945_init(xy_ltc2945_t *dev, void *i2c_handle, uint8_t address,
                    const xy_ltc2945_config_t *config);
int xy_ltc2945_deinit(xy_ltc2945_t *dev);
int xy_ltc2945_read_sample(xy_ltc2945_t *dev, xy_ltc2945_sample_t *sample);
int xy_ltc2945_set_alert_mask(xy_ltc2945_t *dev, uint8_t alert_mask);
int xy_ltc2945_clear_faults(xy_ltc2945_t *dev, uint8_t fault_mask);

#ifdef __cplusplus
}
#endif

#endif
