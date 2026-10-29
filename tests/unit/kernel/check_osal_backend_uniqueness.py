#!/usr/bin/env python3
"""Two questions the OSAL backends were not being asked.

1. **Is there one implementation per backend?** `components/kernel/osal/src/osal_baremetal.c` was a
   complete second copy of the bare-metal backend - 78 of the 79 entries, last touched five weeks
   before the live file at `backend/baremetal/xy_os_baremetal.c`. Nothing compiled it, nothing tested
   it, and no build file named it. It read as a maintained implementation, and the design notes still
   expect a file at that path. It is gone; this rule is what keeps the next copy from settling in.

2. **Does every published-looking entry have a declaration?** A function named `xy_os_*` is part of the
   API surface whether or not a header mentions it. The RTX5 backend implements fifteen names that no
   header declares (`xy_os_thread_create`, `xy_os_mutex_create`, ...) - the previous API generation,
   which makes it look like a current backend with 67 entries rather than an old one. Those are pinned
   with the count, and the pin fails in both directions: a new undeclared symbol is a defect, and so is
   a pinned one that gets declared, because then the pin is stale and the gain is off the record.

The verdict is a pure function over a mapping so the paired tests can damage one item at a time.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
OSAL = ROOT / "components" / "kernel" / "osal"
COMPONENTS = ROOT / "components"

#: Files that legitimately define OSAL entries outside a backend directory. Keyed by path relative to
#: the OSAL component, with the reason, so an entry here is a decision rather than a shrug.
SHARED_SOURCES = {
    "src/xy_os_tick.c": "tick accounting, backend-independent by design",
    "src/xy_os_timer_sw.c": "software timer list, used by the backends that have no timer hardware",
}

#: How many entries a file must define before it is treated as a backend implementation rather than a
#: helper. Well below the smallest backend (67) and well above any helper file in the tree.
BACKEND_THRESHOLD = 20

#: Undeclared entries that exist today, per file, with the reason. Fails when the set grows AND when it
#: shrinks - a pinned gap that quietly closes leaves the record saying there is a gap.
PINNED_UNDECLARED = {
    "backend/rtx5/xy_os_rtx5.c": (
        {
            "xy_os_delay_isr",
            "xy_os_event_flags_create",
            "xy_os_get_sys_time_ms",
            "xy_os_get_sys_time_us",
            "xy_os_is_in_isr",
            "xy_os_mempool_create",
            "xy_os_msgqueue_create",
            "xy_os_mutex_create",
            "xy_os_semaphore_create",
            "xy_os_thread_create",
            "xy_os_thread_delay",
            "xy_os_thread_delay_until",
            "xy_os_thread_set_name",
            "xy_os_thread_yield_isr",
            "xy_os_timer_create",
        },
        "the previous API generation; this backend has not been migrated",
    ),
}

DEFINITION = re.compile(r"^[A-Za-z_][\w \t\*]*\b(xy_os_\w+)\s*\(", re.M)
DECLARATION = re.compile(r"\b(xy_os_\w+)\s*\(")


def implementation_sources() -> dict[str, set[str]]:
    """Every OSAL .c file that defines at least one xy_os_* entry, keyed by path relative to OSAL."""
    found: dict[str, set[str]] = {}
    for path in sorted(OSAL.rglob("*.c")):
        relative = path.relative_to(OSAL)
        if "tests" in relative.parts or "examples" in relative.parts or "portable" in relative.parts:
            continue
        names = set(DEFINITION.findall(read_code(path)))
        if names:
            found[str(relative)] = names
    return found


def declared_entries() -> set[str]:
    """Every xy_os_* name mentioned by a header anywhere in the tree."""
    names: set[str] = set()
    for path in sorted(COMPONENTS.rglob("*.h")):
        names |= set(DECLARATION.findall(read_code(path)))
    return names


def placement_findings(sources: dict[str, set[str]]) -> list[str]:
    """One implementation per backend: a big file is a backend, and backends live in backend/<name>/."""
    problems: list[str] = []
    for relative, names in sorted(sources.items()):
        if len(names) < BACKEND_THRESHOLD:
            continue
        if relative.startswith("backend/"):
            continue
        if relative in SHARED_SOURCES:
            continue
        problems.append(
            f"{relative}: defines {len(names)} OSAL entries but is not a backend directory and is not "
            f"a pinned shared source - this is a second implementation of a backend")
    return problems


def declaration_findings(sources: dict[str, set[str]], declared: set[str]) -> list[str]:
    """Every published-looking entry must be declared in some header."""
    problems: list[str] = []
    for relative, names in sorted(sources.items()):
        pinned, _reason = PINNED_UNDECLARED.get(relative, (set(), ""))
        for name in sorted((names - declared) - pinned):
            problems.append(
                f"{relative}:{name}: defined but declared in no header - either declare it or make it "
                f"static")
    return problems


def stale_pin_findings(sources: dict[str, set[str]], declared: set[str]) -> list[str]:
    """A pinned gap has to fail in both directions.

    Judged against the whole walk, so it only means anything for the real tree - the paired cases call
    it separately, because against a one-file fixture every other pinned name looks closed.
    """
    problems: list[str] = []
    for relative, (pinned, reason) in sorted(PINNED_UNDECLARED.items()):
        still_undeclared = sources.get(relative, set()) - declared
        for name in sorted(pinned - still_undeclared):
            problems.append(
                f"{relative}:{name}: is declared now or was removed, so the pin is stale ({reason}) - "
                f"tighten it so the gain is on the record")
    return problems


def main() -> int:
    sources = implementation_sources()
    if not sources:
        print("osal_backend_uniqueness failed: no OSAL sources were found - the walk is broken")
        return 1

    declared = declared_entries()
    problems = (placement_findings(sources) + declaration_findings(sources, declared) +
                stale_pin_findings(sources, declared))

    # Say what was inspected every run: a rule that silently sees nothing reads exactly like a rule
    # that holds everywhere.
    inspected = ", ".join(f"{Path(relative).name} {len(names)}" for relative, names in
                          sorted(sources.items()))
    if problems:
        print("osal_backend_uniqueness failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1

    print(f"osal_backend_uniqueness ok ({len(sources)} sources: {inspected}; "
          f"{len(declared)} declared names; "
          f"{sum(len(names) for names, _ in PINNED_UNDECLARED.values())} pinned undeclared)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
