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

/* Priority mapping: both FreeRTOS and XY OSAL increase toward higher priority. */
static int freertos_priority_valid(xy_os_priority_t priority)
{
    return priority >= XY_OS_PRIORITY_IDLE && priority <= XY_OS_PRIORITY_ISR;
}

static UBaseType_t xy_to_freertos_priority(xy_os_priority_t priority)
{
    const uint32_t freertos_span = (uint32_t)configMAX_PRIORITIES - 1U;
    const uint32_t xy_span =
        (uint32_t)XY_OS_PRIORITY_ISR - (uint32_t)XY_OS_PRIORITY_IDLE;
    const uint32_t offset =
        (uint32_t)priority - (uint32_t)XY_OS_PRIORITY_IDLE;

    return (UBaseType_t)((offset * freertos_span) / xy_span);
}

static xy_os_priority_t freertos_to_xy_priority(UBaseType_t priority)
{
    const uint32_t freertos_span = (uint32_t)configMAX_PRIORITIES - 1U;
    const uint32_t xy_span =
        (uint32_t)XY_OS_PRIORITY_ISR - (uint32_t)XY_OS_PRIORITY_IDLE;

    if ((uint32_t)priority > freertos_span) {
        return XY_OS_PRIORITY_ERROR;
    }

    return (xy_os_priority_t)((uint32_t)XY_OS_PRIORITY_IDLE +
                              (((uint32_t)priority * xy_span) / freertos_span));
}

static xy_os_status_t pdstatus_to_xy(BaseType_t status)
{
    return (status == pdPASS) ? XY_OS_OK : XY_OS_ERROR;
}

static xy_os_status_t pdstatus_to_xy_wait(BaseType_t status)
{
    if (status == pdPASS)
        return XY_OS_OK;
    return XY_OS_ERROR_TIMEOUT;
}

static int freertos_tick_value_valid(uint32_t ticks)
{
    return ticks <= (uint32_t)((TickType_t)-1);
}

static int freertos_timeout_to_ticks(uint32_t timeout, TickType_t *ticks)
{
    if (!ticks)
        return 0;
    if (timeout == XY_OS_WAIT_FOREVER) {
        *ticks = portMAX_DELAY;
        return 1;
    }
    if (!freertos_tick_value_valid(timeout))
        return 0;
    *ticks = (TickType_t)timeout;
    return 1;
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
    BaseType_t state = xTaskGetSchedulerState();

    if (state == taskSCHEDULER_NOT_STARTED)
        return XY_OS_KERNEL_READY;
    if (state == taskSCHEDULER_RUNNING)
        return XY_OS_KERNEL_RUNNING;
    if (state == taskSCHEDULER_SUSPENDED)
        return XY_OS_KERNEL_LOCKED;
    return XY_OS_KERNEL_ERROR;
}

xy_os_status_t xy_os_kernel_start(void)
{
    vTaskStartScheduler();
    return XY_OS_ERROR;
}

