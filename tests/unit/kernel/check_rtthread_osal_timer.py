#!/usr/bin/env python3
"""Guard RT-Thread OSAL timer callback ownership and current API semantics."""

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

    new_body = function_body(source, "xy_os_timer_id_t xy_os_timer_new", "const char *xy_os_timer_get_name")
    for required in (
        "ctx = alloc_timer_ctx()",
        "timer_callback,",
        "ctx,",
        "RT_TIMER_FLAG_SOFT_TIMER",
        "RT_TIMER_FLAG_PERIODIC",
        "ctx->timer = timer",
    ):
        if required not in new_body:
            errors.append(f"timer creation is missing callback-owner contract: {required}")
    if "RT_TIMER_CTRL_SET_FLAG" in new_body:
        errors.append("timer creation still uses nonexistent RT_TIMER_CTRL_SET_FLAG")
    if "argument," in new_body.split("rt_timer_create", 1)[-1].split(";", 1)[0]:
        errors.append("RT-Thread timer callback receives user argument instead of timer context")
    if "clear_timer_ctx(ctx)" not in new_body:
        errors.append("timer kernel-create failure must release the reserved context")

    name_body = function_body(
        source,
        "const char *xy_os_timer_get_name",
        "xy_os_status_t xy_os_timer_start",
    )
    if "find_timer_ctx(timer)" not in name_body:
        errors.append("timer name query must reject untracked kernel handles before field access")

    start_body = function_body(source, "xy_os_status_t xy_os_timer_start", "xy_os_status_t xy_os_timer_stop")
    for required in (
        "find_timer_ctx(timer)",
        "ticks == 0U",
        "rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME, &ticks)",
        "if (result != RT_EOK) return XY_OS_ERROR;",
    ):
        if required not in start_body:
            errors.append(f"timer start is missing guarded configuration contract: {required}")

    stop_body = function_body(source, "xy_os_status_t xy_os_timer_stop", "uint32_t xy_os_timer_is_running")
    if "find_timer_ctx(timer)" not in stop_body:
        errors.append("timer stop must reject untracked kernel handles")

    running_body = function_body(source, "uint32_t xy_os_timer_is_running", "xy_os_status_t xy_os_timer_delete")
    if "RT_TIMER_CTRL_GET_STATE" not in running_body:
        errors.append("timer running query must use RT_TIMER_CTRL_GET_STATE")
    if "RT_OBJECT_INIT" in running_body:
        errors.append("timer running query still mistakes object initialization for activation")
    if "find_timer_ctx(timer)" not in running_body:
        errors.append("timer running query must reject untracked kernel handles")

    delete_body = function_body(source, "xy_os_status_t xy_os_timer_delete", "/* Event Flags Functions */")
    if "find_timer_ctx(timer)" not in delete_body:
        errors.append("timer deletion must reject untracked kernel handles")
    if delete_body.find("rt_timer_delete(timer)") > delete_body.find("ctx->timer = NULL") >= 0:
        errors.append("timer delete clears context before kernel deletion succeeds")
    if "clear_timer_ctx(ctx)" not in delete_body:
        errors.append("timer deletion must release its complete context after kernel success")

    if errors:
        print("rtthread_osal_timer failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_timer_ok callback=context api=current lifecycle=atomic")
    return 0


if __name__ == "__main__":
    sys.exit(main())
