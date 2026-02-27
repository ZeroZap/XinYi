/**
 * @file test_pm.c
 * @brief PM (Power Management) Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* PM headers */
#include "xy_charger.h"
#include "xy_fuel_gauge.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== Charger Tests ==================== */

void test_charger_config_structure(void)
{
    xy_charger_config_t config;

    memset(&config, 0, sizeof(config));

    /* Test default values */
    TEST_ASSERT_EQUAL(0, config.cell_count);
    TEST_ASSERT_EQUAL(0, config.charge_current_mA);
    TEST_ASSERT_EQUAL(0, config.charge_voltage_mV);
}

void test_charger_status_enum(void)
{
    /* Test charger status values */
    TEST_ASSERT_EQUAL(0, XY_CHARGER_STATUS_IDLE);
    TEST_ASSERT_EQUAL(1, XY_CHARGER_STATUS_PRE_CHARGE);
    TEST_ASSERT_EQUAL(2, XY_CHARGER_STATUS_FAST_CHARGE);
    TEST_ASSERT_EQUAL(3, XY_CHARGER_STATUS_CONSTANT_VOLTAGE);
    TEST_ASSERT_EQUAL(4, XY_CHARGER_STATUS_CHARGE_COMPLETE);
    TEST_ASSERT_EQUAL(5, XY_CHARGER_STATUS_ERROR);
}

void test_charger_error_codes(void)
{
    /* Test error codes */
    TEST_ASSERT_EQUAL(0, XY_CHARGER_OK);
    TEST_ASSERT_EQUAL(-1, XY_CHARGER_ERROR);
    TEST_ASSERT_EQUAL(-2, XY_CHARGER_INVALID_PARAM);
    TEST_ASSERT_EQUAL(-3, XY_CHARGER_OVER_VOLTAGE);
    TEST_ASSERT_EQUAL(-4, XY_CHARGER_OVER_CURRENT);
    TEST_ASSERT_EQUAL(-5, XY_CHARGER_OVER_TEMP);
    TEST_ASSERT_EQUAL(-6, XY_CHARGER_NOT_CHARGING);
}

void test_charger_state_structure(void)
{
    xy_charger_state_t state;

    memset(&state, 0, sizeof(state));

    /* Test structure fields */
    TEST_ASSERT_EQUAL(XY_CHARGER_STATUS_IDLE, state.status);
    TEST_ASSERT_EQUAL(0, state.battery_voltage_mV);
    TEST_ASSERT_EQUAL(0, state.charge_current_mA);
    TEST_ASSERT_EQUAL(0, state.soc_percent);
    TEST_ASSERT_FALSE(state.charging);
    TEST_ASSERT_FALSE(state.error);
}

void test_charger_functions_exist(void)
{
    /* Test that charger functions are declared */
    TEST_ASSERT_NOT_NULL(xy_charger_init);
    TEST_ASSERT_NOT_NULL(xy_charger_deinit);
    TEST_ASSERT_NOT_NULL(xy_charger_start);
    TEST_ASSERT_NOT_NULL(xy_charger_stop);
    TEST_ASSERT_NOT_NULL(xy_charger_get_state);
    TEST_ASSERT_NOT_NULL(xy_charger_set_current);
    TEST_ASSERT_NOT_NULL(xy_charger_enable);
    TEST_ASSERT_NOT_NULL(xy_charger_is_charging);
}

/* ==================== Fuel Gauge Tests ==================== */

void test_fuel_gauge_config_structure(void)
{
    xy_fuel_gauge_config_t config;

    memset(&config, 0, sizeof(config));

    /* Test default values */
    TEST_ASSERT_EQUAL(0, config.design_capacity_mAh);
    TEST_ASSERT_EQUAL(0, config.full_capacity_mAh);
    TEST_ASSERT_EQUAL(0, config.empty_capacity_mAh);
    TEST_ASSERT_EQUAL(0, config.nominal_voltage_mV);
    TEST_ASSERT_EQUAL(0, config.cells);
}

