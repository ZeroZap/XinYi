#!/usr/bin/env python3
"""The ADC header declares forty functions; the three backends implement 40, 28 and 12 of them.

`components/hal/inc/xy_hal_adc.h` is one contract for five platforms, and the platforms do not
implement the same subset. Measured on real archives with `nm -g --defined-only .../libxy_hal.a`:

    ADC   stm32f4 28/40   stm32u5 12/40   wch 40/40   stm32l4 0 (branch builds no ADC)   pc 40/40
    DAC   stm32f4 14/14   stm32u5 14/14   wch 14/14   pc 14/14

That is not a build configuration anyone intended; it is the residue of a header written once and
three ports written at different times. It mattered the day a documentation page taught
`xy_hal_adc_value_to_mv()` - implemented on F4 and CH32V307, absent on U5 - because a call that
links on one platform and not on the next is exactly what a header is supposed to prevent.

The PC backend is included because it is the one column that can be exercised on a host: it carries
the whole surface as a simulation, so the two families have a place where every entry is at least
callable and observable. That says nothing about the parts - a simulation of a conversion is not a
conversion - which is why the per-backend gaps below are still the measurement that matters.

So the five-way difference is measured here rather than remembered: declarations are read from the
header, definitions from each backend's sources, and the difference per (backend, module) must equal
a pin that carries a reason. Two directions are reported, and they are different failures:

* a function that the pin says is missing and the source now defines -> the pin is stale, shrink it
  (the documented coverage changed, so the page that quotes it changed too);
* a function that used to be defined and is not any more -> a regression, which no comment can carry.

A third rule covers the whole tree: a declaration that no backend defines at all is not a porting
gap, it is a function that does not exist. `components/device/inc/xy_adc.h` is that shape - twenty
`xy_adc_dev_*` declarations and no implementation anywhere - and the ADDC guide now says so.

Source-level only: this claims nothing about what the functions do, and nothing about runtime.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HAL = ROOT / "components" / "hal"

#: module name -> the header that declares its contract.
MODULES = {"adc": "xy_hal_adc.h", "dac": "xy_hal_dac.h"}

#: backend -> every source directory its `xy_hal` archive is built from. F4 also compiles the shared
#: `stm32/src` helpers, so a definition there counts for F4 and not for the others.
BACKENDS: dict[str, tuple[str, ...]] = {
    "stm32f4": ("components/hal/stm32/stm32f4", "components/hal/stm32/src"),
    "stm32u5": ("components/hal/stm32/stm32u5",),
    "wch": ("components/hal/wch/ch32x/src",),
    "pc": ("components/hal/PC",),
}

#: Control keywords a column-0 line can begin with. A definition's return type is never one of them,
#: while a call that happens to sit at column 0 starts with one - `return xy_hal_adc_read(...)` is the
#: shape that made a naive "first word is a type" rule over-count.
NOT_A_TYPE = {
    "return", "if", "else", "while", "for", "switch", "case", "do", "goto", "break", "continue",
    "sizeof", "typedef", "struct", "union", "enum",
}

#: (backend, module) -> the functions that backend does not define, with the reason the gap stands.
#: Every entry is compared exactly: an empty set is a claim that the backend implements the whole
#: header, which is how the three DAC backends are recorded.
PINNED_MISSING: dict[tuple[str, str], tuple[frozenset[str], str]] = {
    ("stm32f4", "adc"): (
        frozenset({
            "xy_hal_adc_disable_temp_sensor", "xy_hal_adc_enable_temp_sensor",
            "xy_hal_adc_get_continuous", "xy_hal_adc_get_max_value",
            "xy_hal_adc_get_resolution", "xy_hal_adc_get_sampling_time",
            "xy_hal_adc_get_trigger", "xy_hal_adc_get_vrefint_value",
            "xy_hal_adc_set_continuous", "xy_hal_adc_set_sampling_time",
            "xy_hal_adc_set_trigger", "xy_hal_adc_value_to_mv",
        }),
        "the F4 wrapper is the polling path plus the window watchdog: read/read_nb/read_multi/"
        "read_dma and the channel/start/stop/callback calls. The getters and setters for resolution, "
        "full scale, sampling time, trigger and continuous mode, the internal-sensor enables and "
        "value_to_mv were never ported to this backend",
    ),
    ("stm32f4", "dac"): (frozenset(), "the F4 DAC wrapper implements the whole 14-function header"),
    ("stm32u5", "adc"): (
        frozenset({
            "xy_hal_adc_calibrate", "xy_hal_adc_control", "xy_hal_adc_disable_temp_sensor",
            "xy_hal_adc_disable_vbat", "xy_hal_adc_disable_vrefint",
            "xy_hal_adc_disable_window_watchdog", "xy_hal_adc_enable_temp_sensor",
            "xy_hal_adc_enable_vbat", "xy_hal_adc_enable_vrefint",
            "xy_hal_adc_enable_window_watchdog", "xy_hal_adc_event_handler",
            "xy_hal_adc_get_continuous", "xy_hal_adc_get_oversampling",
            "xy_hal_adc_get_prescaler", "xy_hal_adc_get_sampling_time", "xy_hal_adc_get_state",
            "xy_hal_adc_get_temp_value", "xy_hal_adc_get_trigger", "xy_hal_adc_get_vbat_value",
            "xy_hal_adc_get_vrefint_value", "xy_hal_adc_get_window_threshold",
            "xy_hal_adc_read_multi", "xy_hal_adc_set_continuous", "xy_hal_adc_set_oversampling",
            "xy_hal_adc_set_prescaler", "xy_hal_adc_set_sampling_time", "xy_hal_adc_set_trigger",
            "xy_hal_adc_set_window_threshold",
        }),
        "the U5 wrapper is a twelve-function subset: init/deinit, config_channels, start/stop, "
        "read/read_nb/read_dma, register_callback, resolution, max value and value_to_mv. Everything "
        "else in the header - internal sensors, VBAT, oversampling, prescaler, window watchdog, "
        "multi-channel read, control/calibrate/state - was never implemented on this backend",
    ),
    ("stm32u5", "dac"): (frozenset(), "the U5 DAC wrapper implements the whole 14-function header"),
    ("wch", "adc"): (frozenset(), "the CH32V307 ADC wrapper implements the whole 40-function header, "
                                 "which is why it is the only backend a full-surface sample can run on"),
    ("wch", "dac"): (frozenset(), "the CH32V307 DAC wrapper implements the whole 14-function header"),
    ("pc", "adc"): (
        frozenset(),
        "the host simulation implements all 40 entries, which is what makes an ADC consumer "
        "linkable and observable without a board - a conversion is modelled from an injected "
        "voltage, so this column says nothing about any part",
    ),
    ("pc", "dac"): (
        frozenset(),
        "the host simulation implements all 14 entries; the output value, the waveform advance and "
        "the events are observable through the public API",
    ),
}

DECLARATION = re.compile(r"\b(xy_hal_(?:adc|dac)_[a-z0-9_]*)\s*\(")
DEFINITION = re.compile(r"^[A-Za-z][\w \t\*]*?\b(xy_hal_(?:adc|dac)_[a-z0-9_]*)\s*\(", re.M)
#: The PC backend's own injection helpers (`xy_hal_adc_pc_set_input`, and the older
#: `xy_hal_adc_init_pc`) share the family prefix but are not public entries - they are declared in
#: `xy_hal_pc.h`, not in the module header. Counting them made the PC column read 47 of 40.
INTERNAL_HELPER = re.compile(r"_pc(?:_|$)")


def defined_functions(root: Path, directories: tuple[str, ...], module: str) -> set[str]:
    """Function names a backend defines for `module`, read from its sources."""
    names: set[str] = set()
    for directory in directories:
        for path in sorted((root / directory).glob("*.c")):
            source = read_code(path)
            for match in DEFINITION.finditer(source):
                prefix = source[source.rfind("\n", 0, match.start(1)) + 1:match.start(1)]
                first = prefix.strip().split(" ")[0].strip("*") if prefix.strip() else ""
                if first in NOT_A_TYPE:
                    continue
                if INTERNAL_HELPER.search(match.group(1)):
                    continue
                if match.group(1).startswith(f"xy_hal_{module}_"):
                    names.add(match.group(1))
    return names


def findings(root: Path | None = None,
             pins: dict[tuple[str, str], tuple[frozenset[str], str]] | None = None) -> list[str]:
    tree = ROOT if root is None else root
    pinned = PINNED_MISSING if pins is None else pins
    problems: list[str] = []

    declared: dict[str, set[str]] = {}
    for module, header in MODULES.items():
        path = tree / "components" / "hal" / "inc" / header
        if not path.is_file():
            problems.append(f"{header}: the module header is gone, so nothing can be measured")
            continue
        names = set(DECLARATION.findall(read_code(path)))
        if not names:
            problems.append(f"{header}: not one declaration was extracted - the reader is broken, "
                            f"and a header that declares nothing reads the same as a complete one")
        declared[module] = names

    implemented: dict[tuple[str, str], set[str]] = {}
    for backend, directories in BACKENDS.items():
        for module in MODULES:
            if not any((tree / directory).is_dir() for directory in directories):
                problems.append(f"{backend}: none of {', '.join(directories)} exists - the backend "
                                f"moved, and a missing directory would otherwise read as 'implements "
                                f"nothing'")
                continue
            implemented[(backend, module)] = defined_functions(tree, directories, module)

    # A declaration nobody implements is not a porting gap: it is a function that does not exist.
    for module, names in declared.items():
        everywhere = set().union(*(defined for (backend, other), defined in implemented.items()
                                   if other == module)) if implemented else set()
        orphans = sorted(names - everywhere)
        if orphans:
            problems.append(
                f"xy_hal_{module}.h declares {len(orphans)} function(s) that no backend defines: "
                f"{', '.join(orphans)} - a declaration is a promise a call will link, and this tree "
                f"delivers it on none of {len(BACKENDS)} backends")

    for key, (expected, reason) in sorted(pinned.items()):
        backend, module = key
        if key not in implemented:
            problems.append(f"{backend}/{module}: pinned here, but this backend does not exist or "
                            f"defines nothing - the pin has outlived the code it described")
            continue
        if not reason.strip():
            problems.append(f"{backend}/{module}: pinned without a reason - an unexplained gap is a "
                            f"silence")
        actual = declared.get(module, set()) - implemented[key]
        newly_missing = sorted(actual - expected)
        now_defined = sorted(expected - actual)
        if newly_missing:
            problems.append(
                f"{backend}/{module}: no longer defines {', '.join(newly_missing)} - these were "
                f"defined before and the pin does not say they may disappear")
        if now_defined:
            problems.append(
                f"{backend}/{module}: the pin still lists {', '.join(now_defined)} as missing, but "
                f"they are now defined - shrink the pin and the coverage claim that quotes it")
    return problems


def report(root: Path | None = None) -> str:
    tree = ROOT if root is None else root
    counts: list[str] = []
    for module, header in MODULES.items():
        path = tree / "components" / "hal" / "inc" / header
        total = len(set(DECLARATION.findall(read_code(path))))
        per = []
        for backend, directories in BACKENDS.items():
            defined = defined_functions(tree, directories, module)
            per.append(f"{backend} {len(defined)}/{total}")
        counts.append(f"{module} {total} declared ({', '.join(per)})")
    gaps = sum(len(expected) for expected, _ in PINNED_MISSING.values())
    return (f"hal_adc_dac_surface ok ({'; '.join(counts)}; {gaps} (backend, function) coverage gaps "
            f"pinned with a reason)")


def main() -> int:
    problems = findings()
    if problems:
        for problem in problems:
            print(problem)
        return 1
    print(report())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
