/**
 * @file xy_os_freertos.c
 * @brief XY OSAL FreeRTOS Implementation
 * @version 1.0.0
 */

#include "../xy_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include <string.h>

/* Priority mapping: FreeRTOS 0=lowest, XY OSAL 0=lowest (same direction) */
static UBaseType_t xy_to_freertos_priority(xy_os_priority_t xy_prio)
{
    UBaseType_t prio = (UBaseType_t)xy_prio;
    if (prio >= configMAX_PRIORITIES)
        prio = configMAX_PRIORITIES - 1;
    return prio;
}

static xy_os_priority_t freertos_to_xy_priority(UBaseType_t fr_prio)
{
    return (xy_os_priority_t)fr_prio;
}

static xy_os_status_t pdstatus_to_xy(BaseType_t status)
{
    return (status == pdPASS) ? XY_OS_OK : XY_OS_ERROR;
}

/* Kernel Control */
xy_os_status_t xy_os_kernel_init(void)
{
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                     uint32_t id_size)
{
    if (version) {
        version->api = (1 << 16);
        version->kernel =
            (tskKERNEL_VERSION_MAJOR << 16) | (tskKERNEL_VERSION_MINOR << 8);
    }
    if (id_buf && id_size > 0) {
        strncpy(id_buf, "FreeRTOS", id_size - 1);
        id_buf[id_size - 1] = '\0';
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
               ? XY_OS_KERNEL_RUNNING
               : XY_OS_KERNEL_READY;
}

xy_os_status_t xy_os_kernel_start(void)
{
    vTaskStartScheduler();
    return XY_OS_OK;
}

int32_t xy_os_kernel_lock(void)
{
    vTaskSuspendAll();
    return 0;
}
int32_t xy_os_kernel_unlock(void)
{
    xTaskResumeAll();
    return 0;
}
int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    (void)lock;
    return 0;
}

uint32_t xy_os_kernel_get_tick_count(void)
{
    return (uint32_t)xTaskGetTickCount();
}
uint32_t xy_os_kernel_get_tick_freq(void)
{
    return configTICK_RATE_HZ;
}
uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return (uint32_t)xTaskGetTickCount();
}
uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return configTICK_RATE_HZ;
}

/* Thread Management */
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t func, void *argument,
                                   const xy_os_thread_attr_t *attr)
{
    if (!func)
        return NULL;
    const char *name = (attr && attr->name) ? attr->name : "task";
    uint16_t stack   = (attr && attr->stack_size)
                           ? (attr->stack_size / sizeof(StackType_t))
                           : 256;
    UBaseType_t prio =
        attr ? xy_to_freertos_priority(attr->priority) : (tskIDLE_PRIORITY + 1);
    TaskHandle_t handle;
    return (xTaskCreate(func, name, stack, argument, prio, &handle) == pdPASS)
               ? (xy_os_thread_id_t)handle
               : NULL;
}

const char *xy_os_thread_get_name(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h =
        thread_id ? (TaskHandle_t)thread_id : xTaskGetCurrentTaskHandle();
    return h ? pcTaskGetName(h) : NULL;
}

xy_os_thread_id_t xy_os_thread_get_id(void)
{
    return (xy_os_thread_id_t)xTaskGetCurrentTaskHandle();
}

xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h)
        return XY_OS_THREAD_ERROR;
    eTaskState s = eTaskGetState(h);
    switch (s) {
    case eReady:
        return XY_OS_THREAD_READY;
    case eRunning:
        return XY_OS_THREAD_RUNNING;
    case eBlocked:
    case eSuspended:
        return XY_OS_THREAD_BLOCKED;
    case eDeleted:
        return XY_OS_THREAD_TERMINATED;
    default:
        return XY_OS_THREAD_ERROR;
    }
}

uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t thread_id)
{
    (void)thread_id;
    return 0;
}
uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    return h ? (uint32_t)uxTaskGetStackHighWaterMark(h) : 0;
}

xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                         xy_os_priority_t priority)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h)
        return XY_OS_ERROR_PARAMETER;
    vTaskPrioritySet(h, xy_to_freertos_priority(priority));
    return XY_OS_OK;
}

xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    return h ? freertos_to_xy_priority(uxTaskPriorityGet(h))
             : XY_OS_PRIORITY_ERROR;
}

xy_os_status_t xy_os_thread_yield(void)
{
    taskYIELD();
    return XY_OS_OK;
}
xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h)
        return XY_OS_ERROR_PARAMETER;
    vTaskSuspend(h);
    return XY_OS_OK;
}

xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h)
        return XY_OS_ERROR_PARAMETER;
    vTaskResume(h);
    return XY_OS_OK;
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
    vTaskDelete(NULL);
    while (1)
        ;
}
xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t thread_id)
{
    vTaskDelete((TaskHandle_t)thread_id);
    return XY_OS_OK;
}

uint32_t xy_os_thread_get_count(void)
{
    return (uint32_t)uxTaskGetNumberOfTasks();
}
uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *thread_array,
                                uint32_t array_items)
{
    (void)thread_array;
    (void)array_items;
    return 0;
}

/* Thread Flags - using task notifications */
uint32_t xy_os_thread_flags_set(xy_os_thread_id_t thread_id, uint32_t flags)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    uint32_t previous;

    if (!h)
        return 0x80000000;
    if (xTaskNotifyAndQuery(h, flags, eSetBits, &previous) != pdPASS)
        return 0x80000000;
    return previous | flags;
}

uint32_t xy_os_thread_flags_clear(uint32_t flags)
{
    return ulTaskNotifyValueClear(NULL, flags);
}

uint32_t xy_os_thread_flags_get(void)
{
    uint32_t flags;

    if (xTaskNotifyAndQuery(xTaskGetCurrentTaskHandle(), 0, eNoAction, &flags) != pdPASS)
        return 0x80000000;
    return flags;
}

uint32_t xy_os_thread_flags_wait(uint32_t flags, uint32_t options,
                                 uint32_t timeout)
{
    uint32_t notif;
    TickType_t ticks =
        (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    BaseType_t clear = (options & XY_OS_FLAGS_NO_CLEAR) ? pdFALSE : pdTRUE;

    if (xTaskNotifyWait(0, clear ? flags : 0, &notif, ticks) == pdPASS) {
        if (options & XY_OS_FLAGS_WAIT_ALL) {
            return ((notif & flags) == flags) ? notif : 0x80000000;
        }
        return (notif & flags) ? notif : 0x80000000;
    }
    return 0x80000000;
}

/* Delay Functions */
xy_os_status_t xy_os_delay(uint32_t ticks)
{
    vTaskDelay((TickType_t)ticks);
    return XY_OS_OK;
}

xy_os_status_t xy_os_delay_until(uint32_t ticks)
{
    TickType_t wake_time = (TickType_t)ticks;
    vTaskDelayUntil(&wake_time, 0);
    return XY_OS_OK;
}

/* Timer Management */
xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func,
                                 xy_os_timer_type_t type, void *argument,
                                 const xy_os_timer_attr_t *attr)
{
    if (!func)
        return NULL;
    const char *name   = (attr && attr->name) ? attr->name : "timer";
    UBaseType_t reload = (type == XY_OS_TIMER_PERIODIC) ? pdTRUE : pdFALSE;
    return (xy_os_timer_id_t)xTimerCreate(
        name, 1, reload, argument, (TimerCallbackFunction_t)func);
}

const char *xy_os_timer_get_name(xy_os_timer_id_t timer_id)
{
    return timer_id ? pcTimerGetName((TimerHandle_t)timer_id) : NULL;
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks)
{
    TimerHandle_t t = (TimerHandle_t)timer_id;
    if (!t)
        return XY_OS_ERROR_PARAMETER;
    xTimerChangePeriod(t, (TickType_t)ticks, 0);
    return pdstatus_to_xy(xTimerStart(t, 0));
}

xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t timer_id)
{
    return timer_id ? pdstatus_to_xy(xTimerStop((TimerHandle_t)timer_id, 0))
                    : XY_OS_ERROR_PARAMETER;
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t timer_id)
{
    return (timer_id && xTimerIsTimerActive((TimerHandle_t)timer_id)) ? 1 : 0;
}

xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t timer_id)
{
    return timer_id ? pdstatus_to_xy(xTimerDelete((TimerHandle_t)timer_id, 0))
                    : XY_OS_ERROR_PARAMETER;
}

