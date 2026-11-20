#!/usr/bin/env python3
"""Only the old framework itself may include the old framework.

`components/sensor/inc/` publishes two things that look alike and are not:

  * the **legacy sensor framework** - `xy_sensor.h` and the four headers it pulls in, plus the
    `sensor_*.h` files at the component root. `components/device` is the canonical owner of every
    type in this set (`xy_sensor_type_t`, `xy_sensor_device_t`, `xy_sensor_value_t`, the sample
    fetch/channel get contract), and three of those names carry two different definitions in the
    tree today, so a file that includes the legacy header is getting the *other* framework;
  * **bus shims that are merely misfiled** - `xy_i2c.h`, `xy_spi.h`, `xy_w25qxx.h` and friends are
    the only copy in the tree. They are a moving problem, not a duplicate one, and this rule does
    not touch them.

The transition plan (`docs/plans/2026-11-01-legacy-sensor-to-canonical-transition-plan.md`) counts
its own progress by how many consumers depend on the legacy set, and until this rule existed that
count was prose. Measured here rather than asserted: outside `components/sensor/` and the test tree
the set had **six** consumers - one driver header, four fuel-gauge headers and one fuel-gauge
source. The driver header is gone (P3b): `xy_bmi270.h` included `xy_sensor.h` for a
`xy_sensor_device_t base` member that nothing in the tree reads, and including the canonical
`xy_device.h` instead gives the same member name the canonical type.

MAX17043 was the first to go (P3a, first driver), and the three BQ gauges followed, which leaves this
rule with **no pins at all**: outside `components/sensor/` and the test tree, nothing includes the
legacy framework. The pin mechanism stays because it is the auditable form of "we know, and here is
why"; an empty table is a measurement, not a deletion.

What the pins said while they existed is worth keeping, because it is the reason the migration was
not a rename: the fuel-gauge drivers used the legacy *bus helpers* (`xy_sensor_bus_t`,
`xy_sensor_i2c_read_reg16`), and the canonical `xy_i2c_device_read_reg()` issues a separate transmit
and receive - a STOP in the middle of what these SMBus gauges do as one repeated-start transaction.
Pointing them at that helper would have compiled and changed the traffic on the wire. The canonical
side grew `xy_i2c_device_read_reg_repeated_start()`, `..._read_reg16_repeated_start()` and
`..._write_reg_repeated_start()` (asserted by `test_device_bus_repeated_start`), and then the four
drivers moved onto those, each keeping the legacy 100 ms transfer budget explicitly.

A pin is auditable in both directions: a pinned file that stops including the legacy set has
expired and is reported, and a consumer that appears without a pin is reported.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
SENSOR = REPO_ROOT / "components" / "sensor"

#: The legacy framework, by the two places its headers live. Each name must still exist, so this
#: list cannot rot into a rule about files nobody has any more.
LEGACY_FRAMEWORK = {}

#: Consumers of the legacy framework that have not been migrated, with the reason. Resolving one
#: means giving it a canonical equivalent, not renaming an include.
#:
#: Empty since P3a finished: the four fuel-gauge files that used to be pinned here now hold
#: `xy_i2c_device_t` and read through the repeated-start helpers. The table stays because the next
#: consumer - if one ever appears - has to argue for itself here instead of quietly adding a second
#: definition of `xy_sensor_type_t` to the tree.
PINS: dict[str, str] = {}

#: The framework's own tree is the one place allowed to include the framework.
EXEMPT_TREES = ("components/sensor",)

#: Directories that never hold a consumer of this project's framework headers.
SKIPPED_TREES = ("build", ".git", "third_party", "MCU", ".venv", "__pycache__", "node_modules")

INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*"([^"]+)"', re.M)


def strip_c_comments(text: str) -> str:
    """Remove `//` and `/* */` comments, keeping string and character literals intact.

    The rule is about what the preprocessor reads. `examples/pc_all_components/src/main.c` has
    `/* #include "xy_sensor.h" */` in it as a note to the reader, and reporting that as a consumer
    would be a claim that does not match the file.
    """
    out: list[str] = []
    index, length = 0, len(text)
    while index < length:
        char = text[index]
        if char == "/" and text.startswith("//", index):
            end = text.find("\n", index)
            index = length if end < 0 else end
        elif char == "/" and text.startswith("/*", index):
            end = text.find("*/", index + 2)
            index = length if end < 0 else end + 2
        elif char in "\"'":
            quote = char
            out.append(char)
            index += 1
            while index < length:
                out.append(text[index])
                index += 1
                if text[index - 1] == "\\" and index < length:
                    out.append(text[index])
                    index += 1
                elif text[index - 1] == quote:
                    break
        else:
            out.append(char)
            index += 1
    return "".join(out)


def framework_problems(sensor: Path = SENSOR) -> list[str]:
    """Every legacy header the plan names must still exist under `components/sensor/`."""
    problems: list[str] = []
    for name, relatives in sorted(LEGACY_FRAMEWORK.items()):
        if not any((sensor / relative).is_file() for relative in relatives):
            problems.append(
                f"{name}: listed as legacy framework but no longer exists under "
                f"components/sensor - re-check the transition plan's baseline")
    return problems


def scanned_files(root: Path = REPO_ROOT) -> list[Path]:
    """Every `.c`/`.h` outside the framework tree, the test tree and the vendor/build trees.

    `os.walk` rather than `rglob`, with the skip list applied to the directories themselves: the
    build tree holds tens of thousands of files and walking into it to throw the results away is
    most of the rule's runtime.
    """
    files: list[Path] = []
    exempt = {str(root / tree) for tree in EXEMPT_TREES}
    for directory, subdirectories, filenames in os.walk(root):
        subdirectories[:] = sorted(
            name for name in subdirectories
            if name not in SKIPPED_TREES
            and name != "tests"
            and os.path.join(directory, name) not in exempt)
        for name in sorted(filenames):
            if name.endswith((".c", ".h")):
                files.append(Path(directory) / name)
    return files


def legacy_includes(text: str) -> list[str]:
    """The legacy framework headers this file includes, in source order."""
    found = []
    for name in INCLUDE.findall(strip_c_comments(text)):
        bare = name.rsplit("/", 1)[-1]
        if bare in LEGACY_FRAMEWORK and bare not in found:
            found.append(bare)
    return found


def findings(root: Path = REPO_ROOT, pins: dict[str, str] | None = None) -> list[str]:
    pins = PINS if pins is None else pins
    problems = framework_problems(root / "components" / "sensor")
    seen: dict[str, list[str]] = {}

    files = scanned_files(root)
    if not files:
        problems.append("walk is broken: no source file was scanned")
    for path in files:
        relatives = legacy_includes(path.read_text(encoding="utf-8", errors="replace"))
        if relatives:
            seen[path.relative_to(root).as_posix()] = relatives

    for path, relatives in sorted(seen.items()):
        if path in pins:
            continue
        problems.append(
            f"{path}: includes {', '.join(relatives)} from components/sensor, which the canonical "
            f"components/device owns - the same names carry two different definitions, so the "
            f"file is on the legacy framework; align it or pin it with a reason")

    for path, reason in sorted(pins.items()):
        if not reason.strip():
            problems.append(f"{path}: pinned without stating why the legacy include is tolerable")
        elif not (root / path).is_file():
            problems.append(f"{path}: pinned as a legacy consumer, but the file no longer exists")
        elif path not in seen:
            problems.append(
                f"{path}: pinned as a legacy consumer, but it no longer includes the legacy "
                f"framework - the pin has expired")

    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("legacy_sensor_includes failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    consumers = [path for path in scanned_files()
                 if legacy_includes(path.read_text(encoding="utf-8", errors="replace"))]
    detail = f"{len(consumers)} consumers"
    if PINS:
        detail += " and all of them pinned"
    print(f"legacy_sensor_includes ok ({len(LEGACY_FRAMEWORK)} legacy framework headers, "
          f"{detail}, {len(PINS)} pins)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
