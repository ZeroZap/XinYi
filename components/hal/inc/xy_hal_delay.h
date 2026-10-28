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

/**
 * @brief Redirect the HAL delay to another time source
 *
 * The default implementation drives SysTick directly, which is only safe while
 * the HAL owns it: under an RTOS the kernel reloads SysTick at its tick rate,
 * and reprogramming CMP/CTLR here stops the scheduler tick for good. An RTOS
 * integration installs its own task-aware delay with this hook and the HAL
 * implementations keep working unchanged.
 *
 * @param[in] source Delay in microseconds, or NULL to restore the SysTick path
 */
typedef void (*xy_hal_delay_source_t)(uint32_t us);
void xy_hal_delay_set_source(xy_hal_delay_source_t source);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_DELAY_H */
