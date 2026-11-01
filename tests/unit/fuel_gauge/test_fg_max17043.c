#include "unity.h"
#include "fff.h"
#include "xy_fg_max17043.h"
#include "xy_fuel_gauge.h"
#include "xy_device.h"

#include <stdint.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

#define REG_VCELL           0x02
#define REG_SOC             0x04
#define REG_VER             0x08
#define REG_CONFIG          0x0C
#define REG_CRATE           0x16

static uint16_t fake_regs[256];
static uint8_t fake_read_failures[256];
static uint8_t fake_write_failures[256];
static xy_i2c_device_t last_bus;
static uint8_t last_write_reg;
static uint16_t last_write_value;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VALUE_FUNC(xy_error_t, xy_i2c_device_init, xy_i2c_device_t *, void *, uint16_t, uint32_t)
FAKE_VALUE_FUNC(xy_error_t, xy_i2c_device_read_reg16_repeated_start,
                xy_i2c_device_t *, uint8_t, uint16_t *)
FAKE_VALUE_FUNC(xy_error_t, xy_i2c_device_write_reg_repeated_start,
                xy_i2c_device_t *, uint8_t, const uint8_t *, size_t)

static xy_error_t xy_i2c_device_init_impl(xy_i2c_device_t *dev, void *handle,
                                          uint16_t address, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    memset(dev, 0, sizeof(*dev));
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout ? timeout : 1000U;
    dev->base.initialized = true;
    last_bus = *dev;
    return XY_DEVICE_OK;
}

static xy_error_t xy_i2c_device_read_reg16_impl(xy_i2c_device_t *dev, uint8_t reg,
                                                uint16_t *value)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_NOT_NULL(value);

    if (fake_read_failures[reg] > 0) {
        fake_read_failures[reg]--;
        return XY_DEVICE_IO_ERROR;
    }

    *value = fake_regs[reg];
    return XY_DEVICE_OK;
}

static void fake_fail_reads(uint8_t reg, uint8_t failures)
{
    fake_read_failures[reg] = failures;
}

static void fake_fail_writes(uint8_t reg, uint8_t failures)
{
    fake_write_failures[reg] = failures;
}

static xy_error_t xy_i2c_device_write_reg16_impl(xy_i2c_device_t *dev, uint8_t reg,
                                                 const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(2U, len);

    if (fake_write_failures[reg] > 0) {
        fake_write_failures[reg]--;
        return XY_DEVICE_IO_ERROR;
    }

    /* SMBus word order: the driver must put the least significant byte first. */
    last_write_value = (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
    fake_regs[reg] = last_write_value;
    last_write_reg = reg;
    return XY_DEVICE_OK;
}

static void reset_sensor_fakes(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_i2c_device_init);
    RESET_FAKE(xy_i2c_device_read_reg16_repeated_start);
    RESET_FAKE(xy_i2c_device_write_reg_repeated_start);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.return_val = 1000;
    xy_i2c_device_init_fake.custom_fake = xy_i2c_device_init_impl;
    xy_i2c_device_read_reg16_repeated_start_fake.custom_fake = xy_i2c_device_read_reg16_impl;
    xy_i2c_device_write_reg_repeated_start_fake.custom_fake = xy_i2c_device_write_reg16_impl;
}

void setUp(void)
{
    memset(fake_regs, 0, sizeof(fake_regs));
    memset(fake_read_failures, 0, sizeof(fake_read_failures));
    memset(fake_write_failures, 0, sizeof(fake_write_failures));
    memset(&last_bus, 0, sizeof(last_bus));
    last_write_reg = 0;
    last_write_value = 0xFFFF;
    reset_sensor_fakes();

    fake_regs[REG_VER] = 0x0011;
    fake_regs[REG_VCELL] = 0xC350;
    fake_regs[REG_SOC] = 0x4B00U;
    fake_regs[REG_CRATE] = (uint16_t)(int16_t)-1000;
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *registered_max17043(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("MAX17043");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_max17043_register((void *)0x17043, 0));
        fg = xy_fuel_gauge_device_get("MAX17043");
    }
    TEST_ASSERT_NOT_NULL(fg);
    return fg;
}

