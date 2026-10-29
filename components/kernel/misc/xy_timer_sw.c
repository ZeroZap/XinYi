/**
 * @file xy_timer_sw.c
 * @brief Software Timer Module for Bare-metal Applications
 * @version 1.0.0
 * @date 2026-02-28
 *
 * This module provides a simple software timer list for bare-metal applications.
 * It requires a periodic call to xy_timer_sw_poll() from main loop or timer ISR.
 */

#include "xy_timer_sw.h"
#include <string.h>

#define XY_TIMER_SW_MAX_TIMERS 16

typedef struct {
    xy_timer_sw_callback_t callback;
    void *arg;
    uint32_t interval;
    uint32_t reload;
    uint32_t counter;
    uint8_t running;
    uint8_t periodic;
    uint8_t reserved;
} xy_timer_sw_entry_t;

static xy_timer_sw_entry_t g_timers[XY_TIMER_SW_MAX_TIMERS];
static uint8_t g_initialized = 0;

void xy_timer_sw_init(void)
{
    memset(g_timers, 0, sizeof(g_timers));
    g_initialized = 1;
}

xy_timer_sw_id_t xy_timer_sw_create(uint32_t interval, xy_timer_sw_callback_t callback,
                                    void *arg, uint8_t periodic)
{
    if (!g_initialized || !callback) {
        return XY_TIMER_SW_INVALID_ID;
    }

    for (int i = 0; i < XY_TIMER_SW_MAX_TIMERS; i++) {
        if (!g_timers[i].running) {
            g_timers[i].callback = callback;
            g_timers[i].arg = arg;
            g_timers[i].interval = interval;
            g_timers[i].reload = periodic ? interval : 0;
            g_timers[i].counter = interval;
            g_timers[i].running = 1;
            g_timers[i].periodic = periodic;
            return (xy_timer_sw_id_t)(i + 1);
        }
    }

    return XY_TIMER_SW_INVALID_ID;
}

xy_timer_sw_error_t xy_timer_sw_start(xy_timer_sw_id_t id)
{
    if (!g_initialized || id == XY_TIMER_SW_INVALID_ID) {
        return XY_TIMER_SW_ERR_INVALID;
    }

    int idx = (int)id - 1;
    if (idx < 0 || idx >= XY_TIMER_SW_MAX_TIMERS) {
        return XY_TIMER_SW_ERR_INVALID;
    }

    g_timers[idx].running = 1;
    g_timers[idx].counter = g_timers[idx].interval;
    return XY_TIMER_SW_OK;
}

xy_timer_sw_error_t xy_timer_sw_stop(xy_timer_sw_id_t id)
{
    if (!g_initialized || id == XY_TIMER_SW_INVALID_ID) {
        return XY_TIMER_SW_ERR_INVALID;
    }

    int idx = (int)id - 1;
    if (idx < 0 || idx >= XY_TIMER_SW_MAX_TIMERS) {
        return XY_TIMER_SW_ERR_INVALID;
    }

    g_timers[idx].running = 0;
    return XY_TIMER_SW_OK;
}

xy_timer_sw_error_t xy_timer_sw_delete(xy_timer_sw_id_t id)
{
    if (!g_initialized || id == XY_TIMER_SW_INVALID_ID) {
        return XY_TIMER_SW_ERR_INVALID;
    }

    int idx = (int)id - 1;
    if (idx < 0 || idx >= XY_TIMER_SW_MAX_TIMERS) {
        return XY_TIMER_SW_ERR_INVALID;
    }

    g_timers[idx].running = 0;
    g_timers[idx].callback = NULL;
    g_timers[idx].arg = NULL;
    return XY_TIMER_SW_OK;
}

void xy_timer_sw_poll(void)
{
    if (!g_initialized) {
        return;
    }

    for (int i = 0; i < XY_TIMER_SW_MAX_TIMERS; i++) {
        if (g_timers[i].running && g_timers[i].callback) {
            /* Decrement first, then fire on reaching zero. The other order - fire when the counter was
             * already zero at the top of the poll - fires one tick late: a timer started for 3 ticks ran
             * on the 4th poll, and every timer in the system was a tick behind what it asked for. */
            if (g_timers[i].counter > 0) {
                g_timers[i].counter--;
            }
            if (g_timers[i].counter == 0) {
                g_timers[i].callback(g_timers[i].arg);

                if (g_timers[i].periodic) {
                    g_timers[i].counter = g_timers[i].interval;
                } else {
                    g_timers[i].running = 0;
                }
            }
        }
    }
}
