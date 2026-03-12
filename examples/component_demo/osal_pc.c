/**
 * @file osal_pc.c
 * @brief Minimal OSAL Stubs for PC Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include "xy_os.h"

uint32_t xy_os_tick_get(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

xy_os_status_t xy_os_delay(uint32_t ms)
{
    usleep(ms * 1000);
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_init(void) { return XY_OS_OK; }
xy_os_status_t xy_os_kernel_start(void) { return XY_OS_OK; }
xy_os_status_t xy_os_kernel_lock(void) { return XY_OS_OK; }
xy_os_status_t xy_os_kernel_unlock(void) { return XY_OS_OK; }

xy_os_status_t xy_os_thread_create(xy_os_thread_id_t *t, const char *n, xy_os_thread_func_t f, void *a, uint8_t p, uint16_t s)
{ (void)t;(void)n;(void)f;(void)a;(void)p;(void)s; return XY_OS_OK; }
xy_os_status_t xy_os_thread_delete(xy_os_thread_id_t t) { (void)t; return XY_OS_OK; }

xy_os_status_t xy_os_mutex_create(xy_os_mutex_id_t *m) { (void)m; return XY_OS_OK; }
xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t m) { (void)m; return XY_OS_OK; }
xy_os_status_t xy_os_mutex_lock(xy_os_mutex_id_t m, uint32_t t) { (void)m;(void)t; return XY_OS_OK; }
xy_os_status_t xy_os_mutex_unlock(xy_os_mutex_id_t m) { (void)m; return XY_OS_OK; }

xy_os_status_t xy_os_semaphore_create(xy_os_semaphore_id_t *s, uint16_t i, uint16_t m)
{ (void)s;(void)i;(void)m; return XY_OS_OK; }
xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t s) { (void)s; return XY_OS_OK; }
xy_os_status_t xy_os_semaphore_take(xy_os_semaphore_id_t s, uint32_t t) { (void)s;(void)t; return XY_OS_OK; }
xy_os_status_t xy_os_semaphore_put(xy_os_semaphore_id_t s) { (void)s; return XY_OS_OK; }

xy_os_status_t xy_os_msgqueue_create(xy_os_msgqueue_id_t *m, uint16_t c, uint16_t s)
{ (void)m;(void)c;(void)s; return XY_OS_OK; }
xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t m) { (void)m; return XY_OS_OK; }
xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t m, const void *msg, uint8_t p, uint32_t t)
{ (void)m;(void)msg;(void)p;(void)t; return XY_OS_OK; }
xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t m, void *msg, uint8_t *p, uint32_t t)
{ (void)m;(void)msg;(void)p;(void)t; return XY_OS_OK; }

xy_os_status_t xy_os_timer_create(xy_os_timer_id_t *t, const char *n, xy_os_timer_func_t f, void *a, uint32_t i, bool o)
{ (void)t;(void)n;(void)f;(void)a;(void)i;(void)o; return XY_OS_OK; }
xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t t) { (void)t; return XY_OS_OK; }
xy_os_status_t xy_os_timer_start(xy_os_timer_id_t t, uint32_t k) { (void)t;(void)k; return XY_OS_OK; }
xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t t) { (void)t; return XY_OS_OK; }

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t e, uint32_t f)
{ (void)e;(void)f; return 0; }
uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t e, uint32_t f, uint32_t o, uint32_t t)
{ (void)e;(void)f;(void)o;(void)t; return 0; }
xy_os_status_t xy_os_event_flags_create(xy_os_event_flags_id_t *e, uint32_t f)
{ (void)e;(void)f; return XY_OS_OK; }
xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t e) { (void)e; return XY_OS_OK; }

xy_os_status_t xy_os_mempool_create(xy_os_mempool_id_t *m, uint16_t c, uint16_t s)
{ (void)m;(void)c;(void)s; return XY_OS_OK; }
xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t m) { (void)m; return XY_OS_OK; }
void *xy_os_mempool_alloc(xy_os_mempool_id_t m, uint32_t t) { (void)m;(void)t; return NULL; }
xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t m, void *b) { (void)m;(void)b; return XY_OS_OK; }
