#!/usr/bin/env python3
"""Guard RT-Thread OSAL event-flag semantics."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    errors: list[str] = []

    clear_body = function_body(
        source,
        "uint32_t xy_os_event_flags_clear",
        "uint32_t xy_os_event_flags_get",
    )
    for required in (
        "previous = event->set;",
        "event->set &= ~flags;",
        "return previous;",
    ):
        if required not in clear_body:
            errors.append(
                "RT-Thread event-flags clear must return the complete previous value and "
                f"clear only requested bits; missing: {required}"
            )

    get_body = function_body(
        source,
        "uint32_t xy_os_event_flags_get",
        "uint32_t xy_os_event_flags_wait",
    )
    for required in (
        "level = rt_spin_lock_irqsave(&event->spinlock);",
        "current = event->set;",
        "rt_spin_unlock_irqrestore(&event->spinlock, level);",
        "return current;",
    ):
        if required not in get_body:
            errors.append(
                "RT-Thread event-flags get must read the event set under its spinlock; "
                f"missing: {required}"
            )
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
    if "if (options & XY_OS_FLAGS_NO_CLEAR)" in wait_body:
        errors.append(
            "RT-Thread event-flags wait retains inverted XY_OS_FLAGS_NO_CLEAR semantics"
        )

    if errors:
        print("rtthread_osal_event_flags failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_event_flags_ok clear=selective wait=no-clear-correct")
    return 0


if __name__ == "__main__":
    sys.exit(main())
