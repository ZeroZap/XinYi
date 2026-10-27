#!/usr/bin/env python3
"""Guard the RT-Thread OSAL kernel lock/unlock/restore-lock nesting contract.

The OSAL contract is: lock() returns the previous lock state, unlock() returns
the previous lock state, and restore_lock(saved) restores that state and returns
the resulting state. RT-Thread critical sections are nesting-counted, so an
implementation that always returns zero and unconditionally calls
rt_exit_critical() cannot honour restore_lock(): it releases a critical section
owned by an outer caller and silently breaks mutual exclusion.
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"

LEVEL = "rt_critical_level()"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    errors: list[str] = []

    lock_body = function_body(
        source, "int32_t xy_os_kernel_lock(void)", "int32_t xy_os_kernel_unlock(void)"
    )
    unlock_body = function_body(
        source, "int32_t xy_os_kernel_unlock(void)", "int32_t xy_os_kernel_restore_lock"
    )
    restore_body = function_body(
        source, "int32_t xy_os_kernel_restore_lock(int32_t lock)", "\n}\n"
    )

    # lock(): must publish the real previous nesting level, not a placeholder.
    if "rt_enter_critical();" not in lock_body:
        errors.append("xy_os_kernel_lock must enter the RT-Thread critical section")
    if LEVEL not in lock_body:
        errors.append(
            "xy_os_kernel_lock must report the previous critical nesting level instead of a "
            "constant placeholder"
        )
    if "return 0;" in lock_body:
        errors.append(
            "xy_os_kernel_lock must not return a constant zero lock state; callers need the "
            "previous level to restore it later"
        )
    if "rt_exit_critical" in lock_body:
        errors.append("xy_os_kernel_lock must not exit a critical section")

    # unlock(): must refuse to exit below level zero rather than underflowing.
    if "rt_exit_critical();" not in unlock_body:
        errors.append("xy_os_kernel_unlock must exit the RT-Thread critical section")
    if LEVEL not in unlock_body:
        errors.append(
            "xy_os_kernel_unlock must read the current nesting level before exiting the "
            "critical section"
        )
    guard = unlock_body.find(LEVEL)
    native_exit = unlock_body.find("rt_exit_critical();")
    if guard < 0 or native_exit < 0 or guard > native_exit:
        errors.append(
            "xy_os_kernel_unlock must reject an empty critical section before calling "
            "rt_exit_critical(), because exit at level zero underflows the nesting counter"
        )
    if "-1" not in unlock_body:
        errors.append(
            "xy_os_kernel_unlock must fail closed with a negative status when there is no "
            "critical section to release"
        )
    if "return 0;" in unlock_body and "-1" not in unlock_body:
        errors.append("xy_os_kernel_unlock still reports a constant zero state")

    # restore_lock(): must honour its argument instead of releasing unconditionally.
    if "XY_UNUSED(lock);" in restore_body:
        errors.append(
            "xy_os_kernel_restore_lock must not discard the recorded lock state"
        )
    if "lock < 0" not in restore_body:
        errors.append(
            "xy_os_kernel_restore_lock must reject a negative lock state before touching the "
            "critical section"
        )
    validate = restore_body.find("lock < 0")
    first_change = min(
        (index for index in (
            restore_body.find("rt_enter_critical();"),
            restore_body.find("rt_exit_critical();"),
        ) if index >= 0),
        default=-1,
    )
    if validate < 0 or first_change < 0 or validate > first_change:
        errors.append(
            "xy_os_kernel_restore_lock must validate the requested state before changing the "
            "critical nesting level"
        )
    if LEVEL not in restore_body:
        errors.append(
            "xy_os_kernel_restore_lock must compare the requested state against the live "
            "critical nesting level"
        )
    for required in ("rt_enter_critical();", "rt_exit_critical();"):
        if required not in restore_body:
            errors.append(
                "xy_os_kernel_restore_lock must be able to move the nesting level in both "
                f"directions; missing: {required}"
            )

    if errors:
        print("rtthread_osal_kernel_lock failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_kernel_lock_ok lock=previous-level restore=exact-level")
    return 0


if __name__ == "__main__":
    sys.exit(main())
