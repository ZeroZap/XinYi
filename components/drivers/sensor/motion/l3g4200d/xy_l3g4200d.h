#ifndef XY_L3G4200D_H
#define XY_L3G4200D_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_L3G4200D_ADDR 0x69U
typedef struct { int32_t x_mdps,y_mdps,z_mdps; } xy_l3g4200d_data_t;
typedef struct { xy_i2c_device_t i2c_dev; xy_l3g4200d_data_t data; uint16_t range_dps; uint8_t initialized; } xy_l3g4200d_t;
xy_error_t xy_l3g4200d_init(xy_l3g4200d_t *dev,void *i2c,uint8_t addr);
xy_error_t xy_l3g4200d_deinit(xy_l3g4200d_t *dev);
xy_error_t xy_l3g4200d_data_ready(xy_l3g4200d_t *dev,uint8_t *ready);
xy_error_t xy_l3g4200d_read(xy_l3g4200d_t *dev,xy_l3g4200d_data_t *data);
xy_error_t xy_l3g4200d_set_range(xy_l3g4200d_t *dev,uint16_t dps);
#endif
