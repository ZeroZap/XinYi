/**
 * @file xy_pm.h
 * @brief Power Management System Main Header
 * @version 1.0.0
 * @date 2026-03-12
 */

#ifndef XY_PM_H
#define XY_PM_H

#include "xy_charger.h"
#include "xy_fuel_gauge.h"
#include "../hal/inc/xy_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== PM System Error Codes ==================== */

#define XY_PM_OK                    0
#define XY_PM_ERROR                 (-1)
#define XY_PM_INVALID_PARAM         (-2)
#define XY_PM_NOT_INITIALIZED       (-3)

/* ==================== PM System Configuration ==================== */

#ifndef XY_PM_DEFAULT_BATTERY_CAPACITY_MAH
#define XY_PM_DEFAULT_BATTERY_CAPACITY_MAH 2000
#endif

#ifndef XY_PM_DEFAULT_CHARGE_CURRENT_MAH
#define XY_PM_DEFAULT_CHARGE_CURRENT_MAH 1000
#endif

/* ==================== PM System State ==================== */

/**
 * @brief Power management system state
 */
typedef struct {
    bool initialized;
    xy_charger_state_t charger_state;
    xy_battery_state_t battery_state;
    uint32_t system_voltage_mV;
    bool power_good;
} xy_pm_system_state_t;

/* ==================== PM System Operations ==================== */

/**
 * @brief Initialize power management system
 * @return XY_PM_OK on success
 */
int xy_pm_init(void);

/**
 * @brief Deinitialize power management system
 * @return XY_PM_OK on success
 */
int xy_pm_deinit(void);

/**
 * @brief Get PM system state
 * @param state Pointer to system state structure
 * @return XY_PM_OK on success
 */
int xy_pm_get_state(xy_pm_system_state_t *state);

/**
 * @brief Start charging
 * @return XY_PM_OK on success
 */
int xy_pm_start_charging(void);

/**
 * @brief Stop charging
 * @return XY_PM_OK on success
 */
int xy_pm_stop_charging(void);

/**
 * @brief Check if system is charging
 * @return true if charging
 */
bool xy_pm_is_charging(void);

/**
 * @brief Get battery state of charge
 * @return SOC percentage (0-100)
 */
uint8_t xy_pm_get_soc(void);

/**
 * @brief Get battery voltage
 * @return Battery voltage in mV
 */
uint32_t xy_pm_get_battery_voltage(void);

/**
 * @brief Update PM system (call periodically)
 * @return XY_PM_OK on success
 */
int xy_pm_update(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_PM_H */
