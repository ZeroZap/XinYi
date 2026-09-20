#ifndef XY_INA226_H
#define XY_INA226_H

#include "xy_dev_i2c.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INA226_ADDR_GND 0x40U
#define INA226_ADDR_VREF 0x41U
#define INA226_ADDR_SDA 0x42U
#define INA226_ADDR_SCL 0x43U

#define INA226_REG_CONFIG 0x00U
#define INA226_REG_SHUNT_VOLT 0x01U
#define INA226_REG_BUS_VOLT 0x02U
#define INA226_REG_POWER 0x03U
#define INA226_REG_CURRENT 0x04U
#define INA226_REG_CALIB 0x05U
#define INA226_REG_MASK_EN 0x06U
#define INA226_REG_ALERT_LIMIT 0x07U
#define INA226_REG_MFG_ID 0xFEU
#define INA226_REG_DIE_ID 0xFFU

#define INA226_MFG_ID_VALUE 0x5449U
#define INA226_DIE_ID_VALUE 0x2260U

#define XY_INA_OK 0
#define XY_INA_ERROR (-1)
#define XY_INA_INVALID_PARAM (-2)
#define XY_INA_NOT_FOUND (-3)

typedef enum {
    XY_INA_AVG_1 = 0,
    XY_INA_AVG_4,
    XY_INA_AVG_16,
    XY_INA_AVG_64,
    XY_INA_AVG_128,
    XY_INA_AVG_256,
    XY_INA_AVG_512,
    XY_INA_AVG_1024,
} xy_ina_avg_t;

typedef struct {
    uint32_t shunt_resistor_uohm;
    uint32_t current_lsb_ua;
    xy_ina_avg_t avg_samples;
    uint16_t alert_current_ma;
} xy_ina_config_t;

typedef struct {
    float voltage_mv;
    float current_ma;
    float power_mw;
    float shunt_voltage_uv;
    uint32_t timestamp;
} xy_ina_data_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    uint8_t addr;
    xy_ina_config_t config;
    xy_ina_data_t data;
    uint16_t calib_value;
    uint8_t initialized;
} xy_ina_t;

int xy_ina_init(xy_ina_t *ina, void *i2c_handle, uint8_t addr,
                const xy_ina_config_t *config);
int xy_ina_deinit(xy_ina_t *ina);
int xy_ina_read(xy_ina_t *ina);
int xy_ina_get_voltage(xy_ina_t *ina, float *voltage_mv);
int xy_ina_get_current(xy_ina_t *ina, float *current_ma);
int xy_ina_get_power(xy_ina_t *ina, float *power_mw);
int xy_ina_get_shunt_voltage(xy_ina_t *ina, float *voltage_uv);
int xy_ina_enable_alert(xy_ina_t *ina, bool enable);

#ifdef __cplusplus
}
#endif

#endif
