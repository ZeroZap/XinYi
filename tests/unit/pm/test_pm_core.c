#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_pm.h"

int xy_pm_deinit(void);
int xy_pm_update(void);
int xy_pm_enter_sleep(void);
int xy_pm_enter_shutdown(void);
int xy_pm_wakeup(void);
int xy_charger_deinit(void);
int xy_charger_set_current(uint32_t current_mA);
int xy_charger_enable(bool enable);
int xy_charger_get_state(xy_charger_state_t *state);
int xy_fuel_gauge_get_state(xy_battery_state_t *state);
uint8_t xy_fuel_gauge_get_soh(void);
uint8_t xy_pm_estimate_soc_from_voltage(uint32_t voltage_mV);
void xy_pm_platform_set_fallback_tick(uint32_t tick);
int xy_pm_platform_get_charger_enable_level(void);

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_pm_platform_contracts(void)
{
    TEST_ASSERT_EQUAL_STRING("PC", xy_pm_get_platform_name());
    TEST_ASSERT_TRUE(xy_pm_is_platform(XY_PLATFORM_ID_PC));
    TEST_ASSERT_FALSE(xy_pm_is_platform(XY_PLATFORM_ID_STM32));
    TEST_ASSERT_FALSE(xy_pm_is_platform(XY_PLATFORM_ID_UNKNOWN));
    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_charger_hw_init());
    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_charger_hw_enable(1));
    TEST_ASSERT_EQUAL_INT(1, xy_pm_platform_get_charger_enable_level());
    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_charger_hw_disable());
    TEST_ASSERT_EQUAL_INT(0, xy_pm_platform_get_charger_enable_level());
}

static void test_pm_lifecycle_and_charging(void)
{
    xy_pm_system_state_info_t state;

    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_deinit());
    TEST_ASSERT_EQUAL_INT(XY_PM_INVALID_PARAM, xy_pm_get_state(NULL));
    memset(&state, 0xA5, sizeof(state));
    TEST_ASSERT_EQUAL_INT(XY_PM_NOT_INITIALIZED, xy_pm_get_state(&state));
    TEST_ASSERT_EQUAL_INT(XY_PM_SYSTEM_STATE_INIT, state.state);

    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_init());
    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_init());
    TEST_ASSERT_EQUAL_UINT(3700U, xy_pm_get_battery_voltage_mV());
    TEST_ASSERT_EQUAL_UINT(3700U, xy_pm_get_battery_voltage());
    TEST_ASSERT_EQUAL_UINT(40U, xy_pm_get_battery_percent());
    TEST_ASSERT_EQUAL_UINT(50U, xy_pm_get_soc());

    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_get_state(&state));
    if (state.system_voltage_mV == 0U) {
        TEST_ASSERT_EQUAL_UINT(0U, state.system_voltage_mV);
    } else {
        TEST_ASSERT_EQUAL_UINT(3700U, state.system_voltage_mV);
    }

    TEST_ASSERT_FALSE(xy_pm_is_charging());
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_pm_start_charging());
    TEST_ASSERT_TRUE(xy_pm_is_charging());
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_pm_stop_charging());
    TEST_ASSERT_FALSE(xy_pm_is_charging());

    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_enter_sleep());
    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_wakeup());
    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_enter_shutdown());
    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_deinit());
    TEST_ASSERT_EQUAL_UINT(0U, xy_pm_get_soc());
    TEST_ASSERT_EQUAL_UINT(0U, xy_pm_get_battery_voltage());
}