int32_t xy_os_kernel_lock(void)
{
    BaseType_t state = xTaskGetSchedulerState();

    if (state == taskSCHEDULER_SUSPENDED)
        return 1;
    if (state != taskSCHEDULER_RUNNING)
        return -1;
    vTaskSuspendAll();
    return 0;
}
int32_t xy_os_kernel_unlock(void)
{
    BaseType_t state = xTaskGetSchedulerState();

    if (state == taskSCHEDULER_RUNNING)
        return 0;
    if (state != taskSCHEDULER_SUSPENDED)
        return -1;
    (void)xTaskResumeAll();
    return 1;
}
int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    BaseType_t state;

    if (lock != 0 && lock != 1)
        return -1;
    state = xTaskGetSchedulerState();
    if (state != taskSCHEDULER_RUNNING && state != taskSCHEDULER_SUSPENDED)
        return -1;
    if (lock == 1 && state == taskSCHEDULER_RUNNING)
        vTaskSuspendAll();
    else if (lock == 0 && state == taskSCHEDULER_SUSPENDED)
        (void)xTaskResumeAll();
    return lock;
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
    configSTACK_DEPTH_TYPE stack = 256U;

    if (!func)
        return NULL;
    if (attr) {
        uint32_t stack_words;

        if (!freertos_priority_valid(attr->priority) ||
            attr->attr_bits != XY_OS_THREAD_DETACHED || attr->cb_mem ||
            attr->cb_size != 0U || attr->stack_mem || attr->tz_module != 0U ||
            attr->reserved != 0U)
            return NULL;
        if (attr->stack_size != 0U) {
            if (attr->stack_size % sizeof(StackType_t))
                return NULL;
            stack_words = attr->stack_size / sizeof(StackType_t);
            if (stack_words == 0U ||
                stack_words > (uint32_t)((configSTACK_DEPTH_TYPE)-1))
                return NULL;
            stack = (configSTACK_DEPTH_TYPE)stack_words;
        }
    }
    const char *name = (attr && attr->name) ? attr->name : "task";
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
    /* FreeRTOS does not expose a portable per-task configured stack depth
       unless trace metadata is enabled; zero is the documented unavailable value. */
    (void)thread_id;
    return 0U;
}
uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    return h ? (uint32_t)uxTaskGetStackHighWaterMark(h) * sizeof(StackType_t) : 0;
}

xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                         xy_os_priority_t priority)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h || !freertos_priority_valid(priority))
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

/* FreeRTOS has no native detach/join ownership contract for this adapter. */
xy_os_status_t xy_os_thread_detach(xy_os_thread_id_t thread_id)
{
    return thread_id ? XY_OS_ERROR : XY_OS_ERROR_PARAMETER;
}
xy_os_status_t xy_os_thread_join(xy_os_thread_id_t thread_id)
{
    return thread_id ? XY_OS_ERROR : XY_OS_ERROR_PARAMETER;
}
void xy_os_thread_exit(void)
{
    vTaskDelete(NULL);
    while (1)
        ;
}
xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t thread_id)
{
    /* FreeRTOS provides no status/retirement observation for vTaskDelete(). */
    return thread_id ? XY_OS_ERROR : XY_OS_ERROR_PARAMETER;
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
static int freertos_thread_flags_args_valid(uint32_t flags, uint32_t options)
{
    if (flags == 0U || (flags & 0x80000000U))
        return 0;
    if (options & ~(XY_OS_FLAGS_WAIT_ALL | XY_OS_FLAGS_NO_CLEAR))
        return 0;
    return 1;
}

uint32_t xy_os_thread_flags_set(xy_os_thread_id_t thread_id, uint32_t flags)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    uint32_t previous;

    if (!h || !freertos_thread_flags_args_valid(flags, 0U))
        return 0x80000000;
    if (xTaskNotifyAndQuery(h, flags, eSetBits, &previous) != pdPASS)
        return 0x80000000;
    return previous | flags;
}

uint32_t xy_os_thread_flags_clear(uint32_t flags)
{
    if (!freertos_thread_flags_args_valid(flags, 0U))
        return 0x80000000;
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
    TickType_t ticks;
    TickType_t start;

    if (!freertos_timeout_to_ticks(timeout, &ticks) ||
        !freertos_thread_flags_args_valid(flags, options))
        return 0x80000000;
    BaseType_t wait_all = (options & XY_OS_FLAGS_WAIT_ALL) ? pdTRUE : pdFALSE;
    BaseType_t clear = (options & XY_OS_FLAGS_NO_CLEAR) ? pdFALSE : pdTRUE;

    start = xTaskGetTickCount();
    for (;;) {
        if (xTaskNotifyWait(0, 0, &notif, ticks) != pdPASS)
            return 0x80000000;
        if ((!wait_all && (notif & flags)) ||
            (wait_all && ((notif & flags) == flags))) {
            if (clear)
                (void)ulTaskNotifyValueClear(NULL, flags);
            return notif;
        }
        if (!wait_all || timeout == 0U)
            return 0x80000000;
        if (timeout != XY_OS_WAIT_FOREVER) {
            TickType_t elapsed = xTaskGetTickCount() - start;
            if (elapsed >= ticks)
                return 0x80000000;
            ticks -= elapsed;
            start = xTaskGetTickCount();
        }
    }
}

