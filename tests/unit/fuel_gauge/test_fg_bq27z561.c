#include "unity.h"
#include "fff.h"
#include "xy_fuel_gauge.h"
#include "xy_sensor_device.h"

#include <stdint.h>
#include <string.h>

int xy_fuel_gauge_bq27z561_register(void *i2c_handle, uint8_t addr);

#define BQ27Z561_ADDR        0x55
#define REG_DEVICE_ID        0x02
#define REG_TEMP             0x06
#define REG_VOLT             0x08
#define REG_NOM_CAP          0x12
#define REG_REM_CAP          0x14
#define REG_CYCLE_CNT        0x2A
#define REG_SOC              0x2C
#define REG_CURR             0x58
#define REG_AVG_CURR         0x5A
#define REG_SOH              0x7A

static uint16_t fake_reg16[256];
static uint8_t fake_reg8[256];
static uint8_t fail_reg16_reads[256];
static uint8_t fail_reg8_reads[256];
static int fail_reg16 = -1;
static int fail_reg8 = -1;
static uint32_t reg16_read_counts[256];
static xy_sensor_bus_t last_bus;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VOID_FUNC(xy_sensor_bus_config_i2c, xy_sensor_bus_t *, void *, uint8_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read_reg16, xy_sensor_bus_t *, uint8_t, uint16_t *)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read_reg, xy_sensor_bus_t *, uint8_t, uint8_t *)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write_reg16, xy_sensor_bus_t *, uint8_t, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_read, xy_sensor_bus_t *, uint8_t, uint8_t *, uint16_t)
FAKE_VALUE_FUNC(int, xy_sensor_i2c_write, xy_sensor_bus_t *, uint8_t, const uint8_t *, uint16_t)
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

    reg16_read_counts[reg]++;
    if (fail_reg16_reads[reg] > 0) {
        fail_reg16_reads[reg]--;
        return -1;
    }
    if ((int)reg == fail_reg16) {
        return -1;
    }

    *value = fake_reg16[reg];
    return 0;
}

static int xy_sensor_i2c_read_reg_impl(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *value)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);
    TEST_ASSERT_NOT_NULL(value);

    if (fail_reg8_reads[reg] > 0) {
        fail_reg8_reads[reg]--;
        return -1;
    }
    if ((int)reg == fail_reg8) {
        return -1;
    }

    *value = fake_reg8[reg];
    return 0;
}

static void reset_sensor_fakes(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_sensor_bus_config_i2c);
    RESET_FAKE(xy_sensor_i2c_read_reg16);
    RESET_FAKE(xy_sensor_i2c_read_reg);
    RESET_FAKE(xy_sensor_i2c_write_reg16);
    RESET_FAKE(xy_sensor_i2c_read);
    RESET_FAKE(xy_sensor_i2c_write);
    RESET_FAKE(xy_sensor_i2c_write_reg);
    RESET_FAKE(xy_sensor_spi_read);
    RESET_FAKE(xy_sensor_spi_write);
    RESET_FAKE(xy_sensor_check_device_id);
    RESET_FAKE(xy_sensor_bus_config_spi);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.return_val = 1000;
    xy_sensor_bus_config_i2c_fake.custom_fake = xy_sensor_bus_config_i2c_impl;
    xy_sensor_i2c_read_reg16_fake.custom_fake = xy_sensor_i2c_read_reg16_impl;
    xy_sensor_i2c_read_reg_fake.custom_fake = xy_sensor_i2c_read_reg_impl;
    xy_sensor_i2c_write_reg16_fake.return_val = -1;
    xy_sensor_i2c_read_fake.return_val = -1;
    xy_sensor_i2c_write_fake.return_val = -1;
    xy_sensor_i2c_write_reg_fake.return_val = -1;
    xy_sensor_spi_read_fake.return_val = -1;
    xy_sensor_spi_write_fake.return_val = -1;
    xy_sensor_check_device_id_fake.return_val = -1;
}

