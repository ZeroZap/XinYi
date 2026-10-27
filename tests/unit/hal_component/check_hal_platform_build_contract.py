#!/usr/bin/env python3
"""Fail-closed contract for the STM32 HAL platform branches.

The stm32f4 wrappers are compiled only when ``STM32_HAL_ENABLED`` is defined.
Because CMake happily builds an empty translation unit, a platform branch that
forgets the define still "passes" while the ``xy_hal`` archive exports no HAL
symbols at all — that was the state of both the STM32F4 and the STM32L4
(reference-board) branches.  This guard makes that class of failure explicit:

* every STM32 branch that compiles guarded wrappers must define the guard;
* a wrapper may only be left out of a branch through a recorded decision
  (deferred list, or a dedicated per-family replacement with its own EXCLUDE);
* a source that is not yet migrated may never be re-added silently.

It is a source-level guard: it does not compile anything and claims no runtime
evidence.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HAL = ROOT / "components" / "hal"
HAL_CMAKE = HAL / "CMakeLists.txt"
F4_DIR = HAL / "stm32" / "stm32f4"
L4_DIR = HAL / "stm32" / "stm32l4"
GUARD = "STM32_HAL_ENABLED"

# Wrappers excluded from the STM32F4 branch because their signatures still
# drift from the current public HAL contracts.  Shrinking this list is the
# migration work; growing it requires an explicit decision, so an unexpected
# addition fails here instead of silently reducing the built surface.
EXPECTED_F4_DEFERRED = {
    "xy_hal_dma.c",
    "xy_hal_i2c.c",
    "xy_hal_rtc.c",
    "xy_hal_spi.c",
    "xy_hal_timer.c",
    "xy_hal_uart.c",
}

# STM32F4 wrappers the STM32L4 branch deliberately does not build: the F4
# implementation uses F4-only peripheral contracts (ADC->CCR, 3-cycle sample
# time), so it cannot stand in for an L4 wrapper.
EXPECTED_L4_F4_EXCLUDED = {
    "xy_hal_adc.c",     # no L4 ADC wrapper yet (F4 contract is F4-only)
    "xy_hal_dma.c",     # dedicated L4 wrapper
    "xy_hal_flash.c",   # dedicated L4 wrapper
    "xy_hal_gpio.c",    # dedicated L4 wrapper
    "xy_hal_i2c.c",     # dedicated L4 wrapper
    "xy_hal_rtc.c",     # dedicated L4 wrapper
    "xy_hal_spi.c",     # dedicated L4 wrapper
    "xy_hal_timer.c",   # dedicated L4 wrapper
    "xy_hal_uart.c",    # dedicated L4 wrapper
    "xy_hal_wdg.c",     # dedicated L4 wrapper
}

# Sources that a branch deliberately does not build at all.
KNOWN_UNBUILT = {
    # STM32L4 branch: QSPI wrapper still drifts from the public QSPI header.
    "stm32/stm32l4/xy_hal_qspi.c",
}


def branch_body(text: str, name: str) -> str:
    marker = f'elseif(HAL_PLATFORM STREQUAL "{name}")'
    start = text.find(marker)
    assert start >= 0, f"HAL CMakeLists.txt has no {name} branch"
    rest = text[start + len(marker):]
    end = rest.find("elseif(HAL_PLATFORM STREQUAL")
    tail = rest.find("endif()")
    candidates = [pos for pos in (end, tail) if pos >= 0]
    assert candidates, f"{name} branch is not terminated"
    return rest[: min(candidates)]


def halo_defines(body: str) -> set[str]:
    match = re.search(r"set\(HAL_DEFINES(.*?)\)", body, re.S)
    assert match, "HAL_DEFINES is not set in this branch"
    return {token for token in match.group(1).split()}


def deferred_sources(body: str) -> set[str]:
    match = re.search(r"set\(XY_HAL_F4_DEFERRED_SOURCES(.*?)\)", body, re.S)
    if match is None:
        return set()
    return {token for token in match.group(1).split()}


def excluded_basenames(body: str) -> set[str]:
    names = set()
    for match in re.finditer(r'list\(FILTER\w*\s+HAL_SOURCES\s+EXCLUDE\w*\s+REGEX\s+"([^"]+)"', body):
        pattern = match.group(1).replace("\\\\", "\\")
        name = pattern.strip("^$").lstrip("/").replace("\\.", ".")
        names.add(name)
    return names


def guarded_sources(directory: Path) -> set[str]:
    return {
        path.name
        for path in sorted(directory.glob("*.c"))
        if GUARD in path.read_text(encoding="utf-8")
    }


def main() -> int:
    text = HAL_CMAKE.read_text(encoding="utf-8")
    f4 = branch_body(text, "STM32F4")
    l4 = branch_body(text, "STM32L4")

    # 1. Both STM32 branches must define the guard that the wrappers require.
    for name, body in (("STM32F4", f4), ("STM32L4", l4)):
        defines = halo_defines(body)
        assert GUARD in defines, (
            f"{name} branch does not define {GUARD}: guarded wrappers would compile "
            "to empty translation units and the platform gate would export no HAL symbols"
        )

    # 2. The F4 deferral list must match the recorded decision exactly.
    f4_deferred = deferred_sources(f4)
    assert f4_deferred == EXPECTED_F4_DEFERRED, (
        "STM32F4 deferred wrapper set changed: "
        f"expected {sorted(EXPECTED_F4_DEFERRED)}, got {sorted(f4_deferred)}"
    )
    for name in sorted(f4_deferred):
        assert (F4_DIR / name).is_file(), f"deferred wrapper {name} does not exist"

    # 3. Excluded wrappers must actually be excluded, not just listed.
    f4_excluded = excluded_basenames(f4)
    for name in sorted(f4_deferred):
        assert name in f4_excluded, f"STM32F4 lists {name} as deferred but does not filter it out"

    # 4. A guard-only wrapper must never disappear without a recorded decision:
    #    compiled by the branch, deferred, or replaced by a dedicated per-family
    #    implementation that the branch appends.
    l4_appended = set(re.findall(r'"stm32/stm32l4/(xy_hal_\w+\.c)"', l4))
    l4_excluded = excluded_basenames(l4)
    assert l4_excluded & set(guarded_sources(F4_DIR)) == EXPECTED_L4_F4_EXCLUDED, (
        "STM32L4 branch no longer excludes the expected stm32f4 wrappers: "
        f"expected {sorted(EXPECTED_L4_F4_EXCLUDED)}, "
        f"got {sorted(l4_excluded & set(guarded_sources(F4_DIR)))}"
    )
    for basename in sorted(guarded_sources(F4_DIR)):
        if basename in f4_deferred:
            continue
        if basename in l4_appended:
            assert basename in l4_excluded, (
                f"STM32L4 appends {basename} but still globs the stm32f4 copy; "
                "duplicate definitions would only surface after the guard is defined"
            )
        assert (F4_DIR / basename).is_file(), basename

    # 5. Every appended per-family source must exist, and a branch may not build
    #    two files with the same basename.
    for rel in sorted(l4_appended):
        assert (L4_DIR / rel).is_file(), f"STM32L4 appends missing source {rel}"
    duplicated = sorted(set(l4_appended) & (guarded_sources(F4_DIR) - l4_excluded))
    assert not duplicated, f"STM32L4 branch builds duplicate basenames: {duplicated}"

    # 6. Known-unbuilt sources must stay known: they may not silently vanish.
    known_unbuilt = (
        deferred_sources(f4)
        | EXPECTED_L4_F4_EXCLUDED
        | {Path(rel).name for rel in KNOWN_UNBUILT}
    )

    u5 = branch_body(text, "STM32U5")
    u5_appended = set(re.findall(r'"stm32/stm32u5/(xy_hal_\w+\.c)"', u5))
    assert not u5_appended, "STM32U5 branch now appends explicit sources; extend this guard"

    built = (guarded_sources(F4_DIR) - f4_deferred - known_unbuilt) | l4_appended
    assert built, "no HAL wrapper would be compiled on any STM32 branch"

    print("hal_platform_build_contract=passed")
    print(f"  guarded_f4_wrappers={len(guarded_sources(F4_DIR))}")
    print(f"  f4_deferred={','.join(sorted(f4_deferred))}")
    print(f"  l4_dedicated={','.join(sorted(l4_appended))}")
    print(f"  l4_f4_excluded={','.join(sorted(EXPECTED_L4_F4_EXCLUDED))}")
    print(f"  known_unbuilt={','.join(sorted(KNOWN_UNBUILT))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
