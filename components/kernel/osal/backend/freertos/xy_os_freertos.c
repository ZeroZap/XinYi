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

typedef struct {
    TaskHandle_t handle;
    xy_os_thread_func_t func;
    void *argument;
    uint32_t stack_size;
    uint8_t active;
    char name[16];
} freertos_thread_context_t;

#define FREERTOS_THREAD_CONTEXT_CAPACITY 32U
static freertos_thread_context_t
    g_freertos_thread_contexts[FREERTOS_THREAD_CONTEXT_CAPACITY];

static freertos_thread_context_t *freertos_find_thread_context(TaskHandle_t handle)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_THREAD_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_thread_contexts[i].active &&
            g_freertos_thread_contexts[i].handle == handle)
            return &g_freertos_thread_contexts[i];
    }
    return NULL;
}

static freertos_thread_context_t *freertos_alloc_thread_context(void)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_THREAD_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_thread_contexts[i].active)
            return &g_freertos_thread_contexts[i];
    }
    return NULL;
}

static void freertos_clear_thread_context(TaskHandle_t handle)
{
    freertos_thread_context_t *context = freertos_find_thread_context(handle);

    if (context) {
        context->handle = NULL;
        context->active = 0U;
    }
}

static void freertos_thread_entry(void *argument)
{
    freertos_thread_context_t *context = (freertos_thread_context_t *)argument;

    if (context) {
        context->handle = xTaskGetCurrentTaskHandle();
        if (context->func)
            context->func(context->argument);
        /* Clear by the running task handle; context is already bound before
           the entry can run, and this also centralizes retirement state. */
        freertos_clear_thread_context(context->handle);
    }
    vTaskDelete(NULL);
    for (;;) {
    }
}

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
    uint32_t stack_size = 256U * sizeof(StackType_t);

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
            stack_size = attr->stack_size;
        }
    }
    const char *name = (attr && attr->name) ? attr->name : "task";
    UBaseType_t prio =
        attr ? xy_to_freertos_priority(attr->priority) : (tskIDLE_PRIORITY + 1);
    TaskHandle_t handle;
    freertos_thread_context_t *context = freertos_alloc_thread_context();

    if (!context)
        return NULL;
    memset(context, 0, sizeof(*context));
    context->func = func;
    context->argument = argument;
    context->stack_size = stack_size;
    context->active = 1U;
    strncpy(context->name, name, sizeof(context->name) - 1U);
    if (xTaskCreate(freertos_thread_entry, name, stack, context, prio, &handle) != pdPASS) {
        memset(context, 0, sizeof(*context));
        return NULL;
    }
    /* A running scheduler may retire the task before xTaskCreate() returns. */
    if (context->active)
        context->handle = handle;
    else
        return NULL;
    return (xy_os_thread_id_t)handle;
}

const char *xy_os_thread_get_name(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h =
        thread_id ? (TaskHandle_t)thread_id : xTaskGetCurrentTaskHandle();
    if (!h || !freertos_find_thread_context(h))
        return NULL;
    return freertos_find_thread_context(h)->name;
}

xy_os_thread_id_t xy_os_thread_get_id(void)
{
    TaskHandle_t h = xTaskGetCurrentTaskHandle();

    return (h && freertos_find_thread_context(h)) ? (xy_os_thread_id_t)h : NULL;
}

xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h || !freertos_find_thread_context(h))
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
    freertos_thread_context_t *context =
        freertos_find_thread_context((TaskHandle_t)thread_id);

    return context ? context->stack_size : 0U;
}
uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    UBaseType_t high_water_words;

    if (!h || !freertos_find_thread_context(h))
        return 0U;
    high_water_words = uxTaskGetStackHighWaterMark(h);
    if (high_water_words > UINT32_MAX / sizeof(StackType_t))
        return 0U;
    return (uint32_t)high_water_words * sizeof(StackType_t);
}

xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t thread_id,
                                         xy_os_priority_t priority)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h || !freertos_find_thread_context(h) || !freertos_priority_valid(priority))
        return XY_OS_ERROR_PARAMETER;
    vTaskPrioritySet(h, xy_to_freertos_priority(priority));
    return XY_OS_OK;
}

xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    return (h && freertos_find_thread_context(h)) ? freertos_to_xy_priority(uxTaskPriorityGet(h))
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
    if (!h || !freertos_find_thread_context(h))
        return XY_OS_ERROR_PARAMETER;
    vTaskSuspend(h);
    return XY_OS_OK;
}

xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t thread_id)
{
    TaskHandle_t h = (TaskHandle_t)thread_id;
    if (!h || !freertos_find_thread_context(h))
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
    freertos_clear_thread_context(xTaskGetCurrentTaskHandle());
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
    /* FreeRTOS exposes kernel-global task inventory, not this adapter's owner set. */
    return 0U;
}
uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *thread_array,
                                uint32_t array_items)
{
    /* Do not publish NULL placeholders or untracked kernel-global handles. */
    (void)thread_array;
    (void)array_items;
    return 0U;
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

    if (!h || !freertos_find_thread_context(h) || !freertos_thread_flags_args_valid(flags, 0U))
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
    TimerHandle_t timer;
    xy_os_timer_func_t func;
    void *argument;
    uint8_t active;
    char name[16];
} fr_timer_context_t;

#define FREERTOS_TIMER_CONTEXT_CAPACITY 32U
static fr_timer_context_t g_freertos_timer_contexts[FREERTOS_TIMER_CONTEXT_CAPACITY];

static fr_timer_context_t *freertos_alloc_timer_context(void)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_TIMER_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_timer_contexts[i].active) {
            memset(&g_freertos_timer_contexts[i], 0, sizeof(g_freertos_timer_contexts[i]));
            g_freertos_timer_contexts[i].active = 1U;
            return &g_freertos_timer_contexts[i];
        }
    }
    return NULL;
}

static fr_timer_context_t *freertos_find_timer_context(TimerHandle_t timer)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_TIMER_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_timer_contexts[i].active &&
            g_freertos_timer_contexts[i].timer == timer)
            return &g_freertos_timer_contexts[i];
    }
    return NULL;
}

static void freertos_clear_timer_context(fr_timer_context_t *context)
{
    if (context)
        memset(context, 0, sizeof(*context));
}

static void fr_timer_callback(TimerHandle_t timer)
{
    fr_timer_context_t *context = (fr_timer_context_t *)pvTimerGetTimerID(timer);

    if (context && context->timer == timer && context->func)
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
    context = freertos_alloc_timer_context();
    if (!context)
        return NULL;
    context->func     = func;
    context->argument = argument;
    const char *name   = (attr && attr->name) ? attr->name : "timer";
    UBaseType_t reload = (type == XY_OS_TIMER_PERIODIC) ? pdTRUE : pdFALSE;
    timer = xTimerCreate(name, 1, reload, context, fr_timer_callback);
    if (!timer) {
        freertos_clear_timer_context(context);
        return NULL;
    }
    context->timer = timer;
    strncpy(context->name, name, sizeof(context->name) - 1U);
    return (xy_os_timer_id_t)timer;
}

const char *xy_os_timer_get_name(xy_os_timer_id_t timer_id)
{
    TimerHandle_t timer = (TimerHandle_t)timer_id;

    fr_timer_context_t *context = freertos_find_timer_context(timer);

    return context ? context->name : NULL;
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t timer_id, uint32_t ticks)
{
    TimerHandle_t t = (TimerHandle_t)timer_id;
    TickType_t native_ticks;

    if (!t || !freertos_find_timer_context(t))
        return XY_OS_ERROR_PARAMETER;
    if (ticks == 0U || !freertos_timeout_to_ticks(ticks, &native_ticks))
        return XY_OS_ERROR_PARAMETER;
    return pdstatus_to_xy(xTimerChangePeriod(t, native_ticks, 0));
}

xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t timer_id)
{
    TimerHandle_t timer = (TimerHandle_t)timer_id;

    return freertos_find_timer_context(timer)
               ? pdstatus_to_xy(xTimerStop(timer, 0))
               : XY_OS_ERROR_PARAMETER;
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t timer_id)
{
    TimerHandle_t timer = (TimerHandle_t)timer_id;

    return (freertos_find_timer_context(timer) && xTimerIsTimerActive(timer)) ? 1 : 0;
}

xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t timer_id)
{
    TimerHandle_t timer = (TimerHandle_t)timer_id;
    fr_timer_context_t *context;
    BaseType_t result;

    if (!timer)
        return XY_OS_ERROR_PARAMETER;
    context = freertos_find_timer_context(timer);
    if (!context)
        return XY_OS_ERROR_PARAMETER;
    result = xTimerDelete(timer, 0);
    if (result == pdPASS)
        freertos_clear_timer_context(context);
    return pdstatus_to_xy(result);
}

/* Event Flags */
typedef struct {
    EventGroupHandle_t event_group;
    char name[16];
} fr_event_flags_t;

#define FREERTOS_EVENT_FLAGS_CONTEXT_CAPACITY 32U
static fr_event_flags_t g_freertos_event_flags_contexts[FREERTOS_EVENT_FLAGS_CONTEXT_CAPACITY];

static int freertos_event_flags_context_available(void)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_EVENT_FLAGS_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_event_flags_contexts[i].event_group)
            return 1;
    }
    return 0;
}

static fr_event_flags_t *freertos_find_event_flags_context(EventGroupHandle_t event_group)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_EVENT_FLAGS_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_event_flags_contexts[i].event_group == event_group)
            return &g_freertos_event_flags_contexts[i];
    }
    return NULL;
}

static fr_event_flags_t *freertos_publish_event_flags_context(EventGroupHandle_t event_group,
                                                               const char *name)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_EVENT_FLAGS_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_event_flags_contexts[i].event_group) {
            memset(&g_freertos_event_flags_contexts[i], 0,
                   sizeof(g_freertos_event_flags_contexts[i]));
            g_freertos_event_flags_contexts[i].event_group = event_group;
            if (name)
                strncpy(g_freertos_event_flags_contexts[i].name, name,
                        sizeof(g_freertos_event_flags_contexts[i].name) - 1U);
            return &g_freertos_event_flags_contexts[i];
        }
    }
    return NULL;
}

static void freertos_clear_event_flags_context(EventGroupHandle_t event_group)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_EVENT_FLAGS_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_event_flags_contexts[i].event_group == event_group) {
            memset(&g_freertos_event_flags_contexts[i], 0,
                   sizeof(g_freertos_event_flags_contexts[i]));
            return;
        }
    }
}

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
    EventGroupHandle_t event_group;

    if (attr &&
        (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    if (!freertos_event_flags_context_available())
        return NULL;
    event_group = xEventGroupCreate();
    if (!event_group)
        return NULL;
    if (!freertos_publish_event_flags_context(event_group,
                                              attr ? attr->name : NULL)) {
        vEventGroupDelete(event_group);
        return NULL;
    }
    return (xy_os_event_flags_id_t)event_group;
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    fr_event_flags_t *context =
        freertos_find_event_flags_context((EventGroupHandle_t)ef_id);
    return context ? context->name : NULL;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    if (!ef_id || !freertos_find_event_flags_context((EventGroupHandle_t)ef_id) ||
        !freertos_event_flags_args_valid(flags, 0U))
        return 0x80000000U;
    return (uint32_t)xEventGroupSetBits((EventGroupHandle_t)ef_id,
                                        (EventBits_t)flags);
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    if (!ef_id || !freertos_find_event_flags_context((EventGroupHandle_t)ef_id) ||
        !freertos_event_flags_args_valid(flags, 0U))
        return 0x80000000U;
    return (uint32_t)xEventGroupClearBits((EventGroupHandle_t)ef_id,
                                          (EventBits_t)flags);
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    if (!ef_id || !freertos_find_event_flags_context((EventGroupHandle_t)ef_id))
        return 0x80000000U;
    return (uint32_t)xEventGroupGetBits((EventGroupHandle_t)ef_id);
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    if (!ef_id || !freertos_find_event_flags_context((EventGroupHandle_t)ef_id) ||
        !freertos_event_flags_args_valid(flags, options))
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
    EventGroupHandle_t event_group = (EventGroupHandle_t)ef_id;

    if (!event_group || !freertos_find_event_flags_context(event_group))
        return XY_OS_ERROR_PARAMETER;
    vEventGroupDelete(event_group);
    freertos_clear_event_flags_context(event_group);
    return XY_OS_OK;
}

/* Mutex */
typedef struct {
    SemaphoreHandle_t mutex;
    uint8_t recursive;
    char name[16];
} fr_mutex_t;

#define FREERTOS_MUTEX_CONTEXT_CAPACITY 32U
static fr_mutex_t *g_freertos_mutex_contexts[FREERTOS_MUTEX_CONTEXT_CAPACITY];

static int freertos_mutex_context_available(void)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MUTEX_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_mutex_contexts[i])
            return 1;
    }
    return 0;
}

