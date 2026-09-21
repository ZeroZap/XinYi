#include "xy_lis2dw12.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"
#include <string.h>
static int ready(const xy_lis2dw12_t*d){return d&&d->initialized&&d->i2c_dev.base.initialized&&d->i2c_dev.i2c_handle;}
static xy_error_t wr(xy_lis2dw12_t*d,uint8_t r,uint8_t v){return xy_i2c_device_write_reg(&d->i2c_dev,r,&v,1);}
xy_error_t xy_lis2dw12_init(xy_lis2dw12_t*d,void*h){uint8_t id;xy_error_t e;if(!d||!h)return XY_DEVICE_INVALID_PARAM;memset(d,0,sizeof(*d));e=xy_i2c_device_init(&d->i2c_dev,h,XY_LIS2DW12_ADDR,1000);if(e!=0)return e;e=xy_i2c_device_read_reg(&d->i2c_dev,XY_LIS2DW12_REG_WHO_AM_I,&id,1);if(e==0&&id!=XY_LIS2DW12_WHO_AM_I)e=XY_DEVICE_NOT_FOUND;if(e==0)e=wr(d,XY_LIS2DW12_REG_CTRL2,XY_LIS2DW12_CTRL2_SOFT_RESET);if(e==0)xy_device_delay_ms(10);if(e==0)e=wr(d,XY_LIS2DW12_REG_CTRL2,XY_LIS2DW12_CTRL2_IF_ADD_INC);if(e==0)e=wr(d,XY_LIS2DW12_REG_CTRL1,XY_LIS2DW12_CTRL1_100HZ_LP1);if(e!=0){memset(d,0,sizeof(*d));return e;}d->initialized=1;return 0;}
xy_error_t xy_lis2dw12_deinit(xy_lis2dw12_t*d){xy_error_t e;if(!ready(d))return XY_DEVICE_INVALID_PARAM;e=wr(d,XY_LIS2DW12_REG_CTRL1,0);if(e!=0)return e;d->initialized=0;d->i2c_dev.base.initialized=0;d->i2c_dev.i2c_handle=NULL;return 0;}
xy_error_t xy_lis2dw12_read(xy_lis2dw12_t*d,xy_lis2dw12_sample_t*s){uint8_t b[6];xy_lis2dw12_sample_t n;xy_error_t e;if(!ready(d)||!s)return XY_DEVICE_INVALID_PARAM;e=xy_i2c_device_read_reg(&d->i2c_dev,XY_LIS2DW12_REG_OUT_X_L,b,6);if(e!=0)return e;n.raw_x=(int16_t)((uint16_t)b[0]|((uint16_t)b[1]<<8))>>4;n.raw_y=(int16_t)((uint16_t)b[2]|((uint16_t)b[3]<<8))>>4;n.raw_z=(int16_t)((uint16_t)b[4]|((uint16_t)b[5]<<8))>>4;n.x_mg=(n.raw_x*1952)/1000;n.y_mg=(n.raw_y*1952)/1000;n.z_mg=(n.raw_z*1952)/1000;n.timestamp=xy_hal_sys_get_tick_count();*s=n;d->sample=n;return 0;}
