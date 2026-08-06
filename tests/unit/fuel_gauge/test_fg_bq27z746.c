#include "unity.h"
#include "fff.h"
#include "xy_fg_bq27z746.h"
#include "xy_sensor_device.h"

#include <stdint.h>
#include <string.h>

#define REG_CTRL      0x00
#define REG_TEMP      0x06
#define REG_VOLT      0x08
#define REG_FLAGS     0x0A
#define REG_REM_CAP   0x14
#define REG_SOC       0x2C
#define REG_CURR      0x58
#define REG_AVG_CURR  0x5A
#define REG_SOH       0x7A
#define REG_CYCLE_CNT 0x2A
#define REG_FULL_CAP  0x12

static uint16_t fake_regs[256];
static uint8_t fake_read_failures[256];
static uint32_t fake_read_counts[256];
static xy_sensor_bus_t last_bus;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VOID_FUNC(xy_sensor_bus_config_i2c, xy_sensor_bus_t *, void *, uint8_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read, xy_sensor_bus_t *, uint8_t, uint8_t *, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write, xy_sensor_bus_t *, uint8_t, const uint8_t *, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read_reg, xy_sensor_bus_t *, uint8_t, uint8_t *)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write_reg, xy_sensor_bus_t *, uint8_t, uint8_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read_reg16, xy_sensor_bus_t *, uint8_t, uint16_t *)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write_reg16, xy_sensor_bus_t *, uint8_t, uint16_t)
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

static int xy_sensor_i2c_read_impl(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT16(2, len);

    fake_read_counts[reg]++;

    if (fake_read_failures[reg] > 0) {
        fake_read_failures[reg]--;
        return -1;
    }

    uint16_t value = fake_regs[reg];
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)(value >> 8);
    return 0;
}

static int xy_sensor_i2c_read_reg16_impl(xy_sensor_bus_t *bus, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int ret = xy_sensor_i2c_read_impl(bus, reg, data, sizeof(data));
    if (ret == 0) {
        *value = ((uint16_t)data[1] << 8) | data[0];
    }
    return ret;
}

static void fake_fail_reads(uint8_t reg, uint8_t failures)
{
    fake_read_failures[reg] = failures;
}

static void reset_sensor_fakes(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_sensor_bus_config_i2c);
    RESET_FAKE(xy_sensor_i2c_read);
    RESET_FAKE(xy_sensor_i2c_write);
    RESET_FAKE(xy_sensor_i2c_read_reg);
    RESET_FAKE(xy_sensor_i2c_write_reg);
    RESET_FAKE(xy_sensor_i2c_read_reg16);
    RESET_FAKE(xy_sensor_i2c_write_reg16);
    RESET_FAKE(xy_sensor_spi_read);
    RESET_FAKE(xy_sensor_spi_write);
    RESET_FAKE(xy_sensor_check_device_id);
    RESET_FAKE(xy_sensor_bus_config_spi);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.return_val = 1000;
    xy_sensor_bus_config_i2c_fake.custom_fake = xy_sensor_bus_config_i2c_impl;
    xy_sensor_i2c_read_fake.custom_fake = xy_sensor_i2c_read_impl;
    xy_sensor_i2c_read_reg16_fake.custom_fake = xy_sensor_i2c_read_reg16_impl;
    xy_sensor_i2c_write_fake.return_val = -1;
    xy_sensor_i2c_read_reg_fake.return_val = -1;
    xy_sensor_i2c_write_reg_fake.return_val = -1;
    xy_sensor_i2c_write_reg16_fake.return_val = -1;
    xy_sensor_spi_read_fake.return_val = -1;
    xy_sensor_spi_write_fake.return_val = -1;
    xy_sensor_check_device_id_fake.return_val = -1;
}

