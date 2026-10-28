/**
 * @file xy_hal_time.c
 * @brief WCH implementation of the redirectable HAL millisecond timebase
 */

#include "xy_hal_time.h"

#include "xy_hal_tick_internal.h"

/* Read by xy_hal_tick_ms_internal(); NULL keeps the SysTick derivation. */
xy_hal_tick_source_t xy_hal_tick_registered_source;

void xy_hal_tick_set_source(xy_hal_tick_source_t source) {
    xy_hal_tick_registered_source = source;
}

uint32_t xy_hal_time_ms(void) {
    return xy_hal_tick_ms_internal();
}
