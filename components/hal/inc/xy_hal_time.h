/**
 * @file xy_hal_time.h
 * @brief HAL millisecond timebase that a kernel may own
 */

#ifndef XY_HAL_TIME_H
#define XY_HAL_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Redirect the HAL millisecond timebase to another time source
 *
 * The default source derives milliseconds from SysTick, which is only correct
 * while the HAL owns it. A kernel that reloads SysTick at its own tick rate
 * turns the counter into a saw-tooth, and a difference of two readings then
 * underflows - every polling wait fires at once and callers that ignore the
 * result drop data silently. An RTOS integration points this at the kernel's
 * own monotonic tick and the HAL polling loops keep working unchanged.
 *
 * @param[in] source Milliseconds since boot, or NULL to restore the SysTick path
 */
typedef uint32_t (*xy_hal_tick_source_t)(void);
void xy_hal_tick_set_source(xy_hal_tick_source_t source);

/**
 * @brief Monotonic milliseconds since boot
 *
 * Returns the registered source's value when one is installed, otherwise the
 * platform default. This is the same value the HAL polling loops use, so a
 * caller can measure what a peripheral wait is really being compared against.
 */
uint32_t xy_hal_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_TIME_H */
