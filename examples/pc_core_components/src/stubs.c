/**
 * @file stubs.c
 * @brief Stub functions for missing dependencies
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Tick stubs */
uint32_t xy_tick_get(void) {
    return 0;
}

/* Timer stubs */
void* xy_timer_sw_create(uint32_t period, void (*callback)(void*), void *arg, bool periodic) {
    (void)period;
    (void)callback;
    (void)arg;
    (void)periodic;
    return (void*)1;
}

void xy_timer_sw_stop(void *timer) {
    (void)timer;
}

void xy_timer_sw_delete(void *timer) {
    (void)timer;
}

/* String stub if needed */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}
