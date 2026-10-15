#!/usr/bin/env python3
"""Guard RT-Thread OSAL memory-pool ownership and query semantics."""

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
        source,
        "xy_os_mempool_id_t xy_os_mempool_new",
        "const char *xy_os_mempool_get_name",
    )
    for required in (
        "if (block_count == 0U || block_size == 0U) return NULL;",
        "rt_mp_create(name, block_count, block_size)",
    ):
        if required not in new_body:
            errors.append(f"memory-pool create must validate dimensions and use RT-Thread ownership: missing {required}")
    for forbidden in ("rt_malloc(", "rt_free(", "total_size"):
        if forbidden in new_body:
            errors.append(f"memory-pool create must not allocate an unused caller-side buffer: found {forbidden}")

    queries = {
        "xy_os_mempool_get_capacity": "return mp->block_total_count;",
        "xy_os_mempool_get_block_size": "return mp->block_size;",
        "xy_os_mempool_get_count": "return mp->block_total_count - mp->block_free_count;",
        "xy_os_mempool_get_space": "return mp->block_free_count;",
    }
    boundaries = list(queries) + ["xy_os_mempool_delete"]
    for index, (name, required) in enumerate(queries.items()):
        body = function_body(source, f"uint32_t {name}", boundaries[index + 1])
        if required not in body:
            errors.append(f"{name} must report tracked RT-Thread pool metadata: missing {required}")
        if "return 0; // Not available" in body:
            errors.append(f"{name} still reports a false zero placeholder")

    if errors:
        print("rtthread_osal_mempool failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_mempool_ok owner=rtthread queries=tracked")
    return 0


if __name__ == "__main__":
    sys.exit(main())
