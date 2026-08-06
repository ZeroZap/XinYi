/**
 * @file test_fuel_gauge_smbus_hardware_smoke_example.c
 * @brief Build-guarded smoke skeleton for Fuel Gauge SMBus board validation.
 *
 * This host-only smoke keeps the intended board validation flow compiled and
 * documented without claiming real SMBus hardware evidence from fake I2C data.
 */

#include "unity.h"
#include "xy_fg_bq40z50.h"
#include "xy_sensor_device.h"

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

static uint8_t g_regs[256][4];
static uint8_t g_read_failures[256];
static uint32_t g_tick;
static unsigned g_i2c_config_calls;
static unsigned g_i2c_read_calls;
static unsigned g_failed_reads;
static xy_sensor_bus_t g_last_bus;

uint32_t xy_os_tick_get(void)
{
    return g_tick;
}

static void set16(uint8_t reg, uint16_t value)
{
    g_regs[reg][0] = (uint8_t)(value & 0xFFu);
    g_regs[reg][1] = (uint8_t)(value >> 8);
}

static void set32(uint8_t reg, uint32_t value)
{
    g_regs[reg][0] = (uint8_t)(value & 0xFFu);
    g_regs[reg][1] = (uint8_t)(value >> 8);
    g_regs[reg][2] = (uint8_t)(value >> 16);
    g_regs[reg][3] = (uint8_t)(value >> 24);
}

static void fail_reads(uint8_t reg, uint8_t failures)
{
    g_read_failures[reg] = failures;
}

void xy_sensor_bus_config_i2c(xy_sensor_bus_t *bus, void *handle, uint8_t address)
{
    TEST_ASSERT_NOT_NULL(bus);
    bus->type = XY_SENSOR_BUS_I2C;
    bus->bus_handle = handle;
    bus->address = address;
    bus->chip_select = 0;
    g_last_bus = *bus;
    g_i2c_config_calls++;
}

void xy_sensor_bus_config_spi(xy_sensor_bus_t *bus, void *handle, uint8_t chip_select)
{
    (void)bus;
    (void)handle;
    (void)chip_select;
    TEST_FAIL_MESSAGE("Fuel Gauge SMBus smoke must not configure SPI");
}

int xy_sensor_i2c_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);
    TEST_ASSERT_NOT_NULL(data);
    if (len != 2U && len != 4U) {
        TEST_FAIL_MESSAGE("Fuel Gauge SMBus smoke only models 2-byte/4-byte register reads");
    }

    g_i2c_read_calls++;
    if (g_read_failures[reg] > 0U) {
        g_read_failures[reg]--;
        g_failed_reads++;
        return -1;
    }

    memcpy(data, g_regs[reg], len);
    return 0;
}

int xy_sensor_i2c_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int xy_sensor_i2c_read_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *value)
{
    (void)bus;
    (void)reg;
    (void)value;
    return -1;
}

int xy_sensor_i2c_write_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t value)
{
    (void)bus;
    (void)reg;
    (void)value;
    return -1;
}

int xy_sensor_i2c_read_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret = xy_sensor_i2c_read(bus, reg, buf, sizeof(buf));
    if (ret == 0) {
        *value = ((uint16_t)buf[1] << 8) | buf[0];
    }
    return ret;
}

int xy_sensor_i2c_write_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t value)
{
    (void)bus;
    (void)reg;
    (void)value;
    return -1;
}

int xy_sensor_spi_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int xy_sensor_spi_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int xy_sensor_check_device_id(xy_sensor_bus_t *bus, uint8_t whoami_reg, uint8_t expected_id)
{
    (void)bus;
    (void)whoami_reg;
    (void)expected_id;
    return -1;
}

void setUp(void)
{
    memset(g_regs, 0, sizeof(g_regs));
    memset(g_read_failures, 0, sizeof(g_read_failures));
    memset(&g_last_bus, 0, sizeof(g_last_bus));
    g_tick = 1000U;
    g_i2c_config_calls = 0U;
    g_i2c_read_calls = 0U;
    g_failed_reads = 0U;

    set16(REG_CTRL, 0x4050);
    set16(REG_TEMP, 3001);
    set16(REG_VOLT, 15234);
    set16(REG_CURR, (uint16_t)(int16_t)-654);
    set16(REG_AVG_CURR, (uint16_t)(int16_t)-543);
    set16(REG_CYCLE_CNT, 222);
    set16(REG_SOC, 7550);
    set16(REG_REM_CAP, 5100);
    set16(REG_FULL_CAP, 6800);
    set16(REG_CELL1_VOLT, 3810);
    set16(REG_CELL2_VOLT, 3808);
    set16(REG_CELL3_VOLT, 3812);
    set16(REG_CELL4_VOLT, 3804);
    set16(REG_BAT_STATUS, 0x0009);
    set32(REG_PROT_STATUS, 0x00000051);
    set16(REG_BAL_STATUS, 0x0005);
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *register_and_init_smoke_device(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ40Z50");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_bq40z50_register((void *)0x4050, BQ40Z50_ADDR));
        fg = xy_fuel_gauge_device_get("BQ40Z50");
    }
    TEST_ASSERT_NOT_NULL(fg);
    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    return fg;
}

