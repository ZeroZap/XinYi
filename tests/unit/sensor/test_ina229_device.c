#include "unity.h"
#include "xy_ina229.h"

#include <string.h>

#define MAX_FRAMES 32U
typedef struct { uint8_t tx[6],rx[6],len;int ret; } frame_t;
static frame_t frames[MAX_FRAMES];static size_t nf,idx;static int init_ret;static uint32_t tick;
static void qr(uint8_t reg,uint64_t value,uint8_t len,int ret){frame_t*f=&frames[nf++];f->len=len+1;f->tx[0]=(reg<<2)|1;f->ret=ret;for(uint8_t i=0;i<len;i++)f->rx[len-i]=(uint8_t)(value>>(8*i));}
static void qw(uint8_t reg,uint16_t value,int ret){frame_t*f=&frames[nf++];f->len=3;f->tx[0]=reg<<2;f->tx[1]=value>>8;f->tx[2]=value;f->ret=ret;}
xy_error_t xy_spi_device_init(xy_spi_device_t*d,void*h,void*cs,uint32_t speed,uint8_t mode){TEST_ASSERT_EQUAL_UINT32(10000000,speed);TEST_ASSERT_EQUAL_UINT8(1,mode);if(init_ret)return init_ret;memset(d,0,sizeof(*d));d->base.initialized=1;d->spi_handle=h;d->cs_pin=cs;return 0;}
xy_error_t xy_spi_device_transfer(xy_spi_device_t*d,const uint8_t*tx,uint8_t*rx,size_t len){TEST_ASSERT_TRUE(d->base.initialized);TEST_ASSERT_LESS_THAN(nf,idx);frame_t*f=&frames[idx++];TEST_ASSERT_EQUAL_UINT(f->len,len);TEST_ASSERT_EQUAL_UINT8_ARRAY(f->tx,tx,len);if(f->ret>=0)memcpy(rx,f->rx,len);return f->ret?f->ret:(int)len;}
uint32_t xy_os_tick_get(void){return tick;}
void setUp(void){memset(frames,0,sizeof(frames));nf=idx=0;init_ret=0;tick=77;}
void tearDown(void){}
static xy_ina22x_config_t cfg(void){xy_ina22x_config_t c={1000U,100U,0xFB68U,0};return c;}
static void init_ok(xy_ina229_t*d){int bus,cs;xy_ina22x_config_t c=cfg();qr(0x3E,0x5449,2,0);qr(0x3F,0x2291,2,0);qw(0,0,0);qw(1,0xFB68,0);qw(2,1310,0);TEST_ASSERT_EQUAL_INT(0,xy_ina229_init(d,&bus,&cs,&c));}
static void test_init_spi_frames(void){xy_ina229_t d;init_ok(&d);TEST_ASSERT_TRUE(d.initialized);TEST_ASSERT_EQUAL_UINT(5,idx);}
static void test_read_converts_signed_values(void){xy_ina229_t d;xy_ina22x_sample_t s;init_ok(&d);qr(4,0xFFF000,3,0);qr(5,0x010000,3,0);qr(6,0xFF80,2,0);qr(7,0xFFE000,3,0);qr(8,10,3,0);qr(9,20,5,0);qr(10,0xFFFFFFFFFEULL,5,0);TEST_ASSERT_EQUAL_INT(0,xy_ina229_read(&d,&s));TEST_ASSERT_FLOAT_WITHIN(.001,-80,s.shunt_voltage_uv);TEST_ASSERT_FLOAT_WITHIN(.001,-51.2,s.current_ma);TEST_ASSERT_FLOAT_WITHIN(.001,-.2,s.charge_mc);TEST_ASSERT_FLOAT_WITHIN(.001,-1,s.die_temperature_c);TEST_ASSERT_EQUAL_UINT32(77,s.timestamp);}
static void test_short_transfer_and_error_preserve_output(void){xy_ina229_t d;xy_ina22x_sample_t s;init_ok(&d);memset(&s,0xA5,sizeof(s));xy_ina22x_sample_t old=s;qr(4,0,3,2);TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,xy_ina229_read(&d,&s));TEST_ASSERT_EQUAL_MEMORY(&old,&s,sizeof(s));}
static void test_identity_deinit_and_invalid_cs(void){xy_ina229_t d;int bus,cs;xy_ina22x_config_t c=cfg();TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,xy_ina229_init(&d,&bus,NULL,&c));qr(0x3E,0x5449,2,0);qr(0x3F,0x2281,2,0);TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,xy_ina229_init(&d,&bus,&cs,&c));init_ok(&d);qw(1,0,XY_DEVICE_TIMEOUT);TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_ina229_deinit(&d));TEST_ASSERT_TRUE(d.initialized);qw(1,0,0);TEST_ASSERT_EQUAL_INT(0,xy_ina229_deinit(&d));TEST_ASSERT_FALSE(d.initialized);}
int main(void){UNITY_BEGIN();RUN_TEST(test_init_spi_frames);RUN_TEST(test_read_converts_signed_values);RUN_TEST(test_short_transfer_and_error_preserve_output);RUN_TEST(test_identity_deinit_and_invalid_cs);return UNITY_END();}
