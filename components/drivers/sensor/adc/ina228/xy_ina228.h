#ifndef XY_INA228_H
#define XY_INA228_H

#include "xy_ina22x.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_INA228_ADDR_MIN 0x40U
#define XY_INA228_ADDR_MAX 0x4FU

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_ina22x_core_t core;
    uint32_t owner_cookie;
    uint8_t address;
    uint8_t initialized;
} xy_ina228_t;

int xy_ina228_init(xy_ina228_t *dev, void *i2c_handle, uint8_t address,
                   const xy_ina22x_config_t *config);
int xy_ina228_deinit(xy_ina228_t *dev);
int xy_ina228_read(xy_ina228_t *dev, xy_ina22x_sample_t *sample);
int xy_ina228_set_alert_limit(xy_ina228_t *dev, xy_ina22x_alert_limit_t limit,
                              uint16_t raw_value);
int xy_ina228_get_diagnostic(xy_ina228_t *dev, uint16_t *diagnostic);

#ifdef __cplusplus
}
#endif

#endif
