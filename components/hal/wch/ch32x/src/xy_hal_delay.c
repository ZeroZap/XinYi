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

/* See xy_hal_delay_set_source(): an RTOS installs its own delay here so this
   file never reprograms SysTick while the kernel is using it. */
static xy_hal_delay_source_t delay_source;

void xy_hal_delay_set_source(xy_hal_delay_source_t source) {
    delay_source = source;
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
    if (delay_source != NULL) {
        delay_source(us);
        return;
    }
    delay_ticks((uint64_t)us * ticks_per_us());
}

void xy_hal_delay_ms(uint32_t ms) {
    uint32_t chunk;
    if (ms == 0U)
        return;
    if (delay_source != NULL) {
        /* Hand the source whole seconds where possible so a millisecond count
           cannot overflow the microsecond conversion. */
        while (ms > (UINT32_MAX / 1000U)) {
            delay_source(1000000U);
            ms -= 1000U;
        }
        delay_source(ms * 1000U);
        return;
    }
    chunk = ms;
    delay_ticks((uint64_t)chunk * ticks_per_us() * 1000U);
}

#endif /* MCU_CH32 */
