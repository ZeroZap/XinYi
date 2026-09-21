#ifndef XY_ADXL362_H
#define XY_ADXL362_H
#include "xy_device.h"
#include <stdint.h>
#define XY_ADXL362_REG_DEVID_AD 0x00U
#define XY_ADXL362_REG_FILTER_CTL 0x2CU
#define XY_ADXL362_REG_POWER_CTL 0x2DU
#define XY_ADXL362_REG_XDATA 0x0EU
#define XY_ADXL362_CMD_READ 0x0BU
#define XY_ADXL362_CMD_WRITE 0x0AU
#define XY_ADXL362_DEVID_AD 0xADU
#define XY_ADXL362_FILTER_100HZ_2G 0x18U
#define XY_ADXL362_MEASUREMENT 0x02U
#define XY_ADXL362_STANDBY 0x00U
typedef struct { int16_t raw_x; int16_t raw_y; int16_t raw_z; uint32_t timestamp; } xy_adxl362_sample_t;
typedef struct { xy_spi_device_t spi_dev; xy_adxl362_sample_t sample; uint8_t initialized; } xy_adxl362_t;
xy_error_t xy_adxl362_init(xy_adxl362_t *dev, void *spi_handle, void *cs_pin);
xy_error_t xy_adxl362_deinit(xy_adxl362_t *dev);
xy_error_t xy_adxl362_read(xy_adxl362_t *dev, xy_adxl362_sample_t *sample);
#endif
