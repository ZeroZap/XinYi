#!/usr/bin/env python3
"""One tag name, two bodies.

`struct`, `union` and `enum` tags are a namespace the two other collision guards cannot see, because
they are not typedefs. A tag collision is a hard error in any translation unit that includes both
headers, and it is the one that stopped the umbrella header from pointing at the canonical Device
framework:

    components/xy_def.h:153        struct xy_device { struct xy_obj parent; uint8_t device_id; };
    components/device/inc/xy_device.h:194
                                   struct xy_device { const char *name; xy_dev_type_t type; ... };

`xy.h` includes both, so nothing that included the umbrella header could compile once the canonical
Device header was reachable through it. That copy was dead - zero references to `struct xy_obj`,
`xy_spinlock` or its members anywhere in the tree - and removing it is what let P2 land.

Tags are rare in this tree compared with typedefs (153 against 1162): most types are anonymous
`typedef struct { ... } name_t;`. That is exactly why the class went unnoticed - a collision needs two
headers to bother naming a tag at all.

The rule judges presence, not content: C forbids a second definition of a tag in a translation unit
whatever its body holds, so identical bodies collide just as different ones do. `struct x` and
`union x` do not collide - they are separate namespaces. Comments are stripped through `read_code()`,
so a commented-out definition is not a definition.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]

#: Copied-in code that names its own tags under names this project also uses.
VENDORED: dict[str, str] = {
    "components/crypto/curve25519-cortexm0-20150813":
        "vendored curve25519, ships its own STM32F0 peripheral headers",
    "components/net/at/AT-Command-V2": "vendored AT command samples",
    "components/net/at/rtthread-at": "RT-Thread's AT framework, vendored",
    "components/net/at/FreeRTOS-Cellular-Interface": "AWS cellular interface, vendored",
    "components/net/at/ats/ats-simcom":
        "SIMCOM A76XX AT command reference; its sources are not built (the net CMakeLists lists them "
        "commented out) and its headers are the vendor's own",
    "components/kernel/osal/portable": "FreeRTOS Kernel V10.4.6 port layer, vendored from Amazon",
}

#: Tags carrying two bodies. The owner is decided for this one - components/device is canonical - but
#: the old framework has not been migrated off yet, so the collision still exists and is recorded
#: rather than hidden. It reports itself stale the day the migration lands.
PINS: dict[str, str] = {
    "struct xy_sensor_device":
        "components/device/inc/xy_device.h is canonical (a Device base plus a sensor api pointer); "
        "components/sensor/inc/xy_sensor_device.h is the un-migrated old framework and holds the "
        "fields itself - both are reachable and neither can be included in one translation unit",
}

#: Build output, vendored code and host tests. `tests/` is skipped: a fake vendor header is meant to
#: differ from the real one and the two never meet.
SKIPPED = frozenset({"build", "third_party", ".git", "MCU", ".venv", "__pycache__", "node_modules",
                     "tests"})

#: `struct foo {` / `union foo {` / `enum foo {`, with the body captured by brace matching below.
TAG = re.compile(r"\b(struct|union|enum)\s+([A-Za-z_]\w*)\s*\{")


def corpus(root: Path = ROOT) -> dict[str, list[str]]:
    """Every tag in the project's headers, mapped to the files that define it.

    The body is deliberately not kept: C forbids a second definition whatever it holds, so the verdict
    depends only on how many headers define the tag. Extracting bodies here was dead work - no mutation
    of the brace matching could change a finding, which meant the code was untested and untestable.
    """
    found: dict[str, list[str]] = {}
    for directory, subdirectories, filenames in os.walk(root):
        subdirectories[:] = [name for name in subdirectories if name not in SKIPPED]
        relative = os.path.relpath(directory, root).replace(os.sep, "/")
        if any(relative == vendored or relative.startswith(vendored + "/") for vendored in VENDORED):
            continue
        for filename in filenames:
            if not filename.endswith(".h"):
                continue
            path = Path(directory) / filename
            text = read_code(path)
            for match in TAG.finditer(text):
                key = f"{match.group(1)} {match.group(2)}"
                files = found.setdefault(key, [])
                if path.relative_to(root).as_posix() not in files:
                    files.append(path.relative_to(root).as_posix())
    return found


def findings(root: Path = ROOT, pins: dict[str, str] | None = None) -> list[str]:
    pins = PINS if pins is None else pins
    tags = corpus(root)
    problems: list[str] = []

    if not tags:
        return ["walk is broken: no tag was found anywhere, so nothing was compared"]

    collisions = {name: files for name, files in tags.items() if len(files) > 1}

    for name in sorted(collisions):
        if name in pins:
            continue
        files = sorted(collisions[name])
        problems.append(
            f"{name}: defined in {len(files)} headers ({', '.join(files)}) with no pin - a translation "
            f"unit that includes two of them does not compile, and which two depends on the caller")

    for name, reason in sorted(pins.items()):
        if not reason.strip():
            problems.append(f"{name}: pinned without stating why the collision is tolerable")
        elif name not in collisions:
            problems.append(
                f"{name}: pinned as a collision, but it is now defined once - the pin has expired")
        elif any(not (root / path).is_file() for path in collisions[name]):
            problems.append(f"{name}: pinned against a file that no longer exists")

    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("tag_definition_collisions failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    tags = corpus()
    collisions = sum(1 for files in tags.values() if len(files) > 1)
    print(f"tag_definition_collisions ok ({len(tags)} tags, {collisions} defined more than once, "
          f"each with a reason; {len(VENDORED)} vendored subtrees excluded)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
