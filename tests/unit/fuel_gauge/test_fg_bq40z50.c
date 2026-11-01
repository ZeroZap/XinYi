#include "unity.h"
#include "fff.h"
#include "xy_fg_bq40z50.h"
#include "xy_device.h"

#include <stdint.h>
#include <string.h>

#define REG_CTRL        0x00
#define REG_TEMP        0x06
#define REG_VOLT        0x08
#define REG_CURR        0x0A
#define REG_AVG_CURR    0x0B
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

/*: The legacy sensor bus allowed 100 ms per transfer. The migration has to keep that figure
 *: explicit, because xy_i2c_device_init() reads a zero timeout as 1000 ms - a tenfold widening
 *: of the wedged-bus budget that nothing else in this suite would notice. */
#define EXPECTED_IO_TIMEOUT_MS 100U

static uint8_t fake_regs[256][4];
static uint8_t fake_read_failures[256];
static xy_i2c_device_t last_bus;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VALUE_FUNC(xy_error_t, xy_i2c_device_init, xy_i2c_device_t *, void *, uint16_t, uint32_t)
FAKE_VALUE_FUNC(xy_error_t, xy_i2c_device_read_reg_repeated_start,
                xy_i2c_device_t *, uint8_t, uint8_t *, size_t)
FAKE_VALUE_FUNC(xy_error_t, xy_i2c_device_read_reg16_repeated_start,
                xy_i2c_device_t *, uint8_t, uint16_t *)

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

static uint16_t fake_get16(uint8_t reg)
{
    return ((uint16_t)fake_regs[reg][1] << 8) | fake_regs[reg][0];
}

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

static void assert_canonical_device(const xy_i2c_device_t *dev)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE_MESSAGE(dev->base.initialized, "the driver must reach a live I2C device");
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
}

static xy_error_t xy_i2c_device_read_reg_impl(xy_i2c_device_t *dev, uint8_t reg,
                                              uint8_t *data, size_t len)
{
    assert_canonical_device(dev);
    TEST_ASSERT_NOT_NULL(data);
    /* A gauge word is two bytes and a gauge block is four; anything else is a driver bug. */
    if (len == 2U) {
        TEST_ASSERT_EQUAL_UINT(2U, len);
    } else {
        TEST_ASSERT_EQUAL_UINT(4U, len);
    }

    if (fake_read_failures[reg] > 0) {
        fake_read_failures[reg]--;
        return XY_DEVICE_IO_ERROR;
    }

    memcpy(data, fake_regs[reg], len);
    return XY_DEVICE_OK;
}

static xy_error_t xy_i2c_device_read_reg16_impl(xy_i2c_device_t *dev, uint8_t reg,
                                                uint16_t *value)
{
    assert_canonical_device(dev);
    TEST_ASSERT_NOT_NULL(value);

    if (fake_read_failures[reg] > 0) {
        fake_read_failures[reg]--;
        return XY_DEVICE_IO_ERROR;
    }

    *value = fake_get16(reg);
    return XY_DEVICE_OK;
}

