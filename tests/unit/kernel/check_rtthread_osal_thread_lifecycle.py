#!/usr/bin/env python3
"""Guard RT-Thread OSAL dynamic-thread context ownership and teardown atomicity."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    errors: list[str] = []

    new_body = function_body(
        source, "xy_os_thread_id_t xy_os_thread_new", "const char *xy_os_thread_get_name"
    )
    allocation = new_body.find("ctx = alloc_thread_ctx()")
    creation = new_body.find("rt_thread_create(")
    startup = new_body.find("rt_thread_startup(thread)")
    commit = new_body.find("ctx->thread = thread")
    if min(allocation, creation, startup, commit) < 0:
        errors.append("thread creation is missing explicit context allocation/commit steps")
    elif not allocation < creation < commit < startup:
        errors.append("thread context must be reserved before create and committed before startup")
    if "if (!ctx) return NULL;" not in " ".join(new_body.split()):
        errors.append("context exhaustion must fail before creating a kernel thread")
    if "clear_thread_ctx(ctx)" not in new_body:
        errors.append("thread create/startup failure must release the reserved context")

    if "/* RT-Thread detach is unsupported until owner retirement can be observed. */" not in source:
        errors.append("RT-Thread detach must document its fail-closed unsupported contract")
    detach_body = function_body(
        source, "xy_os_status_t xy_os_thread_detach", "xy_os_status_t xy_os_thread_join"
    )
    if "find_thread_ctx(thread)" not in detach_body:
        errors.append("thread detach must reject untracked handles")
    if "return XY_OS_ERROR;" not in detach_body:
        errors.append("thread detach must fail closed while owner retirement is unobservable")
    if "rt_thread_detach(thread)" in detach_body:
        errors.append("thread detach must not detach a kernel object before owner retirement is observable")

    runtime_functions = (
        ("const char *xy_os_thread_get_name", "xy_os_thread_id_t xy_os_thread_get_id"),
        ("xy_os_thread_state_t xy_os_thread_get_state", "uint32_t xy_os_thread_get_stack_size"),
        ("uint32_t xy_os_thread_get_stack_size", "uint32_t xy_os_thread_get_stack_space"),
        ("uint32_t xy_os_thread_get_stack_space", "xy_os_status_t xy_os_thread_set_priority"),
        ("xy_os_status_t xy_os_thread_set_priority", "xy_os_priority_t xy_os_thread_get_priority"),
        ("xy_os_priority_t xy_os_thread_get_priority", "xy_os_status_t xy_os_thread_yield"),
        ("xy_os_status_t xy_os_thread_suspend", "xy_os_status_t xy_os_thread_resume"),
        ("xy_os_status_t xy_os_thread_resume", "xy_os_status_t xy_os_thread_detach"),
        ("xy_os_status_t xy_os_thread_detach", "xy_os_status_t xy_os_thread_join"),
        ("xy_os_status_t xy_os_thread_join", "xy_os_status_t xy_os_thread_exit"),
    )
    for start, end in runtime_functions:
        body = function_body(source, start, end)
        if "find_thread_ctx(thread)" not in body:
            errors.append(f"{start.split('xy_os_', 1)[-1]} must reject untracked handles")

    get_id_body = function_body(
        source, "xy_os_thread_id_t xy_os_thread_get_id", "xy_os_thread_state_t xy_os_thread_get_state"
    )
    if "rt_thread_self()" not in get_id_body or "find_thread_ctx(thread)" not in get_id_body:
        errors.append("thread get-id must publish only a tracked project-owned current thread")
    if "return (xy_os_thread_id_t)rt_thread_self()" in get_id_body:
        errors.append("thread get-id must not expose an untracked kernel-global handle")

    terminate_body = function_body(
        source, "xy_os_status_t xy_os_thread_terminate", "uint32_t xy_os_thread_get_count"
    )
    normalized_terminate = " ".join(terminate_body.split())
    find_pos = normalized_terminate.find("ctx = find_thread_ctx(thread)")
    reject_pos = normalized_terminate.find("if (!ctx) return XY_OS_ERROR_PARAMETER")
    delete_pos = normalized_terminate.find("rt_thread_delete(thread)")
    clear_pos = normalized_terminate.find("clear_thread_ctx(ctx)")
    if min(find_pos, reject_pos, delete_pos) < 0 or not find_pos < reject_pos < delete_pos:
        errors.append("thread termination must reject untracked handles before kernel deletion")
    if delete_pos < 0 or clear_pos < 0 or clear_pos < delete_pos:
        errors.append("thread termination must clear context only after kernel deletion succeeds")
    if "if (result != RT_EOK) return XY_OS_ERROR;" not in normalized_terminate:
        errors.append("kernel delete failure must preserve the context for retry")

    count_body = function_body(
        source, "uint32_t xy_os_thread_get_count", "uint32_t xy_os_thread_enumerate"
    )
    if "g_thread_ctx" not in count_body or "rt_thread_get_count" in count_body:
        errors.append("thread count must report bounded project-owned contexts only")

    enumerate_body = function_body(
        source, "uint32_t xy_os_thread_enumerate", "/* Thread Flags Functions */"
    )
    if "g_thread_ctx" not in enumerate_body:
        errors.append("thread enumeration must use bounded project-owned contexts")
    if "threads[i] = RT_NULL" in enumerate_body or "rt_thread_get_count" in enumerate_body:
        errors.append("thread enumeration must not return kernel-global counts with null handles")
    if "thread_array[count]" not in enumerate_body:
        errors.append("thread enumeration must publish each tracked handle")

    if errors:
        print("rtthread_osal_thread_lifecycle failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_thread_lifecycle_ok allocation=bounded startup=owned teardown=atomic")
    return 0


if __name__ == "__main__":
    sys.exit(main())
