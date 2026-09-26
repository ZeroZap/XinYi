#include "unity.h"
#include "xy_ina228.h"

#include <string.h>

#define MAX_OPS 32U

typedef struct { uint8_t reg; uint8_t data[5]; uint8_t len; int ret; } op_t;
static op_t reads[MAX_OPS], writes[MAX_OPS];
static size_t nr, ir, nw, iw;
static int init_ret;
static uint32_t tick;

static void qread(uint8_t reg, uint64_t value, uint8_t len, int ret)
{
    op_t *o = &reads[nr++]; o->reg=reg;o->len=len;o->ret=ret;
    for (uint8_t i=0;i<len;i++) o->data[len-1U-i]=(uint8_t)(value>>(8U*i));
}
static void qwrite(uint8_t reg,uint16_t value,int ret)
{ op_t *o=&writes[nw++];o->reg=reg;o->len=2;o->ret=ret;o->data[0]=value>>8;o->data[1]=value; }

xy_error_t xy_i2c_device_init(xy_i2c_device_t*d,void*h,uint16_t a,uint32_t t)
{ (void)a;(void)t;if(init_ret)return init_ret;memset(d,0,sizeof(*d));d->base.initialized=1;d->i2c_handle=h;return 0; }
xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t*d,uint8_t r,uint8_t*b,size_t n)
{ TEST_ASSERT_TRUE(d->base.initialized);TEST_ASSERT_LESS_THAN(nr,ir);op_t*o=&reads[ir++];TEST_ASSERT_EQUAL_HEX8(o->reg,r);TEST_ASSERT_EQUAL_UINT(o->len,n);if(!o->ret)memcpy(b,o->data,n);return o->ret; }
xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t*d,uint8_t r,const uint8_t*b,size_t n)
{ TEST_ASSERT_TRUE(d->base.initialized);TEST_ASSERT_LESS_THAN(nw,iw);op_t*o=&writes[iw++];TEST_ASSERT_EQUAL_HEX8(o->reg,r);TEST_ASSERT_EQUAL_UINT(o->len,n);TEST_ASSERT_EQUAL_UINT8_ARRAY(o->data,b,n);return o->ret; }
uint32_t xy_os_tick_get(void){return tick;}
void setUp(void){memset(reads,0,sizeof(reads));memset(writes,0,sizeof(writes));nr=ir=nw=iw=0;init_ret=0;tick=42;}
void tearDown(void){}

static xy_ina22x_config_t cfg(void){xy_ina22x_config_t c={1000U,100U,XY_INA22X_ADC_CONFIG_DEFAULT,XY_INA22X_SHUNT_RANGE_163_84_MV};return c;}
static void init_ok(xy_ina228_t*d){int bus;qread(0x3E,0x5449,2,0);qread(0x3F,0x2281,2,0);qwrite(0,0,0);qwrite(1,0xFB68,0);qwrite(2,1310,0);TEST_ASSERT_EQUAL_INT(0,xy_ina228_init(d,&bus,0x40,&(xy_ina22x_config_t){1000U,100U,0xFB68,0}));}

