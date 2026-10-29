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

Every one of these questions is a question about *live* CMake. A commented-out branch answers
`dispatched()` from the raw text exactly as well as a live one, and a guard that reads prose
here is green on the thing it exists to catch - the offered-but-undispatched backend. So the
CMake and Kconfig text goes through source_scan.code_only_hash() before any of it is matched.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import code_only_hash  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
ROOT_CMAKE = ROOT / "CMakeLists.txt"
OSAL_CMAKE = ROOT / "components" / "kernel" / "osal" / "CMakeLists.txt"
KERNEL_CMAKE = ROOT / "components" / "kernel" / "CMakeLists.txt"
ROOT_KCONFIG = ROOT / "Kconfig"

# Compiled nowhere, and named so the branches below cannot quietly become dead code.
UNREACHABLE = {
    "rtthread": "backends exist, but the only Kconfig the parser reads is the root one and it "
                "declares no RTTHREAD symbol - the declaration sits in components/kernel/Kconfig, "
                "which nothing parses; kept pending a decision to wire or remove",
    "cmsis_rtx": "same as rtthread: source and a dispatch branch, and the symbol that would select "
                 "it (CONFIG_OSAL_BACKEND_CMSIS_RTX) is declared in no Kconfig file under any prefix",
}

#: Not even a dispatch branch: a directory with sources that nothing can ever build. Kept separate
#: from UNREACHABLE because that set means "a branch exists and the build refuses the selection",
#: which is a weaker claim than "there is no branch at all".
ORPHANED = {
    "rtx5": "compiled nowhere and no OSAL_BACKEND branch names it. Kconfig's RTX5 option is live, but "
            "components/kernel/CMakeLists.txt maps CONFIG_OSAL_BACKEND_RTX5 onto "
            "backend/cmsis_rtx/xy_os_cmsis_rtx.c - so the option selects a different implementation "
            "and this directory is not the one anybody means. It is also a generation behind: 67 of "
            "the 79 entries, fifteen of them previous API names "
            "(see check_osal_backend_uniqueness.py)",
}


def advertised(osal_cmake: str) -> set[str]:
    osal_cmake = code_only_hash(osal_cmake)
    match = re.search(r"set_property\(\s*CACHE\s+OSAL_BACKEND\s+PROPERTY\s+STRINGS\s+([^)]*)\)",
                      osal_cmake)
    return set(match.group(1).split()) if match else set()


def selectable(root_cmake: str) -> set[str]:
    root_cmake = code_only_hash(root_cmake)
    return set(re.findall(r'set\(\s*OSAL_BACKEND\s+"([A-Za-z0-9_]+)"\s+CACHE\s+STRING', root_cmake))


def dispatched(osal_cmake: str) -> set[str]:
    osal_cmake = code_only_hash(osal_cmake)
    return set(re.findall(r'(?:else)?if\(\s*OSAL_BACKEND\s+STREQUAL\s+"([A-Za-z0-9_]+)"',
                          osal_cmake))


#: CMake conditions on CONFIG symbols that the parsed Kconfig never defines. The root Kconfig is the
#: only file the parser reads - it handles no `source` directive - so a symbol declared only in
#: components/kernel/Kconfig or Kconfig.osal is declared nowhere as far as the build is concerned.
#: A condition on one is permanently false, which makes the branch behind it dead code.
PINNED_UNDEFINED_SYMBOLS = {
    "CONFIG_OSAL_BACKEND_RTTHREAD":
        "declared in components/kernel/Kconfig, which the parser never reads",
    "CONFIG_OSAL_BACKEND_RTX5":
        "declared in components/kernel/Kconfig, which the parser never reads",
    "CONFIG_OSAL_BACKEND_CMSIS_RTX":
        "declared in no Kconfig file at all, under either prefix - the branch it guards is "
        "unreachable twice over",
}

CONDITION = re.compile(r"^\s*(?:else)?if\s*\((.*)$")
SYMBOL = re.compile(r"\b(CONFIG_[A-Z0-9_]+)\b")
DECLARED = re.compile(r"^config\s+([A-Za-z0-9_]+)", re.M)


def condition_symbols(cmake_text: str) -> set[str]:
    """CONFIG symbols used inside if()/elseif() conditions - the ones a build actually branches on."""
    found: set[str] = set()
    for line in code_only_hash(cmake_text).splitlines():
        match = CONDITION.match(line)
        if match:
            found |= set(SYMBOL.findall(match.group(1)))
    return found


def parsed_symbols(root_kconfig: str) -> set[str]:
    """Symbols the config parser will define, which is exactly what the root Kconfig declares."""
    return {f"CONFIG_{name}" for name in DECLARED.findall(code_only_hash(root_kconfig))}


