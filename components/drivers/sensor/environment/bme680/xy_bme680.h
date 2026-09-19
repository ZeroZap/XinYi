#ifndef XY_BME680_H
#define XY_BME680_H

#include "bosch/bme68x.h"
#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_BME680_ADDR_HIGH 0x77U

typedef struct {
    int32_t temperature_centi_c;
    uint32_t pressure_pa;
    uint32_t humidity_milli_pct;
    uint32_t gas_ohms;
    uint8_t status;
} xy_bme680_data_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    struct bme68x_dev bosch;
    struct bme68x_conf config;
    struct bme68x_heatr_conf heater;
    xy_bme680_data_t data;
    xy_error_t transport_error;
    uint8_t initialized;
} xy_bme680_t;

xy_error_t xy_bme680_init(xy_bme680_t *dev, void *i2c, uint8_t addr);
xy_error_t xy_bme680_deinit(xy_bme680_t *dev);
xy_error_t xy_bme680_read(xy_bme680_t *dev, xy_bme680_data_t *data);

#endif
