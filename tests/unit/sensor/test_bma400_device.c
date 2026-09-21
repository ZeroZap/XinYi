#include "unity.h"
#include "xy_bma400.h"
#include <string.h>
static unsigned i; static xy_error_t er; static uint8_t raw[6]={1,0,2,0,3,0};
xy_error_t xy_i2c_device_init(xy_i2c_device_t*d,void*h,uint16_t a,uint32_t t){memset(d,0,sizeof(*d));d->base.initialized=1;d->i2c_handle=h;d->dev_addr=a;d->timeout=t;return 0;}
xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t*d,uint8_t r,uint8_t*b,size_t n){TEST_ASSERT_TRUE(d->base.initialized);i++;if(er)return er;if(r==0)*b=XY_BMA400_CHIP_ID;else memcpy(b,raw,n);return 0;}
xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t*d,uint8_t r,const uint8_t*b,size_t n){(void)r;(void)b;(void)n;TEST_ASSERT_TRUE(d->base.initialized);return er;}
void xy_hal_delay_ms(uint32_t ms){(void)ms;} uint32_t xy_hal_sys_get_tick_count(void){return 77;}
void setUp(void){i=0;er=0;}void tearDown(void){}
static void ok(void){xy_bma400_t d;xy_bma400_sample_t s;int bus;TEST_ASSERT_EQUAL_INT(0,xy_bma400_init(&d,&bus));TEST_ASSERT_EQUAL_INT(0,xy_bma400_read(&d,&s));TEST_ASSERT_EQUAL_INT16(1,s.raw_x);TEST_ASSERT_EQUAL_INT16(2,s.raw_y);TEST_ASSERT_EQUAL_UINT32(77,s.timestamp);TEST_ASSERT_EQUAL_INT(0,xy_bma400_deinit(&d));}
static void fail(void){xy_bma400_t d;xy_bma400_sample_t s={11,22,33,44};int bus;TEST_ASSERT_EQUAL_INT(0,xy_bma400_init(&d,&bus));d.sample=s;er=XY_DEVICE_TIMEOUT;TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_bma400_read(&d,&s));TEST_ASSERT_EQUAL_INT16(11,s.raw_x);d.i2c_dev.base.initialized=0;TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,xy_bma400_read(&d,&s));}
int main(void){UNITY_BEGIN();RUN_TEST(ok);RUN_TEST(fail);return UNITY_END();}
