#!/usr/bin/env python3
"""Two headers behind one include guard: whichever is included second disappears.

An include guard is a promise about one file. When two files make the same promise, the second
one is not re-read - and there is no diagnostic, because "already included" is exactly what a
correct guard is supposed to report. The failure is silent in the dangerous direction:

* `projects/USB2P/inc/` had four headers sharing `_USB2P_COMMON_H_` (`usb2can.h`, `usb2dac.h`,
  `usb2log.h`, `usb2p_def.h`). Including `usb2p_def.h` first made the declarations of the other
  three vanish while the translation unit still compiled; the reverse order failed with
  `unknown type name 'uint8_t'`, because `usb2log.h` relies on the header it just suppressed.
* `components/fota/` published `xy_fota.h` twice under `XY_FOTA_H`. `inc/` carried three
  functions (`xy_fota_bank_swap`/`_mark_valid`/`_is_valid`) that `src/xy_fota_bank.c` defines
  and `src/xy_fota_secure.c` calls, and the library's own include path lists the root copy
  first, so a consumer of `xy_fota` could not see them.
* `components/crypto/` published `xy_tiny_crypto.h` twice. The root copy declared 27 entry
  points, `inc/` declares 70 - the whole SHA-1, CSPRNG, X25519/Ed25519, RSA and bigint surface
  was invisible to a caller that resolved the root copy first.

So the rule is checked globally, not per component: a guard name belongs to one header. The
second half of the rule is the guard that cannot block at all - `#ifndef X` followed by
`#define Y` never defines the macro it tests, so the file is re-read on every inclusion
(`usb2uart.h`, `sensor.h`, `atc/core/xy_atc.h` each had this).

What is *not* a finding: two byte-identical copies (three vendored CMSIS headers are the same
file in two sample trees). Identical content means an include order picks the same declarations
either way, so there is nothing to lose.

The collisions that remain are pinned with the reason they are not converged yet, and a pin is
as auditable as a finding: it must still name a live collision, it must still differ, it must
state a reason, and a collision inside a *single directory* may not be pinned at all - include
order cannot excuse it, because there is only one place a `#include "x.h"` can resolve to.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]

#: Trees that are not first-party C. `MCU/` and `third_party/` are vendored; a dotted or build
#: directory is generated.
SKIP_PARTS = frozenset({"third_party", "MCU", "node_modules", "__pycache__"})


def is_first_party(path: Path, root: Path) -> bool:
    if path.suffix != ".h":
        return False
    try:
        parts = path.relative_to(root).parts
    except ValueError:
        return False
    for part in parts:
        if part in SKIP_PARTS or part.startswith(".") or part.startswith("build"):
            return False
    return True


#: Collisions that are known, understood and deliberately not converged yet. Each entry is
#: `guard -> (files, reason)`. A pin is not a mute button: `findings()` rejects one whose
#: collision is gone, whose files no longer differ, or that gives no reason.
PINS: dict[str, tuple[tuple[str, ...], str]] = {
    "PORTMACRO_H": (
        ("components/kernel/osal/portable/freertos/GCC/ARM_CM4F/portmacro.h",
         "components/kernel/osal/portable/freertos/GCC/ARM_CM33_NTZ/non_secure/portmacro.h"),
        "two upstream FreeRTOS CPU ports; the guard name is the port's own convention and the "
        "two files are compiled into different targets, never into one translation unit"),
    "XY_ADC_H": (
        ("components/analog_devices/xy_adc.h", "components/device/inc/xy_adc.h"),
        "a legacy AD/DAC helper and the Device-framework ADC driver publish different APIs "
        "under one guard; which one owns the name is an owner decision, not a rename"),
    "XY_FUEL_GAUGE_H": (
        ("components/fuel_gauge/inc/xy_fuel_gauge.h", "components/pm/fuel-gauge/xy_fuel_gauge.h"),
        "`pm/fuel-gauge/` is the 131-line PM-local gauge header the standalone component "
        "replaced; it is removed with the PM slice, and pinning it keeps the deletion visible"),
    "XY_OLED_SSD1306_H": (
        ("components/drivers/display/oled/ssd1306/xy_oled_ssd1306.h",
         "components/sensor/inc/xy_oled_ssd1306.h"),
        "the display driver copy and the sensor-side copy publish different APIs; SSD1306 work "
        "is deferred, so neither copy is being converged"),
    "XY_RGB_MATRIX_H": (
        ("components/drivers/display/lcd_drivers/xy_rgb_matrix.h",
         "components/drivers/display/led_drivers/serial_rgb/xy_rgb_matrix.h"),
        "a 177-line LCD-side matrix API and a 274-line LED-side one; which owner keeps the name "
        "is an open display decision, and both are currently buildable"),
    "XY_STDIO_H": (
        ("components/clib/xy_clib/xy_stdio.h", "components/trace/xy_log/inc/xy_stdio.h"),
        "the project stdio (202 lines) and a 20-line trace-oriented wrapper; both directories "
        "are on the same include path set, so the order decides - converging them changes the "
        "stdio surface and needs its own slice"),
    "_BANK_BATTERY_H_": (
        ("projects/Bank/battery.h", "projects/Bank/bank/inc/bank-battery.h"),
        "`projects/Bank` carries the same header names at its root and under `bank/inc/`; it is "
        "not part of any CMake build, and renaming inside a project tree is a Bank decision"),
    "_BANK_DEF_H_": (
        ("projects/Bank/bank.h", "projects/Bank/bank/inc/bank-def.h"),
        "same Bank root/`bank/inc/` duplication as `_BANK_BATTERY_H_`; not built, not converged"),
    "_CMDLINE_H_": (
        ("components/net/at/AT-Command-V2/samples/linux/include/cli.h",
         "components/net/at/AT-Command-V2/samples/none_os/framework/cli.h"),
        "two sample applications in the vendored AT command tree, each with its own copy; they "
        "are never compiled together and the tree is kept close to upstream"),
    "_COM_DEF_H_": (
        ("components/net/at/AT-Command-V2/samples/linux/include/comdef.h",
         "components/net/at/AT-Command-V2/samples/none_os/framework/comdef.h"),
        "same sample-tree duplication as `_CMDLINE_H_`"),
    "_XY_TYPEDEF_H_": (
        ("components/xy_typedef.h", "components/clib/xy_clib/xy_typedef.h"),
        "the two copies define NULL/true/false and ssize_t differently; converging them changes "
        "base type semantics for every translation unit and needs an architecture decision"),
}

DIRECTIVE = re.compile(r"^\s*#\s*(\w+)\s*(\w+)?")
GUARD_TRAILER = re.compile(r"^\s*#\s*endif\b")


def first_guard(text: str) -> tuple[str | None, str | None]:
    """The file's guard as `(macro it checks, macro it defines)`, or `(None, None)`.

    "A leading `#ifndef`" is not enough to call something a guard: `at_log.h` opens with
    `#ifndef LOG_TAG` around the logging macro block, and reading that as a guard reported a
    defect in a correct file. A guard wraps the *whole* header, so the `#endif` that closes it
    has to be the last directive in the file. That structural test is what separates the two,
    and it also means an unterminated conditional is "no guard" rather than a guard.

    The `#define` is the next directive inside the conditional. Anything else means the file
    tests a macro it never defines, returned as `(name, None)` so the caller can say so.
    """
    lines = text.splitlines()
    opening = None
    opening_match = None
    for index, line in enumerate(lines):
        match = DIRECTIVE.match(line)
        if match:
            opening, opening_match = index, match
            break
    if opening is None or opening_match is None:
        return None, None
    if opening_match.group(1) != "ifndef" or not opening_match.group(2):
        return None, None
    name = opening_match.group(2)

    depth = 0
    closing = None
    for index in range(opening, len(lines)):
        inner = DIRECTIVE.match(lines[index])
        if not inner:
            continue
        if inner.group(1) in ("if", "ifdef", "ifndef"):
            depth += 1
        elif inner.group(1) == "endif":
            depth -= 1
            if depth == 0:
                closing = index
                break
    if closing is None:
        return None, None
    for line in lines[closing + 1:]:
        if DIRECTIVE.match(line):
            return None, None

    defined = None
    for line in lines[opening + 1:closing]:
        inner = DIRECTIVE.match(line)
        if inner:
            defined = inner.group(2) if inner.group(1) == "define" else None
            break
    return name, defined


def headers(root: Path = ROOT) -> dict[str, str]:
    """Every first-party header's comment-stripped text, keyed by repository-relative path."""
    found: dict[str, str] = {}
    for path in sorted(root.rglob("*.h")):
        if not is_first_party(path, root):
            continue
        found[path.relative_to(root).as_posix()] = read_code(path)
    return found