static void test_smoke_documents_bq40z50_smbus_init_fetch_flow(void)
{
    xy_fuel_gauge_t *fg = register_and_init_smoke_device();
    uint16_t voltage = 0U;
    int16_t current = 0;
    uint8_t soc = 0U;

    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_config_calls);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, g_last_bus.type);
    TEST_ASSERT_EQUAL_UINT8(BQ40Z50_ADDR, g_last_bus.address);

    g_tick = 5050U;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(5050U, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234U, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_current(fg, &current));
    TEST_ASSERT_EQUAL_INT16(-654, current);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_soc(fg, &soc));
    TEST_ASSERT_EQUAL_UINT8(75U, soc);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT(15U, g_i2c_read_calls);
}

static void test_smoke_keeps_snapshot_and_outputs_on_transient_smbus_failure(void)
{
    xy_fuel_gauge_t *fg = register_and_init_smoke_device();
    uint16_t voltage = 0xAAAAU;
    uint32_t last_timestamp;

    g_tick = 5050U;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    last_timestamp = fg->latest.timestamp;

    fail_reads(REG_VOLT, 3U);
    g_tick = 6060U;
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT(3U, g_failed_reads);
    TEST_ASSERT_EQUAL_UINT32(last_timestamp, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_get_battery_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234U, voltage);

    voltage = 0xAAAAU;
    fail_reads(REG_VOLT, 3U);
    TEST_ASSERT_EQUAL(XY_FG_ERROR, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, voltage);
    TEST_ASSERT_EQUAL_UINT32(last_timestamp, fg->latest.timestamp);
}

static void test_smoke_distinguishes_cached_and_direct_balance_status_paths(void)
{
    xy_fuel_gauge_t *fg = register_and_init_smoke_device();
    uint8_t balance_status = 0xAAU;
    uint32_t read_calls_after_fetch;

    g_tick = 5050U;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    read_calls_after_fetch = g_i2c_read_calls;
    TEST_ASSERT_EQUAL_UINT8(0x05U, xy_fuel_gauge_bq40z50_get_balance_status(fg));

    set16(REG_BAL_STATUS, 0x000AU);
    TEST_ASSERT_EQUAL_UINT8(0x05U, xy_fuel_gauge_bq40z50_get_balance_status(fg));
    TEST_ASSERT_EQUAL_UINT32(read_calls_after_fetch, g_i2c_read_calls);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_bq40z50_read_balance_status(fg, &balance_status));
    TEST_ASSERT_EQUAL_UINT8(0x0AU, balance_status);
    TEST_ASSERT_EQUAL_UINT8(0x05U, xy_fuel_gauge_bq40z50_get_balance_status(fg));
    TEST_ASSERT_EQUAL_UINT32(read_calls_after_fetch + 1U, g_i2c_read_calls);

    balance_status = 0xCCU;
    fail_reads(REG_BAL_STATUS, 3U);
    TEST_ASSERT_EQUAL(XY_FG_ERROR,
                      xy_fuel_gauge_bq40z50_read_balance_status(fg, &balance_status));
    TEST_ASSERT_EQUAL_UINT8(0xCCU, balance_status);
    TEST_ASSERT_EQUAL_UINT8(0x05U, xy_fuel_gauge_bq40z50_get_balance_status(fg));
    TEST_ASSERT_EQUAL_UINT32(read_calls_after_fetch + 4U, g_i2c_read_calls);
}

static void test_smoke_record_template_must_stay_pending_without_real_board_logs(void)
{
    TEST_PASS_MESSAGE("Host fake-I2C smoke is contract coverage only; validation record stays pending until real SMBus board logs are captured.");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_smoke_documents_bq40z50_smbus_init_fetch_flow);
    RUN_TEST(test_smoke_keeps_snapshot_and_outputs_on_transient_smbus_failure);
    RUN_TEST(test_smoke_distinguishes_cached_and_direct_balance_status_paths);
    RUN_TEST(test_smoke_record_template_must_stay_pending_without_real_board_logs);
    return UNITY_END();
}