static fr_mutex_t *freertos_find_mutex_context(fr_mutex_t *mutex)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MUTEX_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_mutex_contexts[i] == mutex)
            return mutex;
    }
    return NULL;
}

static void freertos_publish_mutex_context(fr_mutex_t *mutex)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MUTEX_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_mutex_contexts[i]) {
            g_freertos_mutex_contexts[i] = mutex;
            return;
        }
    }
}

static void freertos_clear_mutex_context(fr_mutex_t *mutex)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MUTEX_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_mutex_contexts[i] == mutex) {
            g_freertos_mutex_contexts[i] = NULL;
            return;
        }
    }
}

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    fr_mutex_t *context;

    if (attr && (attr->attr_bits & ~(XY_OS_MUTEX_RECURSIVE)) != 0U)
        return NULL;
    if (attr && (attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    if (!freertos_mutex_context_available())
        return NULL;
    context = (fr_mutex_t *)pvPortMalloc(sizeof(*context));
    if (!context)
        return NULL;
    memset(context, 0, sizeof(*context));
    context->recursive =
        (attr && (attr->attr_bits & XY_OS_MUTEX_RECURSIVE)) ? 1U : 0U;
    if (attr && attr->name)
        strncpy(context->name, attr->name, sizeof(context->name) - 1U);
    context->mutex = context->recursive ? xSemaphoreCreateRecursiveMutex()
                                        : xSemaphoreCreateMutex();
    if (!context->mutex) {
        vPortFree(context);
        return NULL;
    }
    freertos_publish_mutex_context(context);
    return (xy_os_mutex_id_t)context;
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;
    return freertos_find_mutex_context(context) ? context->name : NULL;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;
    TickType_t ticks;

    if (!freertos_find_mutex_context(context) || !context->mutex ||
        !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    if (context->recursive)
        return pdstatus_to_xy_wait(xSemaphoreTakeRecursive(context->mutex, ticks));
    return pdstatus_to_xy_wait(xSemaphoreTake(context->mutex, ticks));
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;

    if (!freertos_find_mutex_context(context) || !context->mutex)
        return XY_OS_ERROR_PARAMETER;
    if (context->recursive)
        return pdstatus_to_xy(xSemaphoreGiveRecursive(context->mutex));
    return pdstatus_to_xy(xSemaphoreGive(context->mutex));
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;
    TaskHandle_t owner;

    if (!freertos_find_mutex_context(context) || !context->mutex)
        return NULL;
    owner = xSemaphoreGetMutexHolder(context->mutex);
    if (!owner || !freertos_find_thread_context(owner))
        return NULL;
    return (xy_os_thread_id_t)owner;
}

xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    fr_mutex_t *context = (fr_mutex_t *)mutex_id;
    if (!freertos_find_mutex_context(context) || !context->mutex)
        return XY_OS_ERROR_PARAMETER;
    if (xSemaphoreGetMutexHolder(context->mutex) != NULL)
        return XY_OS_ERROR_RESOURCE;
    vSemaphoreDelete(context->mutex);
    freertos_clear_mutex_context(context);
    vPortFree(context);
    return XY_OS_OK;
}

/* Semaphore */
typedef struct {
    SemaphoreHandle_t semaphore;
    uint8_t active;
    char name[16];
} fr_semaphore_context_t;

#define FREERTOS_SEMAPHORE_CONTEXT_CAPACITY 32U
static fr_semaphore_context_t
    g_freertos_semaphore_contexts[FREERTOS_SEMAPHORE_CONTEXT_CAPACITY];

static fr_semaphore_context_t *freertos_alloc_semaphore_context(void)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_SEMAPHORE_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_semaphore_contexts[i].active) {
            memset(&g_freertos_semaphore_contexts[i], 0,
                   sizeof(g_freertos_semaphore_contexts[i]));
            g_freertos_semaphore_contexts[i].active = 1U;
            return &g_freertos_semaphore_contexts[i];
        }
    }
    return NULL;
}

