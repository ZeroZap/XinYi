#!/usr/bin/env python3
"""Guard RT-Thread OSAL mutex/semaphore context ownership and teardown atomicity."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def check_lifecycle(
    source: str,
    kind: str,
    new_start: str,
    new_end: str,
    create_call: str,
    delete_start: str,
    delete_end: str,
    delete_call: str,
    errors: list[str],
) -> None:
    new_body = function_body(source, new_start, new_end)
    allocation = new_body.find(f"ctx = alloc_{kind}_ctx()")
    creation = new_body.find(create_call)
    commit = new_body.find(f"ctx->{kind} = {kind}")
    if min(allocation, creation, commit) < 0:
        errors.append(f"{kind} creation is missing explicit context allocation/commit steps")
    elif not allocation < creation < commit:
        errors.append(f"{kind} context must be reserved before kernel create and committed afterward")
    if f"if (!ctx) return NULL;" not in " ".join(new_body.split()):
        errors.append(f"{kind} context exhaustion must fail before kernel allocation")
    if f"clear_{kind}_ctx(ctx)" not in new_body:
        errors.append(f"{kind} kernel-create failure must release the reserved context")

    delete_body = function_body(source, delete_start, delete_end)
    find_pos = delete_body.find(f"find_{kind}_ctx({kind})")
    delete_pos = delete_body.find(delete_call)
    clear_pos = delete_body.find(f"clear_{kind}_ctx(ctx)")
    if find_pos < 0:
        errors.append(f"{kind} deletion must resolve the project-owned context")
    if delete_pos < 0 or clear_pos < 0 or clear_pos < delete_pos:
        errors.append(f"{kind} deletion must clear context only after kernel deletion succeeds")
    if "if (!ctx) return XY_OS_ERROR_PARAMETER;" not in " ".join(delete_body.split()):
        errors.append(f"untracked {kind} handles must fail closed before kernel deletion")
    if "if (result != RT_EOK) return XY_OS_ERROR;" not in " ".join(delete_body.split()):
        errors.append(f"{kind} kernel-delete failure must preserve context for retry")


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    errors: list[str] = []

    check_lifecycle(
        source,
        "mutex",
        "xy_os_mutex_id_t xy_os_mutex_new",
        "const char *xy_os_mutex_get_name",
        "rt_mutex_create(name, flag)",
        "xy_os_status_t xy_os_mutex_delete",
        "/* Semaphore Functions */",
        "rt_mutex_delete(mutex)",
        errors,
    )
    check_lifecycle(
        source,
        "sem",
        "xy_os_semaphore_id_t xy_os_semaphore_new",
        "const char *xy_os_semaphore_get_name",
        "rt_sem_create(name, initial_count, flag)",
        "xy_os_status_t xy_os_semaphore_delete",
        "/* Memory Pool Functions */",
        "rt_sem_delete(sem)",
        errors,
    )

    sem_new = function_body(
        source,
        "xy_os_semaphore_id_t xy_os_semaphore_new",
        "const char *xy_os_semaphore_get_name",
    )
    if "max_count == 0U || initial_count > max_count" not in sem_new:
        errors.append("semaphore creation must reject zero capacity and initial_count overflow")

    if errors:
        print("rtthread_osal_sync_lifecycle failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_sync_lifecycle_ok allocation=bounded teardown=atomic handles=owned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
