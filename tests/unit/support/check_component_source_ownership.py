#!/usr/bin/env python3
"""A component may use a sibling component, but it may not compile its sources.

`components/sensor/CMakeLists.txt` used to append ten canonical Device-model
driver files that live under `components/drivers/` to its own source list:

    list(APPEND SENSOR_DRIVERS
        "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/sht30/xy_sht30.c"
        ... nine more ...
    )

Those same files are collected by `components/drivers/CMakeLists.txt` into
`xy_drivers`. Any configuration with `DRIVER_SENSOR=ON` and
`COMPONENT_SENSOR=ON` therefore compiled each of them twice: an `nm -g
--defined-only` sweep of one such build found **80** globals defined in both
`libxy_drivers.a` and `libsensor_component.a`.

Nothing fails in that state, which is why it needs a rule. A static archive is
only searched, so the implementation a program actually gets is decided by
archive order, not by the build - and two copies of a driver carry two copies of
whatever state it keeps.

The fix is the same shape as the HAL-backend rule: one library owns the
implementation (`xy_sensor_drivers_core`), and the consumer links it.

What is not a finding: headers or include directories owned by another
component (declarations are the public surface), linking a sibling target, and
board/project CMakeLists outside `components/`, which compose images rather than
own reusable implementations.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
COMPONENTS = REPO_ROOT / "components"

# A `.c` token: quoted or bare, optionally built from CMake path variables.
SOURCE_TOKEN = re.compile(r"""[^\s"'();#]*\.c(?![A-Za-z0-9_])""")

# Variables that name a directory we can resolve without evaluating CMake.
_DIR_VARS = {
    "${CMAKE_CURRENT_LIST_DIR}": None,      # replaced per file
    "${CMAKE_CURRENT_SOURCE_DIR}": None,    # replaced per file
    "${CMAKE_SOURCE_DIR}": REPO_ROOT,
    "${PROJECT_SOURCE_DIR}": REPO_ROOT,
}

SKIP_OWNERS = {"third_party"}


def strip_comments(text: str) -> str:
    """Drop `#` comments so prose about another component is not a finding."""
    out = []
    for line in text.splitlines():
        in_quote = False
        cut = line
        for index, char in enumerate(line):
            if char == '"':
                in_quote = not in_quote
            elif char == "#" and not in_quote:
                cut = line[:index]
                break
        out.append(cut)
    return "\n".join(out)


def _resolve(token: str, here: Path) -> Path | None:
    path = token
    for name, value in _DIR_VARS.items():
        if name in path:
            path = path.replace(name, str(here if value is None else value))
    if "${" in path:
        return None  # a variable we cannot evaluate; do not guess
    candidate = Path(path)
    return candidate if candidate.is_absolute() else here / candidate


def component_of(path: Path) -> str | None:
    try:
        rel = path.resolve().relative_to(COMPONENTS)
    except ValueError:
        return None
    return rel.parts[0] if rel.parts else None


def cross_component_sources(text: str, here: Path) -> list[tuple[str, str]]:
    """Return (reference, owning component) pairs compiled by the wrong owner."""
    mine = component_of(here)
    found: list[tuple[str, str]] = []
    for token in SOURCE_TOKEN.findall(strip_comments(text)):
        resolved = _resolve(token, here)
        if resolved is None:
            continue
        owner = component_of(resolved)
        if owner is None or owner == mine or owner in SKIP_OWNERS:
            continue
        found.append((token, owner))
    return found


def scanned_cmakelists() -> list[Path]:
    return [p for p in sorted(COMPONENTS.rglob("CMakeLists.txt"))
            if "third_party" not in p.relative_to(COMPONENTS).parts]


def findings() -> list[str]:
    problems: list[str] = []
    for path in scanned_cmakelists():
        text = path.read_text(encoding="utf-8", errors="replace")
        for ref, owner in cross_component_sources(text, path.parent):
            problems.append(
                f"{path.relative_to(REPO_ROOT).as_posix()} compiles `{ref}` owned by "
                f"components/{owner}; link that component's library instead so the "
                f"implementation has one owner")
    return problems


def main() -> int:
    problems = findings()
    scanned = scanned_cmakelists()
    if problems:
        print("component_source_ownership failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"component_source_ownership ok ({len(scanned)} component CMakeLists, "
          f"0 sibling-component sources compiled)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
