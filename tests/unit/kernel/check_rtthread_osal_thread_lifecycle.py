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
    )
    for start, end in runtime_functions:
        body = function_body(source, start, end)
        if "find_thread_ctx(thread)" not in body:
            errors.append(f"{start.split('xy_os_', 1)[-1]} must reject untracked handles")

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

    if errors:
        print("rtthread_osal_thread_lifecycle failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_thread_lifecycle_ok allocation=bounded startup=owned teardown=atomic")
    return 0


if __name__ == "__main__":
    sys.exit(main())
