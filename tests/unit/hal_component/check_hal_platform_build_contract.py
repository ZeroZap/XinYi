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
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

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
    "xy_hal_rtc.c",
    "xy_hal_timer.c",
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
KNOWN_UNBUILT: set[str] = set()

# Every dedicated STM32L4 wrapper the branch appends declares which vendor HAL
# module header it needs.  A family HAL module is only compiled when the
# platform's own ``stm32l4xx_hal_conf.h`` enables it; without the matching
# ``HAL_<MODULE>_MODULE_ENABLED`` the wrapper's types and constants are simply
# undeclared and the file cannot build.  That is exactly how the QSPI wrapper
# was mis-diagnosed as "signature drift" while it was the module enable that
# was missing, so the mapping is asserted instead of remembered.
L4_MODULE_REQUIREMENTS = {
    "xy_hal_delay.c": None,  # HAL_Delay from the HAL core, no module gate
    "xy_hal_dma.c": "HAL_DMA_MODULE_ENABLED",
    "xy_hal_flash.c": "HAL_FLASH_MODULE_ENABLED",
    "xy_hal_gpio.c": "HAL_GPIO_MODULE_ENABLED",
    "xy_hal_i2c.c": "HAL_I2C_MODULE_ENABLED",
    "xy_hal_qspi.c": "HAL_QSPI_MODULE_ENABLED",
    "xy_hal_rtc.c": "HAL_RTC_MODULE_ENABLED",
    # uwTick and HAL_GetTick() come from the HAL core, so no module gate.
    "xy_hal_time.c": None,
    "xy_hal_spi.c": "HAL_SPI_MODULE_ENABLED",
    "xy_hal_sys.c": "HAL_RCC_MODULE_ENABLED",
    "xy_hal_timer.c": "HAL_TIM_MODULE_ENABLED",
    "xy_hal_uart.c": "HAL_UART_MODULE_ENABLED",
    "xy_hal_wdg.c": "HAL_IWDG_MODULE_ENABLED",
}

L4_HAL_CONF = L4_DIR / "stm32l4xx_hal_conf.h"


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
        if GUARD in read_code(path)
    }


def main() -> int:
    text = read_raw(HAL_CMAKE)
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
    # ...and the reverse: a wrapper may only be filtered out when it is on the
    # recorded deferral list, so a migrated wrapper cannot be silently dropped
    # from the platform gate again.
    stray_filters = sorted(f4_excluded - f4_deferred)
    assert not stray_filters, (
        "STM32F4 filters out wrappers that are not on XY_HAL_F4_DEFERRED_SOURCES: "
        f"{stray_filters}; a migrated wrapper must be built, not silently excluded"
    )

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

    # 7. No dedicated STM32L4 wrapper may be left orphaned: every stm32l4/*.c
    #    this branch ships has to be appended to HAL_SOURCES.
    l4_sources = {path.name for path in sorted(L4_DIR.glob("*.c"))}
    orphans = sorted(l4_sources - l4_appended)
    assert not orphans, f"STM32L4 branch never builds these wrappers: {orphans}"

    # 8. Every appended STM32L4 wrapper must declare its required HAL module and
    #    that module must actually be enabled by the platform HAL config.
    conf_text = read_code(L4_HAL_CONF)
    enabled_modules = set(re.findall(r"^#define\s+(HAL_\w+_MODULE_ENABLED)\b", conf_text, re.M))
    for basename in sorted(l4_appended):
        assert basename in L4_MODULE_REQUIREMENTS, (
            f"{basename} is built by the STM32L4 branch but has no recorded HAL module "
            "requirement; add it to L4_MODULE_REQUIREMENTS"
        )
        module = L4_MODULE_REQUIREMENTS[basename]
        if module is None:
            continue
        assert module in enabled_modules, (
            f"stm32l4/{basename} needs {module} but stm32l4/stm32l4xx_hal_conf.h does not "
            "enable it: the wrapper cannot compile and the platform gate would be wrong again"
        )
        assert f'#include "stm32l4xx_hal_{module.split("_")[1].lower()}.h"' in conf_text, (
            f"stm32l4xx_hal_conf.h enables {module} but never includes its family header"
        )

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
