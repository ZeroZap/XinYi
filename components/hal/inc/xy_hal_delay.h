/**
 * @file xy_hal_delay.h
 * @brief HAL Delay Functions
 */

#ifndef XY_HAL_DELAY_H
#define XY_HAL_DELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void xy_hal_delay_ms(uint32_t ms);
void xy_hal_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_DELAY_H */
