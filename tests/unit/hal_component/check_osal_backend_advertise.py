#!/usr/bin/env python3
"""The OSAL backend offered in the cache must be a backend this build can actually select.

Three things have to agree, and nothing was making them:

  * what the OSAL component advertises (set_property CACHE OSAL_BACKEND PROPERTY STRINGS),
    which is what a person sees in cmake-gui or in the cache editor;
  * what the root build selects (the set(OSAL_BACKEND ... CACHE STRING ... FORCE) calls);
  * what the OSAL component has a dispatch branch for.

They disagreed: four backends were advertised, the root could only ever select two, and the
two extra ones were not merely unavailable - a selection naming either of them was replaced
by the Kconfig choice without a word. A build picking a different kernel than the one
requested is the kind of result that makes every later measurement ambiguous.

Backends that exist as sources but are unreachable from this build are listed with a reason,
so the dispatch branches are not silently dead code.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
ROOT_CMAKE = ROOT / "CMakeLists.txt"
OSAL_CMAKE = ROOT / "components" / "kernel" / "osal" / "CMakeLists.txt"

# Compiled nowhere, and named so the branches below cannot quietly become dead code.
UNREACHABLE = {
    "rtthread": "backends exist, but the root build derives OSAL_BACKEND from Kconfig and "
                "no Kconfig symbol selects it; kept pending a decision to wire or remove",
    "cmsis_rtx": "same as rtthread: source and dispatch branch only, not selectable",
}


def advertised(osal_cmake: str) -> set[str]:
    match = re.search(r"set_property\(\s*CACHE\s+OSAL_BACKEND\s+PROPERTY\s+STRINGS\s+([^)]*)\)",
                      osal_cmake)
    return set(match.group(1).split()) if match else set()


def selectable(root_cmake: str) -> set[str]:
    return set(re.findall(r'set\(\s*OSAL_BACKEND\s+"([A-Za-z0-9_]+)"\s+CACHE\s+STRING', root_cmake))


def dispatched(osal_cmake: str) -> set[str]:
    return set(re.findall(r'(?:else)?if\(\s*OSAL_BACKEND\s+STREQUAL\s+"([A-Za-z0-9_]+)"',
                          osal_cmake))


def findings(root_cmake: str, osal_cmake: str,
            unreachable: dict[str, str] | None = None) -> list[str]:
    unreachable = UNREACHABLE if unreachable is None else unreachable
    offered = advertised(osal_cmake)
    chosen = selectable(root_cmake)
    branches = dispatched(osal_cmake)
    problems: list[str] = []

    if not offered or not chosen or not branches:
        return ["could not parse one of the three sets (advertised / selectable / branches) - "
                "a parser that finds nothing is not the same as three sets that agree"]

    for name in sorted(offered - chosen):
        problems.append(f"{name} is offered in the cache but the root build can only select "
                        f"{sorted(chosen)}; selecting it would be replaced without a word")
    for name in sorted(chosen - offered):
        problems.append(f"{name} is selectable but not offered, so nobody would find it")

    for name in sorted(branches - offered - set(unreachable)):
        problems.append(f"there is a dispatch branch for {name}, which is neither offered nor "
                        f"declared unreachable - it is dead code nobody has decided about")
    for name in sorted(set(unreachable) - branches):
        problems.append(f"{name} is declared unreachable but has no dispatch branch; the "
                        f"declaration is stale")
    for name, reason in sorted(unreachable.items()):
        if not reason.strip():
            problems.append(f"{name} is declared unreachable without a reason")
    return problems


FENCE = re.compile(r"```[^\n]*\n(.*?)```", re.S)
DOC_COMMAND = re.compile(r"-DOSAL_BACKEND=([A-Za-z0-9_]+)")


def doc_command_findings(docs: dict[str, str], chosen: set[str]) -> list[str]:
    """Copy-pasteable commands in docs/design must name a backend this build accepts.

    Prose may explain what is unavailable; a fenced command that a reader will paste must
    not. Only fenced blocks are checked, so the surrounding explanation stays free.
    """
    problems: list[str] = []
    for where, text in sorted(docs.items()):
        for block in FENCE.findall(text):
            for line in block.splitlines():
                for value in DOC_COMMAND.findall(line):
                    if value not in chosen:
                        problems.append(
                            f"{where}: a fenced command tells readers to pass "
                            f"-DOSAL_BACKEND={value}, but this build only selects "
                            f"{sorted(chosen)} and refuses the rest")
    return problems


def design_docs(root: Path = ROOT) -> dict[str, str]:
    docs: dict[str, str] = {}
    for path in sorted((root / "docs" / "design").rglob("*.md")):
        docs[path.relative_to(root).as_posix()] = path.read_text(encoding="utf-8",
                                                                errors="replace")
    return docs


def main() -> int:
    root_text = ROOT_CMAKE.read_text(encoding="utf-8")
    problems = findings(root_text, OSAL_CMAKE.read_text(encoding="utf-8"))
    problems.extend(doc_command_findings(design_docs(), selectable(root_text)))
    if problems:
        print("osal_backend_advertise failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"osal_backend_advertise ok (advertised=selectable={sorted(selectable(ROOT_CMAKE.read_text(encoding='utf-8')))}, "
          f"unreachable={sorted(UNREACHABLE)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