static void test_charger_contracts(void)
{
    xy_charger_config_t cfg = {
        .cell_count = 0,
        .charge_current_mA = 0,
        .charge_voltage_mV = 0,
        .pre_charge_current_mA = 120,
        .termination_current_mA = 80,
        .temp_min_celsius = 0,
        .temp_max_celsius = 45,
    };
    xy_charger_state_t state;

    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_charger_deinit());
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_NOT_CHARGING, xy_charger_start());
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_INVALID_PARAM, xy_charger_get_state(NULL));
    memset(&state, 0xA5, sizeof(state));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_NOT_CHARGING, xy_charger_get_state(&state));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_STATUS_IDLE, state.status);

    TEST_ASSERT_EQUAL_INT(XY_CHARGER_INVALID_PARAM, xy_charger_init(NULL));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_charger_init(&cfg));
    TEST_ASSERT_FALSE(xy_charger_is_charging());
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_charger_set_current(750));
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_charger_enable(true));
    TEST_ASSERT_TRUE(xy_charger_is_charging());
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_charger_get_state(&state));
    TEST_ASSERT_TRUE(state.charging);
    if (state.status == XY_CHARGER_STATUS_FAST_CHARGE) {
        TEST_ASSERT_EQUAL_INT(XY_CHARGER_STATUS_FAST_CHARGE, state.status);
    } else if (state.status == XY_CHARGER_STATUS_PRE_CHARGE) {
        TEST_ASSERT_EQUAL_INT(XY_CHARGER_STATUS_PRE_CHARGE, state.status);
    } else {
        TEST_ASSERT_EQUAL_INT(XY_CHARGER_STATUS_CONSTANT_VOLTAGE, state.status);
    }
    TEST_ASSERT_EQUAL_UINT(3700U, state.battery_voltage_mV);
    TEST_ASSERT_EQUAL_UINT(40U, state.soc_percent);

    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_charger_enable(false));
    TEST_ASSERT_FALSE(xy_charger_is_charging());
    TEST_ASSERT_EQUAL_INT(XY_CHARGER_OK, xy_charger_deinit());
}

static void test_fuel_gauge_and_adc_contracts(void)
{
    xy_fuel_gauge_config_t cfg = {
        .design_capacity_mAh = 2000,
        .full_capacity_mAh = 2000,
        .nominal_voltage_mV = 3700,
        .cells = 1,
    };
    xy_battery_state_t state;

    TEST_ASSERT_EQUAL_INT(XY_PM_OK, xy_pm_adc_init());
    TEST_ASSERT_EQUAL_UINT(3700U, xy_pm_get_battery_voltage_mV());
    TEST_ASSERT_EQUAL_UINT(100U, xy_pm_estimate_soc_from_voltage(4200));
    TEST_ASSERT_EQUAL_UINT(40U, xy_pm_estimate_soc_from_voltage(3700));
    TEST_ASSERT_EQUAL_UINT(0U, xy_pm_estimate_soc_from_voltage(3400));

    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_deinit());
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_INVALID_PARAM, xy_fuel_gauge_init(NULL));
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_ERROR, xy_fuel_gauge_update(3700, 0, 25));
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_init(&cfg));
    TEST_ASSERT_EQUAL_UINT(50U, xy_fuel_gauge_get_soc());
    TEST_ASSERT_EQUAL_UINT(100U, xy_fuel_gauge_get_soh());
    TEST_ASSERT_EQUAL_UINT(1000U, xy_fuel_gauge_get_remaining_mAh());
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_update(4200, 500, 25));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(60U, xy_fuel_gauge_get_soc());
    TEST_ASSERT_LESS_OR_EQUAL_UINT(70U, xy_fuel_gauge_get_soc());
    TEST_ASSERT_GREATER_THAN_UINT(0U, xy_fuel_gauge_get_time_to_full());
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_get_state(&state));
    TEST_ASSERT_EQUAL_UINT(3700U, state.voltage_mV);
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_reset());
    TEST_ASSERT_EQUAL_UINT(100U, xy_fuel_gauge_get_soc());
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_deinit());
}

static void test_fuel_gauge_uses_platform_tick(void)
{
    xy_fuel_gauge_config_t cfg = {
        .design_capacity_mAh = 1000,
        .full_capacity_mAh = 1000,
        .nominal_voltage_mV = 3700,
        .cells = 1,
    };

    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_deinit());
    xy_pm_platform_set_fallback_tick(1000U);
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_init(&cfg));
    TEST_ASSERT_EQUAL_UINT(500U, xy_fuel_gauge_get_remaining_mAh());

    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_update(3700, 3600, 25));
    TEST_ASSERT_EQUAL_UINT(500U, xy_fuel_gauge_get_remaining_mAh());

    xy_pm_platform_set_fallback_tick(2000U);
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_update(3700, 3600, 25));
    TEST_ASSERT_EQUAL_UINT(501U, xy_fuel_gauge_get_remaining_mAh());
    TEST_ASSERT_EQUAL_INT(XY_FUEL_GAUGE_OK, xy_fuel_gauge_deinit());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pm_platform_contracts);
    RUN_TEST(test_pm_lifecycle_and_charging);
    RUN_TEST(test_charger_contracts);
    RUN_TEST(test_fuel_gauge_and_adc_contracts);
    RUN_TEST(test_fuel_gauge_uses_platform_tick);
    return UNITY_END();
}
