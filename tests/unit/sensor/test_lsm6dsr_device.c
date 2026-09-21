#include "unity.h"
#include "xy_lsm6dsr.h"
#include <string.h>
static xy_error_t er; static uint8_t a[6] = {1,0,2,0,3,0}, g[6] = {4,0,5,0,6,0};
xy_error_t xy_i2c_device_init(xy_i2c_device_t *d, void *h, uint16_t x, uint32_t t) { memset(d,0,sizeof(*d)); d->base.initialized=1; d->i2c_handle=h; d->dev_addr=x; d->timeout=t; return 0; }
xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *d, uint8_t r, uint8_t *b, size_t n) { TEST_ASSERT_TRUE(d->base.initialized); if(er) return er; if(r==0x0f) *b=XY_LSM6DSR_WHOAMI; else memcpy(b,r==0x28?a:g,n); return 0; }
xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *d, uint8_t r, const uint8_t *b, size_t n) { (void)r;(void)b;(void)n; TEST_ASSERT_TRUE(d->base.initialized); return er; }
void xy_hal_delay_ms(uint32_t x) { (void)x; } uint32_t xy_hal_sys_get_tick_count(void) { return 77; } void setUp(void) { er=0; } void tearDown(void) {}
static void ok(void) { xy_lsm6dsr_t d; xy_lsm6dsr_sample_t s; int bus; TEST_ASSERT_EQUAL_INT(0,xy_lsm6dsr_init(&d,&bus)); TEST_ASSERT_EQUAL_INT(0,xy_lsm6dsr_read(&d,&s)); TEST_ASSERT_EQUAL_INT16(1,s.accel_x); TEST_ASSERT_EQUAL_INT16(4,s.gyro_x); TEST_ASSERT_EQUAL_UINT32(77,s.timestamp); TEST_ASSERT_EQUAL_INT(0,xy_lsm6dsr_deinit(&d)); }
static void fail(void) { xy_lsm6dsr_t d; xy_lsm6dsr_sample_t s={11,22,33,44,55,66,77}; int bus; TEST_ASSERT_EQUAL_INT(0,xy_lsm6dsr_init(&d,&bus)); d.sample=s; er=XY_DEVICE_TIMEOUT; TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,xy_lsm6dsr_read(&d,&s)); TEST_ASSERT_EQUAL_INT16(11,s.accel_x); d.i2c_dev.base.initialized=0; TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,xy_lsm6dsr_read(&d,&s)); }
int main(void) { UNITY_BEGIN(); RUN_TEST(ok); RUN_TEST(fail); return UNITY_END(); }
