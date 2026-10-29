#!/usr/bin/env python3
"""Guard RT-Thread OSAL timeout representability and wait-forever mapping.

RT-Thread wait APIs take a signed tick count where any negative value blocks
forever, so a finite OSAL timeout above INT32_MAX would silently convert a
bounded wait into an unbounded one. Every wait entry point must therefore route
its timeout through one checked conversion.
"""

from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"

HELPER = "rtthread_timeout_to_ticks"

# function start marker -> (end marker, native call that must receive ticks)
WAIT_ENTRIES = (
    ("uint32_t xy_os_event_flags_wait", "xy_os_status_t xy_os_event_flags_delete", "rt_event_recv("),
    ("xy_os_status_t xy_os_mutex_acquire", "xy_os_status_t xy_os_mutex_release", "rt_mutex_take("),
    ("xy_os_status_t xy_os_semaphore_acquire", "xy_os_status_t xy_os_semaphore_release", "rt_sem_take("),
    ("void *xy_os_mempool_alloc", "xy_os_status_t xy_os_mempool_free", "rt_mp_alloc("),
    ("xy_os_status_t xy_os_msgqueue_put", "xy_os_status_t xy_os_msgqueue_get", "rt_mq_send"),
    ("xy_os_status_t xy_os_msgqueue_get", "uint32_t xy_os_msgqueue_get_capacity", "rt_mq_recv("),
    ("xy_os_status_t xy_os_delay(", "xy_os_status_t xy_os_delay_until", "rt_thread_delay("),
    ("xy_os_status_t xy_os_delay_until", "/* Timer Functions */", "rt_thread_delay("),
)


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = read_code(BACKEND)
    errors: list[str] = []

    if f"static int {HELPER}(uint32_t" not in source:
        errors.append(
            "RT-Thread backend must expose one checked uint32_t -> rt_int32_t timeout "
            "conversion instead of narrowing OSAL timeouts inline"
        )
    else:
        helper = function_body(source, f"static int {HELPER}(", "\n}\n") + "\n}"
        for required in ("XY_OS_WAIT_FOREVER", "RT_WAITING_FOREVER", "0x7FFFFFFFU"):
            if required not in helper:
                errors.append(
                    "RT-Thread timeout conversion must map XY_OS_WAIT_FOREVER explicitly and "
                    f"reject finite ticks above the signed native range; missing: {required}"
                )

    for start, end, native_call in WAIT_ENTRIES:
        body = function_body(source, start, end)
        name = start.split("(")[0].replace("xy_os_status_t ", "").replace("void *", "").strip()
        if f"{HELPER}(" not in body:
            errors.append(
                f"{name} must convert its OSAL timeout through {HELPER}() before any native wait"
            )
        converted = body.find(f"{HELPER}(")
        native = body.find(native_call)
        if native < 0:
            errors.append(f"{name} no longer performs its expected native wait call")
        elif converted < 0 or converted > native:
            errors.append(
                f"{name} must validate timeout representability before calling {native_call}"
            )
        for rejected in (
            "timeout == XY_OS_WAIT_FOREVER) ?",
            "xy_os_delay_until",
        ):
            if rejected in body:
                errors.append(
                    f"{name} must not branch on the OSAL forever sentinel or duplicate "
                    "delay logic outside the shared conversion"
                )
        if "xy_os_mempool_alloc" in name:
            if "return NULL" not in body:
                errors.append(
                    "xy_os_mempool_alloc must fail closed on an unrepresentable timeout"
                )
        elif "0x80000000" in body:
            if "0x80000000U" not in body:
                errors.append(
                    f"{name} must return the OSAL error sentinel unchanged on rejection"
                )
        elif "XY_OS_ERROR_PARAMETER" not in body:
            errors.append(
                f"{name} must return a parameter error when the finite timeout cannot be "
                "represented by the native tick type"
            )

    if errors:
        print("rtthread_osal_timeout failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_timeout_ok wait_forever=mapped finite=checked")
    return 0


if __name__ == "__main__":
    sys.exit(main())
