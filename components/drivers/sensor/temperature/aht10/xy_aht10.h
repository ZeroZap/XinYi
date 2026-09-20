#ifndef XY_AHT10_H
#define XY_AHT10_H

#include "xy_dev_i2c.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_AHT10_DEFAULT_ADDRESS 0x38U

typedef struct {
    int32_t temperature_centi_c;
    uint32_t humidity_centi_pct;
} xy_aht10_data_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_aht10_data_t data;
    uint8_t initialized;
} xy_aht10_t;

xy_error_t xy_aht10_init(xy_aht10_t *dev, void *i2c_handle, uint8_t address);
xy_error_t xy_aht10_deinit(xy_aht10_t *dev);
xy_error_t xy_aht10_read(xy_aht10_t *dev, xy_aht10_data_t *out);

#ifdef __cplusplus
}
#endif

#endif
