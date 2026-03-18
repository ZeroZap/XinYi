/**
 * @file xy_pm.h
 * @brief Power Management
 */

#ifndef XY_PM_H
#define XY_PM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    XY_PM_MODE_ACTIVE = 0,
    XY_PM_MODE_SLEEP,
    XY_PM_MODE_DEEP_SLEEP,
    XY_PM_MODE_SHUTDOWN
} xy_pm_mode_t;

int xy_pm_init(void);
int xy_pm_set_mode(xy_pm_mode_t mode);
int xy_pm_get_battery_voltage(void);
int xy_pm_get_battery_percent(void);

#endif /* XY_PM_H */
