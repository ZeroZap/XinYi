#ifndef XY_CMM905_H
#define XY_CMM905_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#define XY_CMM905_ADDR 0x30U
#define XY_CMM905_REG_STATUS 0x02U
#define XY_CMM905_REG_DATA 0x04U

/* The legacy owner scaled the raw 16-bit readings by 10 and labelled them micro-tesla; that constant
 * has no datasheet backing in the repository (the MM905 sensitivity depends on the specific part),
 * so this owner returns the raw readings and leaves calibration to the caller, matching the other
 * canonical magnetometer owners (QMC5883L/IST8310/AK09918) that return raw samples. */
typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    uint32_t timestamp;
} xy_cmm905_sample_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_cmm905_sample_t sample;
    uint8_t initialized;
} xy_cmm905_t;

xy_error_t xy_cmm905_init(xy_cmm905_t *dev, void *i2c_handle);
xy_error_t xy_cmm905_deinit(xy_cmm905_t *dev);
xy_error_t xy_cmm905_read(xy_cmm905_t *dev, xy_cmm905_sample_t *sample);

#endif /* XY_CMM905_H */
