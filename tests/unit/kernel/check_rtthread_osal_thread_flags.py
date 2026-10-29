#!/usr/bin/env python3
"""Guard RT-Thread OSAL unsupported thread-flag calls fail closed."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/rtthread/xy_os_rtthread.c"
ERROR_SENTINEL = "return 0x80000000U;"


def function_body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = read_code(BACKEND)
    errors: list[str] = []
    functions = (
        (
            "set",
            "uint32_t xy_os_thread_flags_set",
            "uint32_t xy_os_thread_flags_clear",
        ),
        (
            "clear",
            "uint32_t xy_os_thread_flags_clear",
            "uint32_t xy_os_thread_flags_get",
        ),
        (
            "get",
            "uint32_t xy_os_thread_flags_get",
            "uint32_t xy_os_thread_flags_wait",
        ),
        (
            "wait",
            "uint32_t xy_os_thread_flags_wait",
            "/* Delay Functions */",
        ),
    )

    for name, start, end in functions:
        body = function_body(source, start, end)
        if ERROR_SENTINEL not in body:
            errors.append(
                f"unsupported RT-Thread thread-flags {name} must return the OSAL error sentinel"
            )
        if name in ("clear", "get") and "return 0;" in body:
            errors.append(
                f"unsupported RT-Thread thread-flags {name} must not report false success"
            )

    if errors:
        print("rtthread_osal_thread_flags failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("rtthread_osal_thread_flags_ok unsupported=fail-closed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