static fr_semaphore_context_t *freertos_find_semaphore_context(SemaphoreHandle_t semaphore)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_SEMAPHORE_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_semaphore_contexts[i].active &&
            g_freertos_semaphore_contexts[i].semaphore == semaphore)
            return &g_freertos_semaphore_contexts[i];
    }
    return NULL;
}

static void freertos_clear_semaphore_context(fr_semaphore_context_t *context)
{
    if (context)
        memset(context, 0, sizeof(*context));
}

xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count,
                                         uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    SemaphoreHandle_t sem;
    fr_semaphore_context_t *context;
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
    if (!sem)
        return NULL;
    context = freertos_alloc_semaphore_context();
    if (!context) {
        vSemaphoreDelete(sem);
        return NULL;
    }
    context->semaphore = sem;
    if (attr && attr->name)
        strncpy(context->name, attr->name, sizeof(context->name) - 1U);
    return (xy_os_semaphore_id_t)sem;
}

const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id)
{
    fr_semaphore_context_t *context =
        freertos_find_semaphore_context((SemaphoreHandle_t)semaphore_id);
    return context ? context->name : NULL;
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id,
                                       uint32_t timeout)
{
    SemaphoreHandle_t s = (SemaphoreHandle_t)semaphore_id;
    TickType_t ticks;
    if (!s || !freertos_find_semaphore_context(s) ||
        !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    return pdstatus_to_xy_wait(xSemaphoreTake(s, ticks));
}

xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id)
{
    return (semaphore_id &&
            freertos_find_semaphore_context((SemaphoreHandle_t)semaphore_id))
               ? pdstatus_to_xy(xSemaphoreGive((SemaphoreHandle_t)semaphore_id))
               : XY_OS_ERROR_PARAMETER;
}

xy_os_status_t xy_os_semaphore_release_from_isr(xy_os_semaphore_id_t semaphore_id)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (!semaphore_id ||
        !freertos_find_semaphore_context((SemaphoreHandle_t)semaphore_id))
        return XY_OS_ERROR_PARAMETER;
    if (xSemaphoreGiveFromISR((SemaphoreHandle_t)semaphore_id,
                              &higher_priority_task_woken) != pdPASS)
        return XY_OS_ERROR_RESOURCE;
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return XY_OS_OK;
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id)
{
    return (semaphore_id &&
            freertos_find_semaphore_context((SemaphoreHandle_t)semaphore_id))
               ? (uint32_t)uxSemaphoreGetCount((SemaphoreHandle_t)semaphore_id)
               : 0;
}

xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id)
{
    fr_semaphore_context_t *context;

    if (!semaphore_id ||
        !(context = freertos_find_semaphore_context((SemaphoreHandle_t)semaphore_id)))
        return XY_OS_ERROR_PARAMETER;
    vSemaphoreDelete((SemaphoreHandle_t)semaphore_id);
    freertos_clear_semaphore_context(context);
    return XY_OS_OK;
}

/* Memory Pool — free-list allocator on top of pvPortMalloc */

typedef struct {
    uint32_t  capacity;
    uint32_t  block_size;
    uint32_t  stride;
    volatile uint32_t free_count;
    uint8_t   deleting;
    void     *free_list;
    uint8_t  *pool_mem;
    uint8_t   owns_pool_mem;
    char      name[16];
} fr_pool_t;

#define FREERTOS_MEMPOOL_CONTEXT_CAPACITY 32U
static fr_pool_t *g_freertos_mempool_contexts[FREERTOS_MEMPOOL_CONTEXT_CAPACITY];

static int freertos_mempool_context_available(void)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MEMPOOL_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_mempool_contexts[i])
            return 1;
    }
    return 0;
}

static fr_pool_t *freertos_find_mempool_context(fr_pool_t *pool)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MEMPOOL_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_mempool_contexts[i] == pool)
            return pool;
    }
    return NULL;
}

static void freertos_publish_mempool_context(fr_pool_t *pool)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MEMPOOL_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_mempool_contexts[i]) {
            g_freertos_mempool_contexts[i] = pool;
            return;
        }
    }
}

