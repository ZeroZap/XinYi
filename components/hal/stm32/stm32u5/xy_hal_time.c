/**
 * @file xy_hal_time.c
 * @brief STM32U5 implementation of the redirectable HAL millisecond timebase
 *
 * Bare-metal, uwTick advances from SysTick_Handler through
 * xy_hal_sys_tick_irq_handler(). Once FreeRTOS owns SysTick the kernel's handler
 * runs instead and nothing calls HAL_IncTick(), so uwTick stops and every ST HAL
 * wait that compares against HAL_GetTick() stops expiring - the opposite failure
 * to the WCH saw-tooth, but the same root cause: the tick belongs to the kernel.
 *
 * Routing HAL_GetTick() through the registered source fixes both the wrappers
 * and the ST HAL drivers underneath them, because those drivers call HAL_GetTick()
 * rather than reading uwTick. The fallback reads uwTick directly on purpose: a
 * fallback that called HAL_GetTick() would recurse into this override.
 */

#include "xy_hal_time.h"

#include "stm32u5xx_hal.h"

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
