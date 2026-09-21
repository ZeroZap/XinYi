#ifndef XY_APDS9960_H
#define XY_APDS9960_H
#include "xy_dev_i2c.h"
#include <stdint.h>
#define XY_APDS9960_ADDR 0x39U
#define XY_APDS9960_REG_ENABLE 0x80U
#define XY_APDS9960_REG_ID 0x92U
#define XY_APDS9960_REG_CDATAL 0x94U
#define XY_APDS9960_REG_PDATA 0x9CU
#define XY_APDS9960_REG_GFLVL 0xAEU
#define XY_APDS9960_REG_GSTATUS 0xAFU
#define XY_APDS9960_REG_GFIFO_U 0xFCU
#define XY_APDS9960_ENABLE_PON_AEN_PEN_GEN 0x4FU
#define XY_APDS9960_GVALID 0x01U
#define XY_APDS9960_MAX_GESTURE_DATA 128U
typedef struct {uint16_t clear,red,green,blue;uint32_t timestamp;} xy_apds9960_rgb_t;
typedef struct {uint8_t proximity;uint32_t timestamp;} xy_apds9960_proximity_t;
typedef struct {uint8_t level;uint8_t data[XY_APDS9960_MAX_GESTURE_DATA];uint32_t timestamp;} xy_apds9960_gesture_fifo_t;
typedef struct {xy_i2c_device_t i2c_dev;xy_apds9960_rgb_t rgb;xy_apds9960_proximity_t proximity;xy_apds9960_gesture_fifo_t gesture;uint8_t initialized;} xy_apds9960_t;
xy_error_t xy_apds9960_init(xy_apds9960_t*,void*);
xy_error_t xy_apds9960_deinit(xy_apds9960_t*);
xy_error_t xy_apds9960_read_rgb(xy_apds9960_t*,xy_apds9960_rgb_t*);
xy_error_t xy_apds9960_read_proximity(xy_apds9960_t*,xy_apds9960_proximity_t*);
xy_error_t xy_apds9960_read_gesture_fifo(xy_apds9960_t*,xy_apds9960_gesture_fifo_t*);
#endif
