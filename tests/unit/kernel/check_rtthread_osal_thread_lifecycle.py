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

    terminate_body = function_body(
        source, "xy_os_status_t xy_os_thread_terminate", "uint32_t xy_os_thread_get_count"
    )
    delete_pos = terminate_body.find("rt_thread_delete(thread)")
    clear_pos = terminate_body.find("clear_thread_ctx(ctx)")
    if "find_thread_ctx(thread)" not in terminate_body:
        errors.append("thread termination must resolve the project-owned context")
    if delete_pos < 0 or clear_pos < 0 or clear_pos < delete_pos:
        errors.append("thread termination must clear context only after kernel deletion succeeds")
    normalized_terminate = " ".join(terminate_body.split())
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
