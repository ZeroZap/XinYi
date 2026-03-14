/**
 * @file stubs.c
 * @brief Stub functions for PC build
 */

#include <stdint.h>
#include <stdbool.h>

/* Tick stubs */
uint32_t xy_tick_get(void) { return 0; }
uint32_t xy_tick_get_freq(void) { return 1000; }

/* Timer stubs */
void* xy_timer_sw_create(uint32_t period, void (*callback)(void*), void *arg, bool periodic) {
    (void)period; (void)callback; (void)arg; (void)periodic;
    return (void*)1;
}
void xy_timer_sw_stop(void *timer) { (void)timer; }
void xy_timer_sw_delete(void *timer) { (void)timer; }
