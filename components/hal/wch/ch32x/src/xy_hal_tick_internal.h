#ifndef XY_HAL_TICK_INTERNAL_H
#define XY_HAL_TICK_INTERNAL_H

#include <stdint.h>

#include "ch32v30x.h"
#include "xy_hal_time.h"

/*
 * Millisecond timebase shared by the WCH HAL polling loops.
 *
 * Every HAL module used to carry its own copy of this expression; keeping it in
 * one place means a platform change only has to be made once.
 *
 * ---------------------------------------------------------------------------
 * Constraint: this derivation is only valid while the HAL owns SysTick.
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
 * The registered source below is that fix: an RTOS integration calls
 * xy_hal_tick_set_source() with the kernel's own monotonic tick, and every
 * polling loop in this platform keeps using this helper unchanged. Without one
 * installed the SysTick expression still applies, which is correct bare-metal.
 *
 * Status: resolved for the RTOS case by the registered source above. The
 * primitives image now samples xy_hal_time_ms() across a 20 tick kernel delay
 * and prints OPENCH_OSAL_PRIMITIVES_TIMEBASE_OK_A on hardware, which the old
 * saw-tooth derivation could not produce.
 *
 * Measured impact before the fix: an OSAL image that sends four UART markers from a task
 * recorded 4 of 4 xy_hal_uart_send calls as XY_HAL_OK, because TXE is ready on
 * the first poll at 115200 and the timeout branch is never evaluated. The
 * breakage shows up only on a peripheral that actually has to wait, so do not
 * read "no failures yet" as "the timebase works".
 * ---------------------------------------------------------------------------
 */
/* Installed by xy_hal_tick_set_source(); NULL means "derive it from SysTick". */
extern xy_hal_tick_source_t xy_hal_tick_registered_source;

static inline uint32_t xy_hal_tick_ms_internal(void) {
    uint32_t divisor;
    if (xy_hal_tick_registered_source != NULL)
        return xy_hal_tick_registered_source();
    divisor =
        (SysTick->CTLR & (1U << 2U)) != 0U ? SystemCoreClock / 1000U : SystemCoreClock / 8000U;
    return divisor == 0U ? 0U : (uint32_t)(SysTick->CNT / divisor);
}

#endif /* XY_HAL_TICK_INTERNAL_H */