void setUp(void)
{
    memset(fake_regs, 0, sizeof(fake_regs));
    memset(fake_read_failures, 0, sizeof(fake_read_failures));
    memset(fake_read_counts, 0, sizeof(fake_read_counts));
    memset(&last_bus, 0, sizeof(last_bus));
    reset_sensor_fakes();

    fake_regs[REG_CTRL] = 0x0746;
    fake_regs[REG_FLAGS] = 0x0009;
    fake_regs[REG_VOLT] = 3811;
    fake_regs[REG_CURR] = (uint16_t)(int16_t)-321;
    fake_regs[REG_AVG_CURR] = (uint16_t)(int16_t)-222;
    fake_regs[REG_SOC] = 67;
    fake_regs[REG_SOH] = 94;
    fake_regs[REG_TEMP] = 2981;
    fake_regs[REG_FULL_CAP] = 4800;
    fake_regs[REG_REM_CAP] = 3200;
    fake_regs[REG_CYCLE_CNT] = 123;
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *registered_bq27z746(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ27Z746");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_bq27z746_register((void *)0x1234, 0));
        fg = xy_fuel_gauge_device_get("BQ27Z746");
    }
    TEST_ASSERT_NOT_NULL(fg);
    return fg;
}

void test_bq27z746_registers_default_i2c_bus(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    TEST_ASSERT_NOT_NULL(fg);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, xy_sensor_bus_config_i2c_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(BQ27Z746_ADDR, xy_sensor_bus_config_i2c_fake.arg2_val);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, last_bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(BQ27Z746_ADDR, last_bus.address);
}

void test_bq27z746_register_rejects_null_i2c_handle_without_bus_side_effects(void)
{
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_bq27z746_register(NULL, 0));
    TEST_ASSERT_EQUAL_UINT(0, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_NULL(xy_fuel_gauge_device_get("BQ27Z746"));
    TEST_ASSERT_EQUAL_UINT8(0, last_bus.address);
    TEST_ASSERT_NULL(last_bus.bus_handle);
}

