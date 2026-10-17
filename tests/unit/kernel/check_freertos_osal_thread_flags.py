#!/usr/bin/env python3
"""Guard FreeRTOS OSAL thread-flag semantics."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/freertos/xy_os_freertos.c"
PUBLIC_API = ROOT / "components/kernel/osal/xy_os.h"


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    public_api = PUBLIC_API.read_text(encoding="utf-8")
    errors: list[str] = []

    if "return ulTaskNotifyValueClear(NULL, flags);" not in source:
        errors.append(
            "xy_os_thread_flags_clear must atomically return the previous notification "
            "value while clearing only the requested bits"
        )

    required_set_contract = (
        "xTaskNotifyAndQuery(h, flags, eSetBits, &previous) != pdPASS",
        "return previous | flags;",
    )
    for required in required_set_contract:
        if required not in source:
            errors.append(
                "xy_os_thread_flags_set must propagate notification failure and return "
                f"the complete post-set flag value; missing: {required}"
            )

    required_get_contract = (
        "if (xTaskNotifyAndQuery(xTaskGetCurrentTaskHandle(), 0, eNoAction, &flags) != pdPASS)",
        "return 0x80000000;",
    )
    for required in required_get_contract:
        if required not in source:
            errors.append(
                "xy_os_thread_flags_get must fail closed when the FreeRTOS notification "
                f"query is rejected; missing: {required}"
            )

    thread_flags_helper = source.split("/* Thread Flags", 1)[-1].split(
        "uint32_t xy_os_thread_flags_set", 1
    )[0]
    for required in (
        "flags == 0U",
        "flags & 0x80000000U",
        "options & ~(XY_OS_FLAGS_WAIT_ALL | XY_OS_FLAGS_NO_CLEAR)",
    ):
        if required not in thread_flags_helper:
            errors.append(
                "FreeRTOS thread flags must reject empty masks, the reserved error bit, "
                f"and unknown wait options; missing: {required}"
            )

    thread_flags_set = source.split("uint32_t xy_os_thread_flags_set", 1)[-1].split(
        "uint32_t xy_os_thread_flags_clear", 1
    )[0]
    thread_flags_clear = source.split("uint32_t xy_os_thread_flags_clear", 1)[-1].split(
        "uint32_t xy_os_thread_flags_get", 1
    )[0]
    thread_flags_wait = source.split("uint32_t xy_os_thread_flags_wait", 1)[-1].split(
        "/* Delay Functions */", 1
    )[0]
    if "!freertos_thread_flags_args_valid(flags, 0U)" not in thread_flags_set:
        errors.append("FreeRTOS thread-flags set must reject an invalid mask before kernel access")
    if "!freertos_thread_flags_args_valid(flags, 0U)" not in thread_flags_clear:
        errors.append("FreeRTOS thread-flags clear must reject an invalid mask before kernel access")
    if "!freertos_thread_flags_args_valid(flags, options)" not in thread_flags_wait:
        errors.append("FreeRTOS thread-flags wait must reject invalid masks/options before kernel access")

    event_flags_clear = source.split("uint32_t xy_os_event_flags_clear", 1)[-1].split(
        "uint32_t xy_os_event_flags_get", 1
    )[0]
    if "return 0x80000000U;" not in event_flags_clear:
        errors.append(
            "FreeRTOS event-flags clear must fail closed for a NULL or invalid handle "
            "instead of reporting an empty previous value"
        )

    event_helper = source.split("/* Event Flags */", 1)[-1].split(
        "xy_os_event_flags_id_t", 1
    )[0]
    for required in (
        "flags == 0U",
        "flags & 0xFF000000U",
        "options & ~(XY_OS_FLAGS_WAIT_ALL | XY_OS_FLAGS_NO_CLEAR)",
    ):
        if required not in event_helper:
            errors.append(
                "FreeRTOS event flags must reject empty masks, FreeRTOS-reserved control "
                f"bits, and unknown wait options; missing: {required}"
            )

    event_wait = source.split("uint32_t xy_os_event_flags_wait", 1)[-1].split(
        "xy_os_status_t xy_os_event_flags_delete", 1
    )[0]
    for required in (
        "!freertos_event_flags_args_valid(flags, options)",
        "(bits & flags) != flags",
        "(bits & flags) == 0U",
        "return 0x80000000U;",
    ):
        if required not in event_wait:
            errors.append(
                "FreeRTOS event-flags wait must distinguish an expired wait from a "
                f"satisfied condition; missing: {required}"
            )

    for forbidden in (
        "xTaskNotifyStateClear(NULL);",
        "xTaskNotify(xTaskGetCurrentTaskHandle(), ~flags, eSetValueWithOverwrite);",
        "xTaskNotifyWait(0, clear ? flags : 0, &notif, ticks)",
    ):
        if forbidden in source:
            errors.append(f"thread flag clear retains invalid FreeRTOS sequence: {forbidden}")

    thread_wait = source.split("uint32_t xy_os_thread_flags_wait", 1)[-1].split(
        "/* Delay Functions */", 1
    )[0]
    for required in (
        "xTaskNotifyWait(0, 0, &notif, ticks)",
        "wait_all && ((notif & flags) == flags)",
        "ulTaskNotifyValueClear(NULL, flags)",
        "elapsed = xTaskGetTickCount() - start",
        "ticks = (TickType_t)timeout - elapsed",
    ):
        if required not in thread_wait:
            errors.append(
                "FreeRTOS WAIT_ALL must retain partial notifications, wait for remaining bits, "
                f"and clear only after satisfaction; missing: {required}"
            )

    for required in (
        "pdstatus_to_xy_wait",
        "return XY_OS_ERROR_TIMEOUT;",
        "pdstatus_to_xy_wait(xSemaphoreTake(context->mutex, ticks))",
        "pdstatus_to_xy_wait(xSemaphoreTake(s, ticks))",
        "pdstatus_to_xy_wait(xQueueSendToBack(q, msg_ptr, ticks))",
        "pdstatus_to_xy_wait(xQueueReceive(q, msg_ptr, ticks))",
    ):
        if required not in source:
            errors.append(
                "FreeRTOS blocking synchronization operations must map an expired wait to "
                f"XY_OS_ERROR_TIMEOUT; missing: {required}"
            )

    queue_reset = source.split("xy_os_status_t xy_os_msgqueue_reset", 1)[-1].split(
        "xy_os_status_t xy_os_msgqueue_delete", 1
    )[0]
    for required in (
        "!context || !context->queue",
        "context->capacity == 0U",
        "context->msg_size == 0U",
        "xQueueReset(context->queue)",
    ):
        if required not in queue_reset:
            errors.append(
                "FreeRTOS queue reset must validate the wrapper, native handle, and metadata; "
                f"missing: {required}"
            )

    timer_delete = source.split("xy_os_status_t xy_os_timer_delete", 1)[-1].split(
        "/* Event Flags */", 1
    )[0]
    for required in (
        "if (!timer_id)",
        "pvTimerGetTimerID((TimerHandle_t)timer_id)",
        "if (!context)",
        "return XY_OS_ERROR_RESOURCE;",
        "if (result == pdPASS)",
        "vPortFree(context);",
    ):
        if required not in timer_delete:
            errors.append(
                "FreeRTOS timer delete must validate callback context and free it only after "
                f"native deletion succeeds; missing: {required}"
            )

    event_flags_get = source.split("uint32_t xy_os_event_flags_get", 1)[-1].split(
        "uint32_t xy_os_event_flags_wait", 1
    )[0]
    if "if (!ef_id)" not in event_flags_get or "return 0x80000000U;" not in event_flags_get:
        errors.append("FreeRTOS event-flags get must fail closed for a NULL handle")

    event_flags_delete = source.split("xy_os_status_t xy_os_event_flags_delete", 1)[-1]
    for required in ("if (ef_id)", "vEventGroupDelete((EventGroupHandle_t)ef_id)", "XY_OS_ERROR_PARAMETER"):
        if required not in event_flags_delete:
            errors.append(
                "FreeRTOS event-flags delete must validate the handle and avoid native access "
                f"for NULL; missing: {required}"
            )

    timer_new = source.split("xy_os_timer_id_t xy_os_timer_new", 1)[-1].split(
        "const char *xy_os_timer_get_name", 1
    )[0]
    for required in (
        "type != XY_OS_TIMER_ONCE && type != XY_OS_TIMER_PERIODIC",
        "attr->attr_bits != 0U",
        "attr->cb_mem",
        "attr->cb_size != 0U",
    ):
        if required not in timer_new:
            errors.append(
                "FreeRTOS dynamic timer creation must reject unknown timer types and unsupported "
                f"caller-owned/control attributes before allocation; missing: {required}"
            )

    timer_start = source.split("xy_os_status_t xy_os_timer_start", 1)[-1].split(
        "xy_os_status_t xy_os_timer_stop", 1
    )[0]
    for required in (
        "ticks == 0U",
        "return pdstatus_to_xy(xTimerChangePeriod(t, (TickType_t)ticks, 0));",
    ):
        if required not in timer_start:
            errors.append(
                "FreeRTOS timer start must reject a zero period and propagate the actual "
                f"change-period command result; missing: {required}"
            )
    if "xTimerStart(t, 0)" in timer_start:
        errors.append(
            "FreeRTOS timer start must not hide a failed change-period command behind a "
            "second timer-start command"
        )

    event_flags_new = source.split(
        "xy_os_event_flags_new", 1
    )[-1].split("const char *xy_os_event_flags_get_name", 1)[0]
    for required in ("attr->attr_bits != 0U", "attr->cb_mem", "attr->cb_size != 0U"):
        if required not in event_flags_new:
            errors.append(
                "FreeRTOS event-flags creation must reject unsupported caller-owned/control "
                f"attributes before allocation; missing: {required}"
            )

    mutex_region = source.split("/* Mutex */", 1)[-1].split("/* Semaphore */", 1)[0]
    for required in (
        "typedef struct {",
        "uint8_t recursive;",
        "xSemaphoreTakeRecursive(context->mutex, ticks)",
        "xSemaphoreGiveRecursive(context->mutex)",
        "vSemaphoreDelete(context->mutex);",
        "vPortFree(context);",
    ):
        if required not in mutex_region:
            errors.append(
                "FreeRTOS recursive mutexes must retain their type, use recursive take/give, "
                f"and release wrapper ownership; missing: {required}"
            )

    mutex_new = source.split("xy_os_mutex_id_t xy_os_mutex_new", 1)[-1].split(
        "const char *xy_os_mutex_get_name", 1
    )[0]
    for required in (
        "attr->attr_bits & ~(XY_OS_MUTEX_RECURSIVE)",
        "attr->cb_mem",
        "attr->cb_size != 0U",
    ):
        if required not in mutex_new:
            errors.append(
                "FreeRTOS mutex creation must reject unsupported attributes before allocation; "
                f"missing: {required}"
            )

    stack_size = source.split("uint32_t xy_os_thread_get_stack_size", 1)[-1].split(
        "uint32_t xy_os_thread_get_stack_space", 1
    )[0]
    stack_space = source.split("uint32_t xy_os_thread_get_stack_space", 1)[-1].split(
        "xy_os_status_t xy_os_thread_set_priority", 1
    )[0]
    if "return 0U;" not in stack_size:
        errors.append("FreeRTOS stack-size query must use the documented unavailable value")
    if "uxTaskGetStackHighWaterMark(h) * sizeof(StackType_t)" not in stack_space:
        errors.append(
            "FreeRTOS stack high-water marks are measured in StackType_t words and must be "
            "converted to the OSAL byte contract"
        )

    priority_mapping = source.split("static int freertos_priority_valid", 1)[-1].split(
        "static xy_os_status_t pdstatus_to_xy", 1
    )[0]
    for required in (
        "priority >= XY_OS_PRIORITY_IDLE && priority <= XY_OS_PRIORITY_ISR",
        "configMAX_PRIORITIES - 1U",
        "XY_OS_PRIORITY_ISR",
        "XY_OS_PRIORITY_IDLE",
        "priority",
    ):
        if required not in priority_mapping:
            errors.append(
                "FreeRTOS priorities must use a bounded monotonic mapping across the OSAL "
                f"priority domain; missing: {required}"
            )
    if "if (prio >= configMAX_PRIORITIES)" in priority_mapping:
        errors.append("FreeRTOS priority mapping must not collapse all high OSAL priorities")

    thread_new = source.split("xy_os_thread_id_t xy_os_thread_new", 1)[-1].split(
        "const char *xy_os_thread_get_name", 1
    )[0]
    if "!freertos_priority_valid(attr->priority)" not in thread_new:
        errors.append("FreeRTOS thread creation must reject invalid explicit priorities")
    for required in (
        "attr->attr_bits != XY_OS_THREAD_DETACHED",
        "attr->cb_mem",
        "attr->cb_size != 0U",
        "attr->stack_mem",
        "attr->tz_module != 0U",
        "attr->reserved != 0U",
        "attr->stack_size % sizeof(StackType_t)",
        "(configSTACK_DEPTH_TYPE)-1",
    ):
        if required not in thread_new:
            errors.append(
                "FreeRTOS dynamic thread creation must reject unsupported caller-owned/joinable "
                f"attributes and non-representable byte stack sizes; missing: {required}"
            )
    if "uint16_t stack" in thread_new:
        errors.append(
            "FreeRTOS thread creation must not narrow the requested stack depth before validating it"
        )

    delay_until = source.split("xy_os_status_t xy_os_delay_until", 1)[-1].split(
        "/* Timer Management */", 1
    )[0]
    for required in (
        "xTaskGetTickCount()",
        "(int32_t)(target - now) <= 0",
        "vTaskDelay(target - now);",
    ):
        if required not in delay_until:
            errors.append(
                "FreeRTOS absolute delay must compute a wrap-safe positive delta from the "
                f"current tick; missing: {required}"
            )
    if "vTaskDelayUntil(&wake_time, 0)" in delay_until:
        errors.append(
            "FreeRTOS absolute delay must not call vTaskDelayUntil with the target as the "
            "previous-wake value and a zero increment"
        )

    priority_set = source.split("xy_os_status_t xy_os_thread_set_priority", 1)[-1].split(
        "xy_os_priority_t xy_os_thread_get_priority", 1
    )[0]
    if "!freertos_priority_valid(priority)" not in priority_set:
        errors.append("FreeRTOS priority setter must reject invalid priorities before kernel access")

    mempool_new = source.split("xy_os_mempool_id_t xy_os_mempool_new", 1)[-1].split(
        "const char *xy_os_mempool_get_name", 1
    )[0]
    for required in (
        "uint32_t pool_bytes;",
        "if (block_count > UINT32_MAX / bsz)",
        "pool_bytes = block_count * bsz;",
        "attr->mp_size < pool_bytes",
        "pvPortMalloc(pool_bytes)",
    ):
        if required not in mempool_new:
            errors.append(
                "FreeRTOS memory-pool creation must calculate checked storage bytes once; "
                f"missing: {required}"
            )

    kernel_locking = source.split("int32_t xy_os_kernel_lock", 1)[-1].split(
        "uint32_t xy_os_kernel_get_tick_count", 1
    )[0]
    for required in (
        "xTaskGetSchedulerState()",
        "taskSCHEDULER_SUSPENDED",
        "taskSCHEDULER_RUNNING",
        "vTaskSuspendAll();",
        "xTaskResumeAll();",
        "lock != 0 && lock != 1",
    ):
        if required not in kernel_locking:
            errors.append(
                "FreeRTOS kernel lock/unlock/restore must preserve and restore the observable "
                f"scheduler lock state; missing: {required}"
            )
    if kernel_locking.count("return 0;") >= 3:
        errors.append(
            "FreeRTOS kernel lock APIs must not unconditionally report an unlocked previous state"
        )

    kernel_state = source.split("xy_os_kernel_state_t xy_os_kernel_get_state", 1)[-1].split(
        "xy_os_status_t xy_os_kernel_start", 1
    )[0]
    for required in (
        "taskSCHEDULER_NOT_STARTED",
        "XY_OS_KERNEL_READY",
        "taskSCHEDULER_RUNNING",
        "XY_OS_KERNEL_RUNNING",
        "taskSCHEDULER_SUSPENDED",
        "XY_OS_KERNEL_LOCKED",
        "XY_OS_KERNEL_ERROR",
    ):
        if required not in kernel_state:
            errors.append(
                "FreeRTOS kernel state must preserve ready, running, locked, and unknown "
                f"scheduler states; missing: {required}"
            )

    kernel_start = source.split("xy_os_status_t xy_os_kernel_start", 1)[-1].split(
        "int32_t xy_os_kernel_lock", 1
    )[0]
    if "return XY_OS_ERROR;" not in kernel_start:
        errors.append(
            "FreeRTOS kernel start must fail closed if vTaskStartScheduler unexpectedly returns"
        )
    if "return XY_OS_OK;" in kernel_start:
        errors.append(
            "FreeRTOS kernel start must not report success after the scheduler failed to start"
        )

    for token in (
        "xy_os_semaphore_release_from_isr",
        "xSemaphoreGiveFromISR",
        "portYIELD_FROM_ISR",
    ):
        if token not in source:
            errors.append(f"FreeRTOS ISR-safe semaphore release must preserve token: {token}")

    thread_detach = source.split("xy_os_status_t xy_os_thread_detach", 1)[-1].split(
        "xy_os_status_t xy_os_thread_join", 1
    )[0]
    thread_join = source.split("xy_os_status_t xy_os_thread_join", 1)[-1].split(
        "void xy_os_thread_exit", 1
    )[0]
    for name, body in (("detach", thread_detach), ("join", thread_join)):
        if "return thread_id ? XY_OS_ERROR : XY_OS_ERROR_PARAMETER;" not in body:
            errors.append(
                f"FreeRTOS thread {name} must reject NULL and fail closed because ownership "
                "retirement/waiting is not observable"
            )
    if "return XY_OS_OK;" in thread_detach:
        errors.append("FreeRTOS thread detach must not report unsupported ownership as success")

    terminate = source.split("xy_os_status_t xy_os_thread_terminate", 1)[-1]
    if "if (!thread_id)" not in terminate:
        errors.append("FreeRTOS thread terminate must reject a NULL handle before kernel access")

    semaphore_new = source.split("xy_os_semaphore_id_t xy_os_semaphore_new", 1)[-1].split(
        "const char *xy_os_semaphore_get_name", 1
    )[0]
    for required in (
        "max_count == 0U",
        "initial_count > max_count",
        "max_count > (uint32_t)((UBaseType_t)-1)",
        "return NULL;",
        "attr->attr_bits != 0U",
        "attr->cb_mem",
        "attr->cb_size != 0U",
    ):
        if required not in semaphore_new:
            errors.append(
                "FreeRTOS semaphore creation must reject invalid counts and unsupported "
                f"attributes before allocation; missing: {required}"
            )

    msgqueue_new = source.split("xy_os_msgqueue_id_t xy_os_msgqueue_new", 1)[-1].split(
        "const char *xy_os_msgqueue_get_name", 1
    )[0]
    for required in (
        "attr->attr_bits != 0U",
        "attr->cb_mem",
        "attr->cb_size != 0U",
        "msg_count > (uint32_t)((UBaseType_t)-1)",
        "msg_size > (uint32_t)((UBaseType_t)-1)",
    ):
        if required not in msgqueue_new:
            errors.append(
                "FreeRTOS message-queue creation must reject unsupported attributes before "
                f"allocation; missing: {required}"
            )

    if errors:
        print("freertos_osal_thread_flags failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("freertos_osal_thread_flags_ok clear=atomic-selective previous=value")
    return 0


if __name__ == "__main__":
    sys.exit(main())