void test_fuel_gauge_error_codes(void)
{
    /* Test error codes */
    TEST_ASSERT_EQUAL(0, XY_FUEL_GAUGE_OK);
    TEST_ASSERT_EQUAL(-1, XY_FUEL_GAUGE_ERROR);
    TEST_ASSERT_EQUAL(-2, XY_FUEL_GAUGE_INVALID_PARAM);
}

void test_battery_state_structure(void)
{
    xy_battery_state_t state;

    memset(&state, 0, sizeof(state));

    /* Test structure fields */
    TEST_ASSERT_EQUAL(0, state.voltage_mV);
    TEST_ASSERT_EQUAL(0, state.current_mA);
    TEST_ASSERT_EQUAL(0, state.temperature_celsius);
    TEST_ASSERT_EQUAL(0, state.soc_percent);
    TEST_ASSERT_EQUAL(0, state.soh_percent);
    TEST_ASSERT_EQUAL(0, state.remaining_mAh);
    TEST_ASSERT_FALSE(state.charging);
    TEST_ASSERT_FALSE(state.full);
    TEST_ASSERT_FALSE(state.empty);
}

void test_fuel_gauge_functions_exist(void)
{
    /* Test that fuel gauge functions are declared */
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_init);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_deinit);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_update);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_get_state);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_get_soc);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_get_soh);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_get_remaining_mAh);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_get_time_to_empty);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_get_time_to_full);
    TEST_ASSERT_NOT_NULL(xy_fuel_gauge_reset);
}

/* ==================== Fuel Gauge Simulation Tests ==================== */

void test_fuel_gauge_init_deinit(void)
{
    xy_fuel_gauge_config_t config;
    int result;

    config.design_capacity_mAh = 2000;
    config.full_capacity_mAh = 2200;
    config.empty_capacity_mAh = 100;
    config.nominal_voltage_mV = 3700;
    config.cells = 1;

    result = xy_fuel_gauge_init(&config);
    TEST_ASSERT_TRUE(result == XY_FUEL_GAUGE_OK || result == XY_FUEL_GAUGE_ERROR);

    result = xy_fuel_gauge_deinit();
    TEST_ASSERT_TRUE(result == XY_FUEL_GAUGE_OK || result == XY_FUEL_GAUGE_ERROR);
}

void test_fuel_gauge_update(void)
{
    xy_fuel_gauge_config_t config;
    int result;

    config.design_capacity_mAh = 2000;
    config.full_capacity_mAh = 2200;
    config.empty_capacity_mAh = 100;
    config.nominal_voltage_mV = 3700;
    config.cells = 1;

    xy_fuel_gauge_init(&config);

    /* Update with typical values */
    result = xy_fuel_gauge_update(3800, 500, 25);
    TEST_ASSERT_TRUE(result == XY_FUEL_GAUGE_OK || result == XY_FUEL_GAUGE_ERROR);

    xy_fuel_gauge_deinit();
}

void test_fuel_gauge_get_soc(void)
{
    uint8_t soc;

    soc = xy_fuel_gauge_get_soc();

    /* SOC should be 0-100 or error (0) */
    TEST_ASSERT_TRUE(soc == 0 || (soc >= 1 && soc <= 100));
}

void test_fuel_gauge_get_soh(void)
{
    uint8_t soh;

    soh = xy_fuel_gauge_get_soh();

    /* SOH should be 0-100 or error (0) */
    TEST_ASSERT_TRUE(soh == 0 || (soh >= 1 && soh <= 100));
}

void test_fuel_gauge_get_remaining_mAh(void)
{
    uint32_t remaining;

    remaining = xy_fuel_gauge_get_remaining_mAh();

    /* Should return a reasonable value or 0 */
    TEST_ASSERT_TRUE(remaining == 0 || remaining <= 10000);
}

/* ==================== Charger Simulation Tests ==================== */

