#!/usr/bin/env python3
"""Guard the WCH HAL millisecond timebase into a single, documented place.

Every HAL module used to repeat its own SysTick->CNT-to-milliseconds expression.
That duplication hid a real constraint: the expression is only correct while the
HAL owns SysTick, and a FreeRTOS build reloads SysTick at the tick rate, turning
CNT into a saw-tooth whose differences underflow and fire every timeout at once.

Consolidating it means the platform fix - whatever replaces SysTick on RTOS
builds - only has to be made once. This guard keeps that property:

  * migrated sources must call xy_hal_tick_ms_internal() and must not read
    SysTick->CNT themselves;
  * the shared helper must keep handling both SysTick clock sources
    (SystemCoreClock/1000 and /8000) and must keep the OPEN ISSUE note that
    records the RTOS ownership constraint;
  * the cycle CSRs are forbidden: rdcycle and mcycle both trap as illegal
    instructions on the CH32V307 QingKe core, verified on hardware;
  * xy_hal_sys.c stays deferred because it programs SysTick on purpose;
  * a HAL source that reads SysTick->CNT without being listed fails.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
HAL = ROOT / "components/hal/wch/ch32x/src"
HELPER = HAL / "xy_hal_tick_internal.h"

MIGRATED = {
    "xy_hal_adc.c",
    "xy_hal_can.c",
    "xy_hal_dma.c",
    "xy_hal_i2c.c",
    "xy_hal_i2s.c",
    "xy_hal_rng.c",
    "xy_hal_spi.c",
    "xy_hal_uart.c",
}

# xy_hal_sys.c installs SysTick as the HAL's own free-running counter and derives
# xy_hal_sys_get_tick_count() from it, so it touches SysTick->CNT by design rather
# than to time a polling loop.
DEFERRED = {
    "xy_hal_sys.c",
}


def code_only(text: str) -> str:
    """Strip comments so prose about SysTick->CNT never trips the guard."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def main() -> int:
    errors: list[str] = []

    if not HELPER.is_file():
        errors.append("xy_hal_tick_internal.h is missing from the WCH HAL")
    else:
        helper = HELPER.read_text(encoding="utf-8")
        helper_code = code_only(helper)
        if "SysTick->CNT" not in helper_code:
            errors.append(
                "the shared timebase must keep deriving milliseconds from SysTick->CNT until a "
                "source the RTOS does not own replaces it"
            )
        if "SystemCoreClock / 8000U" not in helper:
            errors.append("the shared timebase must keep handling the /8 SysTick clock source")
        if "OPEN ISSUE" not in helper:
            errors.append(
                "the shared timebase must keep the OPEN ISSUE note recording that SysTick "
                "belongs to the RTOS once FreeRTOS runs"
            )
        if "rdcycle" in helper_code or "mcycle" in helper_code:
            errors.append(
                "the shared timebase must not read the cycle CSRs: both rdcycle and mcycle "
                "trap as illegal instructions (mcause 2) on the CH32V307 QingKe core"
            )

    sources = sorted(path.name for path in HAL.glob("xy_hal_*.c"))
    overlap = MIGRATED & DEFERRED
    if overlap:
        errors.append(f"files listed as both migrated and deferred: {sorted(overlap)}")

    systick_users = {
        name
        for name in sources
        if "SysTick->CNT" in code_only((HAL / name).read_text(encoding="utf-8"))
    }
    unclassified = systick_users - MIGRATED - DEFERRED
    for name in sorted(unclassified):
        errors.append(
            f"{name} reads SysTick->CNT itself and is neither migrated nor deferred: call "
            "xy_hal_tick_ms_internal() or list it in DEFERRED with its reason"
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
            errors.append(f"{name} still reads SysTick->CNT instead of the shared timebase")

    for name in sorted(DEFERRED):
        if name not in sources:
            errors.append(f"deferred source is missing: {name}")

    if errors:
        print("wch_hal_tick_timebase failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "wch_hal_tick_timebase_ok "
        f"migrated={len(MIGRATED)} deferred={len(DEFERRED)} systick_users={len(systick_users)}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
