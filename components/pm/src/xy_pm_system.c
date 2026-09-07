/**
 * @file xy_pm_system.c
 * @brief Power Management System Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_pm.h"
#include <string.h>

/* OS tick is provided by platform layer via xy_pm_tick_get() */
#ifndef xy_os_tick_get
#define xy_os_tick_get xy_pm_tick_get
#endif

/* Stub logging macros */
#define xy_log_i(fmt, ...)
#define xy_log_w(fmt, ...)
#define xy_log_d(fmt, ...)

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* PM 系统控制块 */
typedef struct {
    bool initialized;
    xy_pm_system_state_info_t state;
    xy_charger_config_t charger_config;
    xy_fuel_gauge_config_t fg_config;
    uint32_t last_update_time;
    uint32_t update_interval_ms;
} xy_pm_ctrl_t;

static xy_pm_ctrl_t s_pm;

/* 默认配置 */
#define PM_UPDATE_INTERVAL_MS  1000  /* 1 秒更新一次 */

int xy_pm_init(void)
{
    if (s_pm.initialized) {
        xy_log_w("PM already initialized\n");
        return XY_PM_OK;
    }

    memset(&s_pm, 0, sizeof(s_pm));

    /* 默认充电器配置 */
    s_pm.charger_config.cell_count = 1;
    s_pm.charger_config.charge_current_mA = XY_PM_DEFAULT_CHARGE_CURRENT_MAH;
    s_pm.charger_config.charge_voltage_mV = 4200;
    s_pm.charger_config.pre_charge_current_mA = 100;
    s_pm.charger_config.termination_current_mA = 100;

    /* 默认电量计配置 */
    s_pm.fg_config.design_capacity_mAh = XY_PM_DEFAULT_BATTERY_CAPACITY_MAH;
    s_pm.fg_config.full_capacity_mAh = XY_PM_DEFAULT_BATTERY_CAPACITY_MAH;
    s_pm.fg_config.nominal_voltage_mV = 3700;
    s_pm.fg_config.cells = 1;

    /* 初始化 ADC */
    xy_pm_adc_init();

    /* 初始化充电器 */
    xy_charger_init(&s_pm.charger_config);

    /* 初始化电量计 */
    xy_fuel_gauge_init(&s_pm.fg_config);

    s_pm.update_interval_ms = PM_UPDATE_INTERVAL_MS;
    s_pm.last_update_time = xy_os_tick_get();
    s_pm.initialized = true;

    xy_log_i("PM System initialized: capacity=%dmAh, charge_current=%dmA\n",
             XY_PM_DEFAULT_BATTERY_CAPACITY_MAH, XY_PM_DEFAULT_CHARGE_CURRENT_MAH);

    return XY_PM_OK;
}

int xy_pm_deinit(void)
{
    if (!s_pm.initialized) return XY_PM_OK;

    xy_charger_stop();
    xy_charger_deinit();
    xy_fuel_gauge_deinit();

    memset(&s_pm, 0, sizeof(s_pm));
    xy_log_i("PM System deinitialized\n");
    return XY_PM_OK;
}

int xy_pm_update(void)
{
    if (!s_pm.initialized) return XY_PM_NOT_INITIALIZED;

    uint32_t now = xy_os_tick_get();

    /* 按间隔更新 */
    if (now - s_pm.last_update_time < s_pm.update_interval_ms) {
        return XY_PM_OK;
    }

    s_pm.last_update_time = now;

    /* 读取电池电压 */
    uint32_t voltage = xy_pm_get_battery_voltage_mV();
    uint8_t soc = xy_pm_get_battery_percent();

    /* 更新电量计 */
    xy_fuel_gauge_update(voltage, 0, 25);

    /* 更新充电器状态 */
    xy_charger_state_t chg_state;
    xy_charger_get_state(&chg_state);

    /* 更新系统状态 */
    s_pm.state.system_voltage_mV = voltage;
    s_pm.state.battery_state.soc_percent = soc;
    s_pm.state.battery_state.voltage_mV = voltage;
    s_pm.state.charger_state = chg_state;
    s_pm.state.power_good = (voltage > 3000);

    /* 自动充电控制 */
    if (s_pm.state.enabled && soc < 95 && !chg_state.charging) {
        xy_charger_start();
    } else if (soc >= 100 && chg_state.charging) {
        xy_charger_stop();
    }

    xy_log_d("PM Update: V=%dmV SOC=%d%% CHG=%d\n",
             voltage, soc, chg_state.charging);

    return XY_PM_OK;
}

int xy_pm_get_state(xy_pm_system_state_info_t *state)
{
    if (!state) return XY_PM_INVALID_PARAM;

    if (!s_pm.initialized) {
        memset(state, 0, sizeof(xy_pm_system_state_info_t));
        return XY_PM_NOT_INITIALIZED;
    }

    /* 先更新状态 */
    xy_pm_update();

    memcpy(state, &s_pm.state, sizeof(xy_pm_system_state_info_t));
    return XY_PM_OK;
}

int xy_pm_start_charging(void)
{
    if (!s_pm.initialized) return XY_PM_NOT_INITIALIZED;

    s_pm.state.enabled = true;
    return xy_charger_start();
}

int xy_pm_stop_charging(void)
{
    if (!s_pm.initialized) return XY_PM_NOT_INITIALIZED;

    s_pm.state.enabled = false;
    return xy_charger_stop();
}

bool xy_pm_is_charging(void)
{
    return s_pm.initialized && xy_charger_is_charging();
}

uint8_t xy_pm_get_soc(void)
{
    if (!s_pm.initialized) return 0;
    return xy_fuel_gauge_get_soc();
}

uint32_t xy_pm_get_battery_voltage(void)
{
    if (!s_pm.initialized) return 0;
    return xy_pm_get_battery_voltage_mV();
}

/* 低功耗模式控制 */
int xy_pm_enter_sleep(void)
{
    int result;

    if (!s_pm.initialized) return XY_PM_NOT_INITIALIZED;
    if (s_pm.state.state == XY_PM_SYSTEM_STATE_SLEEP) return XY_PM_ERROR_INVALID_MODE;
    xy_log_i("PM: Entering sleep mode\n");
    result = xy_pm_platform_enter_sleep();
    if (result != XY_PM_OK) return result;
    s_pm.state.state = XY_PM_SYSTEM_STATE_SLEEP;
    return result;
}

int xy_pm_enter_shutdown(void)
{
    if (!s_pm.initialized) return XY_PM_NOT_INITIALIZED;
    return XY_PM_ERROR_NOT_SUPPORTED;
}

xy_pm_status_t xy_pm_shutdown(void)
{
    return (xy_pm_status_t)xy_pm_enter_shutdown();
}

int xy_pm_wakeup(void)
{
    int result;

    if (!s_pm.initialized) return XY_PM_NOT_INITIALIZED;
    if (s_pm.state.state != XY_PM_SYSTEM_STATE_SLEEP) return XY_PM_ERROR_INVALID_MODE;
    xy_log_i("PM: Waking up from sleep\n");
    result = xy_pm_platform_wakeup();
    if (result != XY_PM_OK) return result;
    s_pm.state.state = XY_PM_SYSTEM_STATE_IDLE;
    return result;
}
