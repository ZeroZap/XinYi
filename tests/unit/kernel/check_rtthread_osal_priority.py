#!/usr/bin/env python3
"""Guard RT-Thread OSAL priority range and monotonic mapping semantics."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    errors: list[str] = []

    to_rt = function_body(source, "static rt_uint8_t xy_to_rt_priority", "static xy_os_priority_t rt_to_xy_priority")
    for required in (
        "RT_THREAD_PRIORITY_MAX - 1U",
        "XY_OS_PRIORITY_ISR",
        "xy_priority - (uint32_t)XY_OS_PRIORITY_IDLE",
    ):
        if required not in to_rt:
            errors.append(f"XY-to-RT priority mapping is missing bounded scaling: {required}")
    if "max_priority - (rt_uint8_t)priority" in to_rt:
        errors.append("XY-to-RT priority mapping still permits unsigned underflow")

    to_xy = function_body(source, "static xy_os_priority_t rt_to_xy_priority", "/* Kernel Functions */")
    for required in (
        "RT_THREAD_PRIORITY_MAX - 1U",
        "XY_OS_PRIORITY_ISR",
        "(uint32_t)XY_OS_PRIORITY_IDLE +",
        "rt_priority > rt_lowest",
    ):
        if required not in to_xy:
            errors.append(f"RT-to-XY priority mapping is missing bounded inverse scaling: {required}")

    new_body = function_body(source, "xy_os_thread_id_t xy_os_thread_new", "const char *xy_os_thread_get_name")
    if "rtthread_priority_valid(attr->priority)" not in new_body:
        errors.append("thread creation must reject NONE/error/reserved/out-of-range priorities")

    set_body = function_body(source, "xy_os_status_t xy_os_thread_set_priority", "xy_os_priority_t xy_os_thread_get_priority")
    if "rtthread_priority_valid(priority)" not in set_body:
        errors.append("priority setter must reject NONE/error/reserved/out-of-range priorities")

    if errors:
        print("rtthread_osal_priority failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_priority_ok range=bounded direction=monotonic")
    return 0


if __name__ == "__main__":
    sys.exit(main())
