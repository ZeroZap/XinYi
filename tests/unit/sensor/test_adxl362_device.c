#include "unity.h"
#include "xy_adxl362.h"
#include <string.h>
static xy_error_t er; static uint8_t raw[6]={1,0,2,0,3,0};
xy_error_t xy_spi_device_init(xy_spi_device_t*d,void*h,void*cs,uint32_t speed,uint8_t mode){memset(d,0,sizeof(*d));d->base.initialized=1;d->spi_handle=h;d->cs_pin=cs;d->speed=speed;d->mode=mode;return 0;}
void xy_spi_device_cs(xy_spi_device_t*d,bool s){(void)d;(void)s;}
xy_error_t xy_spi_device_transfer(xy_spi_device_t*d,const uint8_t*tx,uint8_t*rx,size_t n){TEST_ASSERT_TRUE(d->base.initialized);if(er)return er;memset(rx,0,n);if(tx[0]==XY_ADXL362_CMD_READ&&n>2){if(tx[1]==0)* (rx+2)=XY_ADXL362_DEVID_AD;else memcpy(rx+2,raw,n-2);}return 0;}
uint32_t xy_hal_sys_get_tick_count(void){return 77;}
void setUp(void){er=0;}void tearDown(void){}
static void ok(void){xy_adxl362_t d;xy_adxl362_sample_t s;int bus,cs;TEST_ASSERT_EQUAL_INT(0,xy_adxl362_init(&d,&bus,&cs));TEST_ASSERT_EQUAL_INT(0,xy_adxl362_read(&d,&s));TEST_ASSERT_EQUAL_INT16(1,s.raw_x);TEST_ASSERT_EQUAL_INT16(2,s.raw_y);TEST_ASSERT_EQUAL_UINT32(77,s.timestamp);TEST_ASSERT_EQUAL_INT(0,xy_adxl362_deinit(&d));}
static void fail(void){xy_adxl362_t d;xy_adxl362_sample_t s={11,22,33,44};int bus,cs;TEST_ASSERT_EQUAL_INT(0,xy_adxl362_init(&d,&bus,&cs));d.sample=s;er=XY_DEVICE_TIMEOUT;TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_adxl362_read(&d,&s));TEST_ASSERT_EQUAL_INT16(11,s.raw_x);d.spi_dev.base.initialized=0;TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,xy_adxl362_read(&d,&s));}
int main(void){UNITY_BEGIN();RUN_TEST(ok);RUN_TEST(fail);return UNITY_END();}
