#ifndef XY_ADT7420_H
#define XY_ADT7420_H

#include "xy_dev_i2c.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ADT7420 I2C addresses (7-bit, A1/A0 strapping) */
#define XY_ADT7420_ADDR_DEFAULT 0x48U
#define XY_ADT7420_ADDR_ALT1    0x49U
#define XY_ADT7420_ADDR_ALT2    0x4AU
#define XY_ADT7420_ADDR_ALT3    0x4BU

/* Register map */
#define XY_ADT7420_REG_TEMP_MSB 0x00U
#define XY_ADT7420_REG_TEMP_LSB 0x01U
#define XY_ADT7420_REG_STATUS   0x02U
#define XY_ADT7420_REG_CONFIG   0x03U
#define XY_ADT7420_REG_ID       0x0BU
#define XY_ADT7420_REG_RESET    0x2FU

/* Configuration register bits */
#define XY_ADT7420_CONFIG_RES_16BIT 0x80U
#define XY_ADT7420_CONFIG_RES_13BIT 0x00U

/* Manufacturer identification code returned by the read-only ID register */
#define XY_ADT7420_ID_VALUE 0xCBU

typedef struct {
    int32_t temperature_centi_c; /* 0.01 C, signed */
} xy_adt7420_data_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_adt7420_data_t data;
    uint8_t initialized;
} xy_adt7420_t;

xy_error_t xy_adt7420_init(xy_adt7420_t *dev, void *i2c_handle, uint8_t address);
xy_error_t xy_adt7420_deinit(xy_adt7420_t *dev);
xy_error_t xy_adt7420_read(xy_adt7420_t *dev, xy_adt7420_data_t *out);

#ifdef __cplusplus
}
#endif

#endif
