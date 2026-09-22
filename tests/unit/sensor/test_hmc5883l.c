#include "unity.h"
#include "xy_hmc5883l.h"
#include <string.h>

static uint8_t id[3], raw[6], status, gain_reg;
static int read_result, write_result;
static int init_result;
static unsigned writes;

int xy_i2c_device_init(xy_i2c_device_t *d, void *h, uint16_t a, uint32_t t)
{ memset(d,0,sizeof(*d)); d->i2c_handle=h; d->dev_addr=a; d->timeout=t; d->base.initialized=true; return init_result; }
int xy_i2c_device_read_reg(xy_i2c_device_t *d,uint8_t r,uint8_t *p,size_t n)
{ (void)d; if(read_result)return read_result; if(r==0x0A)memcpy(p,id,n);else if(r==0x09)*p=status;else if(r==0x01)*p=gain_reg;else memcpy(p,raw,n);return XY_DEVICE_OK; }
int xy_i2c_device_write_reg(xy_i2c_device_t*d,uint8_t r,const uint8_t*p,size_t n)
{ (void)d;(void)r;(void)p;(void)n;writes++;return write_result; }
void setUp(void){memset(id,0,sizeof(id));memset(raw,0,sizeof(raw));status=0;gain_reg=0x20U;read_result=write_result=init_result=0;writes=0;}
void tearDown(void){}
static void init_ok(xy_hmc5883l_t*d){int bus;id[0]='H';id[1]='4';id[2]='3';TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_init(d,&bus));}
static void test_identity_and_config(void){xy_hmc5883l_t d;init_ok(&d);TEST_ASSERT_TRUE(d.initialized);TEST_ASSERT_EQUAL_UINT8(0x1E,d.i2c_dev.dev_addr);TEST_ASSERT_EQUAL_UINT(3,writes);}
static void test_wrong_identity_rejected(void){xy_hmc5883l_t d;int bus;id[0]='Q';TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,xy_hmc5883l_init(&d,&bus));TEST_ASSERT_FALSE(d.initialized);}
static void test_xyz_reorder_and_atomic_error(void){xy_hmc5883l_t d;xy_hmc5883l_data_t o={7,8,9};init_ok(&d);uint8_t v[]={0,1,0,3,0,2};memcpy(raw,v,6);TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_read(&d,&o));TEST_ASSERT_EQUAL_INT16(1,o.x);TEST_ASSERT_EQUAL_INT16(2,o.y);TEST_ASSERT_EQUAL_INT16(3,o.z);read_result=XY_DEVICE_TIMEOUT;o=(xy_hmc5883l_data_t){7,8,9};TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_hmc5883l_read(&d,&o));TEST_ASSERT_EQUAL_INT16(7,o.x);}
static void test_ready(void){xy_hmc5883l_t d;uint8_t r=0;init_ok(&d);status=1;TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_data_ready(&d,&r));TEST_ASSERT_EQUAL_UINT8(1,r);}
static void test_gain_validation_and_transport(void){xy_hmc5883l_t d;xy_hmc5883l_gain_t gain;init_ok(&d);TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_set_gain(&d,XY_HMC5883L_GAIN_1_30_GA));TEST_ASSERT_EQUAL_UINT(4,writes);TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_get_gain(&d,&gain));TEST_ASSERT_EQUAL_INT(XY_HMC5883L_GAIN_1_30_GA,gain);TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,xy_hmc5883l_set_gain(&d,(xy_hmc5883l_gain_t)0x40U));write_result=XY_DEVICE_TIMEOUT;TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_hmc5883l_set_gain(&d,XY_HMC5883L_GAIN_8_10_GA));}
static void test_field_conversion(void){xy_hmc5883l_t d;xy_hmc5883l_field_t f;init_ok(&d);gain_reg=0xE0U;uint8_t v[]={0x0A,0x00,0x08,0xCA,0x05,0x00};memcpy(raw,v,6);TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_read_field(&d,&f));TEST_ASSERT_EQUAL_INT32(1000, f.x_mgauss);TEST_ASSERT_EQUAL_INT32(500, f.y_mgauss);TEST_ASSERT_EQUAL_INT32(1000, f.z_mgauss);}
static void test_default_gain_uses_distinct_xy_and_z_sensitivity(void){xy_hmc5883l_t d;xy_hmc5883l_field_t f;init_ok(&d);gain_reg=0x20U;uint8_t v[]={0x04,0x42,0x03,0xD4,0x02,0x21};memcpy(raw,v,6);TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_read_field(&d,&f));TEST_ASSERT_EQUAL_INT32(1000, f.x_mgauss);TEST_ASSERT_EQUAL_INT32(500, f.y_mgauss);TEST_ASSERT_EQUAL_INT32(1000, f.z_mgauss);}
static void test_read_overflow_preserves_output(void){xy_hmc5883l_t d;xy_hmc5883l_data_t o={7,8,9};init_ok(&d);uint8_t v[]={0xF0,0x00,0x00,0x01,0x00,0x02};memcpy(raw,v,6);TEST_ASSERT_EQUAL_INT(XY_ERROR_OVERFLOW,xy_hmc5883l_read(&d,&o));TEST_ASSERT_EQUAL_INT16(7,o.x);TEST_ASSERT_EQUAL_INT16(8,o.y);TEST_ASSERT_EQUAL_INT16(9,o.z);TEST_ASSERT_EQUAL_INT16(0,d.data.x);}
static void test_read_field_preserves_all_public_state_on_gain_failure(void)
{
    xy_hmc5883l_t d;
    xy_hmc5883l_field_t o={11,22,33};
    xy_hmc5883l_field_t snapshot=o;
    xy_hmc5883l_data_t data_snapshot;
    xy_hmc5883l_gain_t gain_snapshot;
    init_ok(&d);
    d.data=(xy_hmc5883l_data_t){7,8,9};
    d.gain=XY_HMC5883L_GAIN_1_30_GA;
    data_snapshot=d.data;
    gain_snapshot=d.gain;
    uint8_t v[]={0x01,0x00,0x00,0x80,0x00,0x40};
    memcpy(raw,v,6);
    gain_reg=0x40U;
    TEST_ASSERT_EQUAL_INT(XY_ERROR_FAIL,xy_hmc5883l_read_field(&d,&o));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot,&o,sizeof(o));
    TEST_ASSERT_EQUAL_MEMORY(&data_snapshot,&d.data,sizeof(d.data));
    TEST_ASSERT_EQUAL_INT(gain_snapshot,d.gain);
}
static void test_read_field_preserves_all_public_state_on_gain_transport_error(void)
{
    xy_hmc5883l_t d;
    xy_hmc5883l_field_t o={11,22,33};
    xy_hmc5883l_field_t snapshot=o;
    xy_hmc5883l_data_t data_snapshot;
    xy_hmc5883l_gain_t gain_snapshot;
    init_ok(&d);
    d.data=(xy_hmc5883l_data_t){7,8,9};
    d.gain=XY_HMC5883L_GAIN_1_30_GA;
    data_snapshot=d.data;
    gain_snapshot=d.gain;
    uint8_t v[]={0x01,0x00,0x00,0x80,0x00,0x40};
    memcpy(raw,v,6);
    read_result=XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_hmc5883l_read_field(&d,&o));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot,&o,sizeof(o));
    TEST_ASSERT_EQUAL_MEMORY(&data_snapshot,&d.data,sizeof(d.data));
    TEST_ASSERT_EQUAL_INT(gain_snapshot,d.gain);
}
static void test_init_identity_failure_clears_nested_lifecycle(void){xy_hmc5883l_t d;int bus;memset(&d,0xA5,sizeof(d));id[0]='Q';TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,xy_hmc5883l_init(&d,&bus));TEST_ASSERT_FALSE(d.initialized);TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);TEST_ASSERT_NULL(d.i2c_dev.i2c_handle);TEST_ASSERT_EQUAL_UINT(0,writes);}
static void test_deinit_clears_transport(void){xy_hmc5883l_t d;init_ok(&d);TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,xy_hmc5883l_deinit(&d));TEST_ASSERT_FALSE(d.initialized);TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);TEST_ASSERT_NULL(d.i2c_dev.i2c_handle);}
static void test_init_config_failure_is_atomic(void){xy_hmc5883l_t d;int bus;id[0]='H';id[1]='4';id[2]='3';write_result=XY_DEVICE_TIMEOUT;TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_hmc5883l_init(&d,&bus));TEST_ASSERT_FALSE(d.initialized);TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);}
static void test_init_helper_failure_clears_nested_lifecycle(void){xy_hmc5883l_t d;int bus;memset(&d,0xA5,sizeof(d));init_result=XY_DEVICE_TIMEOUT;TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_hmc5883l_init(&d,&bus));TEST_ASSERT_FALSE(d.initialized);TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);TEST_ASSERT_NULL(d.i2c_dev.i2c_handle);TEST_ASSERT_EQUAL_UINT(0,writes);}
int main(void){UNITY_BEGIN();RUN_TEST(test_identity_and_config);RUN_TEST(test_wrong_identity_rejected);RUN_TEST(test_xyz_reorder_and_atomic_error);RUN_TEST(test_ready);RUN_TEST(test_gain_validation_and_transport);RUN_TEST(test_field_conversion);RUN_TEST(test_default_gain_uses_distinct_xy_and_z_sensitivity);RUN_TEST(test_read_overflow_preserves_output);RUN_TEST(test_read_field_preserves_all_public_state_on_gain_failure);RUN_TEST(test_read_field_preserves_all_public_state_on_gain_transport_error);RUN_TEST(test_init_identity_failure_clears_nested_lifecycle);RUN_TEST(test_deinit_clears_transport);RUN_TEST(test_init_config_failure_is_atomic);RUN_TEST(test_init_helper_failure_clears_nested_lifecycle);return UNITY_END();}