/* Delay Functions */
xy_os_status_t xy_os_delay(uint32_t ticks)
{
    TickType_t native_ticks;

    if (!freertos_timeout_to_ticks(ticks, &native_ticks))
        return XY_OS_ERROR_PARAMETER;
    vTaskDelay(native_ticks);
    return XY_OS_OK;
}

xy_os_status_t xy_os_delay_until(uint32_t ticks)
{
    TickType_t target;
    TickType_t now;

    if (!freertos_timeout_to_ticks(ticks, &target))
        return XY_OS_ERROR_PARAMETER;
    now = xTaskGetTickCount();

    if ((int32_t)(target - now) <= 0)
        return XY_OS_OK;
    vTaskDelay(target - now);
    return XY_OS_OK;
}

/* Timer Management */
typedef struct {
    xy_os_timer_func_t func;
    void *argument;
} fr_timer_context_t;

static void fr_timer_callback(TimerHandle_t timer)
{
    fr_timer_context_t *context = (fr_timer_context_t *)pvTimerGetTimerID(timer);

    if (context && context->func)
        context->func(context->argument);
}

xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t func,
                                 xy_os_timer_type_t type, void *argument,
                                 const xy_os_timer_attr_t *attr)
{
    fr_timer_context_t *context;
    TimerHandle_t timer;

    if (!func || (type != XY_OS_TIMER_ONCE && type != XY_OS_TIMER_PERIODIC))
        return NULL;
    if (attr &&
        (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    context = (fr_timer_context_t *)pvPortMalloc(sizeof(*context));
    if (!context)
        return NULL;
    context->func     = func;
    context->argument = argument;
    const char *name   = (attr && attr->name) ? attr->name : "timer";
    UBaseType_t reload = (type == XY_OS_TIMER_PERIODIC) ? pdTRUE : pdFALSE;
    timer = xTimerCreate(name, 1, reload, context, fr_timer_callback);
    if (!timer)
        vPortFree(context);
    return (xy_os_timer_id_t)timer;
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
    if (ticks == 0U || !freertos_tick_value_valid(ticks))
        return XY_OS_ERROR_PARAMETER;
    return pdstatus_to_xy(xTimerChangePeriod(t, (TickType_t)ticks, 0));
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
    fr_timer_context_t *context;
    BaseType_t result;

    if (!timer_id)
        return XY_OS_ERROR_PARAMETER;
    context = (fr_timer_context_t *)pvTimerGetTimerID((TimerHandle_t)timer_id);
    if (!context)
        return XY_OS_ERROR_RESOURCE;
    result = xTimerDelete((TimerHandle_t)timer_id, 0);
    if (result == pdPASS)
        vPortFree(context);
    return pdstatus_to_xy(result);
}

/* Event Flags */
static int freertos_event_flags_args_valid(uint32_t flags, uint32_t options)
{
    if (flags == 0U || (flags & 0xFF000000U))
        return 0;
    if (options & ~(XY_OS_FLAGS_WAIT_ALL | XY_OS_FLAGS_NO_CLEAR))
        return 0;
    return 1;
}

xy_os_event_flags_id_t
xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    if (attr &&
        (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    return (xy_os_event_flags_id_t)xEventGroupCreate();
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    (void)ef_id;
    return NULL;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    if (!ef_id || !freertos_event_flags_args_valid(flags, 0U))
        return 0x80000000U;
    return (uint32_t)xEventGroupSetBits((EventGroupHandle_t)ef_id,
                                        (EventBits_t)flags);
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    if (!ef_id || !freertos_event_flags_args_valid(flags, 0U))
        return 0x80000000U;
    return (uint32_t)xEventGroupClearBits((EventGroupHandle_t)ef_id,
                                          (EventBits_t)flags);
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    if (!ef_id)
        return 0x80000000U;
    return (uint32_t)xEventGroupGetBits((EventGroupHandle_t)ef_id);
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    if (!ef_id || !freertos_event_flags_args_valid(flags, options))
        return 0x80000000U;
    TickType_t ticks;
    if (!freertos_timeout_to_ticks(timeout, &ticks))
        return 0x80000000U;
    BaseType_t wait_all = (options & XY_OS_FLAGS_WAIT_ALL) ? pdTRUE : pdFALSE;
    BaseType_t clear    = (options & XY_OS_FLAGS_NO_CLEAR) ? pdFALSE : pdTRUE;
    EventBits_t bits = xEventGroupWaitBits(
        (EventGroupHandle_t)ef_id, (EventBits_t)flags, clear, wait_all, ticks);
    if ((wait_all && ((bits & flags) != flags)) ||
        (!wait_all && ((bits & flags) == 0U)))
        return 0x80000000U;
    return (uint32_t)bits;
}

xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id)
{
    if (ef_id)
        vEventGroupDelete((EventGroupHandle_t)ef_id);
    return ef_id ? XY_OS_OK : XY_OS_ERROR_PARAMETER;
}

/* Mutex */
typedef struct {
    SemaphoreHandle_t mutex;
    uint8_t recursive;
} fr_mutex_t;

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    fr_mutex_t *context;

    if (attr && (attr->attr_bits & ~(XY_OS_MUTEX_RECURSIVE)) != 0U)
        return NULL;
    if (attr && (attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    context = (fr_mutex_t *)pvPortMalloc(sizeof(*context));
    if (!context)
        return NULL;
    context->recursive =
        (attr && (attr->attr_bits & XY_OS_MUTEX_RECURSIVE)) ? 1U : 0U;
    context->mutex = context->recursive ? xSemaphoreCreateRecursiveMutex()
                                        : xSemaphoreCreateMutex();
    if (!context->mutex) {
        vPortFree(context);
        return NULL;
    }
    return (xy_os_mutex_id_t)context;
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    (void)mutex_id;
    return NULL;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;
    TickType_t ticks;

    if (!context || !context->mutex || !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    if (context->recursive)
        return pdstatus_to_xy_wait(xSemaphoreTakeRecursive(context->mutex, ticks));
    return pdstatus_to_xy_wait(xSemaphoreTake(context->mutex, ticks));
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;

    if (!context)
        return XY_OS_ERROR_PARAMETER;
    if (context->recursive)
        return pdstatus_to_xy(xSemaphoreGiveRecursive(context->mutex));
    return pdstatus_to_xy(xSemaphoreGive(context->mutex));
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;
    return context ? (xy_os_thread_id_t)xSemaphoreGetMutexHolder(context->mutex) : NULL;
}

xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;
    if (!context)
        return XY_OS_ERROR_PARAMETER;
    vSemaphoreDelete(context->mutex);
    vPortFree(context);
    return XY_OS_OK;
}

/* Semaphore */
xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count,
                                         uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    SemaphoreHandle_t sem;
    if (attr && (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    if (max_count == 0U || initial_count > max_count ||
        max_count > (uint32_t)((UBaseType_t)-1))
        return NULL;
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
    TickType_t ticks;
    if (!s || !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    return pdstatus_to_xy_wait(xSemaphoreTake(s, ticks));
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
    uint8_t  *pool_mem;
    uint8_t   owns_pool_mem;
    char      name[16];
} fr_pool_t;

xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    uint32_t bsz;
    uint32_t pool_bytes;

    if (block_count == 0U || block_size == 0U)
        return NULL;
    if (attr && (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    if (attr && !attr->mp_mem && attr->mp_size != 0U)
        return NULL;
    bsz = (block_size < sizeof(void *)) ? (uint32_t)sizeof(void *) : block_size;
    if (block_count > UINT32_MAX / bsz)
        return NULL;
    pool_bytes = block_count * bsz;

    fr_pool_t *p = (fr_pool_t *)pvPortMalloc(sizeof(fr_pool_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));
    p->capacity   = block_count;
    p->block_size = bsz;
    if (attr && attr->name) {
        strncpy(p->name, attr->name, sizeof(p->name) - 1);
    }

    uint8_t *mem;
    if (attr && attr->mp_mem) {
        if (attr->mp_size < pool_bytes ||
            ((uintptr_t)attr->mp_mem % sizeof(void *)) != 0U) {
            vPortFree(p);
            return NULL;
        }
        mem = (uint8_t *)attr->mp_mem;
    } else {
        mem = (uint8_t *)pvPortMalloc(pool_bytes);
        if (!mem) { vPortFree(p); return NULL; }
        p->owns_pool_mem = 1U;
    }
    p->pool_mem = mem;

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
    TickType_t ticks;
    if (!p || !freertos_timeout_to_ticks(timeout, &ticks))
        return NULL;
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
    uint8_t *candidate = (uint8_t *)block;
    void *item;
    if (!p || !block) return XY_OS_ERROR_PARAMETER;
    if (candidate < p->pool_mem ||
        candidate >= p->pool_mem + p->capacity * p->block_size ||
        ((uintptr_t)(candidate - p->pool_mem) % p->block_size) != 0U)
        return XY_OS_ERROR_PARAMETER;
    taskENTER_CRITICAL();
    for (item = p->free_list; item; item = *(void **)item) {
        if (item == block) {
            taskEXIT_CRITICAL();
            return XY_OS_ERROR_RESOURCE;
        }
    }
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
    if (p->owns_pool_mem)
        vPortFree(p->pool_mem);
    vPortFree(p);
    return XY_OS_OK;
}

/* Message Queue */
typedef struct {
    QueueHandle_t queue;
    uint32_t capacity;
    uint32_t msg_size;
} fr_msgqueue_t;

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    fr_msgqueue_t *context;

    if (attr && (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    if (msg_count == 0U || msg_size == 0U ||
        msg_count > (uint32_t)((UBaseType_t)-1) ||
        msg_size > (uint32_t)((UBaseType_t)-1))
        return NULL;
    context = (fr_msgqueue_t *)pvPortMalloc(sizeof(*context));
    if (!context)
        return NULL;
    context->queue = xQueueCreate((UBaseType_t)msg_count, (UBaseType_t)msg_size);
    if (!context->queue) {
        vPortFree(context);
        return NULL;
    }
    context->capacity = msg_count;
    context->msg_size = msg_size;
    return (xy_os_msgqueue_id_t)context;
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
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    QueueHandle_t q;
    TickType_t ticks;

    if (!context || !context->queue || !msg_ptr ||
        !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    q = context->queue;
    (void)msg_prio;
    return pdstatus_to_xy_wait(xQueueSendToBack(q, msg_ptr, ticks));
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                  uint8_t *msg_prio, uint32_t timeout)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    QueueHandle_t q;
    TickType_t ticks;
    if (!context || !context->queue || !msg_ptr ||
        !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    q = context->queue;
    if (msg_prio)
        *msg_prio = 0;
    return pdstatus_to_xy_wait(xQueueReceive(q, msg_ptr, ticks));
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return context ? context->capacity : 0U;
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return context ? context->msg_size : 0U;
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return context ? (uint32_t)uxQueueMessagesWaiting(context->queue) : 0U;
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return context ? (uint32_t)uxQueueSpacesAvailable(context->queue) : 0U;
}

xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    if (!context || !context->queue || context->capacity == 0U || context->msg_size == 0U)
        return XY_OS_ERROR_PARAMETER;
    return pdstatus_to_xy(xQueueReset(context->queue));
}

xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    if (!context)
        return XY_OS_ERROR_PARAMETER;
    vQueueDelete(context->queue);
    vPortFree(context);
    return XY_OS_OK;
}
