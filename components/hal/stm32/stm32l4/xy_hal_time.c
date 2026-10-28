/**
 * @file xy_hal_time.c
 * @brief STM32L4 implementation of the redirectable HAL millisecond timebase
 *
 * Bare-metal, uwTick advances from SysTick_Handler through
 * xy_hal_sys_tick_irq_handler(). An RTOS whose SysTick handler does not call
 * HAL_IncTick() - the usual shape, since the kernel's own handler replaces the
 * vendor one - leaves uwTick frozen, and every ST HAL wait that compares against
 * HAL_GetTick() then stops expiring. A board that keeps calling
 * xy_hal_sys_tick_irq_handler() from its own handler keeps uwTick alive, but the
 * hook is a contract for every integration rather than a repair of one board.
 *
 * Routing HAL_GetTick() through the registered source fixes both the wrappers
 * and the ST HAL drivers underneath them, because those drivers call HAL_GetTick()
 * rather than reading uwTick. The fallback reads uwTick directly on purpose: a
 * fallback that called HAL_GetTick() would recurse into this override.
 */

#include "xy_hal_time.h"

#include "stm32l4xx_hal.h"

static xy_hal_tick_source_t xy_hal_stm32_tick_source;

void xy_hal_tick_set_source(xy_hal_tick_source_t source)
{
    xy_hal_stm32_tick_source = source;
}

uint32_t xy_hal_time_ms(void)
{
    if (xy_hal_stm32_tick_source != NULL) {
        return xy_hal_stm32_tick_source();
    }
    return uwTick;
}

/**
 * @brief Strong override of the weak ST definition
 *
 * ST declares HAL_GetTick() weak in stm32l4xx_hal.c precisely so an
 * integration can replace it; keeping it strong here means an RTOS build does
 * not have to patch vendor code to keep its timeouts alive.
 */
uint32_t HAL_GetTick(void)
{
    return xy_hal_time_ms();
}
