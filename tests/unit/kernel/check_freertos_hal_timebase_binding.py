#!/usr/bin/env python3
"""Guard when the FreeRTOS backend hands its tick to the HAL millisecond clock.

The registered source returns xTaskGetTickCount() * portTICK_PERIOD_MS. Before
the scheduler starts that product is 0 no matter how long the board has been
booting, so installing the source from xy_os_kernel_init() replaces the live
uwTick-based clock with a constant zero for the whole pre-scheduler window.
Anything initialised between kernel init and kernel start that waits on
HAL_GetTick() then waits against a value that never moves: the wait cannot
expire, so a HAL that should fail closed instead hangs.

Installing the source immediately before vTaskStartScheduler() keeps the
pre-scheduler clock on the platform default and only redirects it once the
kernel tick it reports actually exists. This guard pins that ordering.
"""

from pathlib import Path
import re
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import code_only, read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/freertos/xy_os_freertos.c"
OSAL_CMAKE = ROOT / "components/kernel/osal/CMakeLists.txt"

FUNCTION_PATTERN = r"xy_os_status_t %s\(void\)\s*\{"


def function_body(source: str, name: str, errors: list[str]) -> str:
    match = re.search(FUNCTION_PATTERN % re.escape(name), source)
    if match is None:
        errors.append(f"{name}() is missing from the FreeRTOS backend")
        return ""
    end = source.find("\n}\n", match.end())
    if end < 0:
        errors.append(f"{name}() has no closing brace")
        return ""
    return source[match.end() : end]


def main() -> int:
    if not BACKEND.is_file():
        print(f"freertos_hal_timebase_binding failed: {BACKEND} is missing", file=sys.stderr)
        return 1

    source = read_code(BACKEND)
    code = code_only(source)
    errors: list[str] = []

    kernel_init = function_body(code, "xy_os_kernel_init", errors)
    kernel_start = function_body(code, "xy_os_kernel_start", errors)

    if "freertos_bind_hal_timebase(" in kernel_init:
        errors.append(
            "xy_os_kernel_init() must not install the kernel tick as the HAL timebase: "
            "xTaskGetTickCount() is 0 until the scheduler runs, so every HAL_GetTick() "
            "wait during pre-scheduler init would compare against a constant"
        )
    if "freertos_bind_hal_timebase(" not in kernel_start:
        errors.append(
            "xy_os_kernel_start() must install the kernel tick as the HAL timebase before "
            "the scheduler runs"
        )
    elif "vTaskStartScheduler(" in kernel_start:
        bind_offset = kernel_start.index("freertos_bind_hal_timebase(")
        start_offset = kernel_start.index("vTaskStartScheduler(")
        if bind_offset > start_offset:
            errors.append(
                "the HAL timebase must be bound before vTaskStartScheduler(); vTaskStartScheduler() "
                "does not return once the scheduler runs"
            )

    bind_calls = len(re.findall(r"^\s*freertos_bind_hal_timebase\(\s*\)\s*;", code, re.MULTILINE))
    if bind_calls != 1:
        errors.append(
            f"the HAL timebase must be bound from exactly one place, found {bind_calls}"
        )

    if "xy_hal_tick_set_source(freertos_hal_tick_ms)" not in code:
        errors.append(
            "the backend must publish freertos_hal_tick_ms through xy_hal_tick_set_source()"
        )
    if not re.search(
        r"return\s*\(uint32_t\)\s*\(\s*xTaskGetTickCount\(\)\s*\*\s*portTICK_PERIOD_MS\s*\)",
        code,
    ):
        errors.append(
            "freertos_hal_tick_ms must report xTaskGetTickCount() * portTICK_PERIOD_MS"
        )
    if "xy_hal_tick_set_source != NULL" not in code:
        errors.append(
            "the optional HAL hook must be null-checked so a platform without it links"
        )

    # A guard that can be switched off by the build is not a guard. The hook header
    # used to be behind __has_include(), so on a target whose include path lacked
    # components/hal/inc the whole binding compiled to nothing without a warning.
    if "__has_include" in code:
        errors.append(
            "the HAL tick hook must not be gated on __has_include(): a missing include "
            "root silently removes the binding instead of failing the build"
        )
    if not re.search(r'#include\s+"xy_hal_time\.h"', code):
        errors.append('the backend must include "xy_hal_time.h" so the hook always compiles')

    cmake = read_raw(OSAL_CMAKE) if OSAL_CMAKE.is_file() else ""
    if "components/hal/inc" not in cmake:
        errors.append(
            "xy_osal must put components/hal/inc on its own include path, or the backend "
            "cannot reach xy_hal_time.h"
        )

    if errors:
        print("freertos_hal_timebase_binding failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    print("freertos_hal_timebase_binding_ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
