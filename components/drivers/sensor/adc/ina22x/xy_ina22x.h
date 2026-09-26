#ifndef XY_INA22X_H
#define XY_INA22X_H

#include "xy_device.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_INA22X_REG_CONFIG       0x00U
#define XY_INA22X_REG_ADC_CONFIG   0x01U
#define XY_INA22X_REG_SHUNT_CAL    0x02U
#define XY_INA22X_REG_VSHUNT       0x04U
#define XY_INA22X_REG_VBUS         0x05U
#define XY_INA22X_REG_DIETEMP      0x06U
#define XY_INA22X_REG_CURRENT      0x07U
#define XY_INA22X_REG_POWER        0x08U
#define XY_INA22X_REG_ENERGY       0x09U
#define XY_INA22X_REG_CHARGE       0x0AU
#define XY_INA22X_REG_MANUFACTURER 0x3EU
#define XY_INA22X_REG_DEVICE_ID    0x3FU

#define XY_INA22X_MANUFACTURER_ID  0x5449U
#define XY_INA228_DIE_ID           0x228U
#define XY_INA229_DIE_ID           0x229U
#define XY_INA22X_ADC_CONFIG_DEFAULT 0xFB68U

typedef enum {
    XY_INA22X_SHUNT_RANGE_163_84_MV = 0,
    XY_INA22X_SHUNT_RANGE_40_96_MV = 1,
} xy_ina22x_shunt_range_t;

typedef struct {
    uint32_t shunt_resistor_uohm;
    uint32_t current_lsb_ua;
    uint16_t adc_config;
    xy_ina22x_shunt_range_t shunt_range;
} xy_ina22x_config_t;

typedef struct {
    float bus_voltage_mv;
    float shunt_voltage_uv;
    float current_ma;
    float power_mw;
    float energy_mj;
    float charge_mc;
    float die_temperature_c;
    uint32_t timestamp;
} xy_ina22x_sample_t;

typedef int (*xy_ina22x_read_fn)(void *context, uint8_t reg, uint8_t *data, uint8_t len);
typedef int (*xy_ina22x_write16_fn)(void *context, uint8_t reg, uint16_t value);

typedef struct {
    xy_ina22x_read_fn read;
    xy_ina22x_write16_fn write16;
    void *context;
} xy_ina22x_transport_t;

typedef struct {
    xy_ina22x_config_t config;
    xy_ina22x_sample_t sample;
    xy_ina22x_transport_t transport;
    uint16_t shunt_cal;
    uint8_t initialized;
} xy_ina22x_core_t;

int xy_ina22x_core_config_valid(const xy_ina22x_config_t *config, uint16_t *shunt_cal);
int xy_ina22x_core_configure(xy_ina22x_core_t *core);
int xy_ina22x_core_read(xy_ina22x_core_t *core);
int xy_ina22x_core_shutdown(xy_ina22x_core_t *core);

#ifdef __cplusplus
}
#endif

#endif
