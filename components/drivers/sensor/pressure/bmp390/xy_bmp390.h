#ifndef XY_BMP390_H
#define XY_BMP390_H

#include "bmp3.h"
#include "xy_dev_i2c.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_BMP390_ADDR_PRIMARY BMP3_ADDR_I2C_PRIM
#define XY_BMP390_ADDR_SECONDARY BMP3_ADDR_I2C_SEC

/**
 * Bosch BMP3 SensorAPI provenance:
 * - upstream: https://github.com/boschsensortec/BMP3_SensorAPI
 * - tag: bmp3_v2.0.6
 * - commit: db4cf8e4140c593b8c3d85f8c6c07335c7ffa9dc
 * - license: BSD-3-Clause (bosch/LICENSE)
 */
typedef struct {
    int64_t temperature_centi_c;
    uint64_t pressure_centi_pa;
} xy_bmp390_data_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    struct bmp3_dev bosch;
    struct bmp3_settings settings;
    xy_bmp390_data_t data;
    xy_error_t transport_error;
    uint8_t initialized;
} xy_bmp390_t;

xy_error_t xy_bmp390_init(xy_bmp390_t *dev, void *i2c_handle, uint8_t addr);
xy_error_t xy_bmp390_deinit(xy_bmp390_t *dev);
xy_error_t xy_bmp390_read(xy_bmp390_t *dev, xy_bmp390_data_t *output);

#ifdef __cplusplus
}
#endif

#endif
