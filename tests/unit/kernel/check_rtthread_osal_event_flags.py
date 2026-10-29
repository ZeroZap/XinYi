#!/usr/bin/env python3
"""Guard RT-Thread OSAL event-flag semantics."""

from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = read_code(BACKEND)
    errors: list[str] = []

    owner_region = function_body(
        source,
        "typedef struct {\n    rt_event_t event;",
        "} event_flags_ctx_t;",
    )
    if "rt_uint8_t initialized;" not in owner_region:
        errors.append("RT-Thread event flags need a bounded owner context; missing initialized flag")

    for required in (
        "#define MAX_EVENT_FLAGS 8",
        "static event_flags_ctx_t g_event_flags_ctx[MAX_EVENT_FLAGS];",
        "static event_flags_ctx_t *alloc_event_flags_ctx(void)",
        "static event_flags_ctx_t *find_event_flags_ctx(rt_event_t event)",
        "static void clear_event_flags_ctx(event_flags_ctx_t *ctx)",
    ):
        if required not in source:
            errors.append(f"RT-Thread event flags need bounded owner helpers; missing: {required}")

    helper_region = function_body(
        source,
        "/* Event Flags Functions */",
        "xy_os_event_flags_id_t xy_os_event_flags_new",
    )
    for required in (
        "static int rtthread_event_flags_args_valid(uint32_t flags, uint32_t options)",
        "flags == 0U",
        "flags & 0x80000000U",
        "options & ~(XY_OS_FLAGS_WAIT_ALL | XY_OS_FLAGS_NO_CLEAR)",
    ):
        if required not in helper_region:
            errors.append(
                "RT-Thread event-flags arguments must reserve the OSAL error bit, reject an "
                f"empty mask, and reject unknown wait options; missing: {required}"
            )

    set_body = function_body(
        source,
        "uint32_t xy_os_event_flags_set",
        "uint32_t xy_os_event_flags_clear",
    )
    for required in (
        "if (!find_event_flags_ctx(event)) return 0x80000000U;",
        "if (!rtthread_event_flags_args_valid(flags, 0U)) return 0x80000000U;",
        "result = rt_event_send(event, flags);",
        "if (result != RT_EOK) return 0x80000000;",
        "return rtthread_event_flags_snapshot(event);",
    ):
        if required not in set_body:
            errors.append(
                "RT-Thread event-flags set must propagate send failure and return the complete "
                f"post-set value; missing: {required}"
            )
    if "? flags :" in set_body:
        errors.append("RT-Thread event-flags set must not return only the requested mask")

    clear_body = function_body(
        source,
        "uint32_t xy_os_event_flags_clear",
        "uint32_t xy_os_event_flags_get",
    )
    for required in (
        "if (!find_event_flags_ctx(event)) return 0x80000000U;",
        "if (!rtthread_event_flags_args_valid(flags, 0U)) return 0x80000000U;",
        "previous = event->set;",
        "event->set &= ~flags;",
        "return previous;",
    ):
        if required not in clear_body:
            errors.append(
                "RT-Thread event-flags clear must return the complete previous value and "
                f"clear only requested bits; missing: {required}"
            )

    helper_body = function_body(
        source,
        "static uint32_t rtthread_event_flags_snapshot",
        "xy_os_event_flags_id_t xy_os_event_flags_new",
    )
    for required in (
        "level = rt_spin_lock_irqsave(&event->spinlock);",
        "current = event->set;",
        "rt_spin_unlock_irqrestore(&event->spinlock, level);",
        "return current;",
    ):
        if required not in helper_body:
            errors.append(
                "RT-Thread event-flags snapshot must read the event set under its spinlock; "
                f"missing: {required}"
            )

    get_body = function_body(
        source,
        "uint32_t xy_os_event_flags_get",
        "uint32_t xy_os_event_flags_wait",
    )
    if "return rtthread_event_flags_snapshot(event);" not in get_body:
        errors.append("RT-Thread event-flags get must return the synchronized event snapshot")
    if "if (!find_event_flags_ctx(event)) return 0x80000000U;" not in get_body:
        errors.append("RT-Thread event-flags get must reject untracked handles")
    if "RT_EVENT_CTRL_GET" in get_body or "rt_event_control(" in get_body:
        errors.append(
            "RT-Thread event-flags get must not depend on the nonexistent "
            "RT_EVENT_CTRL_GET command"
        )

    wait_body = function_body(
        source,
        "uint32_t xy_os_event_flags_wait",
        "xy_os_status_t xy_os_event_flags_delete",
    )
    if "if (!(options & XY_OS_FLAGS_NO_CLEAR))" not in wait_body:
        errors.append(
            "RT-Thread event-flags wait must request RT_EVENT_FLAG_CLEAR only when "
            "XY_OS_FLAGS_NO_CLEAR is absent"
        )
    if "if (!find_event_flags_ctx(event)) return 0x80000000U;" not in wait_body:
        errors.append("RT-Thread event-flags wait must reject untracked handles")
    if "if (options & XY_OS_FLAGS_NO_CLEAR)" in wait_body:
        errors.append(
            "RT-Thread event-flags wait retains inverted XY_OS_FLAGS_NO_CLEAR semantics"
        )
    if "if (!rtthread_event_flags_args_valid(flags, options)) return 0x80000000U;" not in wait_body:
        errors.append(
            "RT-Thread event-flags wait must reject an empty/reserved mask and unknown options "
            "before calling rt_event_recv"
        )

    new_body = function_body(
        source,
        "xy_os_event_flags_id_t xy_os_event_flags_new",
        "const char *xy_os_event_flags_get_name",
    )
    for required in (
        "ctx = alloc_event_flags_ctx();",
        "clear_event_flags_ctx(ctx);",
        "ctx->event = event;",
    ):
        if required not in new_body:
            errors.append(f"RT-Thread event-flags create must bind owner atomically; missing: {required}")

    name_body = function_body(
        source,
        "const char *xy_os_event_flags_get_name",
        "uint32_t xy_os_event_flags_set",
    )
    if "if (!find_event_flags_ctx(event)) return NULL;" not in name_body:
        errors.append("RT-Thread event-flags name must reject untracked handles")

    delete_body = function_body(
        source,
        "xy_os_status_t xy_os_event_flags_delete",
        "/* Mutex Functions */",
    )
    for required in (
        "ctx = find_event_flags_ctx(event);",
        "if (!ctx) return XY_OS_ERROR_PARAMETER;",
        "if (result == RT_EOK) {",
        "clear_event_flags_ctx(ctx);",
    ):
        if required not in delete_body:
            errors.append(f"RT-Thread event-flags delete must preserve owner on failure; missing: {required}")

    if errors:
        print("rtthread_osal_event_flags failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "rtthread_osal_event_flags_ok args=fail-closed set=post-value "
        "clear=selective wait=no-clear-correct"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
