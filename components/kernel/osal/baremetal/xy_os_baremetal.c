/**
 * @file xy_os_baremetal.c
 * @brief XY OSAL Bare-metal Implementation - Minimal RTOS stub
 * @version 1.0.0
 */

#include "../xy_os.h"
#include "../../kernel/xy_tick.h"
#include <string.h>

static volatile uint32_t s_lock_count = 0;
static xy_os_kernel_state_t s_state   = XY_OS_KERNEL_INACTIVE;

// Kernel functions
xy_os_status_t xy_os_kernel_init(void)
{
    s_state = XY_OS_KERNEL_READY;
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                     uint32_t id_size)
{
    if (version) {
        version->api    = (1 << 16);
        version->kernel = 0x00010000;
    }
    if (id_buf && id_size > 0) {
        strncpy_s(id_buf, "Baremetal", id_size - 1);
        id_buf[id_size - 1] = '\0';
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    return s_state;
}
xy_os_status_t xy_os_kernel_start(void)
{
    s_state = XY_OS_KERNEL_RUNNING;
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void)
{
    uint32_t prev = s_lock_count++;
    if (s_lock_count == 1)
        s_state = XY_OS_KERNEL_LOCKED;
    return prev;
}

int32_t xy_os_kernel_unlock(void)
{
    if (s_lock_count > 0)
        s_lock_count--;
    if (s_lock_count == 0)
        s_state = XY_OS_KERNEL_RUNNING;
    return s_lock_count;
}

int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    uint32_t prev = s_lock_count;
    s_lock_count  = lock;
    s_state       = (lock > 0) ? XY_OS_KERNEL_LOCKED : XY_OS_KERNEL_RUNNING;
    return prev;
}

uint32_t xy_os_kernel_get_tick_count(void)
{
    return xy_tick_get();
}
uint32_t xy_os_kernel_get_tick_freq(void)
{
    return 1000;
}
uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return xy_tick_get();
}
uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return 1000;
}

// Delay functions
xy_os_status_t xy_os_delay(uint32_t ticks)
{
    uint32_t start = xy_tick_get();
    while ((xy_tick_get() - start) < ticks)
        ;
    return XY_OS_OK;
}

xy_os_status_t xy_os_delay_until(uint32_t ticks)
{
    uint32_t now = xy_tick_get();
    return (ticks > now) ? xy_os_delay(ticks - now) : XY_OS_OK;
}

// Thread stubs
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t f, void *arg,
                                   const xy_os_thread_attr_t *attr)
{
    return NULL;
}
const char *xy_os_thread_get_name(xy_os_thread_id_t id)
{
    return "main";
}
xy_os_thread_id_t xy_os_thread_get_id(void)
{
    return (void *)0x1;
}
xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t id)
{
    return XY_OS_THREAD_RUNNING;
}
uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t id)
{
    return 0;
}
uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t id,
                                         xy_os_priority_t p)
{
    return XY_OS_ERROR;
}
xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t id)
{
    return XY_OS_PRIORITY_NORMAL;
}
xy_os_status_t xy_os_thread_yield(void)
{
    return XY_OS_OK;
}
xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_detach(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_join(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
void xy_os_thread_exit(void)
{
    while (1)
        ;
}
xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_thread_get_count(void)
{
    return 1;
}
uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *arr, uint32_t n)
{
    if (arr && n > 0) {
        arr[0] = xy_os_thread_get_id();
        return 1;
    }
    return 0;
}

// Thread flags stubs
uint32_t xy_os_thread_flags_set(xy_os_thread_id_t id, uint32_t f)
{
    return 0x80000000;
}
uint32_t xy_os_thread_flags_clear(uint32_t f)
{
    return 0;
}
uint32_t xy_os_thread_flags_get(void)
{
    return 0;
}
uint32_t xy_os_thread_flags_wait(uint32_t f, uint32_t opt, uint32_t to)
{
    return 0x80000000;
}

// All other primitives return NULL/error
xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t f, xy_os_timer_type_t t,
                                 void *arg, const xy_os_timer_attr_t *attr)
{
    return NULL;
}
const char *xy_os_timer_get_name(xy_os_timer_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_timer_start(xy_os_timer_id_t id, uint32_t ticks)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t id)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_timer_is_running(xy_os_timer_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_event_flags_id_t
xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    return NULL;
}
const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t id)
{
    return NULL;
}
uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t id, uint32_t f)
{
    return 0x80000000;
}
uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t id, uint32_t f)
{
    return 0;
}
uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t id)
{
    return 0;
}
uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t id, uint32_t f,
                                uint32_t opt, uint32_t to)
{
    return 0x80000000;
}
xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    return NULL;
}
const char *xy_os_mutex_get_name(xy_os_mutex_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t id, uint32_t to)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max, uint32_t init,
                                         const xy_os_semaphore_attr_t *attr)
{
    return NULL;
}
const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t id, uint32_t to)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t id)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_mempool_id_t xy_os_mempool_new(uint32_t cnt, uint32_t sz,
                                     const xy_os_mempool_attr_t *attr)
{
    return NULL;
}
const char *xy_os_mempool_get_name(xy_os_mempool_id_t id)
{
    return NULL;
}
void *xy_os_mempool_alloc(xy_os_mempool_id_t id, uint32_t to)
{
    return NULL;
}
xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t id, void *blk)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t id)
{
    return 0;
}
uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t id)
{
    return 0;
}
uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t id)
{
    return 0;
}
uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t id)
{
    return XY_OS_ERROR;
}

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t cnt, uint32_t sz,
                                       const xy_os_msgqueue_attr_t *attr)
{
    return NULL;
}
const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t id)
{
    return NULL;
}
xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t id, const void *msg,
                                  uint8_t prio, uint32_t to)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t id, void *msg,
                                  uint8_t *prio, uint32_t to)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t id)
{
    return 0;
}
uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t id)
{
    return 0;
}
uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t id)
{
    return 0;
}
uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t id)
{
    return XY_OS_ERROR;
}
