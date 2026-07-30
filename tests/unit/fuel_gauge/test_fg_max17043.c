#include "unity.h"
#include "fff.h"
#include "xy_fuel_gauge.h"
#include "xy_sensor_device.h"

#include <stdint.h>
#include <string.h>

int xy_fuel_gauge_max17043_register(void *i2c_handle, uint8_t addr);

DEFINE_FFF_GLOBALS;

#define MAX17043_ADDR       0x36
#define REG_VCELL           0x02
#define REG_SOC             0x04
#define REG_VER             0x08
#define REG_CONFIG          0x0C
#define REG_CRATE           0x16

static uint16_t fake_regs[256];
static uint8_t fake_read_failures[256];
static uint8_t fake_write_failures[256];
static xy_sensor_bus_t last_bus;
static uint8_t last_write_reg;
static uint16_t last_write_value;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VOID_FUNC(xy_sensor_bus_config_i2c, xy_sensor_bus_t *, void *, uint8_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read_reg16, xy_sensor_bus_t *, uint8_t, uint16_t *)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write_reg16, xy_sensor_bus_t *, uint8_t, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read, xy_sensor_bus_t *, uint8_t, uint8_t *, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write, xy_sensor_bus_t *, uint8_t, const uint8_t *, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read_reg, xy_sensor_bus_t *, uint8_t, uint8_t *)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write_reg, xy_sensor_bus_t *, uint8_t, uint8_t)
FAKE_VALUE_FUNC(int, xy_sensor_spi_read, xy_sensor_bus_t *, uint8_t, uint8_t *, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_spi_write, xy_sensor_bus_t *, uint8_t, const uint8_t *, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_check_device_id, xy_sensor_bus_t *, uint8_t, uint8_t)
FAKE_VOID_FUNC(xy_sensor_bus_config_spi, xy_sensor_bus_t *, void *, uint8_t)

static void xy_sensor_bus_config_i2c_impl(xy_sensor_bus_t *bus, void *handle, uint8_t address)
{
    TEST_ASSERT_NOT_NULL(bus);
    bus->type = XY_SENSOR_BUS_I2C;
    bus->bus_handle = handle;
    bus->address = address;
    bus->chip_select = 0;
    last_bus = *bus;
}

static int xy_sensor_i2c_read_reg16_impl(xy_sensor_bus_t *bus, uint8_t reg, uint16_t *value)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);
    TEST_ASSERT_NOT_NULL(value);

    if (fake_read_failures[reg] > 0) {
        fake_read_failures[reg]--;
        return -1;
    }

    *value = fake_regs[reg];
    return 0;
}

static void fake_fail_reads(uint8_t reg, uint8_t failures)
{
    fake_read_failures[reg] = failures;
}

static void fake_fail_writes(uint8_t reg, uint8_t failures)
{
    fake_write_failures[reg] = failures;
}

static int xy_sensor_i2c_write_reg16_impl(xy_sensor_bus_t *bus, uint8_t reg, uint16_t value)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);

    if (fake_write_failures[reg] > 0) {
        fake_write_failures[reg]--;
        return -1;
    }

    fake_regs[reg] = value;
    last_write_reg = reg;
    last_write_value = value;
    return 0;
}

