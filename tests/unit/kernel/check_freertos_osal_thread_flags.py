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
    board_cmake = (ROOT / "boards/opench_ch32v307/CMakeLists.txt").read_text(encoding="utf-8")
    board_semaphore = (ROOT / "boards/opench_ch32v307/osal_freertos_semaphore_lifecycle_main.c").read_text(encoding="utf-8")
    board_mutex = (ROOT / "boards/opench_ch32v307/osal_freertos_mutex_lifecycle_main.c").read_text(encoding="utf-8")
    board_queue = (ROOT / "boards/opench_ch32v307/osal_freertos_queue_lifecycle_main.c").read_text(encoding="utf-8")
    board_event_flags = (ROOT / "boards/opench_ch32v307/osal_freertos_event_flags_lifecycle_main.c").read_text(encoding="utf-8")
    board_oneshot_timer = (ROOT / "boards/opench_ch32v307/osal_freertos_oneshot_timer_main.c").read_text(encoding="utf-8")
    board_mempool = (ROOT / "boards/opench_ch32v307/osal_freertos_mempool_lifecycle_main.c").read_text(encoding="utf-8")
    board_thread = (ROOT / "boards/opench_ch32v307/osal_freertos_thread_lifecycle_main.c").read_text(encoding="utf-8")
    board_isr_stress = (ROOT / "boards/opench_ch32v307/osal_freertos_isr_stress_main.c").read_text(encoding="utf-8")
    board_timer_flags = (ROOT / "boards/opench_ch32v307/osal_freertos_timer_flags_main.c").read_text(encoding="utf-8")
    board_primitives = (ROOT / "boards/opench_ch32v307/osal_freertos_primitives_main.c").read_text(encoding="utf-8")
    board_sdk = (ROOT / "boards/opench_ch32v307/osal_freertos_sdk_main.c").read_text(encoding="utf-8")
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
        "ticks -= elapsed",
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
    queue_region = source.split("/* Message Queue */", 1)[-1]
    for required in (
        "char name[16];",
        "memset(context, 0, sizeof(*context));",
        "strncpy(context->name, attr->name, sizeof(context->name) - 1U)",
        "freertos_find_msgqueue_context(context) ? context->name : NULL",
    ):
        if required not in queue_region:
            errors.append(
                "FreeRTOS message queues must preserve an optional bounded name and reject "
                f"stale handles; missing: {required}"
            )
    for required in (
        "typedef struct {\n    EventGroupHandle_t event_group;",
        "freertos_publish_event_flags_context(event_group,",
        "freertos_find_event_flags_context((EventGroupHandle_t)ef_id)",
        "SemaphoreHandle_t mutex;\n    uint8_t recursive;\n    char name[16];",
        "freertos_find_mutex_context(context) ? context->name : NULL",
        "SemaphoreHandle_t semaphore;\n    uint8_t active;\n    char name[16];",
        "freertos_find_semaphore_context((SemaphoreHandle_t)semaphore_id)",
    ):
        if required not in source:
            errors.append(
                "FreeRTOS named synchronization objects must preserve bounded names and "
                f"reject stale handles; missing: {required}"
            )
    for required in (
        "TaskHandle_t handle;",
        "xy_os_thread_func_t func;",
        "void *argument;",
        "uint8_t active;",
        "char name[16];",
        "memset(context, 0, sizeof(*context));",
        "strncpy(context->name, name, sizeof(context->name) - 1U)",
        "return freertos_find_thread_context(h)->name;",
        "TimerHandle_t timer;\n    xy_os_timer_func_t func;",
        "void *argument;",
        "uint8_t active;\n    char name[16];",
        "strncpy(context->name, name, sizeof(context->name) - 1U)",
        "return context ? context->name : NULL;",
    ):
        if required not in source:
            errors.append(
                "FreeRTOS thread and timer names must be retained in adapter-owned bounded "
                f"contexts; missing: {required}"
            )

    timer_callback = source.split("static void fr_timer_callback", 1)[-1].split(
        "xy_os_timer_id_t xy_os_timer_new", 1
    )[0]
    for required in (
        "pvTimerGetTimerID(timer)",
        "context->func(context->argument)",
    ):
        if required not in timer_callback:
            errors.append(
                "FreeRTOS timer callbacks must recover the adapter context and preserve the "
                f"caller argument; missing: {required}"
            )
    timer_create = source.split("xy_os_timer_id_t xy_os_timer_new", 1)[-1].split(
        "const char *xy_os_timer_get_name", 1
    )[0]
    for required in (
        "context->argument = argument;",
        "xTimerCreate(name, 1, reload, context, fr_timer_callback)",
    ):
        if required not in timer_create:
            errors.append(
                "FreeRTOS timer creation must bind the typed callback trampoline and caller "
                f"argument; missing: {required}"
            )
    if "(TimerCallbackFunction_t)" in timer_create or "(void (*)(" in timer_create:
        errors.append("FreeRTOS timer creation must not cast the OSAL callback to a native callback type")
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
        "if (!timer)",
        "freertos_find_timer_context(timer)",
        "if (!context)",
        "return XY_OS_ERROR_PARAMETER;",
        "if (result == pdPASS)",
        "freertos_clear_timer_context(context);",
    ):
        if required not in timer_delete:
            errors.append(
                "FreeRTOS timer delete must validate callback context and free it only after "
                f"native deletion succeeds; missing: {required}"
            )

    event_flags_get = source.split("uint32_t xy_os_event_flags_get", 1)[-1].split(
        "uint32_t xy_os_event_flags_wait", 1
    )[0]
    if "!ef_id || !freertos_find_event_flags_context" not in event_flags_get or "return 0x80000000U;" not in event_flags_get:
        errors.append("FreeRTOS event-flags get must fail closed for a NULL handle")

    event_flags_delete = source.split("xy_os_status_t xy_os_event_flags_delete", 1)[-1]
    for required in (
        "!event_group || !freertos_find_event_flags_context(event_group)",
        "vEventGroupDelete(event_group)",
        "XY_OS_ERROR_PARAMETER",
    ):
        if required not in event_flags_delete:
            errors.append(
                "FreeRTOS event-flags delete must validate the handle and avoid native access "
                f"for NULL; missing: {required}"
            )

    if "freertos_timeout_to_ticks" not in source:
        errors.append("FreeRTOS wait APIs must use one checked timeout conversion helper")
    semaphore_region = source.split("/* Semaphore */", 1)[-1].split(
        "/* Memory Pool", 1
    )[0]
    for required in (
        "FREERTOS_SEMAPHORE_CONTEXT_CAPACITY",
        "freertos_alloc_semaphore_context",
        "freertos_find_semaphore_context",
        "freertos_clear_semaphore_context",
        "context->semaphore = sem",
        "vSemaphoreDelete(sem);",
    ):
        if required not in semaphore_region:
            errors.append(
                "FreeRTOS semaphore operations must publish only adapter-owned handles and "
                f"release an untracked allocation; missing: {required}"
            )
    for function_name in (
        "xy_os_semaphore_acquire",
        "xy_os_semaphore_release",
        "xy_os_semaphore_release_from_isr",
        "xy_os_semaphore_get_count",
        "xy_os_semaphore_delete",
    ):
        function = semaphore_region.split(function_name, 1)[-1]
        if "freertos_find_semaphore_context" not in function:
            errors.append(
                f"{function_name} must reject untracked native semaphore handles before kernel access"
            )
    for function_name in (
        "xy_os_mutex_acquire",
        "xy_os_semaphore_acquire",
        "xy_os_msgqueue_put",
        "xy_os_msgqueue_get",
    ):
        function = source.split(function_name, 1)[-1]
        if "freertos_timeout_to_ticks(timeout, &ticks)" not in function:
            errors.append(f"{function_name} must reject unrepresentable timeout ticks")

    mutex_region = source.split("/* Mutex */", 1)[-1].split("/* Semaphore */", 1)[0]
    for required in (
        "FREERTOS_MUTEX_CONTEXT_CAPACITY",
        "freertos_mutex_context_available",
        "freertos_find_mutex_context",
        "freertos_publish_mutex_context",
        "freertos_clear_mutex_context",
        "freertos_find_mutex_context(context)",
        "freertos_clear_mutex_context(context);",
    ):
        if required not in mutex_region:
            errors.append(
                "FreeRTOS mutex operations must publish only adapter-owned contexts and "
                f"reject stale handles; missing: {required}"
            )

    for function_name, argument_name in (
        ("xy_os_thread_flags_wait", "timeout"),
        ("xy_os_event_flags_wait", "timeout"),
        ("xy_os_delay", "ticks"),
        ("xy_os_delay_until", "ticks"),
    ):
        function = source.split(function_name, 1)[-1]
        if f"freertos_timeout_to_ticks({argument_name}, &" not in function:
            errors.append(f"{function_name} must reject unrepresentable native tick inputs")

    semaphore_delete = source.split("xy_os_status_t xy_os_semaphore_delete", 1)[-1].split(
        "/* Memory Pool", 1
    )[0]
    if "!semaphore_id ||" not in semaphore_delete:
        errors.append("FreeRTOS semaphore delete must reject NULL handles")
    for required in (
        "freertos_find_semaphore_context((SemaphoreHandle_t)semaphore_id)",
        "vSemaphoreDelete((SemaphoreHandle_t)semaphore_id)",
        "freertos_clear_semaphore_context(context)",
    ):
        if required not in semaphore_delete:
            errors.append(f"FreeRTOS semaphore delete must preserve tracked lifecycle: {required}")

    semaphore_new = source.split("xy_os_semaphore_id_t xy_os_semaphore_new", 1)[-1].split(
        "const char *xy_os_semaphore_get_name", 1
    )[0]
    for required in (
        "max_count == 0U",
        "initial_count > max_count",
        "freertos_alloc_semaphore_context()",
        "vSemaphoreDelete(sem)",
    ):
        if required not in semaphore_new:
            errors.append(f"FreeRTOS semaphore creation must be failure-atomic: {required}")

    for required in (
        "osal_freertos_queue_lifecycle_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_QUEUE_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_QUEUE_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "OPENCH_OSAL_QUEUE_LIFECYCLE_PASS_20261018_A",
        "OPENCH_OSAL_QUEUE_LIFECYCLE_STALE_OK_A",
        "opench_ch32v307_osal_freertos_queue_lifecycle_smoke",
    ):
        if required not in board_cmake + board_queue:
            errors.append(f"FreeRTOS queue lifecycle HIL target must remain wired: {required}")
    for required in (
        "osal_freertos_semaphore_lifecycle_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_SEMAPHORE_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_SEMAPHORE_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "OPENCH_OSAL_SEMAPHORE_LIFECYCLE_PASS_20261018_A",
        "opench_ch32v307_osal_freertos_semaphore_lifecycle_smoke",
    ):
        if required not in board_cmake + board_semaphore:
            errors.append(f"FreeRTOS semaphore lifecycle HIL target must remain wired: {required}")
    for required in (
        "osal_freertos_mutex_lifecycle_main.c",
        "OPENCH_OSAL_MUTEX_LIFECYCLE_PASS_20261018_A",
        "opench_ch32v307_osal_freertos_mutex_lifecycle_smoke",
    ):
        if required not in board_cmake + board_mutex:
            errors.append(f"FreeRTOS mutex lifecycle HIL target must remain wired: {required}")
    for required in (
        "osal_freertos_event_flags_lifecycle_main.c",
        "OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_PASS_20261018_A",
        "OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_STALE_OK_A",
        "opench_ch32v307_osal_freertos_event_flags_lifecycle_smoke",
    ):
        if required not in board_cmake + board_event_flags:
            errors.append(f"FreeRTOS event-flags lifecycle HIL target must remain wired: {required}")
    for required in (
        "osal_freertos_oneshot_timer_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_ONESHOT_TIMER_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "OPENCH_OSAL_ONESHOT_TIMER_RESTART_OK_A",
        "OPENCH_OSAL_ONESHOT_TIMER_DELETE_STABLE_OK_A",
        "opench_ch32v307_osal_freertos_oneshot_timer_smoke",
    ):
        if required not in board_cmake + board_oneshot_timer:
            errors.append(f"FreeRTOS one-shot timer lifecycle target must remain identity-bound and wired: {required}")
    for required in (
        "osal_freertos_mempool_lifecycle_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_MEMPOOL_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "OPENCH_OSAL_MEMPOOL_LIFECYCLE_METADATA_OK_A",
        "OPENCH_OSAL_MEMPOOL_LIFECYCLE_EXHAUST_OK_A",
        "OPENCH_OSAL_MEMPOOL_LIFECYCLE_RESOURCE_OK_A",
        "opench_ch32v307_osal_freertos_mempool_lifecycle_smoke",
    ):
        if required not in board_cmake + board_mempool:
            errors.append(f"FreeRTOS memory-pool lifecycle target must remain identity-bound and wired: {required}")
    for required in (
        "osal_freertos_thread_lifecycle_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_THREAD_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_THREAD_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_OK_A",
        "OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_OK_A",
        "OPENCH_OSAL_THREAD_LIFECYCLE_PASS_20261018_A",
        "opench_ch32v307_osal_freertos_thread_lifecycle_smoke",
    ):
        if required not in board_cmake + board_thread:
            errors.append(f"FreeRTOS thread lifecycle target must remain identity-bound and wired: {required}")
    for required in (
        "osal_freertos_isr_stress_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_ISR_STRESS_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_ISR_STRESS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "OPENCH_OSAL_ISR_STRESS_ACCOUNTING_OK_A",
        "OPENCH_OSAL_ISR_STRESS_WORKER_OK_A",
        "opench_ch32v307_osal_freertos_isr_stress_smoke",
    ):
        if required not in board_cmake + board_isr_stress:
            errors.append(f"FreeRTOS ISR stress target must remain identity-bound and wired: {required}")
    for required in (
        "osal_freertos_sdk_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_FREERTOS_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_FREERTOS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "opench_ch32v307_osal_freertos_sdk_smoke",
    ):
        if required not in board_cmake + board_sdk:
            errors.append(f"FreeRTOS SDK OSAL target must remain identity-bound and wired: {required}")
    for required in (
        "osal_freertos_primitives_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_PRIMITIVES_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_PRIMITIVES_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "opench_ch32v307_osal_freertos_primitives_smoke",
    ):
        if required not in board_cmake + board_primitives:
            errors.append(f"FreeRTOS primitives target must remain identity-bound and wired: {required}")
    for required in (
        "osal_freertos_timer_flags_main.c",
        "XINYI_FIRMWARE_COMMIT=\"${_firmware_commit}\"",
        'mark("OPENCH_OSAL_TIMER_FLAGS_BOOT_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        'mark("OPENCH_OSAL_TIMER_FLAGS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\\r\\n");',
        "OPENCH_OSAL_TIMER_FLAGS_PERIODIC_OK_A",
        "OPENCH_OSAL_TIMER_FLAGS_STOP_STABLE_OK_A",
        "opench_ch32v307_osal_freertos_timer_flags_smoke",
    ):
        if required not in board_cmake + board_timer_flags:
            errors.append(f"FreeRTOS timer/thread-flags target must remain identity-bound and wired: {required}")
    for board_name in (
        "event_flags_lifecycle",
        "isr_stress",
        "mempool_lifecycle",
        "mutex_lifecycle",
        "oneshot_timer",
        "primitives",
        "queue_lifecycle",
        "semaphore_lifecycle",
        "thread_lifecycle",
        "timer_flags",
    ):
        board_source = (ROOT / f"boards/opench_ch32v307/osal_freertos_{board_name}_main.c").read_text(
            encoding="utf-8"
        )
        if '#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"' not in board_source:
            errors.append(f"FreeRTOS {board_name} image must fail closed when source identity is absent")
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
        "!freertos_tick_value_valid(ticks)",
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

    timer_region = source.split("/* Timer Management */", 1)[-1].split(
        "/* Event Flags */", 1
    )[0]
    for required in (
        "FREERTOS_TIMER_CONTEXT_CAPACITY",
        "freertos_alloc_timer_context",
        "freertos_find_timer_context",
        "freertos_clear_timer_context",
        "context->timer = timer",
        "char name[16];",
        "strncpy(context->name, name, sizeof(context->name) - 1U)",
        "return context ? context->name : NULL;",
    ):
        if required not in timer_region:
            errors.append(
                "FreeRTOS timer operations must publish only adapter-owned timers; "
                f"missing: {required}"
            )
    for function_name in (
        "xy_os_timer_start",
        "xy_os_timer_stop",
        "xy_os_timer_is_running",
        "xy_os_timer_delete",
    ):
        function = source.split(function_name, 1)[-1]
        if "freertos_find_timer_context" not in function:
            errors.append(
                f"{function_name} must reject untracked native timer handles before kernel access"
            )
    if "pvTimerGetTimerID((TimerHandle_t)timer_id)" in timer_region:
        errors.append(
            "FreeRTOS timer deletion must not trust a native timer context outside the adapter owner table"
        )

    event_flags_region = source.split("/* Event Flags */", 1)[-1].split("/* Mutex */", 1)[0]
    event_flags_new = event_flags_region.split(
        "xy_os_event_flags_new", 1
    )[-1].split("const char *xy_os_event_flags_get_name", 1)[0]
    for required in (
        "FREERTOS_EVENT_FLAGS_CONTEXT_CAPACITY",
        "freertos_event_flags_context_available",
        "freertos_find_event_flags_context",
        "freertos_publish_event_flags_context",
        "freertos_clear_event_flags_context",
        "attr->attr_bits != 0U",
        "attr->cb_mem",
        "attr->cb_size != 0U",
    ):
        if required not in event_flags_region:
            errors.append(
                "FreeRTOS event-flags creation must reject unsupported caller-owned/control "
                f"attributes before allocation; missing: {required}"
            )

    event_flags_region = source.split("/* Event Flags */", 1)[-1].split("/* Mutex */", 1)[0]
    for function_name in (
        "xy_os_event_flags_set",
        "xy_os_event_flags_clear",
        "xy_os_event_flags_get",
        "xy_os_event_flags_wait",
        "xy_os_event_flags_delete",
    ):
        function = event_flags_region.split(function_name, 1)[-1]
        if "freertos_find_event_flags_context" not in function:
            errors.append(
                f"{function_name} must reject untracked native event-group handles before kernel access"
            )
    if "freertos_clear_event_flags_context(event_group);" not in event_flags_region:
        errors.append("FreeRTOS event-flags delete must clear owner state after native deletion")

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

    mutex_new = source.split("xy_os_mutex_id_t xy_os_mutex_new", 1)[-1].split(
        "const char *xy_os_mutex_get_name", 1
    )[0]
    for required in (
        "freertos_mutex_context_available()",
        "freertos_publish_mutex_context(context);",
        "vPortFree(context);",
    ):
        if required not in mutex_new:
            errors.append(f"FreeRTOS mutex creation must preserve bounded ownership: {required}")
    for required in (
        "xy_os_thread_func_t func;",
        "void *argument;",
        "context->active = 1U;",
        "freertos_thread_entry",
        "xTaskCreate(freertos_thread_entry, name, stack, context, prio, &handle)",
        "freertos_clear_thread_context(context->handle);",
        "if (context->active)",
        "context->handle = handle;",
    ):
        if required not in source:
            errors.append(f"FreeRTOS thread exit must retire adapter context: {required}")
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

    mempool_new = source.split("xy_os_mempool_id_t xy_os_mempool_new", 1)[-1].split(
        "const char *xy_os_mempool_get_name", 1
    )[0]
    mempool_region = source.split("/* Memory Pool", 1)[-1].split("/* Message Queue", 1)[0]
    for required in (
        "FREERTOS_MEMPOOL_CONTEXT_CAPACITY",
        "freertos_mempool_context_available",
        "freertos_find_mempool_context",
        "freertos_publish_mempool_context",
        "freertos_clear_mempool_context",
        "freertos_publish_mempool_context(p);",
        "freertos_clear_mempool_context(p);",
    ):
        if required not in mempool_region:
            errors.append(
                "FreeRTOS memory pools must publish only adapter-owned contexts and "
                f"reject stale handles; missing: {required}"
            )
    for function_name in (
        "xy_os_mempool_get_name",
        "xy_os_mempool_alloc",
        "xy_os_mempool_free",
        "xy_os_mempool_get_capacity",
        "xy_os_mempool_get_block_size",
        "xy_os_mempool_get_count",
        "xy_os_mempool_get_space",
        "xy_os_mempool_delete",
    ):
        function = source.split(function_name, 1)[-1]
        if "freertos_find_mempool_context" not in function:
            errors.append(
                f"{function_name} must reject untracked memory-pool handles before native access"
            )

    for required in (
        "attr->attr_bits != 0U",
        "attr->cb_mem",
        "attr->cb_size != 0U",
        "!attr->mp_mem && attr->mp_size != 0U",
    ):
        if required not in mempool_new:
            errors.append(
                "FreeRTOS memory-pool creation must reject unsupported control/storage "
                f"attributes before allocation; missing: {required}"
            )

    mempool_alloc = source.split("void *xy_os_mempool_alloc", 1)[-1].split(
        "xy_os_status_t xy_os_mempool_free", 1
    )[0]
    for required in (
        "TickType_t ticks;",
        "!freertos_timeout_to_ticks(timeout, &ticks)",
    ):
        if required not in mempool_alloc:
            errors.append(
                "FreeRTOS memory-pool allocation must reject unrepresentable native "
                f"timeouts before polling; missing: {required}"
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

    thread_identity = source.split("const char *xy_os_thread_get_name", 1)[-1].split(
        "/* Thread Flags", 1
    )[0]
    for required in (
        "freertos_find_thread_context",
        "if (!h || !freertos_find_thread_context(h))",
    ):
        if required not in thread_identity:
            errors.append(
                "FreeRTOS handle-based thread queries must validate adapter ownership before "
                f"native access; missing: {required}"
            )
    thread_new = source.split("xy_os_thread_id_t xy_os_thread_new", 1)[-1].split(
        "const char *xy_os_thread_get_name", 1
    )[0]
    for required in (
        "freertos_alloc_thread_context",
        "context->handle = handle",
        "memset(context, 0, sizeof(*context));",
    ):
        if required not in thread_new:
            errors.append(
                "FreeRTOS thread creation must register a bounded adapter owner and release the "
                f"reserved slot when native creation fails; missing: {required}"
            )
    if "if (xTaskCreate(func, name, stack, argument, prio, &handle) != pdPASS)\n        return NULL;" in thread_new:
        errors.append(
            "FreeRTOS thread creation must not leak a bounded owner slot after xTaskCreate failure"
        )
    if "freertos_clear_thread_context" not in source:
        errors.append(
            "FreeRTOS thread ownership must provide a cleanup path for exiting adapter-owned threads"
        )

    mutex_owner = source.split("xy_os_thread_id_t xy_os_mutex_get_owner", 1)[-1].split(
        "xy_os_status_t xy_os_mutex_delete", 1
    )[0]
    for required in (
        "xSemaphoreGetMutexHolder(context->mutex)",
        "freertos_find_thread_context(owner)",
        "return (xy_os_thread_id_t)owner;",
    ):
        if required not in mutex_owner:
            errors.append(
                "FreeRTOS mutex owner query must publish only an adapter-owned thread; "
                f"missing: {required}"
            )
    if "return (xy_os_thread_id_t)xSemaphoreGetMutexHolder(context->mutex);" in mutex_owner:
        errors.append(
            "FreeRTOS mutex owner query must validate the native owner before publishing its handle"
        )

    thread_inventory = source.split("uint32_t xy_os_thread_get_count", 1)[-1].split(
        "/* Thread Flags", 1
    )[0]
    for required in (
        "return 0U;",
        "FreeRTOS exposes kernel-global task inventory",
        "Do not publish NULL placeholders or untracked kernel-global handles",
    ):
        if required not in thread_inventory:
            errors.append(
                "FreeRTOS thread inventory must fail closed because kernel-global task "
                f"handles are not adapter-owned; missing: {required}"
            )
    if "uxTaskGetNumberOfTasks" in thread_inventory:
        errors.append(
            "FreeRTOS thread count must not expose kernel-global task inventory as OSAL-owned"
        )

    terminate = source.split("xy_os_status_t xy_os_thread_terminate", 1)[-1]
    if "return thread_id ? XY_OS_ERROR : XY_OS_ERROR_PARAMETER;" not in terminate:
        errors.append(
            "FreeRTOS thread terminate must fail closed because vTaskDelete provides no "
            "observable retirement result"
        )
    if "vTaskDelete((TaskHandle_t)thread_id)" in terminate:
        errors.append("FreeRTOS thread terminate must not report success after unobservable deletion")
    if "freertos_thread_entry" not in source:
        errors.append("FreeRTOS tasks must use an adapter-owned entry trampoline")

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

    msgqueue_region = source.split("/* Message Queue */", 1)[-1]
    for required in (
        "FREERTOS_MSGQUEUE_CONTEXT_CAPACITY",
        "freertos_msgqueue_context_available",
        "freertos_find_msgqueue_context",
        "freertos_publish_msgqueue_context",
        "freertos_clear_msgqueue_context",
        "freertos_publish_msgqueue_context(context);",
        "freertos_clear_msgqueue_context(context);",
    ):
        if required not in msgqueue_region:
            errors.append(
                "FreeRTOS message queues must publish only adapter-owned contexts and "
                f"reject stale handles; missing: {required}"
            )
    for function_name in (
        "xy_os_msgqueue_put",
        "xy_os_msgqueue_get",
        "xy_os_msgqueue_get_capacity",
        "xy_os_msgqueue_get_msg_size",
        "xy_os_msgqueue_get_count",
        "xy_os_msgqueue_get_space",
        "xy_os_msgqueue_reset",
        "xy_os_msgqueue_delete",
    ):
        function = source.split(function_name, 1)[-1]
        if "freertos_find_msgqueue_context" not in function:
            errors.append(
                f"{function_name} must reject untracked native message-queue handles "
                "before kernel access"
            )

    msgqueue_put = source.split("xy_os_status_t xy_os_msgqueue_put", 1)[-1].split(
        "xy_os_status_t xy_os_msgqueue_get", 1
    )[0]
    if "msg_prio != 0U" not in msgqueue_put:
        errors.append(
            "FreeRTOS message queues must reject nonzero message priorities because the "
            "native FIFO queue cannot preserve the public priority contract"
        )

    msgqueue_new = source.split("xy_os_msgqueue_id_t xy_os_msgqueue_new", 1)[-1].split(
        "const char *xy_os_msgqueue_get_name", 1
    )[0]
    for required in ("attr->mq_mem", "attr->mq_size"):
        if required not in msgqueue_new:
            errors.append(
                "FreeRTOS message queues must reject caller-owned queue storage because "
                f"dynamic creation cannot honor {required}"
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
