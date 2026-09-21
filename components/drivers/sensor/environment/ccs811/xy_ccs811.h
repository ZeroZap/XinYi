#ifndef XY_CCS811_H
#define XY_CCS811_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_CCS811_ADDR 0x5AU
#define XY_CCS811_REG_STATUS 0x00U
#define XY_CCS811_REG_MEAS_MODE 0x01U
#define XY_CCS811_REG_ALG_RESULT 0x02U
#define XY_CCS811_REG_HW_ID 0x20U
#define XY_CCS811_APP_START 0xF4U
#define XY_CCS811_HW_ID_VALUE 0x81U
#define XY_CCS811_STATUS_DATA_READY 0x08U
typedef struct {uint16_t eco2_ppm,tvoc_ppb;uint32_t timestamp;} xy_ccs811_sample_t;
typedef struct {xy_i2c_device_t i2c_dev;xy_ccs811_sample_t sample;uint8_t initialized;} xy_ccs811_t;
xy_error_t xy_ccs811_init(xy_ccs811_t*,void*);
xy_error_t xy_ccs811_deinit(xy_ccs811_t*);
xy_error_t xy_ccs811_read(xy_ccs811_t*,xy_ccs811_sample_t*);
#endif
