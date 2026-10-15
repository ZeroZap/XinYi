/**
 * @file xy_os_rtthread.c
 * @brief XY OSAL RT-Thread Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../inc/xy_os.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "rtthread.h"

/* RT-Thread context structures */
typedef struct {
    rt_thread_t thread;
    rt_uint8_t initialized;
    xy_os_thread_func_t callback;
    void *arg;
} thread_ctx_t;

typedef struct {
    rt_mutex_t mutex;
    rt_uint8_t initialized;
} mutex_ctx_t;

typedef struct {
    rt_sem_t sem;
    rt_uint8_t initialized;
} sem_ctx_t;

typedef struct {
    rt_timer_t timer;
    rt_uint8_t initialized;
    xy_os_timer_func_t callback;
    void *arg;
} timer_ctx_t;

typedef struct {
    rt_mq_t mq;
    rt_uint8_t initialized;
} msgqueue_ctx_t;

typedef struct {
    rt_mp_t mp;
    rt_uint8_t initialized;
} mempool_ctx_t;

typedef struct {
    rt_event_t event;
    rt_uint8_t initialized;
} event_flags_ctx_t;

/* Maximum instances */
#define MAX_THREADS 16
#define MAX_MUTEXES 8
#define MAX_SEMAPHORES 8
#define MAX_TIMERS 8
#define MAX_MSGQUEUES 8
#define MAX_MEMPOOLS 8
#define MAX_EVENT_FLAGS 8

static thread_ctx_t g_thread_ctx[MAX_THREADS];
static mutex_ctx_t g_mutex_ctx[MAX_MUTEXES];
static sem_ctx_t g_sem_ctx[MAX_SEMAPHORES];
static timer_ctx_t g_timer_ctx[MAX_TIMERS];
static msgqueue_ctx_t g_msgqueue_ctx[MAX_MSGQUEUES];
static mempool_ctx_t g_mempool_ctx[MAX_MEMPOOLS];
static event_flags_ctx_t g_event_flags_ctx[MAX_EVENT_FLAGS];

static event_flags_ctx_t *alloc_event_flags_ctx(void)
{
    for (size_t i = 0; i < MAX_EVENT_FLAGS; i++) {
        if (!g_event_flags_ctx[i].initialized) {
            return &g_event_flags_ctx[i];
        }
    }
    return NULL;
}

static event_flags_ctx_t *find_event_flags_ctx(rt_event_t event)
{
    for (size_t i = 0; i < MAX_EVENT_FLAGS; i++) {
        if (g_event_flags_ctx[i].initialized && g_event_flags_ctx[i].event == event) {
            return &g_event_flags_ctx[i];
        }
    }
    return NULL;
}

static void clear_event_flags_ctx(event_flags_ctx_t *ctx)
{
    if (!ctx) return;

    ctx->event = NULL;
    ctx->initialized = 0;
}

static mempool_ctx_t *alloc_mempool_ctx(void)
{
    for (size_t i = 0; i < MAX_MEMPOOLS; i++) {
        if (!g_mempool_ctx[i].initialized) {
            return &g_mempool_ctx[i];
        }
    }
    return NULL;
}

static mempool_ctx_t *find_mempool_ctx(rt_mp_t mp)
{
    for (size_t i = 0; i < MAX_MEMPOOLS; i++) {
        if (g_mempool_ctx[i].initialized && g_mempool_ctx[i].mp == mp) {
            return &g_mempool_ctx[i];
        }
    }
    return NULL;
}

static void clear_mempool_ctx(mempool_ctx_t *ctx)
{
    if (!ctx) return;

    ctx->mp = NULL;
    ctx->initialized = 0;
}

static msgqueue_ctx_t *alloc_msgqueue_ctx(void)
{
    for (size_t i = 0; i < MAX_MSGQUEUES; i++) {
        if (!g_msgqueue_ctx[i].initialized) {
            return &g_msgqueue_ctx[i];
        }
    }
    return NULL;
}

static msgqueue_ctx_t *find_msgqueue_ctx(rt_mq_t mq)
{
    for (size_t i = 0; i < MAX_MSGQUEUES; i++) {
        if (g_msgqueue_ctx[i].initialized && g_msgqueue_ctx[i].mq == mq) {
            return &g_msgqueue_ctx[i];
        }
    }
    return NULL;
}

static void clear_msgqueue_ctx(msgqueue_ctx_t *ctx)
{
    if (!ctx) return;

    ctx->mq = NULL;
    ctx->initialized = 0;
}

static timer_ctx_t *alloc_timer_ctx(void)
{
    for (size_t i = 0; i < MAX_TIMERS; i++) {
        if (!g_timer_ctx[i].initialized) {
            return &g_timer_ctx[i];
        }
    }
    return NULL;
}

static timer_ctx_t *find_timer_ctx(rt_timer_t timer)
{
    for (size_t i = 0; i < MAX_TIMERS; i++) {
        if (g_timer_ctx[i].initialized && g_timer_ctx[i].timer == timer) {
            return &g_timer_ctx[i];
        }
    }
    return NULL;
}

static void clear_timer_ctx(timer_ctx_t *ctx)
{
    if (!ctx) return;

    ctx->timer = NULL;
    ctx->initialized = 0;
    ctx->callback = NULL;
    ctx->arg = NULL;
}

static int rtthread_priority_valid(xy_os_priority_t priority)
{
    return priority >= XY_OS_PRIORITY_IDLE && priority <= XY_OS_PRIORITY_ISR;
}

/**
 * @brief Find thread context by handle
 */
static thread_ctx_t *find_thread_ctx(rt_thread_t thread)
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (g_thread_ctx[i].thread == thread) {
            return &g_thread_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new thread context
 */
static thread_ctx_t *alloc_thread_ctx(void)
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (!g_thread_ctx[i].initialized) {
            return &g_thread_ctx[i];
        }
    }
    return NULL;
}

static mutex_ctx_t *find_mutex_ctx(rt_mutex_t mutex)
{
    for (size_t i = 0; i < MAX_MUTEXES; i++) {
        if (g_mutex_ctx[i].initialized && g_mutex_ctx[i].mutex == mutex) {
            return &g_mutex_ctx[i];
        }
    }
    return NULL;
}

static mutex_ctx_t *alloc_mutex_ctx(void)
{
    for (size_t i = 0; i < MAX_MUTEXES; i++) {
        if (!g_mutex_ctx[i].initialized) {
            return &g_mutex_ctx[i];
        }
    }
    return NULL;
}

static void clear_mutex_ctx(mutex_ctx_t *ctx)
{
    if (!ctx) return;

    ctx->mutex = NULL;
    ctx->initialized = 0;
}

static sem_ctx_t *find_sem_ctx(rt_sem_t sem)
{
    for (size_t i = 0; i < MAX_SEMAPHORES; i++) {
        if (g_sem_ctx[i].initialized && g_sem_ctx[i].sem == sem) {
            return &g_sem_ctx[i];
        }
    }
    return NULL;
}

static sem_ctx_t *alloc_sem_ctx(void)
{
    for (size_t i = 0; i < MAX_SEMAPHORES; i++) {
        if (!g_sem_ctx[i].initialized) {
            return &g_sem_ctx[i];
        }
    }
    return NULL;
}

static void clear_sem_ctx(sem_ctx_t *ctx)
{
    if (!ctx) return;

    ctx->sem = NULL;
    ctx->initialized = 0;
}

static void clear_thread_ctx(thread_ctx_t *ctx)
{
    if (!ctx) return;

    ctx->thread = NULL;
    ctx->initialized = 0;
    ctx->callback = NULL;
    ctx->arg = NULL;
}

/**
 * @brief Convert XY priority to RT-Thread
 */
static rt_uint8_t xy_to_rt_priority(xy_os_priority_t priority)
{
    const rt_uint32_t rt_lowest = RT_THREAD_PRIORITY_MAX - 1U;
    const rt_uint32_t xy_priority = (rt_uint32_t)priority;

    /* RT-Thread uses zero as highest; XY uses larger values as higher. */
    return (rt_uint8_t)(rt_lowest -
                        (((xy_priority - (uint32_t)XY_OS_PRIORITY_IDLE) * rt_lowest) /
                         ((uint32_t)XY_OS_PRIORITY_ISR -
                          (uint32_t)XY_OS_PRIORITY_IDLE)));
}

/**
 * @brief Convert RT-Thread priority to XY
 */
static xy_os_priority_t rt_to_xy_priority(rt_uint8_t priority)
{
    const rt_uint32_t rt_lowest = RT_THREAD_PRIORITY_MAX - 1U;
    const rt_uint32_t rt_priority = priority;

    if (rt_priority > rt_lowest) {
        return XY_OS_PRIORITY_ERROR;
    }

    return (xy_os_priority_t)((uint32_t)XY_OS_PRIORITY_IDLE +
                              (((rt_lowest - rt_priority) *
                                ((uint32_t)XY_OS_PRIORITY_ISR -
                                 (uint32_t)XY_OS_PRIORITY_IDLE)) /
                               rt_lowest));
}

/* Kernel Functions */

xy_os_status_t xy_os_kernel_init(void)
{
    // RT-Thread kernel is initialized by rtthread_startup()
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                    uint32_t id_size)
{
    if (version) {
        version->api    = 0x20000; // 2.0.0
        version->kernel = RT_VERSION;
    }
    if (id_buf && id_size > 0) {
        rt_strncpy(id_buf, "RT-Thread", id_size - 1);
        id_buf[id_size - 1] = '\0';
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    return XY_OS_KERNEL_RUNNING; // RT-Thread is always running after startup
}

xy_os_status_t xy_os_kernel_start(void)
{
    // RT-Thread kernel start is handled by rtthread_startup()
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void)
{
    rt_enter_critical();
    return 0;
}

int32_t xy_os_kernel_unlock(void)
{
    rt_exit_critical();
    return 0;
}

int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    XY_UNUSED(lock);
    rt_exit_critical();
    return 0;
}

uint32_t xy_os_kernel_get_tick_count(void)
{
    return (uint32_t)rt_tick_get();
}

uint32_t xy_os_kernel_get_tick_freq(void)
{
    return RT_TICK_PER_SECOND;
}

uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return (uint32_t)rt_tick_get();
}

uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return RT_TICK_PER_SECOND;
}

/* Thread Functions */

xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr)
{
    thread_ctx_t *ctx;

    if (!func) {
        return NULL;
    }

    ctx = alloc_thread_ctx();
    if (!ctx) return NULL;

    /* Reserve the slot before kernel allocation; commit its handle before startup. */
    ctx->initialized = 1;
    ctx->callback = func;
    ctx->arg = argument;

    rt_thread_t thread = RT_NULL;
    rt_err_t result;
    
    const char *name = "thread";
    rt_uint32_t stack_size = 1024;
    rt_uint8_t priority = xy_to_rt_priority(XY_OS_PRIORITY_NORMAL);
    rt_uint32_t tick = 10;

    if (attr) {
        if (attr->name) name = attr->name;
        if (attr->stack_size > 0) stack_size = attr->stack_size;
        if (!rtthread_priority_valid(attr->priority)) {
            clear_thread_ctx(ctx);
            return NULL;
        }
        priority = xy_to_rt_priority(attr->priority);
        if (attr->tick > 0) tick = attr->tick;
    }

    thread = rt_thread_create(name, 
                              (void(*)(void *))func, 
                              argument,
                              stack_size,
                              priority,
                              tick);

    if (thread == RT_NULL) {
        clear_thread_ctx(ctx);
        return NULL;
    }

    ctx->thread = thread;
    result = rt_thread_startup(thread);
    if (result != RT_EOK) {
        rt_thread_delete(thread);
        clear_thread_ctx(ctx);
        return NULL;
    }

    return (xy_os_thread_id_t)thread;
}

const char *xy_os_thread_get_name(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return NULL;

    return thread->name;
}

xy_os_thread_id_t xy_os_thread_get_id(void)
{
    rt_thread_t thread = rt_thread_self();

    /* Do not expose kernel-global threads outside the project-owned set. */
    if (!thread || !find_thread_ctx(thread)) return NULL;

    return (xy_os_thread_id_t)thread;
}

xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return XY_OS_THREAD_ERROR;

    switch (thread->stat) {
    case RT_THREAD_READY:
        return XY_OS_THREAD_READY;
    case RT_THREAD_SUSPEND:
        return XY_OS_THREAD_BLOCKED;
    case RT_THREAD_RUNNING:
        return XY_OS_THREAD_RUNNING;
    case RT_THREAD_CLOSE:
    case RT_THREAD_INIT:
        return XY_OS_THREAD_INACTIVE;
    default:
        return XY_OS_THREAD_ERROR;
    }
}

uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread) || !thread->stack_addr) return 0;
    
    return thread->stack_size;
}

uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread) || !thread->stack_addr) return 0;
    
    rt_uint8_t *top_ptr = (rt_uint8_t *)thread->stack_addr;
    rt_uint32_t used = 0;
    
    // Count used stack space (approximate)
    while (used < thread->stack_size && top_ptr[used] == 0xFF) {
        used++;
    }
    
    return thread->stack_size - used;
}

xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                        xy_os_priority_t priority)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return XY_OS_ERROR_PARAMETER;
    if (!rtthread_priority_valid(priority)) return XY_OS_ERROR_PARAMETER;

    rt_uint8_t rt_priority = xy_to_rt_priority(priority);
    rt_err_t result = rt_thread_control(thread, RT_THREAD_CTRL_CHANGE_PRIORITY, &rt_priority);
    
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return XY_OS_PRIORITY_ERROR;

    return rt_to_xy_priority(thread->current_priority);
}

xy_os_status_t xy_os_thread_yield(void)
{
    rt_thread_yield();
    return XY_OS_OK;
}

xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = rt_thread_suspend(thread);
    if (result == RT_EOK) {
        rt_schedule();
    }
    
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = rt_thread_resume(thread);
    if (result == RT_EOK) {
        rt_schedule();
    }
    
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_thread_detach(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = rt_thread_detach(thread);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_thread_join(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    if (!thread || !find_thread_ctx(thread)) return XY_OS_ERROR_PARAMETER;

    /* RT-Thread has no join primitive; reject a tracked handle explicitly. */
    return XY_OS_ERROR;
}

void xy_os_thread_exit(void)
{
    rt_thread_exit();
}

xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t thread_id)
{
    rt_thread_t thread = (rt_thread_t)thread_id;
    thread_ctx_t *ctx;
    rt_err_t result;

    if (!thread) return XY_OS_ERROR_PARAMETER;

    ctx = find_thread_ctx(thread);
    if (!ctx) return XY_OS_ERROR_PARAMETER;
    result = rt_thread_delete(thread);
    if (result != RT_EOK) return XY_OS_ERROR;

    clear_thread_ctx(ctx);
    return XY_OS_OK;
}

uint32_t xy_os_thread_get_count(void)
{
    uint32_t count = 0;

    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (g_thread_ctx[i].initialized && g_thread_ctx[i].thread) {
            count++;
        }
    }

    return count;
}

uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *thread_array, uint32_t array_items)
{
    uint32_t count = 0;

    if (!thread_array || array_items == 0) return 0;

    for (size_t i = 0; i < MAX_THREADS && count < array_items; i++) {
        if (g_thread_ctx[i].initialized && g_thread_ctx[i].thread) {
            thread_array[count] = (xy_os_thread_id_t)g_thread_ctx[i].thread;
            count++;
        }
    }

    return count;
}

/* Thread Flags Functions */
// RT-Thread doesn't have native thread flags, use event groups instead

uint32_t xy_os_thread_flags_set(xy_os_thread_id_t thread_id, uint32_t flags)
{
    // RT-Thread has no per-thread notification equivalent in this backend.
    XY_UNUSED(thread_id);
    XY_UNUSED(flags);
    return 0x80000000U;
}

uint32_t xy_os_thread_flags_clear(uint32_t flags)
{
    XY_UNUSED(flags);
    return 0x80000000U;
}

uint32_t xy_os_thread_flags_get(void)
{
    return 0x80000000U;
}

uint32_t xy_os_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout)
{
    XY_UNUSED(flags);
    XY_UNUSED(options);
    XY_UNUSED(timeout);
    return 0x80000000U;
}

/* Delay Functions */

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    rt_thread_delay(ticks);
    return XY_OS_OK;
}

xy_os_status_t xy_os_delay_until(uint32_t ticks)
{
    rt_tick_t current = rt_tick_get();
    if (ticks > current) {
        rt_thread_delay(ticks - current);
    }
    return XY_OS_OK;
}

/* Timer Functions */

static void timer_callback(void *parameter)
{
    timer_ctx_t *ctx = (timer_ctx_t *)parameter;
    if (ctx && ctx->callback) {
        ctx->callback(ctx->arg);
    }
}

xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func, xy_os_timer_type_t type,
                                 void *argument, const xy_os_timer_attr_t *attr)
{
    timer_ctx_t *ctx;
    rt_uint8_t flags;

    if (!func) return NULL;
    if (type != XY_OS_TIMER_ONCE && type != XY_OS_TIMER_PERIODIC) return NULL;

    ctx = alloc_timer_ctx();
    if (!ctx) return NULL;

    ctx->initialized = 1;
    ctx->callback = func;
    ctx->arg = argument;

    const char *name = "timer";
    if (attr && attr->name) {
        name = attr->name;
    }

    flags = RT_TIMER_FLAG_SOFT_TIMER;
    flags |= (type == XY_OS_TIMER_PERIODIC) ? RT_TIMER_FLAG_PERIODIC : RT_TIMER_FLAG_ONE_SHOT;

    rt_timer_t timer = rt_timer_create(name,
                                       timer_callback,
                                       ctx,
                                       1000, // Will be set later
                                       flags);

    if (!timer) {
        clear_timer_ctx(ctx);
        return NULL;
    }

    ctx->timer = timer;
    return (xy_os_timer_id_t)timer;
}

const char *xy_os_timer_get_name(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    if (!timer || !find_timer_ctx(timer)) return NULL;

    return timer->parent.name;
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    rt_err_t result;

    if (!timer || ticks == 0U) return XY_OS_ERROR_PARAMETER;
    if (!find_timer_ctx(timer)) return XY_OS_ERROR_PARAMETER;

    result = rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME, &ticks);
    if (result != RT_EOK) return XY_OS_ERROR;

    result = rt_timer_start(timer);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    if (!timer) return XY_OS_ERROR_PARAMETER;
    if (!find_timer_ctx(timer)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = rt_timer_stop(timer);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    rt_uint32_t state = RT_TIMER_FLAG_DEACTIVATED;

    if (!timer || !find_timer_ctx(timer)) return 0;

    if (rt_timer_control(timer, RT_TIMER_CTRL_GET_STATE, &state) != RT_EOK) return 0;
    return (state == RT_TIMER_FLAG_ACTIVATED) ? 1U : 0U;
}

xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t timer_id)
{
    rt_timer_t timer = (rt_timer_t)timer_id;
    timer_ctx_t *ctx;
    rt_err_t result;

    if (!timer) return XY_OS_ERROR_PARAMETER;

    ctx = find_timer_ctx(timer);
    if (!ctx) return XY_OS_ERROR_PARAMETER;

    result = rt_timer_delete(timer);
    if (result != RT_EOK) return XY_OS_ERROR;

    clear_timer_ctx(ctx);
    return XY_OS_OK;
}

/* Event Flags Functions */
// Use RT-Thread event group

