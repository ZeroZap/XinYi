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
    ):
        if forbidden in source:
            errors.append(f"thread flag clear retains invalid FreeRTOS sequence: {forbidden}")

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

    if "xy_os_semaphore_release_from_isr" not in public_api:
        errors.append("OSAL public API must expose an explicit ISR-safe semaphore release")

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

    stack_space = source.split("uint32_t xy_os_thread_get_stack_space", 1)[-1].split(
        "xy_os_status_t xy_os_thread_set_priority", 1
    )[0]
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

    priority_set = source.split("xy_os_status_t xy_os_thread_set_priority", 1)[-1].split(
        "xy_os_priority_t xy_os_thread_get_priority", 1
    )[0]
    if "!freertos_priority_valid(priority)" not in priority_set:
        errors.append("FreeRTOS priority setter must reject invalid priorities before kernel access")

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
        "return NULL;",
    ):
        if required not in semaphore_new:
            errors.append(
                "FreeRTOS semaphore creation must reject zero capacity and an initial "
                f"count above the declared maximum; missing: {required}"
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