def scan(files: dict[str, str]) -> tuple[dict[str, list[str]], dict[str, tuple[str, str | None]]]:
    """`guard -> files` plus `path -> (checked macro, defined macro)` for the broken guards."""
    groups: dict[str, list[str]] = {}
    broken: dict[str, tuple[str, str | None]] = {}
    for name, text in files.items():
        checked, defined = first_guard(text)
        if checked is None:
            continue
        if defined != checked:
            broken[name] = (checked, defined)
            continue
        groups.setdefault(checked, []).append(name)
    for members in groups.values():
        members.sort()
    return groups, broken


def findings(files: dict[str, str] | None = None,
             pins: dict[str, tuple[tuple[str, ...], str]] | None = None) -> list[str]:
    tree = headers() if files is None else files
    pinned = PINS if pins is None else pins
    problems: list[str] = []

    if not tree:
        return ["no header was read - an empty walk reports the same result as a clean one"]

    groups, broken = scan(tree)

    for name, (checked, defined) in sorted(broken.items()):
        if defined is None:
            problems.append(
                f"{name}: `#ifndef {checked}` is not followed by a `#define` - the guard it tests "
                f"is never defined, so the file is re-read on every inclusion")
        else:
            problems.append(
                f"{name}: tests `#ifndef {checked}` but defines `{defined}` - the macro it tests is "
                f"never defined, so nothing is ever blocked and a second inclusion re-declares "
                f"everything in the file")

    collisions = {guard: members for guard, members in groups.items() if len(members) > 1}

    for guard, members in sorted(collisions.items()):
        identical = len({tree[path] for path in members}) == 1
        if identical:
            continue  # same declarations either way; nothing can be lost
        if guard in pinned:
            continue
        if len({Path(path).parent for path in members}) == 1:
            consequence = ("all of them sit in one directory, so no include order can pick between "
                           "them")
        else:
            consequence = "whichever the include path resolves second is silently not read"
        problems.append(
            f"{guard}: shared by {', '.join(members)} - {consequence}. Give each header its own "
            f"guard, forward one to the other, or state in PINS why the collision stands")

    for guard, (members, reason) in sorted(pinned.items()):
        if not reason.strip():
            problems.append(f"{guard}: pinned without a stated reason - a pin nobody can audit is "
                            f"a silent skip")
        listed = sorted(members)
        for path in listed:
            if path not in tree:
                problems.append(f"{guard}: pinned file {path} does not exist")
        live = collisions.get(guard)
        if live is None:
            problems.append(f"{guard}: pinned, but no two headers collide on it any more - the pin "
                            f"outlived the collision and now hides the next one")
            continue
        if live != listed:
            problems.append(f"{guard}: pinned for {', '.join(listed)}, but the colliding headers are "
                            f"now {', '.join(live)} - the pin describes a different defect")
        if len({tree[path] for path in live}) == 1:
            problems.append(f"{guard}: pinned as a divergence, but the pinning files are now "
                            f"identical - the pin outlived the difference")
        if len({Path(path).parent for path in live}) == 1:
            problems.append(f"{guard}: pinned, but all members are in one directory - include "
                            f"order cannot excuse that, so this one cannot be pinned")

    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("include_guard_collisions failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    tree = headers()
    groups, broken = scan(tree)
    collisions = {guard: members for guard, members in groups.items() if len(members) > 1}
    identical = [guard for guard, members in collisions.items()
                 if len({tree[path] for path in members}) == 1]
    print(f"include_guard_collisions ok ({len(tree)} headers, {len(groups)} distinct guards, "
          f"{len(collisions) - len(identical)} divergent collisions all pinned with a reason, "
          f"{len(identical)} identical duplicates, 0 guards that cannot block)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
