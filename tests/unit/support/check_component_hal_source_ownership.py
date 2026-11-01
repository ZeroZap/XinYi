#!/usr/bin/env python3
"""A component may use the HAL, but it may not compile the HAL backend itself.

`xy_hal` is the single owner of the platform backends. When a component lists
those `.c` files as its own sources, the same implementation is compiled twice
and both copies end up in archives that are linked together:

    components/device/CMakeLists.txt compiled `../hal/PC/xy_hal_{pc,gpio_pc,
    i2c_pc,spi_pc,uart_pc}.c` while the root build also compiled the identical
    files into `xy_hal`. A PC configure with `COMPONENT_DEVICE=ON` therefore
    produced **59** `xy_hal_*` definitions in both `libxy_device.a` and
    `libxy_hal.a`.

Nothing fails in that state, which is what makes it worth a rule: a static
archive is only searched, so the implementation a program actually gets is
decided by archive order, not by the build. The two copies also carry their own
copies of any backend state (the PC simulation keeps its pin state in statics),
so "which copy answers" is not a free choice even when the objects are
byte-identical today.

The fix is always the same and is already what the embedded branches did:
`target_link_libraries(<component> PUBLIC xy_hal)`.

What is not a finding: including or exposing `hal/inc` headers (declarations are
the public HAL surface), and `components/hal/CMakeLists.txt` itself, which is the
owner.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
COMPONENTS = REPO_ROOT / "components"

# `.../hal/<platform>/<file>.c` — a backend implementation, not a header.
HAL_SOURCE = re.compile(r'[^\s"\'();]*\bhal/[A-Za-z0-9_]+/[A-Za-z0-9_./-]+\.c\b')

# Components that legitimately reference HAL backend paths in prose only; keep
# the list empty unless a real exception appears, and require a reason with it.
PINS: dict[str, str] = {}


def hal_source_references(text: str) -> list[str]:
    """Return the HAL backend implementation paths a CMakeLists lists."""
    return sorted(set(HAL_SOURCE.findall(text)))


def component_cmakelists() -> list[Path]:
    """Every component CMakeLists that could compile a HAL backend."""
    found = []
    for path in sorted(COMPONENTS.rglob("CMakeLists.txt")):
        rel = path.relative_to(REPO_ROOT).as_posix()
        if rel.startswith("components/hal/"):
            continue  # the owner of the backends
        if rel.startswith("components/third_party/"):
            continue  # vendor tree
        found.append(path)
    return found


def findings() -> list[str]:
    problems: list[str] = []
    for path in component_cmakelists():
        text = path.read_text(encoding="utf-8", errors="replace")
        refs = hal_source_references(text)
        if not refs:
            continue
        rel = path.relative_to(REPO_ROOT).as_posix()
        if rel in PINS:
            continue
        for ref in refs:
            problems.append(
                f"{rel} compiles the HAL backend source `{ref}`; "
                f"link `xy_hal` instead so the backend has one owner"
            )
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("component_hal_source_ownership failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    scanned = component_cmakelists()
    print(f"component_hal_source_ownership ok "
          f"({len(scanned)} component CMakeLists, 0 HAL backend sources compiled, "
          f"{len(PINS)} pinned)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
