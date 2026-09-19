#include "xy_l3g4200d.h"
#include <string.h>
#define WHO 0x0FU
#define CTRL1 0x20U
#define CTRL4 0x23U
#define STATUS 0x27U
#define OUT 0xA8U
static xy_error_t rr(xy_l3g4200d_t*d,uint8_t r,uint8_t*p,size_t n){return xy_i2c_device_read_reg(&d->i2c_dev,r,p,n);}static xy_error_t wr(xy_l3g4200d_t*d,uint8_t r,uint8_t v){return xy_i2c_device_write_reg(&d->i2c_dev,r,&v,1U);}
xy_error_t xy_l3g4200d_init(xy_l3g4200d_t*d,void*h,uint8_t a){uint8_t id;xy_error_t r;if(!d||!h||(a!=0x68U&&a!=0x69U))return XY_DEVICE_INVALID_PARAM;memset(d,0,sizeof(*d));r=xy_i2c_device_init(&d->i2c_dev,h,a,100U);if(r!=XY_DEVICE_OK){memset(d,0,sizeof(*d));return r;}r=rr(d,WHO,&id,1U);if(r!=XY_DEVICE_OK||id!=0xD3U){memset(d,0,sizeof(*d));return r==XY_DEVICE_OK?XY_DEVICE_NOT_FOUND:r;}if((r=wr(d,CTRL4,0x80U))==XY_DEVICE_OK)r=wr(d,CTRL1,0x1FU);if(r!=XY_DEVICE_OK){memset(d,0,sizeof(*d));return r;}d->range_dps=250U;d->initialized=1U;return XY_DEVICE_OK;}
xy_error_t xy_l3g4200d_deinit(xy_l3g4200d_t*d){if(!d||!d->initialized)return XY_DEVICE_INVALID_PARAM;xy_error_t r=wr(d,CTRL1,0x07U);if(r==XY_DEVICE_OK){d->initialized=0;d->i2c_dev.base.initialized=0;}return r;}
xy_error_t xy_l3g4200d_data_ready(xy_l3g4200d_t*d,uint8_t*o){uint8_t s;if(!d||!o||!d->initialized)return XY_DEVICE_INVALID_PARAM;xy_error_t r=rr(d,STATUS,&s,1);if(r==XY_DEVICE_OK)*o=(s>>3)&1U;return r;}
xy_error_t xy_l3g4200d_set_range(xy_l3g4200d_t*d,uint16_t q){uint8_t v=q==250?0x80U:q==500?0x90U:q==2000?0xA0U:0xFFU;if(!d||!d->initialized||v==0xFFU)return XY_DEVICE_INVALID_PARAM;xy_error_t r=wr(d,CTRL4,v);if(r==XY_DEVICE_OK)d->range_dps=q;return r;}
xy_error_t xy_l3g4200d_read(xy_l3g4200d_t*d,xy_l3g4200d_data_t*o){uint8_t b[6];xy_l3g4200d_data_t v;if(!d||!o||!d->initialized||!d->i2c_dev.base.initialized)return XY_DEVICE_INVALID_PARAM;xy_error_t r=rr(d,OUT,b,6);if(r!=XY_DEVICE_OK)return r;int16_t x=(int16_t)((uint16_t)b[1]<<8|b[0]),y=(int16_t)((uint16_t)b[3]<<8|b[2]),z=(int16_t)((uint16_t)b[5]<<8|b[4]);int32_t n=d->range_dps==250?875:d->range_dps==500?1750:7000;v.x_mdps=(int32_t)x*n/100;v.y_mdps=(int32_t)y*n/100;v.z_mdps=(int32_t)z*n/100;d->data=v;*o=v;return XY_DEVICE_OK;}
