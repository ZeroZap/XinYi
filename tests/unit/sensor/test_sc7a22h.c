#include "unity.h"
#include "xy_sc7a22h.h"
#include <string.h>

static uint8_t regs[256];
static int init_result, read_result, write_result, init_establish_transport;
static unsigned delay_ms, reads, writes, write_attempts;
static unsigned read_fail_on_call, write_fail_on_call;

int xy_i2c_device_init(xy_i2c_device_t *d, void *h, uint16_t a, uint32_t t)
{
    memset(d, 0, sizeof(*d));
    if (init_result != XY_DEVICE_OK) return init_result;
    d->i2c_handle = init_establish_transport ? h : NULL;
    d->dev_addr = a;
    d->timeout = t;
    d->base.initialized = init_establish_transport;
    return XY_DEVICE_OK;
}
int xy_i2c_device_read_reg(xy_i2c_device_t *d, uint8_t r, uint8_t *p, size_t n)
{
    (void)d; reads++;
    if (read_fail_on_call == reads) return XY_DEVICE_TIMEOUT;
    if (read_result != XY_DEVICE_OK) return read_result;
    memcpy(p, &regs[r], n); return XY_DEVICE_OK;
}
int xy_i2c_device_write_reg(xy_i2c_device_t *d, uint8_t r, const uint8_t *p, size_t n)
{
    (void)d; write_attempts++;
    if (write_fail_on_call == write_attempts) return XY_DEVICE_TIMEOUT;
    if (write_result != XY_DEVICE_OK) return write_result;
    memcpy(&regs[r], p, n); writes++; return XY_DEVICE_OK;
}
void xy_hal_delay_ms(uint32_t ms) { delay_ms += ms; }

void setUp(void)
{
    memset(regs, 0, sizeof(regs)); regs[0x01] = 0x18U;
    init_result = read_result = write_result = XY_DEVICE_OK;
    init_establish_transport = 1;
    delay_ms = reads = writes = write_attempts = 0U;
    read_fail_on_call = write_fail_on_call = 0U;
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
static void test_init_rejects_incomplete_nested_transport_without_io(void)
{
    xy_sc7a22h_t d;
    int bus;

    memset(&d, 0xA5, sizeof(d));
    init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_init(&d, &bus));
    TEST_ASSERT_FALSE(d.initialized);
    TEST_ASSERT_FALSE(d.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(d.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, reads);
    TEST_ASSERT_EQUAL_UINT(0U, write_attempts);
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
static void test_fifo_vendor_sequence_and_count(void)
{
    xy_sc7a22h_t d; uint16_t count = 0xAAAAU; init_ok(&d);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_enable_fifo(&d));
    TEST_ASSERT_EQUAL_HEX8(0x40, regs[0x05]);
    TEST_ASSERT_EQUAL_HEX8(0x04, regs[0x1C]);
    TEST_ASSERT_EQUAL_HEX8(0x10, regs[0x1D]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, regs[0x1E]);
    TEST_ASSERT_EQUAL_HEX8(0x40, d.com_cfg);
    regs[0x1F] = 0x01U; regs[0x20] = 0x23U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_fifo_count(&d, &count));
    TEST_ASSERT_EQUAL_UINT16(0x123U, count);
    regs[0x1F] = 0x10U; regs[0x20] = 0x00U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_fifo_count(&d, &count));
    TEST_ASSERT_EQUAL_UINT16(256U, count);
}
static void test_fifo_failures_preserve_public_state(void)
{
    xy_sc7a22h_t d;
    uint16_t count;
    unsigned step;

    for (step = 1U; step <= 5U; step++) {
        setUp();
        init_ok(&d);
        write_fail_on_call = write_attempts + step;
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_enable_fifo(&d));
        TEST_ASSERT_EQUAL_HEX8(XY_SC7A22H_DEMO_COM_CFG, d.com_cfg);
        TEST_ASSERT_EQUAL_UINT(write_fail_on_call, write_attempts);
    }

    setUp();
    init_ok(&d);
    count = 0xAAAAU;
    read_fail_on_call = reads + 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_fifo_count(&d, &count));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, count);

    setUp();
    init_ok(&d);
    count = 0xAAAAU;
    read_fail_on_call = reads + 2U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_fifo_count(&d, &count));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, count);
}
static void test_status_ready_read_and_setters_are_atomic(void)
{
    xy_sc7a22h_t d;
    xy_sc7a22h_data_t raw_out = {11,22,33};
    xy_sc7a22h_data_t raw_snapshot = raw_out;
    uint8_t status = 0xA5U;
    uint8_t ready = 0x5AU;
    uint8_t config_snapshot;
    uint8_t range_snapshot;

    init_ok(&d);
    d.data_status = 0x33U;
    d.data = (xy_sc7a22h_data_t){44,55,66};
    config_snapshot = d.acc_conf;
    range_snapshot = d.acc_range;

    read_result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_read_status(&d, &status));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, status);
    TEST_ASSERT_EQUAL_UINT8(0x33U, d.data_status);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_data_ready(&d, &ready));
    TEST_ASSERT_EQUAL_UINT8(0x5AU, ready);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_read(&d, &raw_out));
    TEST_ASSERT_EQUAL_MEMORY(&raw_snapshot, &raw_out, sizeof(raw_out));
    TEST_ASSERT_EQUAL_INT16(44, d.data.x);

    read_result = XY_DEVICE_OK;
    write_result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_set_acc_config(&d, 0x55U));
    TEST_ASSERT_EQUAL_UINT8(config_snapshot, d.acc_conf);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a22h_set_acc_range(&d, 0x02U));
    TEST_ASSERT_EQUAL_UINT8(range_snapshot, d.acc_range);
}
static void test_invalid_nested_lifecycle_rejects_public_ops_without_io(void)
{
    xy_sc7a22h_t d;
    xy_sc7a22h_data_t raw = {1,2,3};
    xy_sc7a22h_accel_t accel = {4,5,6};
    uint8_t status = 7U, ready = 8U;
    uint16_t count = 9U;
    unsigned reads_before, writes_before;

    init_ok(&d);
    d.i2c_dev.base.initialized = 0U;
    reads_before = reads;
    writes_before = write_attempts;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_read_config(&d));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_read_status(&d, &status));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_data_ready(&d, &ready));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_read(&d, &raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_read_accel(&d, &accel));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_set_acc_config(&d, 0x55U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_set_acc_range(&d, 0x02U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_enable_fifo(&d));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_fifo_count(&d, &count));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_power_down(&d));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_deinit(&d));
    TEST_ASSERT_EQUAL_UINT(reads_before, reads);
    TEST_ASSERT_EQUAL_UINT(writes_before, write_attempts);
    TEST_ASSERT_EQUAL_UINT8(7U, status);
    TEST_ASSERT_EQUAL_UINT8(8U, ready);
    TEST_ASSERT_EQUAL_UINT16(9U, count);
}
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_vendor_profile);
    RUN_TEST(test_ready_contract);
    RUN_TEST(test_xyz_and_mg_conversion);
    RUN_TEST(test_read_failure_preserves_output);
    RUN_TEST(test_identity_and_write_failures_are_atomic);
    RUN_TEST(test_init_rejects_incomplete_nested_transport_without_io);
    RUN_TEST(test_power_down_and_deinit_are_fail_closed);
    RUN_TEST(test_fifo_vendor_sequence_and_count);
    RUN_TEST(test_fifo_failures_preserve_public_state);
    RUN_TEST(test_status_ready_read_and_setters_are_atomic);
    RUN_TEST(test_invalid_nested_lifecycle_rejects_public_ops_without_io);
    return UNITY_END();
}