static void freertos_clear_mempool_context(fr_pool_t *pool)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MEMPOOL_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_mempool_contexts[i] == pool) {
            g_freertos_mempool_contexts[i] = NULL;
            return;
        }
    }
}

xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    uint32_t stride;
    uint32_t remainder;
    uint32_t pool_bytes;

    if (block_count == 0U || block_size == 0U)
        return NULL;
    if (attr && (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U))
        return NULL;
    if (attr && !attr->mp_mem && attr->mp_size != 0U)
        return NULL;
    if (!freertos_mempool_context_available())
        return NULL;
    stride = (block_size < sizeof(void *)) ? (uint32_t)sizeof(void *) : block_size;
    remainder = block_size % (uint32_t)sizeof(void *);
    if (block_size >= sizeof(void *) && remainder != 0U) {
        if (block_size > UINT32_MAX - ((uint32_t)sizeof(void *) - remainder))
            return NULL;
        stride = block_size + ((uint32_t)sizeof(void *) - remainder);
    }
    if (block_count > UINT32_MAX / stride)
        return NULL;
    pool_bytes = block_count * stride;

    fr_pool_t *p = (fr_pool_t *)pvPortMalloc(sizeof(fr_pool_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));
    p->capacity   = block_count;
    p->block_size = block_size;
    p->stride     = stride;
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
        void *blk   = mem + (i - 1) * stride;
        *(void **)blk = p->free_list;
        p->free_list  = blk;
        p->free_count++;
    }

    freertos_publish_mempool_context(p);
    return (xy_os_mempool_id_t)p;
}

const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    return freertos_find_mempool_context(p) ? p->name : NULL;
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    TickType_t ticks;
    if (!freertos_find_mempool_context(p) || !freertos_timeout_to_ticks(timeout, &ticks))
        return NULL;
    TickType_t start = xTaskGetTickCount();

    while (1) {
        taskENTER_CRITICAL();
        if (p->deleting) {
            taskEXIT_CRITICAL();
            return NULL;
        }
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
    if (!freertos_find_mempool_context(p) || !block)
        return XY_OS_ERROR_PARAMETER;
    taskENTER_CRITICAL();
    if (p->deleting) {
        taskEXIT_CRITICAL();
        return XY_OS_ERROR_RESOURCE;
    }
    if (candidate < p->pool_mem ||
        candidate >= p->pool_mem + p->capacity * p->stride ||
        ((uintptr_t)(candidate - p->pool_mem) % p->stride) != 0U) {
        taskEXIT_CRITICAL();
        return XY_OS_ERROR_PARAMETER;
    }
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
    return freertos_find_mempool_context(p) ? p->capacity : 0U;
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    return freertos_find_mempool_context(p) ? p->block_size : 0U;
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    if (!freertos_find_mempool_context(p))
        return 0U;
    taskENTER_CRITICAL();
    uint32_t used = p->capacity - p->free_count;
    taskEXIT_CRITICAL();
    return used;
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    if (!freertos_find_mempool_context(p))
        return 0U;
    taskENTER_CRITICAL();
    uint32_t free = p->free_count;
    taskEXIT_CRITICAL();
    return free;
}

xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id)
{
    fr_pool_t *p = (fr_pool_t *)mp_id;
    uint8_t can_delete;

    if (!freertos_find_mempool_context(p))
        return XY_OS_ERROR_PARAMETER;

    taskENTER_CRITICAL();
    can_delete = (p->free_count == p->capacity && !p->deleting) ? 1U : 0U;
    if (can_delete)
        p->deleting = 1U;
    taskEXIT_CRITICAL();
    if (!can_delete)
        return XY_OS_ERROR_RESOURCE;
    if (p->owns_pool_mem)
        vPortFree(p->pool_mem);
    freertos_clear_mempool_context(p);
    vPortFree(p);
    return XY_OS_OK;
}

/* Message Queue */
typedef struct {
    QueueHandle_t queue;
    uint32_t capacity;
    uint32_t msg_size;
    char name[16];
} fr_msgqueue_t;

#define FREERTOS_MSGQUEUE_CONTEXT_CAPACITY 32U
static fr_msgqueue_t *g_freertos_msgqueue_contexts[FREERTOS_MSGQUEUE_CONTEXT_CAPACITY];

