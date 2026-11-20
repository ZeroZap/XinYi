/**
 * SPDX-License-Identifier: MIT
 * @file    test_gps_example_tick_stub.c
 * @brief   Frozen OSAL tick for the `sensor_gps_example` sample target.
 *
 * `components/drivers/sensor/position/gps/xy_gps.c` reaches the clock through
 * `xy_hal_sys_get_tick_count()`. The sample never wants real time: it asserts
 * the "no time has passed since the last update" contract of `gps_get_uptime()`,
 * which is only deterministic with a frozen tick. Keeping the stub in its own
 * translation unit leaves the sample itself free of host-test plumbing.
 */

#include <stdint.h>

#define GPS_EXAMPLE_FROZEN_TICK 1000U

uint32_t xy_hal_sys_get_tick_count(void)
{
    return GPS_EXAMPLE_FROZEN_TICK;
}
