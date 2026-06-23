#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

static void test_pm_platform_contracts(void)
{
    assert(strcmp(xy_pm_get_platform_name(), "PC") == 0);
    assert(xy_pm_is_platform(XY_PLATFORM_ID_PC));
    assert(!xy_pm_is_platform(XY_PLATFORM_ID_STM32));
    assert(!xy_pm_is_platform(XY_PLATFORM_ID_UNKNOWN));
    assert(xy_charger_hw_init() == XY_PM_OK);
    assert(xy_charger_hw_enable(1) == XY_PM_OK);
    assert(xy_charger_hw_disable() == XY_PM_OK);
}

static void test_pm_lifecycle_and_charging(void)
{
    xy_pm_system_state_info_t state;

    assert(xy_pm_deinit() == XY_PM_OK);
    assert(xy_pm_get_state(NULL) == XY_PM_INVALID_PARAM);
    memset(&state, 0xA5, sizeof(state));
    assert(xy_pm_get_state(&state) == XY_PM_NOT_INITIALIZED);
    assert(state.state == XY_PM_SYSTEM_STATE_INIT);

    assert(xy_pm_init() == XY_PM_OK);
    assert(xy_pm_init() == XY_PM_OK);
    assert(xy_pm_get_battery_voltage_mV() == 3700U);
    assert(xy_pm_get_battery_voltage() == 3700U);
    assert(xy_pm_get_battery_percent() == 40U);
    assert(xy_pm_get_soc() == 50U);

    assert(xy_pm_get_state(&state) == XY_PM_OK);
    assert(state.system_voltage_mV == 0U || state.system_voltage_mV == 3700U);
    assert(state.power_good == false || state.power_good == true);

    assert(!xy_pm_is_charging());
    assert(xy_pm_start_charging() == XY_CHARGER_OK);
    assert(xy_pm_is_charging());
    assert(xy_pm_stop_charging() == XY_CHARGER_OK);
    assert(!xy_pm_is_charging());

    assert(xy_pm_enter_sleep() == XY_PM_OK);
    assert(xy_pm_wakeup() == XY_PM_OK);
    assert(xy_pm_enter_shutdown() == XY_PM_OK);
    assert(xy_pm_deinit() == XY_PM_OK);
    assert(xy_pm_get_soc() == 0U);
    assert(xy_pm_get_battery_voltage() == 0U);
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

    assert(xy_charger_deinit() == XY_CHARGER_OK);
    assert(xy_charger_start() == XY_CHARGER_NOT_CHARGING);
    assert(xy_charger_get_state(NULL) == XY_CHARGER_INVALID_PARAM);
    memset(&state, 0xA5, sizeof(state));
    assert(xy_charger_get_state(&state) == XY_CHARGER_NOT_CHARGING);
    assert(state.status == XY_CHARGER_STATUS_IDLE);

    assert(xy_charger_init(NULL) == XY_CHARGER_INVALID_PARAM);
    assert(xy_charger_init(&cfg) == XY_CHARGER_OK);
    assert(!xy_charger_is_charging());
    assert(xy_charger_set_current(750) == XY_CHARGER_OK);
    assert(xy_charger_enable(true) == XY_CHARGER_OK);
    assert(xy_charger_is_charging());
    assert(xy_charger_get_state(&state) == XY_CHARGER_OK);
    assert(state.charging);
    assert(state.status == XY_CHARGER_STATUS_FAST_CHARGE ||
           state.status == XY_CHARGER_STATUS_PRE_CHARGE ||
           state.status == XY_CHARGER_STATUS_CONSTANT_VOLTAGE);
    assert(state.battery_voltage_mV == 3700U);
    assert(state.soc_percent == 40U);

    assert(xy_charger_enable(false) == XY_CHARGER_OK);
    assert(!xy_charger_is_charging());
    assert(xy_charger_deinit() == XY_CHARGER_OK);
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

    assert(xy_pm_adc_init() == XY_PM_OK);
    assert(xy_pm_get_battery_voltage_mV() == 3700U);
    assert(xy_pm_estimate_soc_from_voltage(4200) == 100U);
    assert(xy_pm_estimate_soc_from_voltage(3700) == 40U);
    assert(xy_pm_estimate_soc_from_voltage(3400) == 0U);

    assert(xy_fuel_gauge_deinit() == XY_FUEL_GAUGE_OK);
    assert(xy_fuel_gauge_init(NULL) == XY_FUEL_GAUGE_INVALID_PARAM);
    assert(xy_fuel_gauge_update(3700, 0, 25) == XY_FUEL_GAUGE_ERROR);
    assert(xy_fuel_gauge_init(&cfg) == XY_FUEL_GAUGE_OK);
    assert(xy_fuel_gauge_get_soc() == 50U);
    assert(xy_fuel_gauge_get_soh() == 100U);
    assert(xy_fuel_gauge_get_remaining_mAh() == 1000U);
    assert(xy_fuel_gauge_update(4200, 500, 25) == XY_FUEL_GAUGE_OK);
    assert(xy_fuel_gauge_get_soc() >= 60U);
    assert(xy_fuel_gauge_get_soc() <= 70U);
    assert(xy_fuel_gauge_get_time_to_full() > 0U);
    assert(xy_fuel_gauge_get_state(&state) == XY_FUEL_GAUGE_OK);
    assert(state.voltage_mV == 3700U);
    assert(xy_fuel_gauge_reset() == XY_FUEL_GAUGE_OK);
    assert(xy_fuel_gauge_get_soc() == 100U);
    assert(xy_fuel_gauge_deinit() == XY_FUEL_GAUGE_OK);
}

static void test_fuel_gauge_uses_platform_tick(void)
{
    xy_fuel_gauge_config_t cfg = {
        .design_capacity_mAh = 1000,
        .full_capacity_mAh = 1000,
        .nominal_voltage_mV = 3700,
        .cells = 1,
    };

    assert(xy_fuel_gauge_deinit() == XY_FUEL_GAUGE_OK);
    xy_pm_platform_set_fallback_tick(1000U);
    assert(xy_fuel_gauge_init(&cfg) == XY_FUEL_GAUGE_OK);
    assert(xy_fuel_gauge_get_remaining_mAh() == 500U);

    assert(xy_fuel_gauge_update(3700, 3600, 25) == XY_FUEL_GAUGE_OK);
    assert(xy_fuel_gauge_get_remaining_mAh() == 500U);

    xy_pm_platform_set_fallback_tick(2000U);
    assert(xy_fuel_gauge_update(3700, 3600, 25) == XY_FUEL_GAUGE_OK);
    assert(xy_fuel_gauge_get_remaining_mAh() == 501U);
    assert(xy_fuel_gauge_deinit() == XY_FUEL_GAUGE_OK);
}

int main(void)
{
    test_pm_platform_contracts();
    test_pm_lifecycle_and_charging();
    test_charger_contracts();
    test_fuel_gauge_and_adc_contracts();
    test_fuel_gauge_uses_platform_tick();
    puts("PM component tests passed");
    return 0;
}
