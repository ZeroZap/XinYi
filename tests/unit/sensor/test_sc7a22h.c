#include "unity.h"
#include "xy_sc7a22h.h"
#include <string.h>

static uint8_t regs[256];
static int init_result, read_result, write_result;
static unsigned delay_ms, writes;

int xy_i2c_device_init(xy_i2c_device_t *d, void *h, uint16_t a, uint32_t t)
{
    memset(d, 0, sizeof(*d));
    if (init_result != XY_DEVICE_OK) return init_result;
    d->i2c_handle = h; d->dev_addr = a; d->timeout = t; d->base.initialized = true;
    return XY_DEVICE_OK;
}
int xy_i2c_device_read_reg(xy_i2c_device_t *d, uint8_t r, uint8_t *p, size_t n)
{
    (void)d; if (read_result != XY_DEVICE_OK) return read_result;
    memcpy(p, &regs[r], n); return XY_DEVICE_OK;
}
int xy_i2c_device_write_reg(xy_i2c_device_t *d, uint8_t r, const uint8_t *p, size_t n)
{
    (void)d; if (write_result != XY_DEVICE_OK) return write_result;
    memcpy(&regs[r], p, n); writes++; return XY_DEVICE_OK;
}
void xy_hal_delay_ms(uint32_t ms) { delay_ms += ms; }

void setUp(void)
{
    memset(regs, 0, sizeof(regs)); regs[0x01] = 0x18U;
    init_result = read_result = write_result = XY_DEVICE_OK; delay_ms = writes = 0U;
}
void tearDown(void) {}

static void init_ok(xy_sc7a22h_t *d)
{
    int bus;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_init(d, &bus));
}
static void test_init_vendor_profile(void)
{
    xy_sc7a22h_t d; init_ok(&d);
    TEST_ASSERT_TRUE(d.initialized);
    TEST_ASSERT_EQUAL_HEX8(0x18, d.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_HEX8(0x04, regs[0x7D]);
    TEST_ASSERT_EQUAL_HEX8(0x07, regs[0x40]);
    TEST_ASSERT_EQUAL_HEX8(0x01, regs[0x41]);
    TEST_ASSERT_EQUAL_HEX8(0x50, regs[0x05]);
    TEST_ASSERT_EQUAL_HEX8(0x01, regs[0x06]);
    TEST_ASSERT_EQUAL_HEX8(0x05, regs[0x08]);
    TEST_ASSERT_EQUAL_UINT(6, writes);
    TEST_ASSERT_EQUAL_UINT(12, delay_ms);
}
static void test_ready_contract(void)
{
    xy_sc7a22h_t d; uint8_t ready = 7U; init_ok(&d);
    regs[0x0B] = 0x03U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_data_ready(&d, &ready));
    TEST_ASSERT_EQUAL_UINT8(1, ready);
    regs[0x0B] = 0x01U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_data_ready(&d, &ready));
    TEST_ASSERT_EQUAL_UINT8(0, ready);
}
static void test_xyz_and_mg_conversion(void)
{
    xy_sc7a22h_t d; xy_sc7a22h_accel_t a; init_ok(&d);
    regs[0x0C]=0x20; regs[0x0D]=0x00; regs[0x0E]=0xF0; regs[0x0F]=0x00;
    regs[0x10]=0x08; regs[0x11]=0x00;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_read_accel(&d, &a));
    TEST_ASSERT_EQUAL_INT32(999, a.x_mg);
    TEST_ASSERT_EQUAL_INT32(-499, a.y_mg);
    TEST_ASSERT_EQUAL_INT32(249, a.z_mg);
}
static void test_read_failure_preserves_output(void)
{
    xy_sc7a22h_t d; xy_sc7a22h_accel_t a={11,22,33}, old=a; init_ok(&d);
    read_result=XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_read_accel(&d, &a));
    TEST_ASSERT_EQUAL_MEMORY(&old, &a, sizeof(a));
}
static void test_identity_and_write_failures_are_atomic(void)
{
    xy_sc7a22h_t d; int bus; regs[0x01]=0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_sc7a22h_init(&d,&bus));
    TEST_ASSERT_FALSE(d.initialized); TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);
    setUp(); write_result=XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_init(&d,&bus));
    TEST_ASSERT_FALSE(d.initialized); TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);
}
static void test_power_down_and_deinit_are_fail_closed(void)
{
    xy_sc7a22h_t d; init_ok(&d);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_power_down(&d));
    TEST_ASSERT_EQUAL_HEX8(0x00, regs[0x7D]);
    TEST_ASSERT_TRUE(d.initialized);
    regs[0x7D] = 0x04U; write_result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_deinit(&d));
    TEST_ASSERT_TRUE(d.initialized); TEST_ASSERT_TRUE(d.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_HEX8(0x04, regs[0x7D]);
    write_result = XY_DEVICE_OK;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_deinit(&d));
    TEST_ASSERT_FALSE(d.initialized); TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_HEX8(0x00, regs[0x7D]);
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_vendor_profile);
    RUN_TEST(test_ready_contract);
    RUN_TEST(test_xyz_and_mg_conversion);
    RUN_TEST(test_read_failure_preserves_output);
    RUN_TEST(test_identity_and_write_failures_are_atomic);
    RUN_TEST(test_power_down_and_deinit_are_fail_closed);
    return UNITY_END();
}