void test_bq27z746_register_duplicate_does_not_reconfigure_bus(void)
{
    TEST_ASSERT_NOT_NULL(registered_bq27z746());
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_bq27z746_register((void *)0xBAD, 0x44));
    TEST_ASSERT_EQUAL_UINT(0, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(NULL, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(0, last_bus.address);
}

void test_bq27z746_init_fetch_and_channel_get(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized, "BQ27Z746 init should mark the gauge initialized");
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(2, xy_sensor_i2c_read_fake.call_count);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0x0009, xy_fuel_gauge_bq27z746_get_flags(fg));

    xy_sensor_i2c_read_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 4242;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(4242, fg->latest.timestamp);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(10, xy_sensor_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_FLAGS, xy_sensor_i2c_read_fake.arg1_val);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3811, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-321, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-222, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(67, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(250, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(4800, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(3200, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(123, value);
}

void test_bq27z746_init_retries_transient_device_type_read_failure(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    fg->initialized = false;
    fake_fail_reads(REG_CTRL, 1);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized, "transient BQ27Z746 read retry should still initialize");
    TEST_ASSERT_EQUAL_UINT(3, xy_sensor_i2c_read_fake.call_count);
}

void test_bq27z746_init_fails_after_exhausted_device_type_retries(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    fg->initialized = false;
    fake_fail_reads(REG_CTRL, 3);

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT(3, xy_sensor_i2c_read_fake.call_count);
}

void test_bq27z746_direct_init_failure_clears_stale_private_state(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0x0009, xy_fuel_gauge_bq27z746_get_flags(fg));

    fake_fail_reads(REG_CTRL, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, fg->api->init(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0, xy_fuel_gauge_bq27z746_get_flags(fg));
}

void test_bq27z746_init_propagates_flags_read_failure(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    fg->initialized = false;
    fake_fail_reads(REG_FLAGS, 3);

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(fg->initialized);
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0, xy_fuel_gauge_bq27z746_get_flags(fg));
}

void test_bq27z746_rejects_uninitialized_and_unsupported_channel(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    int32_t value = 123456;

    fg->initialized = false;
    fake_fail_reads(REG_FLAGS, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);

    fake_regs[REG_FLAGS] = 0x0009;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TIME_TO_FULL, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
}

void test_bq27z746_alert_set_get_uses_cached_thresholds(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 10,
        .high_soc_threshold = 95,
        .low_voltage_mv = 3100,
        .high_voltage_mv = 4400,
        .over_current_ma = 3000,
        .over_temp_c = 65,
    };
    xy_fuel_gauge_alert_t readback;
    xy_fuel_gauge_alert_t sentinel = {
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
                      fg->api->alert_get(fg, &sentinel));
    TEST_ASSERT_EQUAL_UINT8(0xA1, sentinel.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(0xB2, sentinel.high_soc_threshold);
    TEST_ASSERT_EQUAL_UINT16(0xC3C4, sentinel.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(0xD5D6, sentinel.high_voltage_mv);
    TEST_ASSERT_EQUAL_INT16(0x1718, sentinel.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(0x191A, sentinel.over_temp_c);

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

void test_bq27z746_fetch_failure_preserves_cached_snapshot(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    xy_os_tick_get_fake.return_val = 4242;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3811, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-321, value);
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      fg->api->channel_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-222, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(67, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(94, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(250, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(4800, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(3200, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(123, value);
    TEST_ASSERT_EQUAL_UINT32(4242, fg->latest.timestamp);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_full(fg));

    fake_regs[REG_VOLT] = 3999;
    fake_regs[REG_CURR] = 456;
    fake_regs[REG_AVG_CURR] = 123;
    fake_regs[REG_SOC] = 88;
    fake_regs[REG_SOH] = 97;
    fake_regs[REG_TEMP] = 3011;
    fake_regs[REG_FULL_CAP] = 5100;
    fake_regs[REG_REM_CAP] = 4500;
    fake_regs[REG_CYCLE_CNT] = 321;
    fake_regs[REG_FLAGS] = 0;

    fake_fail_reads(REG_TEMP, 3);
    fg->latest.timestamp = 7777;
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 8888;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(7777, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3811, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-321, value);
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      fg->api->channel_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-222, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(67, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(94, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(250, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(4800, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(3200, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(123, value);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_full(fg));

    fake_fail_reads(REG_FLAGS, 3);
    fg->latest.timestamp = 9999;
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 1111;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(9999, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3811, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-321, value);
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      fg->api->channel_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-222, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(67, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(94, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(250, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(4800, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(3200, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(123, value);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_full(fg));

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(1111, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3999, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(456, value);
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      fg->api->channel_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(123, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(88, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(97, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(280, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(5100, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(4500, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(321, value);
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0, xy_fuel_gauge_bq27z746_get_flags(fg));
}

void test_bq27z746_fetch_retries_transient_register_read_failures(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    xy_sensor_i2c_read_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 5151;
    fake_fail_reads(REG_VOLT, 2);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(5151, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3811, value);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(12, xy_sensor_i2c_read_fake.call_count);

    fake_regs[REG_FLAGS] = 0;
    xy_sensor_i2c_read_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 6262;
    fake_fail_reads(REG_FLAGS, 2);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(6262, fg->latest.timestamp);
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0, xy_fuel_gauge_bq27z746_get_flags(fg));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(12, xy_sensor_i2c_read_fake.call_count);
}

void test_bq27z746_fetch_stops_after_exhausted_first_register_retry(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    xy_sensor_i2c_read_fake.call_count = 0;
    xy_os_tick_get_fake.call_count = 0;
    memset(fake_read_counts, 0, sizeof(fake_read_counts));
    fake_fail_reads(REG_VOLT, 3);

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(3, xy_sensor_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(3, fake_read_counts[REG_VOLT]);
    TEST_ASSERT_EQUAL_UINT32(0, fake_read_counts[REG_CURR]);
    TEST_ASSERT_EQUAL_UINT32(0, fake_read_counts[REG_AVG_CURR]);
    TEST_ASSERT_EQUAL_UINT32(0, fake_read_counts[REG_SOC]);
    TEST_ASSERT_EQUAL_UINT32(0, fake_read_counts[REG_FLAGS]);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
}

void test_bq27z746_inline_getters_preserve_outputs_on_fetch_failure(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    uint16_t voltage = 0x0746;
    int16_t current = -746;
    uint8_t soc = 74;
    uint8_t soh = 46;
    int16_t temp = -27;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(NULL, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x0746, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(NULL, &current));
    TEST_ASSERT_EQUAL_INT16(-746, current);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(NULL, &soc));
    TEST_ASSERT_EQUAL_UINT8(74, soc);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(NULL, &soh));
    TEST_ASSERT_EQUAL_UINT8(46, soh);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(NULL, &temp));
    TEST_ASSERT_EQUAL_INT16(-27, temp);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(fg, NULL));

    fake_fail_reads(REG_VOLT, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x0746, voltage);

    fake_fail_reads(REG_CURR, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_current(fg, &current));
    TEST_ASSERT_EQUAL_INT16(-746, current);

    fake_fail_reads(REG_SOC, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_soc(fg, &soc));
    TEST_ASSERT_EQUAL_UINT8(74, soc);

    fake_fail_reads(REG_SOH, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_soh(fg, &soh));
    TEST_ASSERT_EQUAL_UINT8(46, soh);

    fake_fail_reads(REG_TEMP, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_temperature(fg, &temp));
    TEST_ASSERT_EQUAL_INT16(-27, temp);
}

void test_bq27z746_status_helpers_handle_null_and_uninitialized(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_charging(NULL));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_full(NULL));
    TEST_ASSERT_EQUAL_UINT16(0, xy_fuel_gauge_bq27z746_get_flags(NULL));

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0x0009, xy_fuel_gauge_bq27z746_get_flags(fg));

    fg->initialized = false;
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0, xy_fuel_gauge_bq27z746_get_flags(fg));
}

void test_bq27z746_direct_api_guards_missing_device_data(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    xy_fuel_gauge_t missing_data = *fg;
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 10,
        .high_soc_threshold = 90,
        .low_voltage_mv = 3200,
        .high_voltage_mv = 4300,
        .over_current_ma = 2500,
        .over_temp_c = 600,
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
                      fg->api->channel_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(12345, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->channel_get(fg, XY_FG_DATA_TIME_TO_FULL, &value));
    TEST_ASSERT_EQUAL_INT32(12345, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    value = 54321;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      fg->api->channel_get(fg, XY_FG_DATA_TIME_TO_FULL, &value));
    TEST_ASSERT_EQUAL_INT32(54321, value);

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

    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_charging(&missing_data));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq27z746_is_full(&missing_data));
    TEST_ASSERT_EQUAL_UINT16(0, xy_fuel_gauge_bq27z746_get_flags(&missing_data));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bq27z746_register_rejects_null_i2c_handle_without_bus_side_effects);
    RUN_TEST(test_bq27z746_registers_default_i2c_bus);
    RUN_TEST(test_bq27z746_register_duplicate_does_not_reconfigure_bus);
    RUN_TEST(test_bq27z746_init_fetch_and_channel_get);
    RUN_TEST(test_bq27z746_init_retries_transient_device_type_read_failure);
    RUN_TEST(test_bq27z746_init_fails_after_exhausted_device_type_retries);
    RUN_TEST(test_bq27z746_direct_init_failure_clears_stale_private_state);
    RUN_TEST(test_bq27z746_init_propagates_flags_read_failure);
    RUN_TEST(test_bq27z746_rejects_uninitialized_and_unsupported_channel);
    RUN_TEST(test_bq27z746_alert_set_get_uses_cached_thresholds);
    RUN_TEST(test_bq27z746_fetch_failure_preserves_cached_snapshot);
    RUN_TEST(test_bq27z746_fetch_retries_transient_register_read_failures);
    RUN_TEST(test_bq27z746_fetch_stops_after_exhausted_first_register_retry);
    RUN_TEST(test_bq27z746_inline_getters_preserve_outputs_on_fetch_failure);
    RUN_TEST(test_bq27z746_status_helpers_handle_null_and_uninitialized);
    RUN_TEST(test_bq27z746_direct_api_guards_missing_device_data);
    return UNITY_END();
}
