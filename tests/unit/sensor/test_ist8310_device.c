#include "unity.h"
#include "xy_ist8310.h"
#include <string.h>
static unsigned i; static uint8_t id=XY_IST8310_WHOAMI, raw[6]={1,0,0xfe,0xff,0x10,0}; static xy_error_t er;
xy_error_t xy_i2c_device_init(xy_i2c_device_t*d,void*h,uint16_t a,uint32_t t){memset(d,0,sizeof(*d));d->base.initialized=1;d->i2c_handle=h;d->dev_addr=a;d->timeout=t;return 0;}
xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t*d,uint8_t r,uint8_t*b,size_t n){TEST_ASSERT_TRUE(d->base.initialized);i++;if(er)return er;if(r==0){*b=id;}else memcpy(b,raw,n);return 0;}
xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t*d,uint8_t r,const uint8_t*b,size_t n){(void)r;(void)b;(void)n;TEST_ASSERT_TRUE(d->base.initialized);return er;}
uint32_t xy_hal_sys_get_tick_count(void){return 222333;}
void setUp(void){i=0;er=0;}void tearDown(void){}
static void ok(void){xy_ist8310_t d;xy_ist8310_sample_t s;int bus;TEST_ASSERT_EQUAL_INT(0,xy_ist8310_init(&d,&bus));TEST_ASSERT_EQUAL_INT(0,xy_ist8310_read(&d,&s));TEST_ASSERT_EQUAL_INT16(1,s.raw_x);TEST_ASSERT_EQUAL_INT16(-2, s.raw_y);TEST_ASSERT_EQUAL_UINT32(222333,s.timestamp);TEST_ASSERT_EQUAL_INT(0,xy_ist8310_deinit(&d));}
static void fail(void){xy_ist8310_t d;xy_ist8310_sample_t s={11,22,33,44};int bus;TEST_ASSERT_EQUAL_INT(0,xy_ist8310_init(&d,&bus));d.sample=s;er=XY_DEVICE_TIMEOUT;TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_ist8310_read(&d,&s));TEST_ASSERT_EQUAL_INT16(11,s.raw_x);d.i2c_dev.base.initialized=0;TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,xy_ist8310_read(&d,&s));}
int main(void){UNITY_BEGIN();RUN_TEST(ok);RUN_TEST(fail);return UNITY_END();}