void setUp(void)
{
    memset(fake_reg16, 0, sizeof(fake_reg16));
    memset(fake_reg8, 0, sizeof(fake_reg8));
    memset(fail_reg16_reads, 0, sizeof(fail_reg16_reads));
    memset(fail_reg8_reads, 0, sizeof(fail_reg8_reads));
    fail_reg16 = -1;
    fail_reg8 = -1;
    memset(reg16_read_counts, 0, sizeof(reg16_read_counts));
    memset(&last_bus, 0, sizeof(last_bus));
    reset_sensor_fakes();

    fake_reg16[REG_DEVICE_ID] = 0x0561;
    fake_reg16[REG_VOLT] = 3700;
    fake_reg16[REG_CURR] = (uint16_t)(int16_t)-123;
    fake_reg16[REG_AVG_CURR] = (uint16_t)(int16_t)-77;
    fake_reg8[REG_SOC] = 66;
    fake_reg8[REG_SOH] = 97;
    fake_reg16[REG_TEMP] = 2981;
    fake_reg16[REG_NOM_CAP] = 3000;
    fake_reg16[REG_REM_CAP] = 1980;
    fake_reg16[REG_CYCLE_CNT] = 42;
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *registered_bq27z561(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ27z561");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_bq27z561_register((void *)0x27561, 0));
        fg = xy_fuel_gauge_device_get("BQ27z561");
    }
    TEST_ASSERT_NOT_NULL(fg);
    return fg;
}

void test_bq27z561_registers_default_i2c_bus(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();

    TEST_ASSERT_NOT_NULL(fg);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_EQUAL_PTR((void *)0x27561, xy_sensor_bus_config_i2c_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(BQ27Z561_ADDR, xy_sensor_bus_config_i2c_fake.arg2_val);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, last_bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x27561, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(BQ27Z561_ADDR, last_bus.address);
}

void test_bq27z561_register_rejects_null_i2c_handle_without_bus_side_effects(void)
{
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_bq27z561_register(NULL, 0));
    TEST_ASSERT_EQUAL_UINT(0, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_NULL(xy_fuel_gauge_device_get("BQ27z561"));
    TEST_ASSERT_EQUAL_UINT8(0, last_bus.address);
    TEST_ASSERT_NULL(last_bus.bus_handle);
}

void test_bq27z561_register_duplicate_does_not_reconfigure_bus(void)
{
    TEST_ASSERT_NOT_NULL(registered_bq27z561());
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_bq27z561_register((void *)0xBAD, 0x44));
    TEST_ASSERT_EQUAL_UINT(0, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(NULL, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(0, last_bus.address);
}

void test_bq27z561_init_reads_device_id(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized, "BQ27Z561 init should mark the gauge initialized");
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_DEVICE_ID, xy_sensor_i2c_read_reg16_fake.arg1_val);
}

void test_bq27z561_init_failure_preserves_uninitialized_state(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();

    fg->initialized = false;
    fail_reg16 = REG_DEVICE_ID;

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT(3, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_DEVICE_ID, xy_sensor_i2c_read_reg16_fake.arg1_val);
}

void test_bq27z561_direct_init_failure_clears_stale_private_state(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(66, value);

    fail_reg16 = REG_DEVICE_ID;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, fg->api->init(fg));

    value = 0x27561;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(0x27561, value);
}

