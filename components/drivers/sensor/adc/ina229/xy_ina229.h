#ifndef XY_INA229_H
#define XY_INA229_H

#include "xy_ina22x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    xy_spi_device_t spi_dev;
    xy_ina22x_core_t core;
    uint8_t initialized;
} xy_ina229_t;

int xy_ina229_init(xy_ina229_t *dev, void *spi_handle, void *cs_pin,
                   const xy_ina22x_config_t *config);
int xy_ina229_deinit(xy_ina229_t *dev);
int xy_ina229_read(xy_ina229_t *dev, xy_ina22x_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif
