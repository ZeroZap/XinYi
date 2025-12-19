/**
 * @file xy_os_rtthread.c
 * @brief XY OSAL RT-Thread Implementation
 * @version 1.0.0
 */

#include "../xy_os.h"
#include <rtthread.h>
#include <string.h>

/* Priority mapping: RT-Thread inverted (0=highest), XY OSAL normal (56=highest)
 */
static rt_uint8_t xy_to_rt_priority(xy_os_priority_t xy_prio)
{
    rt_uint8_t rt_prio = (RT_THREAD_PRIORITY_MAX - 1) - ((uint8_t)xy_prio);
    if (rt_prio >= RT_THREAD_PRIORITY_MAX)
        rt_prio = RT_THREAD_PRIORITY_MAX - 1;
    return rt_prio;
}

static xy_os_priority_t rt_to_xy_priority(rt_uint8_t rt_prio)
{
    return (xy_os_priority_t)((RT_THREAD_PRIORITY_MAX - 1) - rt_prio);
}

static xy_os_status_t rt_err_to_xy(rt_err_t err)
{
    switch (err) {
    case RT_EOK:
        return XY_OS_OK;
    case -RT_ETIMEOUT:
        return XY_OS_ERROR_TIMEOUT;
    case -RT_ENOMEM:
        return XY_OS_ERROR_NO_MEMORY;
    case -RT_EINVAL:
        return XY_OS_ERROR_PARAMETER;
    default:
        return XY_OS_ERROR;
    }
}

/* Kernel */
xy_os_status_t xy_os_kernel_init(void)
{
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                     uint32_t id_size)
{
    if (version) {
        version->api    = (1 << 16);
        version->kernel = (rt_uint32_t)RT_VERSION;
    }
    if (id_buf && id_size > 0) {
        rt_strncpy(id_buf, "RT-Thread", id_size - 1);
        id_buf[id_size - 1] = '\0';
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    return XY_OS_KERNEL_RUNNING;
}
xy_os_status_t xy_os_kernel_start(void)
{
    return XY_OS_OK;
}
int32_t xy_os_kernel_lock(void)
{
    return (int32_t)rt_enter_critical();
}
int32_t xy_os_kernel_unlock(void)
{
    rt_exit_critical();
    return 0;
}
int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    (void)lock;
    return 0;
}
uint32_t xy_os_kernel_get_tick_count(void)
{
    return (uint32_t)rt_tick_get();
}
uint32_t xy_os_kernel_get_tick_freq(void)
{
    return (uint32_t)RT_TICK_PER_SECOND;
}
uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return (uint32_t)rt_tick_get();
}
uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return (uint32_t)RT_TICK_PER_SECOND;
}

/* Thread */
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr)
{
    if (!func)
        return NULL;
    const char *name  = (attr && attr->name) ? attr->name : "thread";
    rt_uint32_t stack = (attr && attr->stack_size) ? attr->stack_size : 1024;
    rt_uint8_t prio =
        attr ? xy_to_rt_priority(attr->priority) : (RT_THREAD_PRIORITY_MAX / 2);
    rt_thread_t thread = rt_thread_create(
        name, (void (*)(void *))func, argument, stack, prio, 10);
    if (thread)
        rt_thread_startup(thread);
    return (xy_os_thread_id_t)thread;
}

const char *xy_os_thread_get_name(xy_os_thread_id_t thread_id)
{
    rt_thread_t t = thread_id ? (rt_thread_t)thread_id : rt_thread_self();
    return t ? t->name : NULL;
}

xy_os_thread_id_t xy_os_thread_get_id(void)
{
    return (xy_os_thread_id_t)rt_thread_self();
}

xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t thread_id)
{
    rt_thread_t t = (rt_thread_t)thread_id;
    if (!t)
        return XY_OS_THREAD_ERROR;
    switch (t->stat) {
    case RT_THREAD_READY:
        return XY_OS_THREAD_READY;
    case RT_THREAD_RUNNING:
        return XY_OS_THREAD_RUNNING;
    case RT_THREAD_SUSPEND:
        return XY_OS_THREAD_BLOCKED;
    case RT_THREAD_CLOSE:
        return XY_OS_THREAD_TERMINATED;
    default:
        return XY_OS_THREAD_ERROR;
    }
}

uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t thread_id)
{
    rt_thread_t t = (rt_thread_t)thread_id;
    return t ? t->stack_size : 0;
}

uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id)
{
    (void)thread_id;
    return 0;
}

xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                         xy_os_priority_t priority)
{
    rt_thread_t t = (rt_thread_t)thread_id;
    if (!t)
        return XY_OS_ERROR_PARAMETER;
    rt_uint8_t prio = xy_to_rt_priority(priority);
    return rt_err_to_xy(
        rt_thread_control(t, RT_THREAD_CTRL_CHANGE_PRIORITY, &prio));
}

xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t thread_id)
{
    rt_thread_t t = (rt_thread_t)thread_id;
    return t ? rt_to_xy_priority(t->current_priority) : XY_OS_PRIORITY_ERROR;
}

xy_os_status_t xy_os_thread_yield(void)
{
    rt_thread_yield();
    return XY_OS_OK;
}
xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t thread_id)
{
    return rt_err_to_xy(rt_thread_suspend((rt_thread_t)thread_id));
}
xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t thread_id)
{
    return rt_err_to_xy(rt_thread_resume((rt_thread_t)thread_id));
}
xy_os_status_t xy_os_thread_detach(xy_os_thread_id_t thread_id)
{
    (void)thread_id;
    return XY_OS_OK;
}
xy_os_status_t xy_os_thread_join(xy_os_thread_id_t thread_id)
{
    (void)thread_id;
    return XY_OS_ERROR;
}
void xy_os_thread_exit(void)
{
    rt_thread_delete(rt_thread_self());
    while (1)
        ;
}
xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t thread_id)
{
    return rt_err_to_xy(rt_thread_delete((rt_thread_t)thread_id));
}
uint32_t xy_os_thread_get_count(void)
{
    return 0;
}
uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *thread_array,
                                uint32_t array_items)
{
    (void)thread_array;
    (void)array_items;
    return 0;
}

/* Thread Flags - using RT-Thread event */
uint32_t xy_os_thread_flags_set(xy_os_thread_id_t thread_id, uint32_t flags)
{
    rt_thread_t t = (rt_thread_t)thread_id;
    if (!t)
        return 0x80000000;
    rt_event_send(&t->event, flags);
    return flags;
}

uint32_t xy_os_thread_flags_clear(uint32_t flags)
{
    rt_thread_t t = rt_thread_self();
    if (!t)
        return 0;
    rt_event_recv(
        &t->event, flags, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 0, RT_NULL);
    return flags;
}

uint32_t xy_os_thread_flags_get(void)
{
    return 0;
}

uint32_t xy_os_thread_flags_wait(uint32_t flags, uint32_t options,
                                 uint32_t timeout)
{
    rt_thread_t t = rt_thread_self();
    if (!t)
        return 0x80000000;
    rt_uint8_t opt =
        (options & XY_OS_FLAGS_WAIT_ALL) ? RT_EVENT_FLAG_AND : RT_EVENT_FLAG_OR;
    if (!(options & XY_OS_FLAGS_NO_CLEAR))
        opt |= RT_EVENT_FLAG_CLEAR;
    rt_uint32_t recved;
    rt_err_t err = rt_event_recv(&t->event, flags, opt, timeout, &recved);
    return (err == RT_EOK) ? recved : 0x80000000;
}

/* Delay */
xy_os_status_t xy_os_delay(uint32_t ticks)
{
    rt_thread_delay(ticks);
    return XY_OS_OK;
}
xy_os_status_t xy_os_delay_until(uint32_t ticks)
{
    rt_tick_t now = rt_tick_get();
    if (ticks > now)
        rt_thread_delay(ticks - now);
    return XY_OS_OK;
}

/* Timer */
xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func,
                                 xy_os_timer_type_t type, void *argument,
                                 const xy_os_timer_attr_t *attr)
{
    if (!func)
        return NULL;
    const char *name = (attr && attr->name) ? attr->name : "timer";
    rt_uint8_t flag  = (type == XY_OS_TIMER_PERIODIC)
                           ? (RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER)
                           : (RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    return (xy_os_timer_id_t)rt_timer_create(
        name, (void (*)(void *))func, argument, 10, flag);
}

const char *xy_os_timer_get_name(xy_os_timer_id_t timer_id)
{
    rt_timer_t t = (rt_timer_t)timer_id;
    return t ? t->parent.name : NULL;
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks)
{
    rt_timer_t t = (rt_timer_t)timer_id;
    if (!t)
        return XY_OS_ERROR_PARAMETER;
    rt_timer_control(t, RT_TIMER_CTRL_SET_TIME, &ticks);
    return rt_err_to_xy(rt_timer_start(t));
}

xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t timer_id)
{
    return rt_err_to_xy(rt_timer_stop((rt_timer_t)timer_id));
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t timer_id)
{
    rt_timer_t t = (rt_timer_t)timer_id;
    return (t && (t->parent.flag & RT_TIMER_FLAG_ACTIVATED)) ? 1 : 0;
}

xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t timer_id)
{
    return rt_err_to_xy(rt_timer_delete((rt_timer_t)timer_id));
}

/* Event Flags */
xy_os_event_flags_id_t
xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    const char *name = (attr && attr->name) ? attr->name : "event";
    return (xy_os_event_flags_id_t)rt_event_create(name, RT_IPC_FLAG_FIFO);
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    rt_event_t e = (rt_event_t)ef_id;
    return e ? e->parent.parent.name : NULL;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    rt_event_t e = (rt_event_t)ef_id;
    if (!e)
        return 0x80000000;
    rt_event_send(e, flags);
    return e->set;
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    rt_event_t e = (rt_event_t)ef_id;
    if (!e)
        return 0;
    rt_uint32_t prev = e->set;
    e->set &= ~flags;
    return prev;
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    rt_event_t e = (rt_event_t)ef_id;
    return e ? e->set : 0;
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    rt_event_t e = (rt_event_t)ef_id;
    if (!e)
        return 0x80000000;
    rt_uint8_t opt =
        (options & XY_OS_FLAGS_WAIT_ALL) ? RT_EVENT_FLAG_AND : RT_EVENT_FLAG_OR;
    if (!(options & XY_OS_FLAGS_NO_CLEAR))
        opt |= RT_EVENT_FLAG_CLEAR;
    rt_uint32_t recved;
    rt_err_t err = rt_event_recv(e, flags, opt, timeout, &recved);
    return (err == RT_EOK) ? recved : 0x80000000;
}

xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id)
{
    return rt_err_to_xy(rt_event_delete((rt_event_t)ef_id));
}

/* Mutex */
xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    const char *name = (attr && attr->name) ? attr->name : "mutex";
    rt_uint8_t flag  = (attr && (attr->attr_bits & XY_OS_MUTEX_PRIO_INHERIT))
                           ? RT_IPC_FLAG_PRIO
                           : RT_IPC_FLAG_FIFO;
    return (xy_os_mutex_id_t)rt_mutex_create(name, flag);
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t m = (rt_mutex_t)mutex_id;
    return m ? m->parent.parent.name : NULL;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    return rt_err_to_xy(rt_mutex_take((rt_mutex_t)mutex_id, timeout));
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    return rt_err_to_xy(rt_mutex_release((rt_mutex_t)mutex_id));
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    rt_mutex_t m = (rt_mutex_t)mutex_id;
    return m ? (xy_os_thread_id_t)m->owner : NULL;
}

xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    return rt_err_to_xy(rt_mutex_delete((rt_mutex_t)mutex_id));
}

/* Semaphore */
xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count,
                                         uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    const char *name = (attr && attr->name) ? attr->name : "sem";
    (void)max_count;
    return (xy_os_semaphore_id_t)rt_sem_create(
        name, initial_count, RT_IPC_FLAG_FIFO);
}

const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t s = (rt_sem_t)semaphore_id;
    return s ? s->parent.parent.name : NULL;
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id,
                                       uint32_t timeout)
{
    return rt_err_to_xy(rt_sem_take((rt_sem_t)semaphore_id, timeout));
}

xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id)
{
    return rt_err_to_xy(rt_sem_release((rt_sem_t)semaphore_id));
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id)
{
    rt_sem_t s = (rt_sem_t)semaphore_id;
    return s ? s->value : 0;
}

xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id)
{
    return rt_err_to_xy(rt_sem_delete((rt_sem_t)semaphore_id));
}

/* Memory Pool */
xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    const char *name = (attr && attr->name) ? attr->name : "pool";
    return (xy_os_mempool_id_t)rt_mp_create(name, block_count, block_size);
}

const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    return mp ? mp->parent.name : NULL;
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout)
{
    return rt_mp_alloc((rt_mp_t)mp_id, timeout);
}

xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp_id, void *block)
{
    (void)mp_id;
    rt_mp_free(block);
    return XY_OS_OK;
}

uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    return mp ? mp->block_total_count : 0;
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    return mp ? mp->block_size : 0;
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    return mp ? (mp->block_total_count - mp->block_free_count) : 0;
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id)
{
    rt_mp_t mp = (rt_mp_t)mp_id;
    return mp ? mp->block_free_count : 0;
}

xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id)
{
    return rt_err_to_xy(rt_mp_delete((rt_mp_t)mp_id));
}

/* Message Queue */
xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    const char *name = (attr && attr->name) ? attr->name : "mq";
    return (xy_os_msgqueue_id_t)rt_mq_create(
        name, msg_size, msg_count, RT_IPC_FLAG_FIFO);
}

const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    return mq ? mq->parent.parent.name : NULL;
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                  const void *msg_ptr, uint8_t msg_prio,
                                  uint32_t timeout)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !msg_ptr)
        return XY_OS_ERROR_PARAMETER;
    (void)msg_prio;
    return rt_err_to_xy(rt_mq_send_wait(mq, msg_ptr, mq->msg_size, timeout));
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                  uint8_t *msg_prio, uint32_t timeout)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    if (!mq || !msg_ptr)
        return XY_OS_ERROR_PARAMETER;
    if (msg_prio)
        *msg_prio = 0;
    return rt_err_to_xy(rt_mq_recv(mq, msg_ptr, mq->msg_size, timeout));
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    return mq ? mq->max_msgs : 0;
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    return mq ? mq->msg_size : 0;
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    return mq ? mq->entry : 0;
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    rt_mq_t mq = (rt_mq_t)mq_id;
    return mq ? (mq->max_msgs - mq->entry) : 0;
}

xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    return rt_err_to_xy(
        rt_mq_control((rt_mq_t)mq_id, RT_IPC_CMD_RESET, RT_NULL));
}

xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    return rt_err_to_xy(rt_mq_delete((rt_mq_t)mq_id));
}