def undefined_symbol_findings(cmake_text: str, root_kconfig: str,
                              pinned: dict[str, str] | None = None) -> list[str]:
    """A branch guarded by a symbol nothing defines is a branch that cannot be taken."""
    pinned = PINNED_UNDEFINED_SYMBOLS if pinned is None else pinned
    used = condition_symbols(cmake_text)
    if not used:
        return ["no CONFIG symbols were found in any condition - a parser that finds nothing is not "
                "the same as every symbol being defined"]
    declared = parsed_symbols(root_kconfig)
    problems: list[str] = []
    for name in sorted(used - declared - set(pinned)):
        problems.append(
            f"{name} is tested in a condition but the parsed Kconfig never defines it, so the branch "
            f"behind it can never be taken")
    for name in sorted(set(pinned) - (used - declared)):
        problems.append(
            f"{name} is pinned as undefined but is either defined now or no longer tested - the pin "
            f"is stale ({pinned[name]})")
    return problems


def backend_directories(osal_dir: Path) -> set[str]:
    """Backend directories that actually hold an implementation."""
    backend = osal_dir / "backend"
    if not backend.is_dir():
        return set()
    return {child.name for child in backend.iterdir()
            if child.is_dir() and any(child.glob("*.c"))}


def directory_findings(branches: set[str], unreachable: dict[str, str],
                       orphaned: dict[str, str], on_disk: set[str]) -> list[str]:
    """A backend directory on disk is a claim that this backend exists here.

    It has to be dispatched by the OSAL component, or declared unreachable / orphaned with a reason.
    Otherwise it is a third kind of file: one that reads as an implementation and compiles nowhere.
    Kept as its own pure function because the paired cases damage one item at a time - folding it into
    findings() judges every fixture against the real tree as well.
    """
    problems: list[str] = []
    for name in sorted(on_disk):
        if name in branches or name in unreachable or name in orphaned:
            continue
        problems.append(
            f"backend/{name}/ holds an implementation, but no OSAL_BACKEND branch dispatches to it "
            f"and it is not declared unreachable or orphaned - say which it is")
    for name in sorted(set(unreachable) - on_disk):
        problems.append(
            f"{name} is listed as unreachable but there is no backend/{name}/ with sources - the "
            f"reason is attached to nothing")
    for name in sorted(set(orphaned) - on_disk):
        problems.append(f"{name} is declared orphaned but there is no backend/{name}/ with sources")
    return problems


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

    for name in sorted(set(ORPHANED) & set(unreachable)):
        problems.append(f"{name} is declared both unreachable and orphaned; pick one")
    for name in sorted(set(ORPHANED) & branches):
        problems.append(f"{name} is declared orphaned but a dispatch branch names it")
    for name in sorted(branches - offered - set(unreachable) - set(ORPHANED)):
        problems.append(f"there is a dispatch branch for {name}, which is neither offered nor "
                        f"declared unreachable - it is dead code nobody has decided about")
    for name in sorted(set(unreachable) - branches):
        problems.append(f"{name} is declared unreachable but has no dispatch branch; the "
                        f"declaration is stale")
    # A selection with no branch is not merely unreachable - the build sets the variable and then
    # compiles no backend at all. Nothing else here catches it: the two set comparisons agree, the
    # branch set is a subset of the offered set, and only this test asks whether the selection a
    # person can make in the cache actually reaches an implementation.
    for name in sorted((offered & chosen) - branches):
        problems.append(f"{name} is offered and selectable but no dispatch branch names it - "
                        f"selecting it would set the cache and build no backend")
    for name, reason in sorted({**unreachable, **ORPHANED}.items()):
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
    osal_text = OSAL_CMAKE.read_text(encoding="utf-8")
    problems = findings(root_text, osal_text)
    problems.extend(directory_findings(dispatched(osal_text), UNREACHABLE, ORPHANED,
                                       backend_directories(OSAL_CMAKE.parent)))
    # The parsed Kconfig is the root one: the parser reads that file and follows no source directive.
    problems.extend(undefined_symbol_findings(
        KERNEL_CMAKE.read_text(encoding="utf-8") + "\n" + osal_text,
        ROOT_KCONFIG.read_text(encoding="utf-8")))
    problems.extend(doc_command_findings(design_docs(), selectable(root_text)))
    if problems:
        print("osal_backend_advertise failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"osal_backend_advertise ok (advertised=selectable={sorted(selectable(ROOT_CMAKE.read_text(encoding='utf-8')))}, "
          f"unreachable={sorted(UNREACHABLE)}, orphaned={sorted(ORPHANED)}, "
          f"undefined-symbols={len(PINNED_UNDEFINED_SYMBOLS)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
