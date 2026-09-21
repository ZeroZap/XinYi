#include "xy_kx023.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"
#include <string.h>
static int ready(const xy_kx023_t*d){return d&&d->initialized&&d->i2c_dev.base.initialized&&d->i2c_dev.i2c_handle;}
static xy_error_t wr(xy_kx023_t*d,uint8_t r,uint8_t v){return xy_i2c_device_write_reg(&d->i2c_dev,r,&v,1);}
xy_error_t xy_kx023_init(xy_kx023_t*d,void*h){uint8_t id;xy_error_t r;if(!d||!h)return XY_DEVICE_INVALID_PARAM;memset(d,0,sizeof(*d));r=xy_i2c_device_init(&d->i2c_dev,h,XY_KX023_ADDR,1000);if(r!=XY_DEVICE_OK)return r;r=xy_i2c_device_read_reg(&d->i2c_dev,XY_KX023_REG_WHO_AM_I,&id,1);if(r==XY_DEVICE_OK&&id!=XY_KX023_WHO_AM_I)r=XY_DEVICE_NOT_FOUND;if(r==XY_DEVICE_OK)r=wr(d,XY_KX023_REG_SOFT_RESET,0x80);if(r==XY_DEVICE_OK){xy_device_delay_ms(10);r=wr(d,XY_KX023_REG_CNTL1,XY_KX023_MODE_STANDBY);}if(r==XY_DEVICE_OK)r=wr(d,XY_KX023_REG_ODCNTL,XY_KX023_ODR_12_5HZ);if(r==XY_DEVICE_OK)r=wr(d,XY_KX023_REG_CNTL1,XY_KX023_MODE_LOW_POWER);if(r!=XY_DEVICE_OK){memset(d,0,sizeof(*d));return r;}d->initialized=1;return XY_DEVICE_OK;}
xy_error_t xy_kx023_deinit(xy_kx023_t*d){xy_error_t r;if(!ready(d))return XY_DEVICE_INVALID_PARAM;r=wr(d,XY_KX023_REG_CNTL1,XY_KX023_MODE_STANDBY);if(r!=XY_DEVICE_OK)return r;d->initialized=0;d->i2c_dev.base.initialized=0;d->i2c_dev.i2c_handle=NULL;return XY_DEVICE_OK;}
xy_error_t xy_kx023_read(xy_kx023_t*d,xy_kx023_sample_t*s){uint8_t b[6];xy_kx023_sample_t n;xy_error_t r;if(!ready(d)||!s)return XY_DEVICE_INVALID_PARAM;r=xy_i2c_device_read_reg(&d->i2c_dev,XY_KX023_REG_XOUT_L,b,6);if(r!=XY_DEVICE_OK)return r;n.raw_x=(int16_t)((uint16_t)b[0]|((uint16_t)b[1]<<8));n.raw_y=(int16_t)((uint16_t)b[2]|((uint16_t)b[3]<<8));n.raw_z=(int16_t)((uint16_t)b[4]|((uint16_t)b[5]<<8));n.timestamp=xy_hal_sys_get_tick_count();*s=n;d->sample=n;return XY_DEVICE_OK;}
