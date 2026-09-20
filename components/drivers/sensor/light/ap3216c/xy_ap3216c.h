#ifndef XY_AP3216C_H
#define XY_AP3216C_H

#include "xy_dev_i2c.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_AP3216C_DEFAULT_ADDRESS 0x1EU

#define XY_AP3216C_REG_SYSTEM_CONFIG 0x00U
#define XY_AP3216C_REG_IR_DATA_L     0x0AU
#define XY_AP3216C_REG_ALS_DATA_L    0x0CU
#define XY_AP3216C_REG_PS_DATA_L     0x0EU

#define XY_AP3216C_MODE_POWER_DOWN 0x00U
#define XY_AP3216C_MODE_ALS        0x01U
#define XY_AP3216C_MODE_PS         0x02U
#define XY_AP3216C_MODE_ALS_PS     0x03U
#define XY_AP3216C_MODE_RESET      0x04U

typedef struct {
    uint32_t illuminance_millilux;
    uint16_t proximity_raw;
    uint16_t infrared_raw;
} xy_ap3216c_data_t;

typedef struct {
    xy_i2c_device_t i2c_dev;
    xy_ap3216c_data_t data;
    uint8_t mode;
    uint8_t initialized;
} xy_ap3216c_t;

xy_error_t xy_ap3216c_init(xy_ap3216c_t *dev, void *i2c_handle, uint8_t address,
                           uint8_t mode);
xy_error_t xy_ap3216c_deinit(xy_ap3216c_t *dev);
xy_error_t xy_ap3216c_read_light(xy_ap3216c_t *dev, uint32_t *illuminance_millilux);
xy_error_t xy_ap3216c_read_proximity(xy_ap3216c_t *dev, uint16_t *proximity_raw);
xy_error_t xy_ap3216c_read_ir(xy_ap3216c_t *dev, uint16_t *infrared_raw);

#ifdef __cplusplus
}
#endif

#endif