static void test_init_identity_and_config(void)
{ xy_ina228_t d;int bus;xy_ina22x_config_t c=cfg();qread(0x3E,0x5449,2,0);qread(0x3F,0x2282,2,0);qwrite(0,0,0);qwrite(1,0xFB68,0);qwrite(2,1310,0);TEST_ASSERT_EQUAL_INT(0,xy_ina228_init(&d,&bus,0x4F,&c));TEST_ASSERT_TRUE(d.initialized);TEST_ASSERT_EQUAL_UINT(3,iw); }
static void test_read_converts_and_stages(void)
{ xy_ina228_t d;xy_ina22x_sample_t s;init_ok(&d);qread(4,0x001000,3,0);qread(5,0x010000,3,0);qread(6,0x0100,2,0);qread(7,0x002000,3,0);qread(8,10,3,0);qread(9,20,5,0);qread(10,30,5,0);qread(11,1,2,0);TEST_ASSERT_EQUAL_INT(0,xy_ina228_read(&d,&s));TEST_ASSERT_FLOAT_WITHIN(0.001,80.0,s.shunt_voltage_uv);TEST_ASSERT_FLOAT_WITHIN(0.001,800.0,s.bus_voltage_mv);TEST_ASSERT_FLOAT_WITHIN(0.001,51.2,s.current_ma);TEST_ASSERT_FLOAT_WITHIN(0.001,3.2,s.power_mw);TEST_ASSERT_FLOAT_WITHIN(0.001,102.4,s.energy_mj);TEST_ASSERT_FLOAT_WITHIN(0.001,48.0,s.charge_mc);TEST_ASSERT_EQUAL_UINT32(42,s.timestamp); }
static void test_config_rejects_calibration_overflow(void)
{ xy_ina22x_config_t c={UINT32_MAX,UINT32_MAX,XY_INA22X_ADC_CONFIG_DEFAULT,XY_INA22X_SHUNT_RANGE_163_84_MV};uint16_t cal=0xA5A5U;TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,xy_ina22x_core_config_valid(&c,&cal));TEST_ASSERT_EQUAL_HEX16(0xA5A5U,cal); }
static void test_config_rejects_noncontinuous_accumulator_mode(void)
{
    xy_ina22x_config_t c = cfg();
    uint16_t cal = 0xA5A5U;
    int bus;
    xy_ina228_t dev;
    size_t reads_before = nr;
    size_t writes_before = nw;

    c.adc_config = 0x7B68U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ina22x_core_config_valid(&c, &cal));
    TEST_ASSERT_EQUAL_HEX16(0xA5A5U, cal);
    memset(&dev, 0xA5, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ina228_init(&dev, &bus, 0x40, &c));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ina228_t){0}, &dev, sizeof(dev));
    TEST_ASSERT_EQUAL_UINT(reads_before, nr);
    TEST_ASSERT_EQUAL_UINT(writes_before, nw);
}
static void test_read_failure_preserves_output(void)
{ xy_ina228_t d;xy_ina22x_sample_t s;init_ok(&d);memset(&s,0xA5,sizeof(s));xy_ina22x_sample_t old=s;qread(4,0,3,XY_DEVICE_TIMEOUT);TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_ina228_read(&d,&s));TEST_ASSERT_EQUAL_MEMORY(&old,&s,sizeof(s));TEST_ASSERT_EQUAL_UINT(1,ir-2); }
static void test_invalid_identity_and_deinit_retry(void)
{ xy_ina228_t d;int bus;xy_ina22x_config_t c=cfg();qread(0x3E,0x5449,2,0);qread(0x3F,0x2291,2,0);TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,xy_ina228_init(&d,&bus,0x40,&c));init_ok(&d);qwrite(1,0,XY_DEVICE_BUSY);TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY,xy_ina228_deinit(&d));TEST_ASSERT_TRUE(d.initialized);qwrite(1,0,0);TEST_ASSERT_EQUAL_INT(0,xy_ina228_deinit(&d));TEST_ASSERT_FALSE(d.initialized); }
static void test_init_failure_clears_owner(void)
{
    xy_ina228_t d;
    int bus;
    xy_ina22x_config_t c = cfg();

    memset(&d, 0xA5, sizeof(d));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ina228_init(&d, NULL, 0x40, &c));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ina228_t){0}, &d, sizeof(d));

    memset(&d, 0xA5, sizeof(d));
    init_ret = XY_DEVICE_BUSY;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_ina228_init(&d, &bus, 0x40, &c));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ina228_t){0}, &d, sizeof(d));

    memset(&d, 0xA5, sizeof(d));
    init_ret = XY_DEVICE_OK;
    qread(0x3E, 0x5449, 2, 0);
    qread(0x3F, 0x2291, 2, 0);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_ina228_init(&d, &bus, 0x40, &c));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ina228_t){0}, &d, sizeof(d));
}

static void test_diagnostic_faults_reject_complete_sample(void)
{
    xy_ina228_t d;
    xy_ina22x_sample_t output;
    xy_ina22x_sample_t cache;
    const uint16_t diagnostics[] = {0U, XY_INA22X_DIAG_MATHOF | XY_INA22X_DIAG_MEMSTAT,
                                    XY_INA22X_DIAG_ENERGYOF | XY_INA22X_DIAG_MEMSTAT,
                                    XY_INA22X_DIAG_CHARGEOF | XY_INA22X_DIAG_MEMSTAT};

    init_ok(&d);
    memset(&output, 0xA5, sizeof(output));
    cache = d.core.sample;
    for (size_t i = 0U; i < sizeof(diagnostics) / sizeof(diagnostics[0]); ++i) {
        qread(4, 0, 3, 0); qread(5, 0, 3, 0); qread(6, 0, 2, 0);
        qread(7, 0, 3, 0); qread(8, 0, 3, 0); qread(9, 0, 5, 0);
        qread(10, 0, 5, 0); qread(11, diagnostics[i], 2, 0);
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ina228_read(&d, &output));
        TEST_ASSERT_EQUAL_MEMORY(&cache, &d.core.sample, sizeof(cache));
        TEST_ASSERT_EQUAL_UINT8(0xA5U, ((uint8_t *)&output)[0]);
    }
}

int main(void){UNITY_BEGIN();RUN_TEST(test_init_identity_and_config);RUN_TEST(test_read_converts_and_stages);RUN_TEST(test_config_rejects_calibration_overflow);RUN_TEST(test_config_rejects_noncontinuous_accumulator_mode);RUN_TEST(test_read_failure_preserves_output);RUN_TEST(test_invalid_identity_and_deinit_retry);RUN_TEST(test_init_failure_clears_owner);RUN_TEST(test_diagnostic_faults_reject_complete_sample);return UNITY_END();}