static void reset_sensor_fakes(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_sensor_bus_config_i2c);
    RESET_FAKE(xy_sensor_i2c_read_reg16);
    RESET_FAKE(xy_sensor_i2c_write_reg16);
    RESET_FAKE(xy_sensor_i2c_read);
    RESET_FAKE(xy_sensor_i2c_write);
    RESET_FAKE(xy_sensor_i2c_read_reg);
    RESET_FAKE(xy_sensor_i2c_write_reg);
    RESET_FAKE(xy_sensor_spi_read);
    RESET_FAKE(xy_sensor_spi_write);
    RESET_FAKE(xy_sensor_check_device_id);
    RESET_FAKE(xy_sensor_bus_config_spi);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.return_val = 1000;
    xy_sensor_bus_config_i2c_fake.custom_fake = xy_sensor_bus_config_i2c_impl;
    xy_sensor_i2c_read_reg16_fake.custom_fake = xy_sensor_i2c_read_reg16_impl;
    xy_sensor_i2c_write_reg16_fake.custom_fake = xy_sensor_i2c_write_reg16_impl;
    xy_sensor_i2c_read_fake.return_val = -1;
    xy_sensor_i2c_write_fake.return_val = -1;
    xy_sensor_i2c_read_reg_fake.return_val = -1;
    xy_sensor_i2c_write_reg_fake.return_val = -1;
    xy_sensor_spi_read_fake.return_val = -1;
    xy_sensor_spi_write_fake.return_val = -1;
    xy_sensor_check_device_id_fake.return_val = -1;
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
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_EQUAL_PTR((void *)0x17043, xy_sensor_bus_config_i2c_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(MAX17043_ADDR, xy_sensor_bus_config_i2c_fake.arg2_val);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, last_bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x17043, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(MAX17043_ADDR, last_bus.address);
}

void test_max17043_init_writes_default_config(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_VER, xy_sensor_i2c_read_reg16_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_i2c_write_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_CONFIG, xy_sensor_i2c_write_reg16_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT16(0, xy_sensor_i2c_write_reg16_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT8(REG_CONFIG, last_write_reg);
    TEST_ASSERT_EQUAL_UINT16(0, last_write_value);
}

void test_max17043_init_propagates_version_read_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    fake_fail_reads(REG_VER, 1);

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_VER, xy_sensor_i2c_read_reg16_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(0, xy_sensor_i2c_write_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, last_write_value);
}

void test_max17043_init_propagates_config_write_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    fake_fail_writes(REG_CONFIG, 1);

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_i2c_write_reg16_fake.call_count);
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

    fake_fail_reads(REG_VER, 1);
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

    TEST_ASSERT_GREATER_OR_EQUAL_UINT(4, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_CRATE, xy_sensor_i2c_read_reg16_fake.arg1_val);
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
    fake_fail_reads(REG_SOC, 1);
    xy_os_tick_get_fake.return_val = 18000;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(previous_timestamp, fg->latest.timestamp);

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
    fake_fail_reads(REG_CRATE, 1);
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

void test_max17043_rejects_unsupported_channel(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
}

void test_max17043_inline_getters_preserve_outputs_on_failure(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    uint16_t voltage = 0x1704;
    int16_t current = -170;
    uint8_t soc = 43;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fake_fail_reads(REG_VCELL, 1);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x1704, voltage);

    fake_fail_reads(REG_CRATE, 1);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_current(fg, &current));
    TEST_ASSERT_EQUAL_INT16(-170, current);

    fake_fail_reads(REG_SOC, 1);
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
    TEST_ASSERT_EQUAL_UINT(0, xy_sensor_i2c_read_reg16_fake.call_count);

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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max17043_registers_default_i2c_bus);
    RUN_TEST(test_max17043_init_writes_default_config);
    RUN_TEST(test_max17043_init_propagates_version_read_failure);
    RUN_TEST(test_max17043_init_propagates_config_write_failure);
    RUN_TEST(test_max17043_direct_init_failure_clears_stale_private_state);
    RUN_TEST(test_max17043_fetch_and_channel_get);
    RUN_TEST(test_max17043_fetch_failure_preserves_cached_snapshot);
    RUN_TEST(test_max17043_fetch_failure_does_not_tick_or_poison_later_retry);
    RUN_TEST(test_max17043_rejects_unsupported_channel);
    RUN_TEST(test_max17043_inline_getters_preserve_outputs_on_failure);
    RUN_TEST(test_max17043_direct_api_guards_preserve_outputs);
    RUN_TEST(test_max17043_alert_set_get_uses_cached_thresholds);
    return UNITY_END();
}
