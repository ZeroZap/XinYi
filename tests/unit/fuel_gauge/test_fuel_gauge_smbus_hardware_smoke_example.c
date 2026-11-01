/**
 * @file test_fuel_gauge_smbus_hardware_smoke_example.c
 * @brief Build-guarded smoke skeleton for Fuel Gauge SMBus board validation.
 *
 * This host-only smoke keeps the intended board validation flow compiled and
 * documented without claiming real SMBus hardware evidence from fake I2C data.
 */

#include "unity.h"
#include "xy_fg_bq27z561.h"
#include "xy_fg_bq40z50.h"
#include "xy_device.h"

#include <stdint.h>
#include <string.h>

#define BQ27Z561_REG_NOM_CAP  0x12
#define BQ27Z561_REG_REM_CAP  0x14
#define BQ27Z561_REG_SOC      0x2C
#define BQ27Z561_REG_CURR     0x58
#define BQ27Z561_REG_AVG_CURR 0x5A

#define REG_CTRL        0x00
#define REG_DEVICE_ID   0x02
#define REG_TEMP        0x06
#define REG_VOLT        0x08
#define REG_CURR        0x0A
#define REG_AVG_CURR    0x0B
#define REG_CYCLE_CNT   0x2A
#define BQ40Z50_REG_SOC 0x2C
#define REG_REM_CAP     0x2E
#define REG_FULL_CAP    0x30
#define REG_CELL1_VOLT  0x3C
#define REG_CELL2_VOLT  0x3E
#define REG_CELL3_VOLT  0x40
#define REG_CELL4_VOLT  0x42
#define REG_BAT_STATUS  0x64
#define REG_PROT_STATUS 0x66
#define REG_BAL_STATUS  0x68
#define REG_SOH         0x7A

static uint8_t g_regs[256][4];
static uint8_t g_read_failures[256];
static uint32_t g_tick;
static unsigned g_i2c_config_calls;
static unsigned g_i2c_read_calls;
static unsigned g_failed_reads;
static xy_i2c_device_t g_last_bus;

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

/* The drivers reach the bus through the canonical Device helpers, so this fixture supplies those
 * and nothing else. There is deliberately no SPI entry point defined: if a gauge driver ever starts
 * configuring SPI on this bus, the build fails at link time instead of a stub silently succeeding. */
xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr,
                             uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    memset(dev, 0, sizeof(*dev));
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout ? timeout : 1000U;
    dev->base.initialized = true;
    g_last_bus = *dev;
    g_i2c_config_calls++;
    return XY_DEVICE_OK;
}

static xy_error_t fake_repeated_start_read(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                           size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_NOT_NULL(data);
    /* One byte (SOH), a SMBus word, or a four-byte block: the three widths these gauges read. */
    if (len != 1U && len != 2U && len != 4U) {
        TEST_FAIL_MESSAGE("Fuel Gauge SMBus smoke only models 1-byte/2-byte/4-byte register reads");
    }

    g_i2c_read_calls++;
    if (g_read_failures[reg] > 0U) {
        g_read_failures[reg]--;
        g_failed_reads++;
        return XY_DEVICE_IO_ERROR;
    }

    memcpy(data, g_regs[reg], len);
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg_repeated_start(xy_i2c_device_t *dev, uint8_t reg,
                                                 uint8_t *data, size_t len)
{
    return fake_repeated_start_read(dev, reg, data, len);
}

xy_error_t xy_i2c_device_read_reg16_repeated_start(xy_i2c_device_t *dev, uint8_t reg,
                                                   uint16_t *value)
{
    uint8_t buf[2];

    TEST_ASSERT_NOT_NULL(value);
    if (fake_repeated_start_read(dev, reg, buf, sizeof(buf)) != XY_DEVICE_OK) {
        return XY_DEVICE_IO_ERROR;
    }

    /* SMBus word order, the same convention the canonical helper applies. */
    *value = (uint16_t)(((uint16_t)buf[1] << 8) | buf[0]);
    return XY_DEVICE_OK;
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
    set16(REG_DEVICE_ID, 0x0561);
    set16(REG_TEMP, 3001);
    set16(REG_VOLT, 15234);
    set16(REG_CURR, (uint16_t)(int16_t)-654);
    set16(REG_AVG_CURR, (uint16_t)(int16_t)-543);
    set16(BQ27Z561_REG_CURR, (uint16_t)(int16_t)-654);
    set16(BQ27Z561_REG_AVG_CURR, (uint16_t)(int16_t)-543);
    set16(REG_CYCLE_CNT, 222);
    set16(BQ40Z50_REG_SOC, 7550);
    g_regs[REG_SOH][0] = 94U;
    set16(BQ27Z561_REG_NOM_CAP, 6800);
    set16(BQ27Z561_REG_REM_CAP, 5100);
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

static xy_fuel_gauge_t *register_and_init_single_cell_smoke_device(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ27z561");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_bq27z561_register((void *)0x27561, BQ27Z561_ADDR));
        fg = xy_fuel_gauge_device_get("BQ27z561");
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
    TEST_ASSERT_TRUE(g_last_bus.base.initialized);
    TEST_ASSERT_EQUAL_UINT8(BQ40Z50_ADDR, g_last_bus.dev_addr);

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

static void test_smoke_documents_bq27z561_single_cell_fetch_flow(void)
{
    xy_fuel_gauge_t *fg = register_and_init_single_cell_smoke_device();
    uint16_t voltage = 0U;
    int16_t current = 0;
    uint8_t soc = 0U;

    TEST_ASSERT_TRUE(g_last_bus.base.initialized);
    TEST_ASSERT_EQUAL_UINT8(BQ27Z561_ADDR, g_last_bus.dev_addr);

    g_regs[BQ27Z561_REG_SOC][0] = 75U;
    g_tick = 5610U;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(5610U, fg->latest.timestamp);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_voltage(fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(15234U, voltage);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_current(fg, &current));
    TEST_ASSERT_EQUAL_INT16(-654, current);
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_soc(fg, &soc));
    TEST_ASSERT_EQUAL_UINT8(75U, soc);
    TEST_ASSERT_EQUAL_UINT8(94U, fg->latest.soh);
    TEST_ASSERT_EQUAL_UINT16(6800U, fg->latest.full_capacity_mah);
    TEST_ASSERT_EQUAL_UINT16(5100U, fg->latest.remain_capacity_mah);
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
    RUN_TEST(test_smoke_documents_bq27z561_single_cell_fetch_flow);
    RUN_TEST(test_smoke_distinguishes_cached_and_direct_balance_status_paths);
    RUN_TEST(test_smoke_record_template_must_stay_pending_without_real_board_logs);
    return UNITY_END();
}
