#include "unity.h"
#include "fff.h"
#include "xy_fg_bq40z50.h"
#include "xy_sensor_device.h"

#include <stdint.h>
#include <string.h>

#define REG_CTRL        0x00
#define REG_TEMP        0x06
#define REG_VOLT        0x08
#define REG_CURR        0x0A
#define REG_CYCLE_CNT   0x2A
#define REG_SOC         0x2C
#define REG_REM_CAP     0x2E
#define REG_FULL_CAP    0x30
#define REG_CELL1_VOLT  0x3C
#define REG_CELL2_VOLT  0x3E
#define REG_CELL3_VOLT  0x40
#define REG_CELL4_VOLT  0x42
#define REG_BAT_STATUS  0x64
#define REG_PROT_STATUS 0x66
#define REG_BAL_STATUS  0x68

static uint8_t fake_regs[256][4];
static uint8_t fake_read_failures[256];
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

static void fake_set16(uint8_t reg, uint16_t value)
{
    fake_regs[reg][0] = (uint8_t)(value & 0xFFu);
    fake_regs[reg][1] = (uint8_t)(value >> 8);
}

static void fake_set32(uint8_t reg, uint32_t value)
{
    fake_regs[reg][0] = (uint8_t)(value & 0xFFu);
    fake_regs[reg][1] = (uint8_t)(value >> 8);
    fake_regs[reg][2] = (uint8_t)(value >> 16);
    fake_regs[reg][3] = (uint8_t)(value >> 24);
}

static void fake_fail_reads(uint8_t reg, uint8_t failures)
{
    fake_read_failures[reg] = failures;
}

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
    if (len == 2U) {
        TEST_ASSERT_EQUAL_UINT(2U, len);
    } else {
        TEST_ASSERT_EQUAL_UINT(4U, len);
    }

    if (fake_read_failures[reg] > 0) {
        fake_read_failures[reg]--;
        return -1;
    }

    memcpy(data, fake_regs[reg], len);
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
    memset(&last_bus, 0, sizeof(last_bus));
    reset_sensor_fakes();

    fake_set16(REG_CTRL, 0x4050);
    fake_set16(REG_BAT_STATUS, 0x0009);
    fake_set32(REG_PROT_STATUS, 0x00000051);
    fake_set16(REG_BAL_STATUS, 0x0005);
    fake_set16(REG_VOLT, 15234);
    fake_set16(REG_CURR, (uint16_t)(int16_t)-654);
    fake_set16(REG_SOC, 7550);
    fake_set16(REG_TEMP, 3001);
    fake_set16(REG_FULL_CAP, 6800);
    fake_set16(REG_REM_CAP, 5100);
    fake_set16(REG_CYCLE_CNT, 222);
    fake_set16(REG_CELL1_VOLT, 3810);
    fake_set16(REG_CELL2_VOLT, 3808);
    fake_set16(REG_CELL3_VOLT, 3812);
    fake_set16(REG_CELL4_VOLT, 3804);
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *registered_bq40z50(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ40Z50");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_bq40z50_register((void *)0x4050, 0));
        fg = xy_fuel_gauge_device_get("BQ40Z50");
    }
    TEST_ASSERT_NOT_NULL(fg);
    return fg;
}

void test_bq40z50_registers_default_i2c_bus(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();

    TEST_ASSERT_NOT_NULL(fg);
    TEST_ASSERT_EQUAL_UINT(1, xy_sensor_bus_config_i2c_fake.call_count);
    TEST_ASSERT_EQUAL_PTR((void *)0x4050, xy_sensor_bus_config_i2c_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(BQ40Z50_ADDR, xy_sensor_bus_config_i2c_fake.arg2_val);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, last_bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x4050, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(BQ40Z50_ADDR, last_bus.address);
}

void test_bq40z50_init_fetch_channel_and_pack_helpers(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    int32_t value = 0;
    uint16_t voltage = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(fg->initialized);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(4, xy_sensor_i2c_read_fake.call_count);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    xy_sensor_i2c_read_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 5050;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(5050, fg->latest.timestamp);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(14, xy_sensor_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_BAL_STATUS, xy_sensor_i2c_read_fake.arg1_val);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(15234, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234, voltage);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-654, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(270, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(6800, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(5100, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(222, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(3810, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 4, &voltage));
    TEST_ASSERT_EQUAL_UINT16(3804, voltage);
}

void test_bq40z50_rejects_invalid_channel_and_cell(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    int32_t value = 0;
    uint16_t voltage = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TIME_TO_EMPTY, &value));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(NULL, &voltage));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(NULL, 1, &voltage));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 5, &voltage));
}

