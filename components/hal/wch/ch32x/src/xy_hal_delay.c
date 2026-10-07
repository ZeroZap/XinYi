/**
 * @file xy_hal_delay.c
 * @brief WCH CH32V30x blocking delay implementation
 */

#include "xy_hal_delay.h"

#ifdef MCU_CH32

#include "ch32v30x.h"

static void delay_ticks(uint64_t ticks) {
    while (ticks != 0U) {
        uint32_t chunk = ticks > UINT32_MAX ? UINT32_MAX : (uint32_t)ticks;
        SysTick->SR &= ~1U;
        SysTick->CMP = chunk;
        SysTick->CTLR |= (1U << 4U);
        SysTick->CTLR |= (1U << 5U) | 1U;
        while ((SysTick->SR & 1U) == 0U) {
        }
        SysTick->CTLR &= ~1U;
        ticks -= chunk;
    }
}

void xy_hal_delay_us(uint32_t us) {
    uint32_t ticks_per_us = SystemCoreClock / 8000000U;
    if (us == 0U)
        return;
    if (ticks_per_us == 0U)
        ticks_per_us = 1U;
    delay_ticks((uint64_t)us * ticks_per_us);
}

void xy_hal_delay_ms(uint32_t ms) {
    uint32_t ticks_per_us = SystemCoreClock / 8000000U;
    if (ms == 0U)
        return;
    if (ticks_per_us == 0U)
        ticks_per_us = 1U;
    delay_ticks((uint64_t)ms * ticks_per_us * 1000U);
}

#endif /* MCU_CH32 */
