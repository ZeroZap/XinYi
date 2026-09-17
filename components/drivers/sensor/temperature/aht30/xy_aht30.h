#ifndef XY_AHT30_H
#define XY_AHT30_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_AHT30_ADDR 0x38U
typedef struct { int32_t temperature_centi_c; uint32_t humidity_centi_pct; } xy_aht30_data_t;
typedef struct { xy_i2c_device_t i2c_dev; xy_aht30_data_t data; uint8_t initialized; } xy_aht30_t;
xy_error_t xy_aht30_init(xy_aht30_t *dev, void *i2c_handle);
xy_error_t xy_aht30_deinit(xy_aht30_t *dev);
xy_error_t xy_aht30_read(xy_aht30_t *dev, xy_aht30_data_t *data);
#endif
