#!/usr/bin/env python3
"""Guard the WCH HAL millisecond timebase against SysTick ownership.

The WCH HAL derived polling timeouts from SysTick->CNT. That is only valid while
the HAL owns SysTick: once FreeRTOS runs, the kernel reloads SysTick at the tick
rate, so CNT becomes a 0..CMP saw-tooth. Dividing it by SystemCoreClock/1000
returns 0 or 1, and subtracting two readings underflows to 0xFFFFFFFF, so every
xy_hal_* polling wait reports a timeout immediately and callers that ignore the
result silently drop data.

MIGRATED files must take their timebase from xy_hal_tick_internal.h, which reads
the machine cycle counter and therefore survives an RTOS owning SysTick. DEFERRED
files are the remaining backlog and must still read SysTick->CNT, so an entry
cannot linger after somebody migrates it. A HAL source that appears in neither set
fails, so new code cannot quietly reintroduce the pattern.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
HAL = ROOT / "components/hal/wch/ch32x/src"
HELPER = HAL / "xy_hal_tick_internal.h"


def code_only(text: str) -> str:
    """Strip comments so prose about SysTick->CNT never trips the guard."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)

MIGRATED = {
    "xy_hal_uart.c",
}

DEFERRED = {
    "xy_hal_adc.c",
    "xy_hal_can.c",
    "xy_hal_dma.c",
    "xy_hal_i2c.c",
    "xy_hal_i2s.c",
    "xy_hal_rng.c",
    "xy_hal_spi.c",
    "xy_hal_sys.c",
}


def main() -> int:
    errors: list[str] = []

    if not HELPER.is_file():
        errors.append("xy_hal_tick_internal.h is missing from the WCH HAL")
    else:
        helper = HELPER.read_text(encoding="utf-8")
        if "rdcycle" not in helper:
            errors.append(
                "xy_hal_tick_internal.h must read the cycle counter: SysTick belongs to the "
                "RTOS once FreeRTOS runs"
            )
        if "SysTick->CNT" in code_only(helper):
            errors.append("the shared HAL timebase must not read SysTick->CNT")

    sources = sorted(path.name for path in HAL.glob("xy_hal_*.c"))
    uses_systick = {
        name
        for name in sources
        if re.search(r"SysTick->CNT", code_only((HAL / name).read_text(encoding="utf-8")))
    }

    overlap = MIGRATED & DEFERRED
    if overlap:
        errors.append(f"files listed as both migrated and deferred: {sorted(overlap)}")

    unclassified = uses_systick - MIGRATED - DEFERRED
    for name in sorted(unclassified):
        errors.append(
            f"{name} reads SysTick->CNT for timing and is neither migrated nor deferred: use "
            "xy_hal_tick_ms_internal() or add it to DEFERRED with a reason"
        )

    for name in sorted(MIGRATED):
        path = HAL / name
        if not path.is_file():
            errors.append(f"migrated source is missing: {name}")
            continue
        source = path.read_text(encoding="utf-8")
        if "xy_hal_tick_internal.h" not in source:
            errors.append(f"{name} must include xy_hal_tick_internal.h")
        if "SysTick->CNT" in code_only(source):
            errors.append(f"{name} still derives time from SysTick->CNT")

    for name in sorted(DEFERRED):
        if name not in sources:
            errors.append(f"deferred source is missing: {name}")
            continue
        if "xy_hal_tick_internal.h" in (HAL / name).read_text(encoding="utf-8"):
            errors.append(
                f"{name} already uses the shared timebase: move it from DEFERRED to MIGRATED"
            )

    if errors:
        print("wch_hal_tick_timebase failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "wch_hal_tick_timebase_ok "
        f"migrated={len(MIGRATED)} deferred={len(DEFERRED)} systick_users={len(uses_systick)}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