void test_bq40z50_alert_set_get_uses_cached_thresholds(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 15,
        .high_soc_threshold = 95,
        .low_voltage_mv = 12000,
        .high_voltage_mv = 16800,
        .over_current_ma = 3200,
        .over_temp_c = 600,
    };
    xy_fuel_gauge_alert_t readback;

    memset(&readback, 0, sizeof(readback));

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(fg, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(NULL, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get_alert(NULL, &readback));

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

void test_bq40z50_status_helpers_handle_null_and_persistent_nack(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();

    TEST_ASSERT_EQUAL_UINT8(0, xy_fuel_gauge_bq40z50_get_balance_status(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, xy_fuel_gauge_bq40z50_get_protection_status(NULL));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_charging(NULL));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(NULL));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_full(NULL));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_protected(NULL));

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fake_fail_reads(REG_BAL_STATUS, 3);
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));
}

void test_bq40z50_init_tolerates_optional_status_read_failures(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    fake_set16(REG_BAT_STATUS, 0x000A);
    fake_set32(REG_PROT_STATUS, 0x00000020);
    fake_set16(REG_BAL_STATUS, 0x000A);
    fake_fail_reads(REG_BAT_STATUS, 3);
    fake_fail_reads(REG_PROT_STATUS, 3);
    fake_fail_reads(REG_BAL_STATUS, 3);

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(fg->initialized);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));
}

void test_bq40z50_fetch_updates_balance_status_cache(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    fake_set16(REG_BAL_STATUS, 0x000A);
    xy_os_tick_get_fake.return_val = 5050;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT8(0x0A, xy_fuel_gauge_bq40z50_get_balance_status(fg));
}

void test_bq40z50_public_helpers_use_cached_snapshot_without_i2c_side_effects(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    uint16_t voltage = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));

    fake_set16(REG_VOLT, 9999);
    fake_set16(REG_CELL1_VOLT, 3999);
    fake_set16(REG_CELL4_VOLT, 3888);
    fake_set16(REG_BAT_STATUS, 0x0002);
    fake_set32(REG_PROT_STATUS, 0x00000000);
    fake_set16(REG_BAL_STATUS, 0x000A);
    xy_sensor_i2c_read_fake.call_count = 0;

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(3810, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 4, &voltage));
    TEST_ASSERT_EQUAL_UINT16(3804, voltage);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));
    TEST_ASSERT_EQUAL_UINT(0U, xy_sensor_i2c_read_fake.call_count);
}

void test_bq40z50_retries_transient_nack_and_preserves_snapshot_on_failure(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    int32_t value = 0;
    uint32_t previous_timestamp = 0;
    uint16_t voltage = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fake_fail_reads(REG_CURR, 1);
    xy_sensor_i2c_read_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 5050;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(5050, fg->latest.timestamp);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(15, xy_sensor_i2c_read_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-654, value);

    previous_timestamp = fg->latest.timestamp;
    fake_set16(REG_VOLT, 9999);
    fake_set16(REG_CURR, 1234);
    fake_set16(REG_SOC, 8800);
    fake_set16(REG_REM_CAP, 6000);
    fake_set16(REG_FULL_CAP, 7200);
    fake_set16(REG_CYCLE_CNT, 333);
    fake_set16(REG_TEMP, 3051);
    fake_set16(REG_CELL1_VOLT, 4011);
    fake_set16(REG_CELL2_VOLT, 4012);
    fake_set16(REG_CELL3_VOLT, 4013);
    fake_set16(REG_CELL4_VOLT, 4014);
    fake_set16(REG_BAT_STATUS, 0x0002);
    fake_set32(REG_PROT_STATUS, 0x00000000);
    fake_set16(REG_BAL_STATUS, 0x000A);
    fake_fail_reads(REG_CURR, 3);
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 6060;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(previous_timestamp, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-654, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));

    fake_fail_reads(REG_BAL_STATUS, 3);
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 7070;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(previous_timestamp, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234, voltage);
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    xy_sensor_i2c_read_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 8080;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(8080, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(9999, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(1234, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(88, value);
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT8(0x0A, xy_fuel_gauge_bq40z50_get_balance_status(fg));
}

void test_bq40z50_retries_discharge_status_path(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();

    fake_set16(REG_BAT_STATUS, 0x000A);
    fake_fail_reads(REG_BAT_STATUS, 1);

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_discharging(fg));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bq40z50_registers_default_i2c_bus);
    RUN_TEST(test_bq40z50_init_fetch_channel_and_pack_helpers);
    RUN_TEST(test_bq40z50_rejects_invalid_channel_and_cell);
    RUN_TEST(test_bq40z50_alert_set_get_uses_cached_thresholds);
    RUN_TEST(test_bq40z50_status_helpers_handle_null_and_persistent_nack);
    RUN_TEST(test_bq40z50_init_tolerates_optional_status_read_failures);
    RUN_TEST(test_bq40z50_fetch_updates_balance_status_cache);
    RUN_TEST(test_bq40z50_public_helpers_use_cached_snapshot_without_i2c_side_effects);
    RUN_TEST(test_bq40z50_retries_transient_nack_and_preserves_snapshot_on_failure);
    RUN_TEST(test_bq40z50_retries_discharge_status_path);
    return UNITY_END();
}
