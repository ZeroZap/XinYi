#ifndef XY_ICM20608_H
#define XY_ICM20608_H

#include "xy_dev_i2c.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_ICM20608_ADDR_DEFAULT 0x68U
#define XY_ICM20608_ADDR_ALT 0x69U
#define XY_ICM20608_WHO_AM_I 0xAEU

#define XY_ICM20608_REG_SMPLRT_DIV 0x19U
#define XY_ICM20608_REG_CONFIG 0x1AU
#define XY_ICM20608_REG_GYRO_CONFIG 0x1BU
#define XY_ICM20608_REG_ACCEL_CONFIG 0x1CU
#define XY_ICM20608_REG_ACCEL_CONFIG2 0x1DU
#define XY_ICM20608_REG_ACCEL_XOUT_H 0x3BU
#define XY_ICM20608_REG_TEMP_OUT_H 0x41U
#define XY_ICM20608_REG_GYRO_XOUT_H 0x43U
#define XY_ICM20608_REG_PWR_MGMT_1 0x6BU
#define XY_ICM20608_REG_PWR_MGMT_2 0x6CU
#define XY_ICM20608_REG_WHO_AM_I 0x75U

typedef enum {
    XY_ICM20608_TRANSPORT_I2C = 0,
    XY_ICM20608_TRANSPORT_SPI = 1,
} xy_icm20608_transport_t;

typedef enum {
    XY_ICM20608_ACCEL_RANGE_2G = 0,
    XY_ICM20608_ACCEL_RANGE_4G = 1,
    XY_ICM20608_ACCEL_RANGE_8G = 2,
    XY_ICM20608_ACCEL_RANGE_16G = 3,
} xy_icm20608_accel_range_t;

typedef enum {
    XY_ICM20608_GYRO_RANGE_250DPS = 0,
    XY_ICM20608_GYRO_RANGE_500DPS = 1,
    XY_ICM20608_GYRO_RANGE_1000DPS = 2,
    XY_ICM20608_GYRO_RANGE_2000DPS = 3,
} xy_icm20608_gyro_range_t;

typedef xy_error_t (*xy_icm20608_spi_read_t)(void *context, uint8_t reg, uint8_t *data,
                                             uint16_t len);
typedef xy_error_t (*xy_icm20608_spi_write_t)(void *context, uint8_t reg, const uint8_t *data,
                                              uint16_t len);

typedef struct {
    int32_t x_mg;
    int32_t y_mg;
    int32_t z_mg;
} xy_icm20608_accel_t;

typedef struct {
    int32_t x_mdps;
    int32_t y_mdps;
    int32_t z_mdps;
} xy_icm20608_gyro_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    void *spi_context;
    xy_icm20608_spi_read_t spi_read;
    xy_icm20608_spi_write_t spi_write;
    xy_icm20608_accel_t accel;
    xy_icm20608_gyro_t gyro;
    int32_t temperature_centi_c;
    xy_icm20608_transport_t transport;
    xy_icm20608_accel_range_t accel_range;
    xy_icm20608_gyro_range_t gyro_range;
    uint16_t odr_hz;
    uint8_t address;
    uint8_t initialized;
} xy_icm20608_t;

xy_error_t xy_icm20608_init_i2c(xy_icm20608_t *dev, void *i2c_handle, uint8_t address);
xy_error_t xy_icm20608_init_spi(xy_icm20608_t *dev, void *context,
                                xy_icm20608_spi_read_t read_fn,
                                xy_icm20608_spi_write_t write_fn);
xy_error_t xy_icm20608_deinit(xy_icm20608_t *dev);
xy_error_t xy_icm20608_set_accel_range(xy_icm20608_t *dev,
                                       xy_icm20608_accel_range_t range);
xy_error_t xy_icm20608_set_gyro_range(xy_icm20608_t *dev,
                                      xy_icm20608_gyro_range_t range);
xy_error_t xy_icm20608_set_odr(xy_icm20608_t *dev, uint16_t odr_hz);
xy_error_t xy_icm20608_read_accel(xy_icm20608_t *dev, xy_icm20608_accel_t *accel);
xy_error_t xy_icm20608_read_gyro(xy_icm20608_t *dev, xy_icm20608_gyro_t *gyro);
xy_error_t xy_icm20608_read_temperature(xy_icm20608_t *dev, int32_t *temperature_centi_c);

#ifdef __cplusplus
}
#endif

#endif
