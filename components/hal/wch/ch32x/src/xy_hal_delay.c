/**
 * @file xy_hal_delay.c
 * @brief WCH CH32V30x blocking delay implementation
 */

#include "xy_hal_delay.h"

#ifdef MCU_CH32

#include "ch32v30x.h"

static uint32_t ticks_per_us(void) {
    uint32_t source_hz =
        (SysTick->CTLR & (1U << 2U)) != 0U ? SystemCoreClock : SystemCoreClock / 8U;
    uint32_t ticks = source_hz / 1000000U;
    return ticks == 0U ? 1U : ticks;
}

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
    if (us == 0U)
        return;
    delay_ticks((uint64_t)us * ticks_per_us());
}

void xy_hal_delay_ms(uint32_t ms) {
    if (ms == 0U)
        return;
    delay_ticks((uint64_t)ms * ticks_per_us() * 1000U);
}

#endif /* MCU_CH32 */
