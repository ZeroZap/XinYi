#!/usr/bin/env python3
"""Guard RT-Thread OSAL context reservation/retirement atomicity.

The FreeRTOS backend reserves each bounded owner slot and retires it inside a
kernel critical section so concurrent creators/deleters cannot observe a
half-reserved or half-retired slot.  The RT-Thread backend uses the same
bounded owner-table design, so it must hold the same atomicity contract with
the RT-Thread scheduler lock (rt_enter_critical / rt_exit_critical).
"""

from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


# (kind, alloc_signature, alloc_next, clear_signature, clear_next)
KINDS = [
    (
        "event_flags",
        "static event_flags_ctx_t *alloc_event_flags_ctx(void)",
        "static event_flags_ctx_t *find_event_flags_ctx",
        "static void clear_event_flags_ctx(event_flags_ctx_t *ctx)",
        "static mempool_ctx_t *alloc_mempool_ctx(void)",
    ),
    (
        "mempool",
        "static mempool_ctx_t *alloc_mempool_ctx(void)",
        "static mempool_ctx_t *find_mempool_ctx",
        "static void clear_mempool_ctx(mempool_ctx_t *ctx)",
        "static msgqueue_ctx_t *alloc_msgqueue_ctx(void)",
    ),
    (
        "msgqueue",
        "static msgqueue_ctx_t *alloc_msgqueue_ctx(void)",
        "static msgqueue_ctx_t *find_msgqueue_ctx",
        "static void clear_msgqueue_ctx(msgqueue_ctx_t *ctx)",
        "static timer_ctx_t *alloc_timer_ctx(void)",
    ),
    (
        "timer",
        "static timer_ctx_t *alloc_timer_ctx(void)",
        "static timer_ctx_t *find_timer_ctx",
        "static void clear_timer_ctx(timer_ctx_t *ctx)",
        "static int rtthread_priority_valid",
    ),
    (
        "thread",
        "static thread_ctx_t *alloc_thread_ctx(void)",
        "static mutex_ctx_t *find_mutex_ctx",
        "static void clear_thread_ctx(thread_ctx_t *ctx)",
        "static rt_uint8_t xy_to_rt_priority",
    ),
    (
        "mutex",
        "static mutex_ctx_t *alloc_mutex_ctx(void)",
        "static void clear_mutex_ctx",
        "static void clear_mutex_ctx(mutex_ctx_t *ctx)",
        "static sem_ctx_t *find_sem_ctx",
    ),
    (
        "sem",
        "static sem_ctx_t *alloc_sem_ctx(void)",
        "static void clear_sem_ctx",
        "static void clear_sem_ctx(sem_ctx_t *ctx)",
        "static void clear_thread_ctx",
    ),
]


def main() -> int:
    source = read_code(BACKEND)
    errors: list[str] = []

    for kind, alloc_sig, alloc_next, clear_sig, clear_next in KINDS:
        alloc_body = function_body(source, alloc_sig, alloc_next)
        enter = alloc_body.find("rt_enter_critical()")
        exit_ = alloc_body.find("rt_exit_critical()")
        reserve = alloc_body.find("initialized = 1")
        if enter < 0 or exit_ < 0 or reserve < 0:
            errors.append(
                f"{kind} context reservation must use rt_enter_critical/rt_exit_critical "
                "and set initialized=1 inside the critical section"
            )
        elif not (enter < reserve < exit_):
            errors.append(
                f"{kind} context reservation must occur inside one critical section "
                "(enter < reserve < exit)"
            )

        clear_body = function_body(source, clear_sig, clear_next)
        if "rt_enter_critical()" not in clear_body or "rt_exit_critical()" not in clear_body:
            errors.append(
                f"{kind} context retirement must clear the owner slot inside one "
                "critical section"
            )

    if "ctx->initialized = 1;" in source:
        errors.append(
            "context reservation must live inside the atomic alloc_*_ctx helper, not "
            "as a non-atomic assignment in the public *_new entry"
        )

    if errors:
        print("rtthread_osal_context_atomicity failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_context_atomicity_ok reservation=atomic retirement=atomic")
    return 0


if __name__ == "__main__":
    sys.exit(main())