static int freertos_msgqueue_context_available(void)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MSGQUEUE_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_msgqueue_contexts[i])
            return 1;
    }
    return 0;
}

static fr_msgqueue_t *freertos_find_msgqueue_context(fr_msgqueue_t *queue)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MSGQUEUE_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_msgqueue_contexts[i] == queue)
            return queue;
    }
    return NULL;
}

static void freertos_publish_msgqueue_context(fr_msgqueue_t *queue)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MSGQUEUE_CONTEXT_CAPACITY; ++i) {
        if (!g_freertos_msgqueue_contexts[i]) {
            g_freertos_msgqueue_contexts[i] = queue;
            return;
        }
    }
}

static void freertos_clear_msgqueue_context(fr_msgqueue_t *queue)
{
    uint32_t i;

    for (i = 0U; i < FREERTOS_MSGQUEUE_CONTEXT_CAPACITY; ++i) {
        if (g_freertos_msgqueue_contexts[i] == queue) {
            g_freertos_msgqueue_contexts[i] = NULL;
            return;
        }
    }
}

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    fr_msgqueue_t *context;

    if (attr && (attr->attr_bits != 0U || attr->cb_mem || attr->cb_size != 0U ||
                 attr->mq_mem || attr->mq_size != 0U))
        return NULL;
    if (!freertos_msgqueue_context_available())
        return NULL;
    if (msg_count == 0U || msg_size == 0U ||
        msg_count > (uint32_t)((UBaseType_t)-1) ||
        msg_size > (uint32_t)((UBaseType_t)-1))
        return NULL;
    context = (fr_msgqueue_t *)pvPortMalloc(sizeof(*context));
    if (!context)
        return NULL;
    memset(context, 0, sizeof(*context));
    context->queue = xQueueCreate((UBaseType_t)msg_count, (UBaseType_t)msg_size);
    if (!context->queue) {
        vPortFree(context);
        return NULL;
    }
    context->capacity = msg_count;
    context->msg_size = msg_size;
    if (attr && attr->name)
        strncpy(context->name, attr->name, sizeof(context->name) - 1U);
    freertos_publish_msgqueue_context(context);
    return (xy_os_msgqueue_id_t)context;
}

const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return freertos_find_msgqueue_context(context) ? context->name : NULL;
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                  const void *msg_ptr, uint8_t msg_prio,
                                  uint32_t timeout)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    QueueHandle_t q;
    TickType_t ticks;

    if (!freertos_find_msgqueue_context(context) || !context->queue || !msg_ptr ||
        msg_prio != 0U ||
        !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    q = context->queue;
    return pdstatus_to_xy_wait(xQueueSendToBack(q, msg_ptr, ticks));
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                  uint8_t *msg_prio, uint32_t timeout)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    QueueHandle_t q;
    TickType_t ticks;
    BaseType_t result;

    if (!freertos_find_msgqueue_context(context) || !context->queue || !msg_ptr ||
        !freertos_timeout_to_ticks(timeout, &ticks))
        return XY_OS_ERROR_PARAMETER;
    q = context->queue;
    result = xQueueReceive(q, msg_ptr, ticks);
    if (result == pdPASS && msg_prio)
        *msg_prio = 0;
    return pdstatus_to_xy_wait(result);
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return freertos_find_msgqueue_context(context) ? context->capacity : 0U;
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return freertos_find_msgqueue_context(context) ? context->msg_size : 0U;
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return freertos_find_msgqueue_context(context)
               ? (uint32_t)uxQueueMessagesWaiting(context->queue)
               : 0U;
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    return freertos_find_msgqueue_context(context)
               ? (uint32_t)uxQueueSpacesAvailable(context->queue)
               : 0U;
}

xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    if (!context || !context->queue || !freertos_find_msgqueue_context(context) ||
        context->capacity == 0U || context->msg_size == 0U)
        return XY_OS_ERROR_PARAMETER;
    return pdstatus_to_xy(xQueueReset(context->queue));
}

xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    fr_msgqueue_t *context = (fr_msgqueue_t *)mq_id;
    if (!freertos_find_msgqueue_context(context) || !context->queue)
        return XY_OS_ERROR_PARAMETER;
    vQueueDelete(context->queue);
    freertos_clear_msgqueue_context(context);
    vPortFree(context);
    return XY_OS_OK;
}
