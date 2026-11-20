#!/usr/bin/env python3
"""`xy_hal.h` is the HAL's front door, and it used to open onto eight of twenty-two peripherals.

Measured on this repository before the header was completed: `components/hal/inc/` held 22
`xy_hal_<peripheral>.h` files and `xy_hal.h` included 8 of them. The other 14 - `can`, `dac`,
`delay`, `dma`, `error`, `exti`, `flash`, `i2s`, `lp_timer`, `rng`, `rtc`, `sys`, `time`, `wdg` -
were reachable only by naming them. That is a quiet failure mode rather than a loud one: in C99 a
call through a missing prototype is an *implicit declaration*, so a consumer that includes the
front door still compiles on many toolchains and then miscompiles the return value, and the
repository's own quick start (`components/hal/examples/hal_quickstart.c`) had to list
`xy_hal_delay.h`, `xy_hal_error.h` and `xy_hal_sys.h` beside the umbrella to be honest.

The umbrella is a two-way contract, so both directions are checked:

* a peripheral header under `components/hal/inc/` that the umbrella does not export -> a consumer of
  the front door gets a partial HAL, and nothing in the build says so;
* an umbrella include that names a header which is not there -> the include is a silent no-op and
  the missing surface looks exactly like a header that was never written.

There is no exemption list today and the mechanism for one is kept on purpose: if a peripheral ever
has to stay out of the umbrella (a header that only makes sense inside one platform backend, say),
the reason has to be written next to the name, and a stale entry is itself a finding.

Source-level only: this reads includes. It says nothing about whether a backend implements what the
headers declare - `hal_adc_dac_surface` and `hal_pc_coverage` own those claims.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HAL_INC = Path("components/hal/inc")
UMBRELLA = "xy_hal.h"
PERIPHERAL = re.compile(r"^xy_hal_[a-z0-9_]+\.h$")
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)

#: header file name -> why it is deliberately not exported by `xy_hal.h`. Empty means the umbrella
#: claims the whole directory, which is the current contract.
EXEMPT: dict[str, str] = {}


def peripheral_headers(tree: Path) -> list[str]:
    """Every peripheral header in `components/hal/inc`, sorted."""
    directory = tree / HAL_INC
    if not directory.is_dir():
        return []
    return sorted(path.name for path in directory.iterdir()
                  if path.is_file() and PERIPHERAL.match(path.name))


def umbrella_includes(tree: Path) -> list[str]:
    """The quoted headers `xy_hal.h` includes, in file order."""
    path = tree / HAL_INC / UMBRELLA
    if not path.is_file():
        return []
    return INCLUDE.findall(read_code(path))


def findings(root: Path | None = None,
             exempt: dict[str, str] | None = None) -> list[str]:
    tree = ROOT if root is None else root
    exempted = EXEMPT if exempt is None else exempt
    problems: list[str] = []

    directory = tree / HAL_INC
    if not directory.is_dir():
        return [f"{HAL_INC.as_posix()}: the header directory is not there - every rule below would "
                f"read an empty tree as a complete umbrella"]
    if not (directory / UMBRELLA).is_file():
        return [f"{HAL_INC.as_posix()}/{UMBRELLA}: the front door is gone"]

    peripherals = peripheral_headers(tree)
    if not peripherals:
        problems.append(f"{HAL_INC.as_posix()}: no `xy_hal_<peripheral>.h` was found - a tree with "
                        f"nothing to export reads the same as a fully exported one")
    includes = umbrella_includes(tree)

    seen: set[str] = set()
    for name in includes:
        if name in seen:
            problems.append(f"{UMBRELLA}: includes {name} twice")
        seen.add(name)
        if "/" in name or "\\" in name:
            problems.append(f"{UMBRELLA}: includes {name}, which is not a header of this directory - "
                            f"the umbrella exports the HAL surface and nothing else")
            continue
        if not (directory / name).is_file():
            problems.append(f"{UMBRELLA}: includes {name}, which does not exist - the include is a "
                            f"silent no-op, so the missing surface looks like an unwritten header")

    for name in peripherals:
        if name in seen:
            continue
        if name in exempted:
            if not exempted[name].strip():
                problems.append(f"{name}: listed as exempt without a reason - an unexplained "
                                f"omission is a silence")
            continue
        problems.append(f"{UMBRELLA}: does not export {name} - a consumer that includes only the "
                        f"front door gets a partial HAL, and C99 reports a missing prototype as an "
                        f"implicit declaration")

    for name, reason in sorted(exempted.items()):
        if not (directory / name).is_file():
            problems.append(f"{name}: exempt, but the header is gone - the exemption has outlived "
                            f"the code it described")
            continue
        if name in seen:
            problems.append(f"{name}: exempt from the umbrella, yet {UMBRELLA} includes it - one of "
                            f"the two is wrong")
        if not reason.strip():
            problems.append(f"{name}: exempt without a reason")

    return problems


def report(root: Path | None = None) -> str:
    tree = ROOT if root is None else root
    peripherals = peripheral_headers(tree)
    includes = umbrella_includes(tree)
    exported = [name for name in peripherals if name in set(includes)]
    return (f"hal_umbrella ok ({len(peripherals)} peripheral header(s), "
            f"{len(exported)}/{len(peripherals)} exported by {UMBRELLA}, "
            f"{len(EXEMPT)} exempt)")


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
