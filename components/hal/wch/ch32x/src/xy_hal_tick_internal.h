#ifndef XY_HAL_TICK_INTERNAL_H
#define XY_HAL_TICK_INTERNAL_H

#include <stdint.h>

#include "ch32v30x.h"

/*
 * Millisecond timebase for HAL polling timeouts.
 *
 * The WCH HAL originally derived milliseconds from SysTick->CNT. That only works
 * while the HAL owns SysTick: once FreeRTOS runs, the kernel reloads SysTick at
 * the tick rate (CMP = configCPU_CLOCK_HZ / configTICK_RATE_HZ), so CNT becomes a
 * 0..CMP saw-tooth. Dividing that by SystemCoreClock/1000 yields 0 or 1, and
 * subtracting two such readings can underflow to 0xFFFFFFFF, which makes every
 * timeout fire immediately and every xy_hal_uart_send drop its payload.
 *
 * Read the machine cycle counter instead: it keeps counting independently of
 * SysTick, so the same HAL code works bare-metal and under an RTOS. A 32-bit
 * cycle counter wraps roughly every 44 s at 96 MHz, and all call sites compare
 * unsigned differences, so wrap-around stays correct.
 */
static inline uint32_t xy_hal_tick_ms_internal(void) {
    uint32_t cycles;
    uint32_t per_ms = SystemCoreClock / 1000U;
    __asm volatile("rdcycle %0" : "=r"(cycles));
    return per_ms == 0U ? 0U : cycles / per_ms;
}

#endif /* XY_HAL_TICK_INTERNAL_H */