static int rtthread_event_flags_args_valid(uint32_t flags, uint32_t options)
{
    if (flags == 0U || (flags & 0x80000000U)) {
        return 0;
    }
    if (options & ~(XY_OS_FLAGS_WAIT_ALL | XY_OS_FLAGS_NO_CLEAR)) {
        return 0;
    }
    return 1;
}

static uint32_t rtthread_event_flags_snapshot(rt_event_t event)
{
    rt_base_t level;
    rt_uint32_t current;

    level = rt_spin_lock_irqsave(&event->spinlock);
    current = event->set;
    rt_spin_unlock_irqrestore(&event->spinlock, level);
    return current;
}

xy_os_event_flags_id_t xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    const char *name = "event";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;
    event_flags_ctx_t *ctx;
    rt_event_t event;

    if (attr && attr->name) {
        name = attr->name;
    }

    ctx = alloc_event_flags_ctx();
    if (!ctx) return NULL;

    ctx->initialized = 1;
    event = rt_event_create(name, flag);
    if (!event) {
        clear_event_flags_ctx(ctx);
        return NULL;
    }

    ctx->event = event;
    return (xy_os_event_flags_id_t)event;
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!find_event_flags_ctx(event)) return NULL;

    return event->parent.parent.name;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!find_event_flags_ctx(event)) return 0x80000000U;
    if (!rtthread_event_flags_args_valid(flags, 0U)) return 0x80000000U;

    rt_err_t result = rt_event_send(event, flags);
    if (result != RT_EOK) return 0x80000000;
    return rtthread_event_flags_snapshot(event);
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    rt_event_t event = (rt_event_t)ef_id;
    rt_base_t level;
    rt_uint32_t previous;

    if (!find_event_flags_ctx(event)) return 0x80000000U;
    if (!rtthread_event_flags_args_valid(flags, 0U)) return 0x80000000U;

    level = rt_spin_lock_irqsave(&event->spinlock);
    previous = event->set;
    event->set &= ~flags;
    rt_spin_unlock_irqrestore(&event->spinlock, level);
    return previous;
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!find_event_flags_ctx(event)) return 0x80000000U;

    return rtthread_event_flags_snapshot(event);
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    rt_event_t event = (rt_event_t)ef_id;
    if (!find_event_flags_ctx(event)) return 0x80000000U;
    if (!rtthread_event_flags_args_valid(flags, options)) return 0x80000000U;

    rt_uint32_t recved;
    rt_uint8_t opt = RT_EVENT_FLAG_AND;
    if (options & XY_OS_FLAGS_WAIT_ALL) {
        opt = RT_EVENT_FLAG_AND;
    } else {
        opt = RT_EVENT_FLAG_OR;
    }

    if (!(options & XY_OS_FLAGS_NO_CLEAR)) {
        opt |= RT_EVENT_FLAG_CLEAR;
    }

    rt_err_t result = rt_event_recv(event, flags, opt, timeout, &recved);
    return (result == RT_EOK) ? recved : 0x80000000;
}

xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id)
{
    rt_event_t event = (rt_event_t)ef_id;
    event_flags_ctx_t *ctx;

    ctx = find_event_flags_ctx(event);
    if (!ctx) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = rt_event_delete(event);
    if (result == RT_EOK) {
        clear_event_flags_ctx(ctx);
        return XY_OS_OK;
    }
    return XY_OS_ERROR;
}

/* Mutex Functions */

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    const char *name = "mutex";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;
    mutex_ctx_t *ctx;
    rt_mutex_t mutex;

    if (attr && attr->name) {
        name = attr->name;
    }

    ctx = alloc_mutex_ctx();
    if (!ctx) return NULL;

    ctx->initialized = 1;
    mutex = rt_mutex_create(name, flag);
    if (!mutex) {
        clear_mutex_ctx(ctx);
        return NULL;
    }

    ctx->mutex = mutex;
    return (xy_os_mutex_id_t)mutex;
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex || !find_mutex_ctx(mutex)) return NULL;

    return mutex->parent.parent.name;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex || !find_mutex_ctx(mutex)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = (timeout == XY_OS_WAIT_FOREVER) ?
                      rt_mutex_take(mutex, RT_WAITING_FOREVER) :
                      rt_mutex_take(mutex, timeout);

    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex || !find_mutex_ctx(mutex)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = rt_mutex_release(mutex);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    if (!mutex || !find_mutex_ctx(mutex)) return NULL;

    return (xy_os_thread_id_t)mutex->owner;
}

xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t mutex = (rt_mutex_t)mutex_id;
    mutex_ctx_t *ctx;
    rt_err_t result;

    if (!mutex) return XY_OS_ERROR_PARAMETER;

    ctx = find_mutex_ctx(mutex);
    if (!ctx) return XY_OS_ERROR_PARAMETER;

    result = rt_mutex_delete(mutex);
    if (result != RT_EOK) return XY_OS_ERROR;

    clear_mutex_ctx(ctx);
    return XY_OS_OK;
}

/* Semaphore Functions */

xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count, uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    const char *name = "sem";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;
    sem_ctx_t *ctx;
    rt_sem_t sem;

    if (max_count == 0U || initial_count > max_count) return NULL;

    if (attr && attr->name) {
        name = attr->name;
    }

    ctx = alloc_sem_ctx();
    if (!ctx) return NULL;

    ctx->initialized = 1;
    sem = rt_sem_create(name, initial_count, flag);
    if (!sem) {
        clear_sem_ctx(ctx);
        return NULL;
    }

    ctx->sem = sem;
    return (xy_os_semaphore_id_t)sem;
}

const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem || !find_sem_ctx(sem)) return NULL;

    return sem->parent.parent.name;
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id, uint32_t timeout)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem || !find_sem_ctx(sem)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = (timeout == XY_OS_WAIT_FOREVER) ?
                      rt_sem_take(sem, RT_WAITING_FOREVER) :
                      rt_sem_take(sem, timeout);

    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem || !find_sem_ctx(sem)) return XY_OS_ERROR_PARAMETER;

    rt_err_t result = rt_sem_release(sem);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_semaphore_release_from_isr(xy_os_semaphore_id_t semaphore_id)
{
    return xy_os_semaphore_release(semaphore_id);
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    if (!sem || !find_sem_ctx(sem)) return 0;

    return sem->value;
}

xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t sem = (rt_sem_t)semaphore_id;
    sem_ctx_t *ctx;
    rt_err_t result;

    if (!sem) return XY_OS_ERROR_PARAMETER;

    ctx = find_sem_ctx(sem);
    if (!ctx) return XY_OS_ERROR_PARAMETER;

    result = rt_sem_delete(sem);
    if (result != RT_EOK) return XY_OS_ERROR;

    clear_sem_ctx(ctx);
    return XY_OS_OK;
}

/* Memory Pool Functions */

xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    const char *name = "memp";
    mempool_ctx_t *ctx;
    rt_mp_t mp;

    if (block_count == 0U || block_size == 0U) return NULL;
    if (attr && attr->name) {
        name = attr->name;
    }

    ctx = alloc_mempool_ctx();
    if (!ctx) return NULL;

    ctx->initialized = 1;
    mp = rt_mp_create(name, block_count, block_size);
    if (!mp) {
        clear_mempool_ctx(ctx);
        return NULL;
    }

    ctx->mp = mp;
    return (xy_os_mempool_id_t)mp;
}

const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !find_mempool_ctx(mp)) return NULL;

    return mp->parent.name;
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !find_mempool_ctx(mp)) return NULL;

    void *block = RT_NULL;
    if (timeout == XY_OS_WAIT_FOREVER) {
        block = rt_mp_alloc(mp, RT_WAITING_FOREVER);
    } else {
        block = rt_mp_alloc(mp, timeout);
    }

    return block;
}

xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp_id, void *block)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !block || !find_mempool_ctx(mp)) return XY_OS_ERROR_PARAMETER;

    rt_mp_free(block);
    return XY_OS_OK;
}

uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !find_mempool_ctx(mp)) return 0;

    return mp->block_total_count;
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !find_mempool_ctx(mp)) return 0;

    return mp->block_size;
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !find_mempool_ctx(mp)) return 0;

    return mp->block_total_count - mp->block_free_count;
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    if (!mp || !find_mempool_ctx(mp)) return 0;

    return mp->block_free_count;
}

xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    mempool_ctx_t *ctx;
    rt_err_t result;

    if (!mp) return XY_OS_ERROR_PARAMETER;

    ctx = find_mempool_ctx(mp);
    if (!ctx) return XY_OS_ERROR_PARAMETER;

    result = rt_mp_delete(mp);
    if (result != RT_EOK) return XY_OS_ERROR;

    clear_mempool_ctx(ctx);
    return XY_OS_OK;
}

/* Message Queue Functions */

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    const char *name = "msgq";
    rt_uint8_t flag = RT_IPC_FLAG_PRIO;
    msgqueue_ctx_t *ctx;
    rt_mq_t mq;

    if (msg_count == 0U || msg_size == 0U) return NULL;

    if (attr && attr->name) {
        name = attr->name;
    }

    ctx = alloc_msgqueue_ctx();
    if (!ctx) return NULL;

    ctx->initialized = 1;
    mq = rt_mq_create(name, msg_size, msg_count, flag);
    if (!mq) {
        clear_msgqueue_ctx(ctx);
        return NULL;
    }

    ctx->mq = mq;
    return (xy_os_msgqueue_id_t)mq;
}

const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !find_msgqueue_ctx(mq)) return NULL;

    return mq->parent.parent.name;
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                 const void *msg_ptr, uint8_t msg_prio,
                                 uint32_t timeout)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    rt_err_t result;

    if (!mq || !msg_ptr || !find_msgqueue_ctx(mq)) return XY_OS_ERROR_PARAMETER;
    XY_UNUSED(msg_prio);

    result = (timeout == XY_OS_WAIT_FOREVER)
                 ? rt_mq_send_wait(mq, msg_ptr, mq->msg_size, RT_WAITING_FOREVER)
                 : ((timeout == 0U) ? rt_mq_send(mq, msg_ptr, mq->msg_size)
                                    : rt_mq_send_wait(mq, msg_ptr, mq->msg_size, timeout));

    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                 uint8_t *msg_prio, uint32_t timeout)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    rt_ssize_t received;

    if (!mq || !msg_ptr || !find_msgqueue_ctx(mq)) return XY_OS_ERROR_PARAMETER;

    received = rt_mq_recv(mq, msg_ptr, mq->msg_size,
                          (timeout == XY_OS_WAIT_FOREVER) ? RT_WAITING_FOREVER : timeout);
    if (received != (rt_ssize_t)mq->msg_size) {
        return XY_OS_ERROR;
    }
    if (msg_prio) {
        *msg_prio = 0U;
    }
    return XY_OS_OK;
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !find_msgqueue_ctx(mq)) return 0;

    return mq->max_msgs;
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !find_msgqueue_ctx(mq)) return 0;

    return mq->msg_size;
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !find_msgqueue_ctx(mq)) return 0;

    return mq->entry;
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !find_msgqueue_ctx(mq)) return 0;

    return mq->max_msgs - mq->entry;
}

xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !find_msgqueue_ctx(mq)) return XY_OS_ERROR_PARAMETER;

    // Clear all messages in queue
    rt_err_t result = rt_mq_control(mq, RT_MQ_CTRL_RESET, RT_NULL);
    return (result == RT_EOK) ? XY_OS_OK : XY_OS_ERROR;
}

xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    msgqueue_ctx_t *ctx;
    rt_err_t result;

    if (!mq) return XY_OS_ERROR_PARAMETER;

    ctx = find_msgqueue_ctx(mq);
    if (!ctx) return XY_OS_ERROR_PARAMETER;

    result = rt_mq_delete(mq);
    if (result != RT_EOK) return XY_OS_ERROR;

    clear_msgqueue_ctx(ctx);
    return XY_OS_OK;
}

#endif /* STM32U5 || STM32U5xx */