void test_bq27z561_fetch_and_channel_get(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED, fg->api->fetch(fg));
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_i2c_read_reg16_fake.call_count);

    fg->initialized = true;
    xy_sensor_i2c_read_reg16_fake.call_count = 0;
    xy_sensor_i2c_read_reg_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 27561;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(27561, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT16(3700, fg->latest.voltage_mv);
    TEST_ASSERT_EQUAL_INT16(-123, fg->latest.current_ma);
    TEST_ASSERT_EQUAL_UINT8(66, fg->latest.soc);
    TEST_ASSERT_EQUAL_UINT8(97, fg->latest.soh);
    TEST_ASSERT_EQUAL_INT16(250, fg->latest.temperature_c);
    TEST_ASSERT_EQUAL_UINT16(3000, fg->latest.full_capacity_mah);
    TEST_ASSERT_EQUAL_UINT16(1980, fg->latest.remain_capacity_mah);
    TEST_ASSERT_EQUAL_UINT32(42, fg->latest.cycle_count);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(7, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(2, xy_sensor_i2c_read_reg_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_CYCLE_CNT, xy_sensor_i2c_read_reg16_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(REG_SOH, xy_sensor_i2c_read_reg_fake.arg1_val);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3700, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-123, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-77, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(66, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(97, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(250, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(3000, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(1980, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(42, value);

}

void test_bq27z561_fetch_failure_preserves_cached_snapshot(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3700, value);

    fake_reg16[REG_VOLT] = 4100;
    fake_reg16[REG_CURR] = (uint16_t)(int16_t)-456;
    fake_reg16[REG_AVG_CURR] = (uint16_t)(int16_t)-333;
    fake_reg8[REG_SOC] = 44;
    fake_reg8[REG_SOH] = 90;
    fake_reg16[REG_TEMP] = 3031;
    fake_reg16[REG_NOM_CAP] = 2800;
    fake_reg16[REG_REM_CAP] = 1234;
    fake_reg16[REG_CYCLE_CNT] = 99;
    fg->latest.timestamp = 12345;
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 54321;

    fail_reg16 = REG_TEMP;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(12345, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);

    fail_reg16 = -1;
    fail_reg8 = REG_SOH;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(12345, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);

    fail_reg8 = -1;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(54321, fg->latest.timestamp);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(4100, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-456, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-333, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(44, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(90, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(300, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(2800, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(1234, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(99, value);
}

void test_bq27z561_fetch_skips_remaining_reads_after_first_reg16_failure(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    xy_sensor_i2c_read_reg16_fake.call_count = 0;
    xy_sensor_i2c_read_reg_fake.call_count = 0;
    memset(reg16_read_counts, 0, sizeof(reg16_read_counts));
    xy_os_tick_get_fake.call_count = 0;

    fail_reg16 = REG_VOLT;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(3, xy_sensor_i2c_read_reg16_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(3, reg16_read_counts[REG_VOLT]);
    TEST_ASSERT_EQUAL_UINT32(0, reg16_read_counts[REG_CURR]);
    TEST_ASSERT_EQUAL_UINT32(0, reg16_read_counts[REG_AVG_CURR]);
    TEST_ASSERT_EQUAL_UINT32(0, reg16_read_counts[REG_TEMP]);
    TEST_ASSERT_EQUAL_UINT32(0, reg16_read_counts[REG_NOM_CAP]);
    TEST_ASSERT_EQUAL_UINT32(0, reg16_read_counts[REG_REM_CAP]);
    TEST_ASSERT_EQUAL_UINT(0, xy_sensor_i2c_read_reg_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
}

void test_bq27z561_fetch_retries_transient_register_read_failures(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    xy_sensor_i2c_read_reg16_fake.call_count = 0;
    xy_sensor_i2c_read_reg_fake.call_count = 0;
    memset(reg16_read_counts, 0, sizeof(reg16_read_counts));
    xy_os_tick_get_fake.return_val = 5610;

    fail_reg16_reads[REG_VOLT] = 2;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(5610, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3700, value);
    TEST_ASSERT_EQUAL_UINT32(4, reg16_read_counts[REG_VOLT]);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(7, xy_sensor_i2c_read_reg16_fake.call_count);

    xy_sensor_i2c_read_reg16_fake.call_count = 0;
    xy_sensor_i2c_read_reg_fake.call_count = 0;
    memset(reg16_read_counts, 0, sizeof(reg16_read_counts));
    fake_reg8[REG_SOC] = 55;
    xy_os_tick_get_fake.return_val = 5611;
    fail_reg8_reads[REG_SOC] = 2;

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(5611, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(55, value);
    TEST_ASSERT_EQUAL_UINT(6, xy_sensor_i2c_read_reg_fake.call_count);
}

void test_bq27z561_rejects_invalid_output_and_unknown_channel(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0x27561;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, (xy_fuel_gauge_data_type_t)99, &value));
    TEST_ASSERT_EQUAL_INT32(0x27561, value);
}

void test_bq27z561_get_failure_preserves_output_value(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0x561;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fail_reg16 = REG_CURR;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(0x561, value);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);

    fail_reg16 = REG_AVG_CURR;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(0x561, value);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);

    fail_reg16 = -1;
    fail_reg8 = REG_SOC;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(0x561, value);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
}

void test_bq27z561_inline_getters_preserve_outputs_on_failure(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    uint16_t voltage = 0x561;
    int16_t current = -561;
    uint8_t soc = 56;
    uint8_t soh = 61;
    int16_t temp = -27;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(NULL, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x561, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(NULL, &current));
    TEST_ASSERT_EQUAL_INT16(-561, current);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(NULL, &soc));
    TEST_ASSERT_EQUAL_UINT8(56, soc);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(NULL, &soh));
    TEST_ASSERT_EQUAL_UINT8(61, soh);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(NULL, &temp));
    TEST_ASSERT_EQUAL_INT16(-27, temp);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(fg, NULL));

    fail_reg16 = REG_VOLT;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x561, voltage);

    fail_reg16 = REG_CURR;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_current(fg, &current));
    TEST_ASSERT_EQUAL_INT16(-561, current);

    fail_reg16 = -1;
    fail_reg8 = REG_SOC;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_soc(fg, &soc));
    TEST_ASSERT_EQUAL_UINT8(56, soc);

    fail_reg8 = REG_SOH;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_soh(fg, &soh));
    TEST_ASSERT_EQUAL_UINT8(61, soh);

    fail_reg8 = -1;
    fail_reg16 = REG_TEMP;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_temperature(fg, &temp));
    TEST_ASSERT_EQUAL_INT16(-27, temp);
}

void test_bq27z561_alert_set_get_uses_cached_thresholds(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 12,
        .high_soc_threshold = 88,
        .low_voltage_mv = 3200,
        .high_voltage_mv = 4300,
        .over_current_ma = 2500,
        .over_temp_c = 60,
    };
    xy_fuel_gauge_alert_t readback = {
        .low_soc_threshold = 0xAA,
        .high_soc_threshold = 0xBB,
        .low_voltage_mv = 0xCCCC,
        .high_voltage_mv = 0xDDDD,
        .over_current_ma = 0x1111,
        .over_temp_c = 0x2222,
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
    TEST_ASSERT_EQUAL_UINT8(0xAA, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(0xBB, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL_UINT16(0xCCCC, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(0xDDDD, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL_INT16(0x1111, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(0x2222, readback.over_temp_c);

    memset(&readback, 0, sizeof(readback));
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

void test_bq27z561_direct_api_guards_preserve_outputs(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    xy_fuel_gauge_t missing_data = *fg;
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 11,
        .high_soc_threshold = 91,
        .low_voltage_mv = 3100,
        .high_voltage_mv = 4400,
        .over_current_ma = 2100,
        .over_temp_c = 610,
    };
    xy_fuel_gauge_alert_t readback = {
        .low_soc_threshold = 0xAA,
        .high_soc_threshold = 0xBB,
        .low_voltage_mv = 0xCCCC,
        .high_voltage_mv = 0xDDDD,
        .over_current_ma = 0x1111,
        .over_temp_c = 0x2222,
    };
    int32_t value = 0x561;

    missing_data.data = NULL;

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->init(NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->init(&missing_data));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->fetch(NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->fetch(&missing_data));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(NULL, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(0x561, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(&missing_data, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(0x561, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, NULL));
    TEST_ASSERT_EQUAL_INT32(0x561, value);

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(0x561, value);

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

void test_bq27z561_reinit_preserves_alert_threshold_cache(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 9,
        .high_soc_threshold = 96,
        .low_voltage_mv = 3150,
        .high_voltage_mv = 4280,
        .over_current_ma = 2200,
        .over_temp_c = 620,
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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bq27z561_register_rejects_null_i2c_handle_without_bus_side_effects);
    RUN_TEST(test_bq27z561_registers_default_i2c_bus);
    RUN_TEST(test_bq27z561_register_duplicate_does_not_reconfigure_bus);
    RUN_TEST(test_bq27z561_init_reads_device_id);
    RUN_TEST(test_bq27z561_init_failure_preserves_uninitialized_state);
    RUN_TEST(test_bq27z561_direct_init_failure_clears_stale_private_state);
    RUN_TEST(test_bq27z561_fetch_and_channel_get);
    RUN_TEST(test_bq27z561_fetch_failure_preserves_cached_snapshot);
    RUN_TEST(test_bq27z561_fetch_skips_remaining_reads_after_first_reg16_failure);
    RUN_TEST(test_bq27z561_fetch_retries_transient_register_read_failures);
    RUN_TEST(test_bq27z561_rejects_invalid_output_and_unknown_channel);
    RUN_TEST(test_bq27z561_get_failure_preserves_output_value);
    RUN_TEST(test_bq27z561_inline_getters_preserve_outputs_on_failure);
    RUN_TEST(test_bq27z561_alert_set_get_uses_cached_thresholds);
    RUN_TEST(test_bq27z561_direct_api_guards_preserve_outputs);
    RUN_TEST(test_bq27z561_reinit_preserves_alert_threshold_cache);
    return UNITY_END();
}
