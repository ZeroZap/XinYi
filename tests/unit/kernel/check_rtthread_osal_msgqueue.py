#!/usr/bin/env python3
"""Guard RT-Thread OSAL message-queue fixed-size binary semantics."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    errors: list[str] = []

    put_body = function_body(
        source,
        "xy_os_status_t xy_os_msgqueue_put",
        "xy_os_status_t xy_os_msgqueue_get",
    )
    for required in (
        "XY_UNUSED(msg_prio);",
        "rt_mq_send(mq, msg_ptr, mq->msg_size)",
        "rt_mq_send_wait(mq, msg_ptr, mq->msg_size, timeout)",
    ):
        if required not in put_body:
            errors.append(
                "RT-Thread message-queue put must send the configured fixed-size binary payload; "
                f"missing: {required}"
            )
    if "strlen(" in put_body:
        errors.append(
            "RT-Thread message-queue put must not derive binary payload length with strlen()"
        )

    get_body = function_body(
        source,
        "xy_os_status_t xy_os_msgqueue_get",
        "uint32_t xy_os_msgqueue_get_capacity",
    )
    for required in (
        "rt_ssize_t received;",
        "received = rt_mq_recv(mq, msg_ptr, mq->msg_size,",
        "(timeout == XY_OS_WAIT_FOREVER) ? RT_WAITING_FOREVER : timeout);",
        "if (received != (rt_ssize_t)mq->msg_size)",
        "*msg_prio = 0U;",
    ):
        if required not in get_body:
            errors.append(
                "RT-Thread message-queue get must use the tracked four-argument API and require "
                f"one complete fixed-size message; missing: {required}"
            )
    if "rt_mq_recv(mq, msg_ptr, mq->msg_size, timeout, &size)" in get_body:
        errors.append("RT-Thread message-queue get retains the obsolete five-argument API")

    new_body = function_body(
        source,
        "xy_os_msgqueue_id_t xy_os_msgqueue_new",
        "const char *xy_os_msgqueue_get_name",
    )
    for required in (
        "msg_count == 0U || msg_size == 0U",
        "ctx = alloc_msgqueue_ctx();",
        "rt_mq_create(name, msg_size, msg_count, flag)",
        "clear_msgqueue_ctx(ctx);",
        "ctx->mq = mq;",
    ):
        if required not in new_body:
            errors.append(
                "RT-Thread message-queue creation must reserve bounded ownership before kernel "
                f"allocation and release it on failure; missing: {required}"
            )

    for start, end in (
        ("const char *xy_os_msgqueue_get_name", "xy_os_status_t xy_os_msgqueue_put"),
        ("xy_os_status_t xy_os_msgqueue_put", "xy_os_status_t xy_os_msgqueue_get"),
        ("xy_os_status_t xy_os_msgqueue_get", "uint32_t xy_os_msgqueue_get_capacity"),
        ("uint32_t xy_os_msgqueue_get_capacity", "uint32_t xy_os_msgqueue_get_msg_size"),
        ("uint32_t xy_os_msgqueue_get_msg_size", "uint32_t xy_os_msgqueue_get_count"),
        ("uint32_t xy_os_msgqueue_get_count", "uint32_t xy_os_msgqueue_get_space"),
        ("uint32_t xy_os_msgqueue_get_space", "xy_os_status_t xy_os_msgqueue_reset"),
        ("xy_os_status_t xy_os_msgqueue_reset", "xy_os_status_t xy_os_msgqueue_delete"),
    ):
        body = function_body(source, start, end)
        if "find_msgqueue_ctx(mq)" not in body:
            errors.append(f"{start} must reject untracked message-queue handles")

    delete_body = function_body(
        source,
        "xy_os_status_t xy_os_msgqueue_delete",
        "#endif /* STM32U5 || STM32U5xx */",
    )
    for required in (
        "ctx = find_msgqueue_ctx(mq);",
        "result = rt_mq_delete(mq);",
        "if (result != RT_EOK) return XY_OS_ERROR;",
        "clear_msgqueue_ctx(ctx);",
    ):
        if required not in delete_body:
            errors.append(
                "RT-Thread message-queue teardown must preserve ownership until kernel delete "
                f"succeeds; missing: {required}"
            )

    if errors:
        print("rtthread_osal_msgqueue failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_msgqueue_ok payload=fixed-binary recv_api=current lifecycle=owned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