static void reset_sensor_fakes(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_i2c_device_init);
    RESET_FAKE(xy_i2c_device_read_reg_repeated_start);
    RESET_FAKE(xy_i2c_device_read_reg16_repeated_start);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.return_val = 1000;
    xy_i2c_device_init_fake.custom_fake = xy_i2c_device_init_impl;
    xy_i2c_device_read_reg_repeated_start_fake.custom_fake = xy_i2c_device_read_reg_impl;
    xy_i2c_device_read_reg16_repeated_start_fake.custom_fake = xy_i2c_device_read_reg16_impl;
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
    fake_set16(REG_AVG_CURR, (uint16_t)(int16_t)-543);
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
    TEST_ASSERT_EQUAL_UINT(1, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR((void *)0x4050, xy_i2c_device_init_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(BQ40Z50_ADDR, xy_i2c_device_init_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT32(EXPECTED_IO_TIMEOUT_MS, xy_i2c_device_init_fake.arg3_val);
    TEST_ASSERT_EQUAL_UINT32(EXPECTED_IO_TIMEOUT_MS, last_bus.timeout);
    TEST_ASSERT_TRUE(last_bus.base.initialized);
    TEST_ASSERT_EQUAL_PTR((void *)0x4050, last_bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(BQ40Z50_ADDR, last_bus.dev_addr);
}

void test_bq40z50_register_rejects_null_handle_without_i2c_side_effects(void)
{
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_bq40z50_register(NULL, 0));
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_NULL(xy_fuel_gauge_device_get("BQ40Z50"));
}

void test_bq40z50_register_duplicate_does_not_reconfigure_bus(void)
{
    TEST_ASSERT_NOT_NULL(registered_bq40z50());
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_bq40z50_register((void *)0xBAD, 0x44));
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(NULL, last_bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(0, last_bus.dev_addr);
}

void test_bq40z50_init_fetch_channel_and_pack_helpers(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    int32_t value = 0;
    uint16_t voltage = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized, "BQ40Z50 core device should be initialized");
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(4, xy_i2c_device_read_reg_repeated_start_fake.call_count);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    xy_i2c_device_read_reg_repeated_start_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 5050;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(1, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(5050, fg->latest.timestamp);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(15, xy_i2c_device_read_reg_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(REG_BAL_STATUS, xy_i2c_device_read_reg_repeated_start_fake.arg1_val);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(15234, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234, voltage);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-654, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-543, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(100, value);

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
    xy_fuel_gauge_t missing_data = *fg;
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 10,
        .high_soc_threshold = 90,
        .low_voltage_mv = 12000,
        .high_voltage_mv = 16800,
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
    int32_t value = 123456;
    uint16_t voltage = 4321;

    missing_data.data = NULL;
    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->init(NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->init(&missing_data));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->fetch(NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->fetch(&missing_data));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(NULL, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(&missing_data, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);

    fg->initialized = false;
    xy_fuel_gauge_deinit(fg);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TIME_TO_EMPTY, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TIME_TO_FULL, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      fg->api->channel_get(fg, XY_FG_DATA_TIME_TO_EMPTY, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      fg->api->channel_get(fg, XY_FG_DATA_TIME_TO_FULL, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(NULL, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(&missing_data, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(NULL, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(&missing_data, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 0, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 5, &voltage));
    TEST_ASSERT_EQUAL_UINT16(4321, voltage);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_set(NULL, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_set(&missing_data, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_set(fg, NULL));

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED, fg->api->alert_set(fg, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED, fg->api->alert_get(fg, &readback));
    TEST_ASSERT_EQUAL_UINT8(0xAA, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(0xBB, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL_UINT16(0xCCCC, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(0xDDDD, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL_INT16(0x1111, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(0x2222, readback.over_temp_c);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_get(NULL, &readback));
    TEST_ASSERT_EQUAL_UINT8(0xAA, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(0xBB, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_get(&missing_data, &readback));
    TEST_ASSERT_EQUAL_UINT16(0xCCCC, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(0xDDDD, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, fg->api->alert_get(fg, NULL));
    TEST_ASSERT_EQUAL_INT16(0x1111, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(0x2222, readback.over_temp_c);

    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_charging(&missing_data));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(&missing_data));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_full(&missing_data));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_protected(&missing_data));
    TEST_ASSERT_EQUAL_UINT32(0, xy_fuel_gauge_bq40z50_get_protection_status(&missing_data));
    TEST_ASSERT_EQUAL_UINT8(0, xy_fuel_gauge_bq40z50_get_balance_status(&missing_data));
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
    memset(&readback, 0, sizeof(readback));

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
    TEST_ASSERT_TRUE_MESSAGE(fg->initialized, "BQ40Z50 core device should be initialized");
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));
}

void test_bq40z50_direct_init_failure_clears_stale_private_state(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    fake_fail_reads(REG_CTRL, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, fg->api->init(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0, xy_fuel_gauge_bq40z50_get_balance_status(fg));
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
    xy_i2c_device_read_reg_repeated_start_fake.call_count = 0;

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
    TEST_ASSERT_EQUAL_UINT(0U, xy_i2c_device_read_reg_repeated_start_fake.call_count);
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
    xy_i2c_device_read_reg_repeated_start_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 5050;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(5050, fg->latest.timestamp);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(15, xy_i2c_device_read_reg_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-654, value);

    previous_timestamp = fg->latest.timestamp;
    fake_set16(REG_VOLT, 9999);
    fake_set16(REG_CURR, 1234);
    fake_set16(REG_AVG_CURR, 1111);
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
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-543, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(100, value);
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

    xy_i2c_device_read_reg_repeated_start_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 8080;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(8080, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(9999, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(1234, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(1111, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(88, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(100, value);
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

void test_bq40z50_inline_getters_preserve_outputs_on_fetch_failure(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    uint16_t voltage = 0x4050;
    int16_t current = -4050;
    uint8_t soc = 40;
    uint8_t soh = 50;
    int16_t temp = -405;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(NULL, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(NULL, &current));
    TEST_ASSERT_EQUAL_INT16(-4050, current);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(NULL, &soc));
    TEST_ASSERT_EQUAL_UINT8(40, soc);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(NULL, &soh));
    TEST_ASSERT_EQUAL_UINT8(50, soh);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(NULL, &temp));
    TEST_ASSERT_EQUAL_INT16(-405, temp);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(fg, NULL));

    fake_fail_reads(REG_VOLT, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    current = -4050;
    fake_fail_reads(REG_CURR, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_current(fg, &current));
    TEST_ASSERT_EQUAL_INT16(-4050, current);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    soc = 40;
    fake_fail_reads(REG_SOC, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_soc(fg, &soc));
    TEST_ASSERT_EQUAL_UINT8(40, soc);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    soh = 50;
    fake_fail_reads(REG_FULL_CAP, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_soh(fg, &soh));
    TEST_ASSERT_EQUAL_UINT8(50, soh);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    temp = -405;
    fake_fail_reads(REG_TEMP, 3);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_temperature(fg, &temp));
    TEST_ASSERT_EQUAL_INT16(-405, temp);
}

void test_bq40z50_late_cell_voltage_failure_preserves_cached_snapshot(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    uint16_t voltage = 0;
    int32_t value = 0;
    uint32_t previous_timestamp = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    previous_timestamp = fg->latest.timestamp;

    fake_set16(REG_VOLT, 9999);
    fake_set16(REG_CURR, 1234);
    fake_set16(REG_AVG_CURR, 1111);
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
    fake_fail_reads(REG_CELL4_VOLT, 3);
    xy_os_tick_get_fake.call_count = 0;
    xy_os_tick_get_fake.return_val = 9090;

    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(previous_timestamp, fg->latest.timestamp);
    TEST_ASSERT_EQUAL_UINT(0, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(3810, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 4, &voltage));
    TEST_ASSERT_EQUAL_UINT16(3804, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg->api->channel_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-654, value);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(fg));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_full(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_protected(fg));
    TEST_ASSERT_EQUAL_UINT32(0x00000051, xy_fuel_gauge_bq40z50_get_protection_status(fg));
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));
}

void test_bq40z50_balance_read_uses_bounded_retry_without_fetch_side_effects(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    xy_fuel_gauge_t missing_data = *fg;
    uint8_t balance = 0xAA;

    missing_data.data = NULL;
    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_read_balance_status(NULL, &balance));
    TEST_ASSERT_EQUAL_UINT8(0xAA, balance);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_read_balance_status(&missing_data, &balance));
    TEST_ASSERT_EQUAL_UINT8(0xAA, balance);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_read_balance_status(fg, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      xy_fuel_gauge_bq40z50_read_balance_status(fg, &balance));
    TEST_ASSERT_EQUAL_UINT8(0xAA, balance);
    TEST_ASSERT_EQUAL_UINT(0U, xy_i2c_device_read_reg16_repeated_start_fake.call_count);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));

    fake_set16(REG_BAL_STATUS, 0x000C);
    fake_fail_reads(REG_BAL_STATUS, 2);
    xy_i2c_device_read_reg16_repeated_start_fake.call_count = 0;
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      xy_fuel_gauge_bq40z50_read_balance_status(fg, &balance));
    TEST_ASSERT_EQUAL_UINT8(0x0C, balance);
    TEST_ASSERT_EQUAL_UINT(3U, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    balance = 0xAA;
    fake_set16(REG_BAL_STATUS, 0x000D);
    fake_fail_reads(REG_BAL_STATUS, 3);
    xy_i2c_device_read_reg16_repeated_start_fake.call_count = 0;
    TEST_ASSERT_EQUAL(XY_FG_ERROR,
                      xy_fuel_gauge_bq40z50_read_balance_status(fg, &balance));
    TEST_ASSERT_EQUAL_UINT8(0xAA, balance);
    TEST_ASSERT_EQUAL_UINT(3U, xy_i2c_device_read_reg16_repeated_start_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0x05, xy_fuel_gauge_bq40z50_get_balance_status(fg));
}

void test_bq40z50_direct_getters_preserve_outputs_on_invalid_or_uninitialized_calls(void)
{
    xy_fuel_gauge_t *fg = registered_bq40z50();
    xy_fuel_gauge_t missing_data = *fg;
    uint16_t voltage = 0x4050;
    int32_t value = 123456;

    missing_data.data = NULL;
    fg->initialized = false;

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(NULL, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(&missing_data, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      fg->api->channel_get(fg, XY_FG_DATA_VOLTAGE, NULL));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(NULL, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(&missing_data, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(NULL, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(&missing_data, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_INITIALIZED,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 1, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 0, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_bq40z50_get_cell_voltage(fg, 5, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x4050, voltage);
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      fg->api->channel_get(fg, XY_FG_DATA_TIME_TO_FULL, &value));
    TEST_ASSERT_EQUAL_INT32(123456, value);
}

void test_bq40z50_register_instance_isolates_multiple_instances(void)
{
    static xy_fuel_gauge_t fg_a;
    static xy_fg_bq40z50_data_t priv_a;
    static xy_fuel_gauge_t fg_b;
    static xy_fg_bq40z50_data_t priv_b;

    fg_a.name = "BQ40Z50_ISO_A";
    fg_b.name = "BQ40Z50_ISO_B";

    uint8_t before = xy_fuel_gauge_device_count();

    TEST_ASSERT_EQUAL(XY_FG_OK,
                      xy_fuel_gauge_bq40z50_register_instance(&fg_a, &priv_a,
                                                              (void *)0xA040, 0x10));
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      xy_fuel_gauge_bq40z50_register_instance(&fg_b, &priv_b,
                                                              (void *)0xB040, 0x20));

    TEST_ASSERT_EQUAL_UINT8(before + 2, xy_fuel_gauge_device_count());
    TEST_ASSERT_EQUAL_PTR(&fg_a, xy_fuel_gauge_device_get("BQ40Z50_ISO_A"));
    TEST_ASSERT_EQUAL_PTR(&fg_b, xy_fuel_gauge_device_get("BQ40Z50_ISO_B"));
    TEST_ASSERT_TRUE(fg_a.data != fg_b.data);

    /* Distinct bus configuration per instance */
    TEST_ASSERT_EQUAL_PTR((void *)0xA040, priv_a.bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(0x10, priv_a.bus.dev_addr);
    TEST_ASSERT_EQUAL_PTR((void *)0xB040, priv_b.bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(0x20, priv_b.bus.dev_addr);

    /* Independent initialization */
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(&fg_a));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(&fg_b));
    TEST_ASSERT_TRUE(priv_a.initialized);
    TEST_ASSERT_TRUE(priv_b.initialized);

    /* Fetch A with one register set, B with another; caches must stay isolated */
    fake_set16(REG_VOLT, 15234);
    fake_set16(REG_CURR, (uint16_t)(int16_t)-654);
    fake_set16(REG_AVG_CURR, (uint16_t)(int16_t)-543);
    fake_set16(REG_SOC, 7550);
    fake_set16(REG_BAT_STATUS, 0x0009);
    xy_os_tick_get_fake.return_val = 1000;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(&fg_a));

    fake_set16(REG_VOLT, 16800);
    fake_set16(REG_CURR, (uint16_t)(int16_t)-100);
    fake_set16(REG_AVG_CURR, (uint16_t)(int16_t)-90);
    fake_set16(REG_SOC, 4400);
    fake_set16(REG_BAT_STATUS, 0x0002);
    xy_os_tick_get_fake.return_val = 2000;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(&fg_b));

    int32_t value = 0;
    TEST_ASSERT_EQUAL(XY_FG_OK, fg_a.api->channel_get(&fg_a, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(15234, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg_a.api->channel_get(&fg_a, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      fg_a.api->channel_get(&fg_a, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-543, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, fg_b.api->channel_get(&fg_b, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(16800, value);
    TEST_ASSERT_EQUAL(XY_FG_OK, fg_b.api->channel_get(&fg_b, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(44, value);
    TEST_ASSERT_EQUAL(XY_FG_OK,
                      fg_b.api->channel_get(&fg_b, XY_FG_DATA_AVERAGE_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-90, value);

    /* Per-instance timestamps */
    TEST_ASSERT_EQUAL_UINT32(1000, fg_a.latest.timestamp);
    TEST_ASSERT_EQUAL_UINT32(2000, fg_b.latest.timestamp);

    /* Per-instance battery-status isolation (charging vs discharging) */
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_charging(&fg_a));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_discharging(&fg_a));
    TEST_ASSERT_FALSE(xy_fuel_gauge_bq40z50_is_charging(&fg_b));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq40z50_is_discharging(&fg_b));
}

void test_bq40z50_register_instance_rejects_null_inputs_without_side_effects(void)
{
    static xy_fuel_gauge_t fg;
    static xy_fg_bq40z50_data_t priv;
    xy_fuel_gauge_t no_name = {0};

    fg.name = "BQ40Z50_NULL";
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_register_instance(NULL, &priv, (void *)0x1, 0));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_register_instance(&fg, NULL, (void *)0x1, 0));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_register_instance(&fg, &priv, NULL, 0));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_bq40z50_register_instance(&no_name, &priv, (void *)0x1, 0));

    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(0, last_bus.dev_addr);
    TEST_ASSERT_NULL(last_bus.i2c_handle);
}

void test_bq40z50_register_instance_rejects_duplicate_name_without_reconfiguring_bus(void)
{
    registered_bq40z50();
    reset_sensor_fakes();
    memset(&last_bus, 0, sizeof(last_bus));

    static xy_fuel_gauge_t fg_dup;
    static xy_fg_bq40z50_data_t priv_dup;
    fg_dup.name = "BQ40Z50";

    TEST_ASSERT_EQUAL(XY_FG_ERROR,
                      xy_fuel_gauge_bq40z50_register_instance(&fg_dup, &priv_dup,
                                                              (void *)0xBAD, 0x44));
    TEST_ASSERT_EQUAL_UINT(0, xy_i2c_device_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(NULL, last_bus.i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(0, last_bus.dev_addr);
    TEST_ASSERT_NULL(fg_dup.api);
    TEST_ASSERT_NULL(fg_dup.data);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bq40z50_register_rejects_null_handle_without_i2c_side_effects);
    RUN_TEST(test_bq40z50_registers_default_i2c_bus);
    RUN_TEST(test_bq40z50_register_duplicate_does_not_reconfigure_bus);
    RUN_TEST(test_bq40z50_init_fetch_channel_and_pack_helpers);
    RUN_TEST(test_bq40z50_rejects_invalid_channel_and_cell);
    RUN_TEST(test_bq40z50_alert_set_get_uses_cached_thresholds);
    RUN_TEST(test_bq40z50_status_helpers_handle_null_and_persistent_nack);
    RUN_TEST(test_bq40z50_init_tolerates_optional_status_read_failures);
    RUN_TEST(test_bq40z50_direct_init_failure_clears_stale_private_state);
    RUN_TEST(test_bq40z50_fetch_updates_balance_status_cache);
    RUN_TEST(test_bq40z50_public_helpers_use_cached_snapshot_without_i2c_side_effects);
    RUN_TEST(test_bq40z50_retries_transient_nack_and_preserves_snapshot_on_failure);
    RUN_TEST(test_bq40z50_retries_discharge_status_path);
    RUN_TEST(test_bq40z50_inline_getters_preserve_outputs_on_fetch_failure);
    RUN_TEST(test_bq40z50_late_cell_voltage_failure_preserves_cached_snapshot);
    RUN_TEST(test_bq40z50_balance_read_uses_bounded_retry_without_fetch_side_effects);
    RUN_TEST(test_bq40z50_direct_getters_preserve_outputs_on_invalid_or_uninitialized_calls);
    RUN_TEST(test_bq40z50_register_instance_isolates_multiple_instances);
    RUN_TEST(test_bq40z50_register_instance_rejects_null_inputs_without_side_effects);
    RUN_TEST(test_bq40z50_register_instance_rejects_duplicate_name_without_reconfiguring_bus);
    return UNITY_END();
}