void test_max17043_registers_default_i2c_bus(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    TEST_ASSERT_NOT_NULL(fg);
    TEST_ASSERT_EQUAL_UINT(1, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR((void *)0x17043, xy_i2c_device_init_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT16(MAX17043_ADDR, xy_i2c_device_init_fake.arg2_val);
    TEST_ASSERT_TRUE(last_bus.base.initialized);
    TEST_ASSERT_EQUAL_PTR((void *)0x17043, last_bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(MAX17043_ADDR, last_bus.dev_addr);
    /* The legacy bus helper gave each transfer 100 ms; the migration must not widen that. */
    TEST_ASSERT_EQUAL_UINT32(100, last_bus.timeout);
}

void test_max17043_register_rejects_null_i2c_handle_without_bus_side_effects(void)
{
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_max17043_register(NULL, 0));
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_NULL(xy_fuel_gauge_device_get("MAX17043"));
    TEST_ASSERT_EQUAL_UINT16(0, last_bus.dev_addr);
    TEST_ASSERT_NULL(last_bus.i2c_handle);
}

void test_max17043_register_duplicate_does_not_reconfigure_bus(void)
{
    TEST_ASSERT_NOT_NULL(registered_max17043());
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_max17043_register((void *)0xBAD, 0x44));
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(NULL, last_bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(0, last_bus.dev_addr);
}

void test_max17043_init_writes_default_config(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized, "MAX17043 init should mark the gauge initialized");
    TEST_ASSERT_EQUAL_UINT(1, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_VER, xy_i2c_device_read_reg16_repeated_start_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(1, xy_i2c_device_write_reg_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_CONFIG, xy_i2c_device_write_reg_repeated_start_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT16(0, last_write_value);
    TEST_ASSERT_EQUAL_UINT8(REG_CONFIG, last_write_reg);
}

void test_max17043_init_retries_transient_version_read_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    fake_fail_reads(REG_VER, 1);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized,
                             "MAX17043 init should mark initialized after transient version read");
    TEST_ASSERT_EQUAL_UINT(2, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_VER, xy_i2c_device_read_reg16_repeated_start_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(1, xy_i2c_device_write_reg_repeated_start_fake.call_count);
}

void test_max17043_init_propagates_persistent_version_read_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    fake_fail_reads(REG_VER, 3);

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT(3, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_VER, xy_i2c_device_read_reg16_repeated_start_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_write_reg_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, last_write_value);
}

void test_max17043_init_retries_transient_config_write_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    fake_fail_writes(REG_CONFIG, 1);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized,
                             "MAX17043 init should mark initialized after transient config write");
    TEST_ASSERT_EQUAL_UINT(1, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2, xy_i2c_device_write_reg_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_CONFIG, last_write_reg);
    TEST_ASSERT_EQUAL_UINT16(0, last_write_value);
}

void test_max17043_init_propagates_persistent_config_write_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    fake_fail_writes(REG_CONFIG, 3);

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT(1, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(3, xy_i2c_device_write_reg_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, last_write_value);
}

void test_max17043_direct_init_failure_clears_stale_private_state(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);

    fake_fail_reads(REG_VER, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, fg->api->init(fg));

    value = 0x17043;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(0x17043, value);
}

void test_max17043_fetch_and_channel_get(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    xy_os_tick_get_fake.return_val = 17043;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(17043, fg->latest.timestamp);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3906, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-208, value);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT(4, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_CRATE, xy_i2c_device_read_reg16_repeated_start_fake.arg1_val);
}

void test_max17043_fetch_failure_preserves_cached_snapshot(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    uint32_t previous_timestamp = 0;
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    xy_os_tick_get_fake.return_val = 17043;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    previous_timestamp = fg->latest.timestamp;

    fake_regs[REG_VCELL] = 0xA000U;
    fake_regs[REG_SOC] = 0x6400U;
    fake_regs[REG_CRATE] = 0x07D0U;
    fake_fail_reads(REG_SOC, 3);
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 18000;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(previous_timestamp, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);

    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3906, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-208, value);

    fake_fail_reads(REG_CRATE, 3);
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 19000;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(previous_timestamp, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);

    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3906, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-208, value);
}