/* Event Flags */
xy_os_event_flags_id_t
xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    (void)attr;
    return (xy_os_event_flags_id_t)xEventGroupCreate();
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    (void)ef_id;
    return NULL;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    return ef_id ? (uint32_t)xEventGroupSetBits(
                       (EventGroupHandle_t)ef_id, (EventBits_t)flags)
                 : 0x80000000;
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    return ef_id ? (uint32_t)xEventGroupClearBits(
                       (EventGroupHandle_t)ef_id, (EventBits_t)flags)
                 : 0;
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    return ef_id ? (uint32_t)xEventGroupGetBits((EventGroupHandle_t)ef_id) : 0;
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    if (!ef_id)
        return 0x80000000;
    BaseType_t wait_all = (options & XY_OS_FLAGS_WAIT_ALL) ? pdTRUE : pdFALSE;
    BaseType_t clear    = (options & XY_OS_FLAGS_NO_CLEAR) ? pdFALSE : pdTRUE;
    TickType_t ticks =
        (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    EventBits_t bits = xEventGroupWaitBits(
        (EventGroupHandle_t)ef_id, (EventBits_t)flags, clear, wait_all, ticks);
    return (uint32_t)bits;
}

xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id)
{
    if (ef_id)
        vEventGroupDelete((EventGroupHandle_t)ef_id);
    return ef_id ? XY_OS_OK : XY_OS_ERROR_PARAMETER;
}

/* Mutex */
xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    SemaphoreHandle_t mutex;
    if (attr && (attr->attr_bits & XY_OS_MUTEX_RECURSIVE)) {
        mutex = xSemaphoreCreateRecursiveMutex();
    } else {
        mutex = xSemaphoreCreateMutex();
    }
    return (xy_os_mutex_id_t)mutex;
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    (void)mutex_id;
    return NULL;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    SemaphoreHandle_t m = (SemaphoreHandle_t)mutex_id;
    if (!m)
        return XY_OS_ERROR_PARAMETER;
    TickType_t ticks =
        (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    return pdstatus_to_xy(xSemaphoreTake(m, ticks));
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    return mutex_id
               ? pdstatus_to_xy(xSemaphoreGive((SemaphoreHandle_t)mutex_id))
               : XY_OS_ERROR_PARAMETER;
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    return mutex_id ? (xy_os_thread_id_t)xSemaphoreGetMutexHolder(
                          (SemaphoreHandle_t)mutex_id)
                    : NULL;
}

xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    if (mutex_id)
        vSemaphoreDelete((SemaphoreHandle_t)mutex_id);
    return mutex_id ? XY_OS_OK : XY_OS_ERROR_PARAMETER;
}

/* Semaphore */
xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count,
                                         uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    (void)attr;
    SemaphoreHandle_t sem;
    if (max_count == 1) {
        sem = xSemaphoreCreateBinary();
        if (sem && initial_count > 0)
            xSemaphoreGive(sem);
    } else {
        sem = xSemaphoreCreateCounting(
            (UBaseType_t)max_count, (UBaseType_t)initial_count);
    }
    return (xy_os_semaphore_id_t)sem;
}

const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id)
{
    (void)semaphore_id;
    return NULL;
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id,
                                       uint32_t timeout)
{
    SemaphoreHandle_t s = (SemaphoreHandle_t)semaphore_id;
    if (!s)
        return XY_OS_ERROR_PARAMETER;
    TickType_t ticks =
        (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    return pdstatus_to_xy(xSemaphoreTake(s, ticks));
}

xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id)
{
    return semaphore_id
               ? pdstatus_to_xy(xSemaphoreGive((SemaphoreHandle_t)semaphore_id))
               : XY_OS_ERROR_PARAMETER;
}

xy_os_status_t xy_os_semaphore_release_from_isr(xy_os_semaphore_id_t semaphore_id)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (!semaphore_id)
        return XY_OS_ERROR_PARAMETER;
    if (xSemaphoreGiveFromISR((SemaphoreHandle_t)semaphore_id,
                              &higher_priority_task_woken) != pdPASS)
        return XY_OS_ERROR_RESOURCE;
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return XY_OS_OK;
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id)
{
    return semaphore_id
               ? (uint32_t)uxSemaphoreGetCount((SemaphoreHandle_t)semaphore_id)
               : 0;
}

xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id)
{
    if (semaphore_id)
        vSemaphoreDelete((SemaphoreHandle_t)semaphore_id);
    return semaphore_id ? XY_OS_OK : XY_OS_ERROR_PARAMETER;
}

/* Memory Pool — free-list allocator on top of pvPortMalloc */

typedef struct {
    uint32_t  capacity;
    uint32_t  block_size;
    volatile uint32_t free_count;
    void     *free_list;
    char      name[16];
    /* pool memory follows either from pvPortMalloc or attr->mp_mem */
} fr_pool_t;

xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    if (block_count == 0 || block_size == 0) return NULL;
    uint32_t bsz = (block_size < sizeof(void *))
                   ? (uint32_t)sizeof(void *) : block_size;

    fr_pool_t *p = (fr_pool_t *)pvPortMalloc(sizeof(fr_pool_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));
    p->capacity   = block_count;
    p->block_size = bsz;
    if (attr && attr->name) {
        strncpy(p->name, attr->name, sizeof(p->name) - 1);
    }

    uint8_t *mem;
    if (attr && attr->mp_mem && attr->mp_size >= block_count * bsz) {
        mem = (uint8_t *)attr->mp_mem;
    } else {
        mem = (uint8_t *)pvPortMalloc(block_count * bsz);
        if (!mem) { vPortFree(p); return NULL; }
    }

    /* Build free-list */
    p->free_list  = NULL;
    p->free_count = 0;
    for (uint32_t i = block_count; i > 0; i--) {
        void *blk   = mem + (i - 1) * bsz;
        *(void **)blk = p->free_list;
        p->free_list  = blk;
        p->free_count++;
    }

    return (xy_os_mempool_id_t)p;
}

const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    return p ? p->name : NULL;
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    if (!p) return NULL;
    TickType_t ticks = (timeout == XY_OS_WAIT_FOREVER)
                       ? portMAX_DELAY : (TickType_t)timeout;
    TickType_t start = xTaskGetTickCount();

    while (1) {
        taskENTER_CRITICAL();
        if (p->free_list) {
            void *blk    = p->free_list;
            p->free_list = *(void **)blk;
            p->free_count--;
            taskEXIT_CRITICAL();
            return blk;
        }
        taskEXIT_CRITICAL();
        if (timeout == 0) return NULL;
        if (timeout != XY_OS_WAIT_FOREVER &&
            (xTaskGetTickCount() - start) >= ticks)
            return NULL;
        taskYIELD();
    }
}

xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp_id, void *block)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    if (!p || !block) return XY_OS_ERROR_PARAMETER;
    taskENTER_CRITICAL();
    *(void **)block = p->free_list;
    p->free_list    = block;
    p->free_count++;
    taskEXIT_CRITICAL();
    return XY_OS_OK;
}

uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    return p ? p->capacity : 0;
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    return p ? p->block_size : 0;
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    if (!p) return 0;
    taskENTER_CRITICAL();
    uint32_t used = p->capacity - p->free_count;
    taskEXIT_CRITICAL();
    return used;
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    if (!p) return 0;
    taskENTER_CRITICAL();
    uint32_t free = p->free_count;
    taskEXIT_CRITICAL();
    return free;
}

xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    if (!p) return XY_OS_ERROR_PARAMETER;
    vPortFree(p);
    return XY_OS_OK;
}

/* Message Queue */
xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    (void)attr;
    return (xy_os_msgqueue_id_t)xQueueCreate(
        (UBaseType_t)msg_count, (UBaseType_t)msg_size);
}

const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id)
{
    (void)mq_id;
    return NULL;
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                  const void *msg_ptr, uint8_t msg_prio,
                                  uint32_t timeout)
{
    QueueHandle_t q = (QueueHandle_t)mq_id;
    if (!q || !msg_ptr)
        return XY_OS_ERROR_PARAMETER;
    (void)msg_prio;
    TickType_t ticks =
        (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    return pdstatus_to_xy(xQueueSendToBack(q, msg_ptr, ticks));
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                  uint8_t *msg_prio, uint32_t timeout)
{
    QueueHandle_t q = (QueueHandle_t)mq_id;
    if (!q || !msg_ptr)
        return XY_OS_ERROR_PARAMETER;
    if (msg_prio)
        *msg_prio = 0;
    TickType_t ticks =
        (timeout == XY_OS_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)timeout;
    return pdstatus_to_xy(xQueueReceive(q, msg_ptr, ticks));
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    return mq_id ? (uint32_t)uxQueueSpacesAvailable((QueueHandle_t)mq_id)
                       + uxQueueMessagesWaiting((QueueHandle_t)mq_id)
                 : 0;
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    (void)mq_id;
    return 0;
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    return mq_id ? (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)mq_id) : 0;
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    return mq_id ? (uint32_t)uxQueueSpacesAvailable((QueueHandle_t)mq_id) : 0;
}

xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    return mq_id ? pdstatus_to_xy(xQueueReset((QueueHandle_t)mq_id))
                 : XY_OS_ERROR_PARAMETER;
}

xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    if (mq_id)
        vQueueDelete((QueueHandle_t)mq_id);
    return mq_id ? XY_OS_OK : XY_OS_ERROR_PARAMETER;
}
