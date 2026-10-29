#!/usr/bin/env python3
"""Guard the STM32 HAL millisecond timebase so a kernel can replace it.

Bare-metal, uwTick advances from SysTick_Handler through
xy_hal_sys_tick_irq_handler(). Once FreeRTOS owns SysTick, the kernel's handler
runs instead and nothing calls HAL_IncTick(), so uwTick freezes and every ST HAL
wait that compares against HAL_GetTick() stops expiring. The fix is a registered
tick source plus a strong HAL_GetTick() override; this guard keeps both in place:

  * each STM32 platform ships xy_hal_time.c implementing the hook;
  * HAL_GetTick() is defined there and is *not* weak, or ST's weak definition
    would win the link and the override would silently stop applying;
  * the fallback reads uwTick rather than calling HAL_GetTick(), which would
    recurse into the override forever;
  * xy_hal_sys_get_tick_count() keeps reporting the same clock.
"""

from pathlib import Path
import re
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import code_only  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HAL = ROOT / "components" / "hal" / "stm32"
# stm32u5 does not publish xy_hal_sys_get_tick_count() yet, so the check only
# requires it where it exists; when U5 grows one it must use the same clock.
PLATFORMS = {
    "stm32l4": ("stm32l4xx_hal.h", True),
    "stm32u5": ("stm32u5xx_hal.h", False),
}


def check_platform(name: str, header: str, require_tick_count: bool) -> list[str]:
    errors: list[str] = []
    time_source = HAL / name / "xy_hal_time.c"
    if not time_source.is_file():
        return [f"{name}: xy_hal_time.c is missing"]

    text = time_source.read_text(encoding="utf-8")
    code = code_only(text)
    if header not in text:
        errors.append(f"{name}: xy_hal_time.c must include {header}")
    for symbol in ("xy_hal_tick_set_source", "xy_hal_time_ms"):
        if symbol not in code:
            errors.append(f"{name}: xy_hal_time.c must implement {symbol}")
    if not re.search(r"uint32_t\s+HAL_GetTick\s*\(\s*void\s*\)", code):
        errors.append(f"{name}: xy_hal_time.c must define HAL_GetTick() void parameter list")
    if re.search(r"__weak\s+uint32_t\s+HAL_GetTick", code):
        errors.append(
            f"{name}: HAL_GetTick() must stay strong, or the weak ST definition wins the link"
        )
    if not re.search(r"return\s+uwTick\s*;", code):
        errors.append(f"{name}: the fallback must read uwTick directly")
    if re.search(r"return\s+HAL_GetTick\s*\(", code):
        errors.append(
            f"{name}: the fallback must not call HAL_GetTick(), which would recurse into the "
            "override"
        )

    sys_source = HAL / name / "xy_hal_sys.c"
    if not sys_source.is_file():
        errors.append(f"{name}: xy_hal_sys.c is missing")
    else:
        sys_text = sys_source.read_text(encoding="utf-8")
        if "xy_hal_sys_get_tick_count" not in sys_text:
            if require_tick_count:
                errors.append(f"{name}: xy_hal_sys.c must keep xy_hal_sys_get_tick_count()")
        elif "HAL_GetTick" not in sys_text:
            errors.append(
                f"{name}: xy_hal_sys_get_tick_count() must report HAL_GetTick(), which the "
                "registered source now backs"
            )
    return errors


def main() -> int:
    errors: list[str] = []
    for name, (header, require_tick_count) in PLATFORMS.items():
        errors.extend(check_platform(name, header, require_tick_count))

    if errors:
        print("stm32_hal_timebase_hook failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    print(f"stm32_hal_timebase_hook_ok platforms={len(PLATFORMS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