void test_max17043_fetch_failure_does_not_tick_or_poison_later_retry(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    xy_os_tick_get_fake.return_val = 17043;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(17043, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);

    fake_regs[REG_VCELL] = 0x8000U;
    fake_regs[REG_SOC] = 0x1900U;
    fake_regs[REG_CRATE] = 0x03E8U;
    fake_fail_reads(REG_CRATE, 3);
    xy_os_tick_get_fake.return_val = 18000;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(17043, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);

    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3906, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-208, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(18000, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(2, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(2560, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(25, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(208, value);
}

void test_max17043_fetch_retries_transient_register_read_failures(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    reset_sensor_fakes();

    fake_fail_reads(REG_VCELL, 1);
    fake_fail_reads(REG_SOC, 2);
    fake_fail_reads(REG_CRATE, 1);
    xy_os_tick_get_fake.return_val = 19000;

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(19000, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(7, xy_i2c_device_read_reg16_repeated_start_fake.call_count);

    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3906, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-208, value);
}

void test_max17043_rejects_unsupported_channel(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0x17043;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(0x17043, value);

    value = 0x4317;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      fg->api->channel_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(0x4317, value);
}

void test_max17043_unsupported_inline_getters_preserve_outputs(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    uint8_t soh = 88;
    int16_t temperature = 251;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    xy_os_tick_get_fake.return_val = 17043;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED, xy_fuel_gauge_get_soh(fg, &soh));
    TEST_ASSERT_EQUAL_UINT8(88, soh);
    TEST_ASSERT_EQUAL_UINT32(17043, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);

    xy_os_tick_get_fake.return_val = 18000;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get_temperature(fg, &temperature));
    TEST_ASSERT_EQUAL_INT16(251, temperature);
    TEST_ASSERT_EQUAL_UINT32(18000, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(2, xy_os_tick_get_fake.call_count);
}

void test_max17043_inline_getters_preserve_outputs_on_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    uint16_t voltage = 0x1704;
    int16_t current = -170;
    uint8_t soc = 43;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fake_fail_reads(REG_VCELL, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x1704, voltage);

    fake_fail_reads(REG_CRATE, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_current(fg, &current));
    TEST_ASSERT_EQUAL_INT16(-170, current);

    fake_fail_reads(REG_SOC, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_soc(fg, &soc));
    TEST_ASSERT_EQUAL_UINT8(43, soc);
}

void test_max17043_direct_api_guards_preserve_outputs(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    xy_fuel_gauge_t missing_data = *fg;
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 15,
        .high_soc_threshold = 90,
        .low_voltage_mv = 3300,
        .high_voltage_mv = 4200,
        .over_current_ma = 1800,
        .over_temp_c = 55,
    };
    xy_fuel_gauge_alert_t readback = {
        .low_soc_threshold = 0xAA,
        .high_soc_threshold = 0xBB,
        .low_voltage_mv = 0xCCCC,
        .high_voltage_mv = 0xDDDD,
        .over_current_ma = 0x1111,
        .over_temp_c = 0x2222,
    };
    int32_t value = 12345;

    missing_data.data = NULL;

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->init(NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->init(&missing_data));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->fetch(NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->fetch(&missing_data));

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED, fg->api->fetch(fg));
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_read_reg16_repeated_start_fake.call_count);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(NULL, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(12345, value);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(&missing_data, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(12345, value);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, NULL));
    TEST_ASSERT_EQUAL_INT32(12345, value);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(12345, value);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_set(NULL, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_set(&missing_data, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_set(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_get(NULL, &readback));
    TEST_ASSERT_EQUAL_UINT8(0xAA, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(0xBB, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_get(&missing_data, &readback));
    TEST_ASSERT_EQUAL_UINT16(0xCCCC, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(0xDDDD, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_get(fg, NULL));
    TEST_ASSERT_EQUAL_INT16(0x1111, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(0x2222, readback.over_temp_c);
}

void test_max17043_alert_set_get_uses_cached_thresholds(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 15,
        .high_soc_threshold = 90,
        .low_voltage_mv = 3300,
        .high_voltage_mv = 4200,
        .over_current_ma = 1800,
        .over_temp_c = 55,
    };
    xy_fuel_gauge_alert_t readback = {
        .low_soc_threshold = 0xA1,
        .high_soc_threshold = 0xB2,
        .low_voltage_mv = 0xC3C4,
        .high_voltage_mv = 0xD5D6,
        .over_current_ma = 0x1718,
        .over_temp_c = 0x191A,
    };

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(fg, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(NULL, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get_alert(NULL, &readback));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get_alert(fg, &readback));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->alert_set(fg, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->alert_get(fg, &readback));
    TEST_ASSERT_EQUAL_UINT8(0xA1, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(0xB2, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL_UINT16(0xC3C4, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(0xD5D6, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL_INT16(0x1718, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(0x191A, readback.over_temp_c);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get_alert(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_set_alert(fg, &alert));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_alert(fg, &readback));

    TEST_ASSERT_EQUAL_UINT8(alert.low_soc_threshold, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(alert.high_soc_threshold, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL_UINT16(alert.low_voltage_mv, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(alert.high_voltage_mv, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL_INT16(alert.over_current_ma, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(alert.over_temp_c, readback.over_temp_c);
}

void test_max17043_reinit_preserves_alert_threshold_cache(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 10,
        .high_soc_threshold = 95,
        .low_voltage_mv = 3200,
        .high_voltage_mv = 4250,
        .over_current_ma = 2100,
        .over_temp_c = 600,
    };
    xy_fuel_gauge_alert_t readback = {0};

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->alert_set(fg, &alert));

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->alert_get(fg, &readback));

    TEST_ASSERT_EQUAL_UINT8(alert.low_soc_threshold, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(alert.high_soc_threshold, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL_UINT16(alert.low_voltage_mv, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(alert.high_voltage_mv, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL_INT16(alert.over_current_ma, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(alert.over_temp_c, readback.over_temp_c);
}

void test_max17043_register_instance_isolates_multiple_instances(void)
{
    static xy_fuel_gauge_t fg_a;
    static xy_fg_max17043_data_t priv_a;
    static xy_fuel_gauge_t fg_b;
    static xy_fg_max17043_data_t priv_b;

    fg_a.name = "MAX17043_ISO_A";
    fg_b.name = "MAX17043_ISO_B";

    uint8_t before = xy_fuel_gauge_device_count();

    TEST_ASSERT_EQUAL(XY_FG_OK,
                      xy_fuel_gauge_max17043_register_instance(&fg_a, &priv_a,
                                                               (void *)0xA000, 0x10));
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      xy_fuel_gauge_max17043_register_instance(&fg_b, &priv_b,
                                                               (void *)0xB000, 0x20));

    TEST_ASSERT_EQUAL_UINT8(before + 2, xy_fuel_gauge_device_count());
    TEST_ASSERT_EQUAL_PTR(&fg_a, xy_fuel_gauge_device_get("MAX17043_ISO_A"));
    TEST_ASSERT_EQUAL_PTR(&fg_b, xy_fuel_gauge_device_get("MAX17043_ISO_B"));
    TEST_ASSERT_TRUE(fg_a.data != fg_b.data);

    /* Distinct bus configuration per instance */
    TEST_ASSERT_EQUAL_PTR((void *)0xA000, priv_a.bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(0x10, priv_a.bus.dev_addr);
    TEST_ASSERT_EQUAL_PTR((void *)0xB000, priv_b.bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(0x20, priv_b.bus.dev_addr);

    /* Independent initialization */
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(&fg_a));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(&fg_b));
    TEST_ASSERT_TRUE(priv_a.initialized);
    TEST_ASSERT_TRUE(priv_b.initialized);

    /* Fetch A with one register set, B with another; caches must stay isolated */
    fake_regs[REG_VCELL] = 0xC350U;
    fake_regs[REG_SOC] = 0x4B00U;
    fake_regs[REG_CRATE] = (uint16_t)(int16_t)-1000;
    xy_os_tick_get_fake.return_val = 1000;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(&fg_a));

    fake_regs[REG_VCELL] = 0x8000U;
    fake_regs[REG_SOC] = 0x1900U;
    fake_regs[REG_CRATE] = (uint16_t)(int16_t)1000;
    xy_os_tick_get_fake.return_val = 2000;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(&fg_b));

    int32_t value = 0;
    TEST_ASSERT_EQUAL(XY_FG_OK, fg_a.api->channel_get(&fg_a, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3906, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg_a.api->channel_get(&fg_a, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, fg_b.api->channel_get(&fg_b, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(2560, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg_b.api->channel_get(&fg_b, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(25, value);

    /* Per-instance timestamps */
    TEST_ASSERT_EQUAL_UINT32(1000, fg_a.latest.timestamp);
    TEST_ASSERT_EQUAL_UINT32(2000, fg_b.latest.timestamp);
}

void test_max17043_register_instance_rejects_null_inputs_without_side_effects(void)
{
    static xy_fuel_gauge_t fg;
    static xy_fg_max17043_data_t priv;
    xy_fuel_gauge_t no_name = {0};

    fg.name = "MAX17043_NULL";
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_max17043_register_instance(NULL, &priv, (void *)0x1, 0));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_max17043_register_instance(&fg, NULL, (void *)0x1, 0));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_max17043_register_instance(&fg, &priv, NULL, 0));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_max17043_register_instance(&no_name, &priv, (void *)0x1, 0));

    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(0, last_bus.dev_addr);
    TEST_ASSERT_NULL(last_bus.i2c_handle);
}

void test_max17043_register_instance_rejects_duplicate_name_without_reconfiguring_bus(void)
{
    registered_max17043();
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    static xy_fuel_gauge_t fg_dup;
    static xy_fg_max17043_data_t priv_dup;
    fg_dup.name = "MAX17043";

    TEST_ASSERT_EQUAL(XY_FG_ERROR,
                      xy_fuel_gauge_max17043_register_instance(&fg_dup, &priv_dup,
                                                               (void *)0xBAD, 0x44));
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(NULL, last_bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(0, last_bus.dev_addr);
    TEST_ASSERT_NULL(fg_dup.api);
    TEST_ASSERT_NULL(fg_dup.data);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max17043_register_rejects_null_i2c_handle_without_bus_side_effects);
    RUN_TEST(test_max17043_registers_default_i2c_bus);
    RUN_TEST(test_max17043_register_duplicate_does_not_reconfigure_bus);
    RUN_TEST(test_max17043_init_writes_default_config);
    RUN_TEST(test_max17043_init_retries_transient_version_read_failure);
    RUN_TEST(test_max17043_init_propagates_persistent_version_read_failure);
    RUN_TEST(test_max17043_init_retries_transient_config_write_failure);
    RUN_TEST(test_max17043_init_propagates_persistent_config_write_failure);
    RUN_TEST(test_max17043_direct_init_failure_clears_stale_private_state);
    RUN_TEST(test_max17043_fetch_and_channel_get);
    RUN_TEST(test_max17043_fetch_failure_preserves_cached_snapshot);
    RUN_TEST(test_max17043_fetch_failure_does_not_tick_or_poison_later_retry);
    RUN_TEST(test_max17043_fetch_retries_transient_register_read_failures);
    RUN_TEST(test_max17043_rejects_unsupported_channel);
    RUN_TEST(test_max17043_unsupported_inline_getters_preserve_outputs);
    RUN_TEST(test_max17043_inline_getters_preserve_outputs_on_failure);
    RUN_TEST(test_max17043_direct_api_guards_preserve_outputs);
    RUN_TEST(test_max17043_alert_set_get_uses_cached_thresholds);
    RUN_TEST(test_max17043_reinit_preserves_alert_threshold_cache);
    RUN_TEST(test_max17043_register_instance_isolates_multiple_instances);
    RUN_TEST(test_max17043_register_instance_rejects_null_inputs_without_side_effects);
    RUN_TEST(test_max17043_register_instance_rejects_duplicate_name_without_reconfiguring_bus);
    return UNITY_END();
}
