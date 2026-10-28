#ifndef XY_HAL_TICK_INTERNAL_H
#define XY_HAL_TICK_INTERNAL_H

#include <stdint.h>

#include "ch32v30x.h"

/*
 * Millisecond timebase shared by the WCH HAL polling loops.
 *
 * Every HAL module used to carry its own copy of this expression; keeping it in
 * one place means a platform change only has to be made once.
 *
 * ---------------------------------------------------------------------------
 * OPEN ISSUE: this source is only valid while the HAL owns SysTick.
 *
 * SysTick->CNT ticks at SystemCoreClock (or /8, selected by CTLR bit 2) and is
 * divided down to milliseconds here. That is correct bare-metal, but once
 * FreeRTOS runs it reloads SysTick at the tick rate
 * (CMP = configCPU_CLOCK_HZ / configTICK_RATE_HZ), so CNT becomes a 0..CMP
 * saw-tooth. The division then returns 0 or 1 and a difference of two readings
 * can underflow to 0xFFFFFFFF, which makes every polling wait time out at once
 * and lets callers that ignore the result drop data silently.
 *
 * The cycle counters cannot replace SysTick on this core: both `rdcycle`
 * (CSR 0xC00) and `mcycle` (CSR 0xB00) raise an illegal instruction trap
 * (mcause = 2) on the CH32V307 QingKe core, which was verified on hardware.
 * A real fix needs a timer the RTOS does not own (a hardware timer used as a
 * free-running counter) or a registered source an RTOS build can point at its
 * own tick.
 * ---------------------------------------------------------------------------
 */
static inline uint32_t xy_hal_tick_ms_internal(void) {
    uint32_t divisor =
        (SysTick->CTLR & (1U << 2U)) != 0U ? SystemCoreClock / 1000U : SystemCoreClock / 8000U;
    return divisor == 0U ? 0U : (uint32_t)(SysTick->CNT / divisor);
}

#endif /* XY_HAL_TICK_INTERNAL_H */
