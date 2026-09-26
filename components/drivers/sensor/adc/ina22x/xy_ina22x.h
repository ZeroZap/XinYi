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
#define XY_INA22X_REG_SHUNT_TEMPCO 0x03U
#define XY_INA22X_REG_VSHUNT       0x04U
#define XY_INA22X_REG_VBUS         0x05U
#define XY_INA22X_REG_DIETEMP      0x06U
#define XY_INA22X_REG_CURRENT      0x07U
#define XY_INA22X_REG_POWER        0x08U
#define XY_INA22X_REG_ENERGY       0x09U
#define XY_INA22X_REG_CHARGE       0x0AU
#define XY_INA22X_REG_DIAG_ALRT    0x0BU
#define XY_INA22X_REG_SHUNT_OV_LIMIT 0x0CU
#define XY_INA22X_REG_SHUNT_UV_LIMIT 0x0DU
#define XY_INA22X_REG_BUS_OV_LIMIT   0x0EU
#define XY_INA22X_REG_BUS_UV_LIMIT   0x0FU
#define XY_INA22X_REG_TEMP_LIMIT     0x10U
#define XY_INA22X_REG_POWER_LIMIT    0x11U
#define XY_INA22X_REG_MANUFACTURER 0x3EU
#define XY_INA22X_REG_DEVICE_ID    0x3FU

#define XY_INA22X_MANUFACTURER_ID  0x5449U
#define XY_INA228_DIE_ID           0x228U
#define XY_INA229_DIE_ID           0x229U
#define XY_INA22X_ADC_CONFIG_DEFAULT 0xFB68U
#define XY_INA22X_ADC_MODE_MASK      0xF000U
#define XY_INA22X_ADC_MODE_CONT_ALL  0xF000U

#define XY_INA22X_DIAG_ENERGYOF 0x0800U
#define XY_INA22X_DIAG_CHARGEOF 0x0400U
#define XY_INA22X_DIAG_MATHOF   0x0200U
#define XY_INA22X_DIAG_TMPOL    0x0080U
#define XY_INA22X_DIAG_SHNTOL   0x0040U
#define XY_INA22X_DIAG_SHNTUL   0x0020U
#define XY_INA22X_DIAG_BUSOL    0x0010U
#define XY_INA22X_DIAG_BUSUL    0x0008U
#define XY_INA22X_DIAG_POL      0x0004U
#define XY_INA22X_DIAG_CNVRF    0x0002U
#define XY_INA22X_DIAG_MEMSTAT  0x0001U
#define XY_INA22X_DIAG_INVALID_SAMPLE_MASK \
    (XY_INA22X_DIAG_ENERGYOF | XY_INA22X_DIAG_CHARGEOF | XY_INA22X_DIAG_MATHOF)

typedef enum {
    XY_INA22X_SHUNT_RANGE_163_84_MV = 0,
    XY_INA22X_SHUNT_RANGE_40_96_MV = 1,
} xy_ina22x_shunt_range_t;

typedef enum {
    XY_INA22X_ALERT_SHUNT_OVER = 0,
    XY_INA22X_ALERT_SHUNT_UNDER,
    XY_INA22X_ALERT_BUS_OVER,
    XY_INA22X_ALERT_BUS_UNDER,
    XY_INA22X_ALERT_TEMP_OVER,
    XY_INA22X_ALERT_POWER_OVER,
    XY_INA22X_ALERT_LIMIT_COUNT,
} xy_ina22x_alert_limit_t;

typedef struct {
    uint32_t shunt_resistor_uohm;
    uint32_t current_lsb_ua;
    uint16_t adc_config;
    uint16_t shunt_tempco_ppm_per_c;
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
int xy_ina22x_core_alert_register(xy_ina22x_alert_limit_t limit, uint8_t *reg);
int xy_ina22x_core_read(xy_ina22x_core_t *core);
int xy_ina22x_core_shutdown(xy_ina22x_core_t *core);
int xy_ina22x_core_set_alert_limit(xy_ina22x_core_t *core, xy_ina22x_alert_limit_t limit,
                                   uint16_t raw_value);
int xy_ina22x_core_get_diagnostic(xy_ina22x_core_t *core, uint16_t *diagnostic);

#ifdef __cplusplus
}
#endif

#endif