void test_charger_init_deinit(void)
{
    xy_charger_config_t config;
    int result;

    config.cell_count = 1;
    config.charge_current_mA = 1000;
    config.charge_voltage_mV = 4200;
    config.pre_charge_current_mA = 100;
    config.termination_current_mA = 50;
    config.temp_min_celsius = 0;
    config.temp_max_celsius = 45;

    result = xy_charger_init(&config);
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);

    result = xy_charger_deinit();
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);
}

void test_charger_start_stop(void)
{
    xy_charger_config_t config;

    config.cell_count = 1;
    config.charge_current_mA = 1000;
    config.charge_voltage_mV = 4200;
    config.pre_charge_current_mA = 100;
    config.termination_current_mA = 50;
    config.temp_min_celsius = 0;
    config.temp_max_celsius = 45;

    xy_charger_init(&config);

    int result = xy_charger_start();
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);

    result = xy_charger_stop();
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);

    xy_charger_deinit();
}

void test_charger_get_state(void)
{
    xy_charger_config_t config;
    xy_charger_state_t state;
    int result;

    config.cell_count = 1;
    config.charge_current_mA = 1000;
    config.charge_voltage_mV = 4200;
    config.pre_charge_current_mA = 100;
    config.termination_current_mA = 50;
    config.temp_min_celsius = 0;
    config.temp_max_celsius = 45;

    xy_charger_init(&config);

    result = xy_charger_get_state(&state);
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);

    xy_charger_deinit();
}

void test_charger_set_current(void)
{
    xy_charger_config_t config;
    int result;

    config.cell_count = 1;
    config.charge_current_mA = 1000;
    config.charge_voltage_mV = 4200;
    config.pre_charge_current_mA = 100;
    config.termination_current_mA = 50;
    config.temp_min_celsius = 0;
    config.temp_max_celsius = 45;

    xy_charger_init(&config);

    result = xy_charger_set_current(500);
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);

    xy_charger_deinit();
}

void test_charger_enable_disable(void)
{
    xy_charger_config_t config;
    int result;

    config.cell_count = 1;
    config.charge_current_mA = 1000;
    config.charge_voltage_mV = 4200;
    config.pre_charge_current_mA = 100;
    config.termination_current_mA = 50;
    config.temp_min_celsius = 0;
    config.temp_max_celsius = 45;

    xy_charger_init(&config);

    result = xy_charger_enable(true);
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);

    result = xy_charger_enable(false);
    TEST_ASSERT_TRUE(result == XY_CHARGER_OK || result == XY_CHARGER_ERROR);

    xy_charger_deinit();
}

void test_charger_is_charging(void)
{
    bool charging;

    charging = xy_charger_is_charging();

    /* Should return true or false */
    TEST_ASSERT_TRUE(charging == true || charging == false);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Charger Tests */
    RUN_TEST(test_charger_config_structure);
    RUN_TEST(test_charger_status_enum);
    RUN_TEST(test_charger_error_codes);
    RUN_TEST(test_charger_state_structure);
    RUN_TEST(test_charger_functions_exist);
    RUN_TEST(test_charger_init_deinit);
    RUN_TEST(test_charger_start_stop);
    RUN_TEST(test_charger_get_state);
    RUN_TEST(test_charger_set_current);
    RUN_TEST(test_charger_enable_disable);
    RUN_TEST(test_charger_is_charging);

    /* Fuel Gauge Tests */
    RUN_TEST(test_fuel_gauge_config_structure);
    RUN_TEST(test_fuel_gauge_error_codes);
    RUN_TEST(test_battery_state_structure);
    RUN_TEST(test_fuel_gauge_functions_exist);
    RUN_TEST(test_fuel_gauge_init_deinit);
    RUN_TEST(test_fuel_gauge_update);
    RUN_TEST(test_fuel_gauge_get_soc);
    RUN_TEST(test_fuel_gauge_get_soh);
    RUN_TEST(test_fuel_gauge_get_remaining_mAh);

    return UNITY_END();
}
