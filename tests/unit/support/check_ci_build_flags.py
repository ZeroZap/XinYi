#!/usr/bin/env python3
"""A workflow must not pass a CMake variable that the build overrides or rejects.

Two ways for a workflow's flag to be a lie, both found in this repository's own workflows:

1. The build sets the variable with FORCE from Kconfig - `set(BUILD_TESTING ${CONFIG_BUILD_TESTING} CACHE
   BOOL "Build tests" FORCE)` - so `-DBUILD_TESTING=OFF` is computed away at configure time. The step looks
   like it disables tests and does not.
2. The build refuses the request with a FATAL_ERROR: `-DOSAL_BACKEND=freertos` is answered with "the request
   cannot take effect, and quietly substituting a different kernel would make every downstream result
   ambiguous". The step does not fail the way it reads, it fails to configure at all.

Both are the same defect seen from the reader's side: a flag in CI that does nothing. That is worse than no
flag, because it fails the first time somebody trusts it.

`-DKCONFIG_OVERRIDES="NAME=VALUE"` is the route that actually takes effect.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
WORKFLOWS = ROOT / ".github" / "workflows"
CMAKE_GLOBS = ("CMakeLists.txt", "cmake/**/*.cmake", "tests/**/CMakeLists.txt", "boards/**/CMakeLists.txt")
SKIP_PREFIXES = ("build/", "tools/.venv/", "MCU/", "boards/third_party/")

#: (workflow file, flag) pairs that are allowed with a reason. Empty on purpose.
PINNED: dict[tuple[str, str], str] = {}

#: flags rejected outright, with the build's own reason.
REJECTED = {
    "-DOSAL_BACKEND=":
        "the backend comes from Kconfig and this flag is rejected with a FATAL_ERROR, so the step fails to "
        "configure instead of building the backend it names - use -DKCONFIG_OVERRIDES=OSAL_BACKEND_<X>=ON",
}

FLAG = re.compile(r"-D([A-Za-z_][A-Za-z0-9_]*)=")
FORCED = re.compile(r"set\(\s*([A-Za-z_][A-Za-z0-9_]*)\b[^)]*\bCACHE\b[^)]*\bFORCE\b")
ANY_OPEN = re.compile(r"\b(if|foreach|while|function|macro|block)\s*\(")
ANY_CLOSE = re.compile(r"\b(endif|endforeach|endwhile|endfunction|endmacro|endblock)\s*\(")


def cmake_sources() -> list[Path]:
    found: list[Path] = []
    for pattern in CMAKE_GLOBS:
        for path in sorted(ROOT.glob(pattern)):
            relative = path.relative_to(ROOT).as_posix()
            if any(relative.startswith(prefix) for prefix in SKIP_PREFIXES):
                continue
            found.append(path)
    return found


def forced_names(paths: list[Path]) -> dict[str, str]:
    """name -> "file:line" of an *unconditional* set(... CACHE ... FORCE).

    Only an unconditional one makes -D<name> ineffective. A FORCE inside `if(NOT CMAKE_BUILD_TYPE)` is the
    project choosing a default, and an explicit -D still wins - flagging that would report a correct
    configuration as broken, and that is how a useful rule gets switched off.
    """
    forced: dict[str, str] = {}
    for path in paths:
        text = path.read_text(encoding="utf-8", errors="replace")
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        depth = 0
        for line_number, line in enumerate(text.split("\n"), start=1):
            if line.lstrip().startswith("#"):
                continue
            opens = len(ANY_OPEN.findall(line))
            closes = len(ANY_CLOSE.findall(line))
            if opens or closes:
                # A line that opens or closes a block is not an unconditional statement, even when it does
                # both at once - `if(A) set(X ... FORCE) endif()` is the shape that used to slip through.
                depth = max(0, depth + opens - closes)
                continue
            if depth:
                continue
            match = FORCED.search(line)
            if match:
                forced.setdefault(match.group(1), f"{relative}:{line_number}")
    return forced


def findings(paths: list[Path] | None = None, pinned: dict[tuple[str, str], str] | None = None,
             cmake_paths: list[Path] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    candidates = paths if paths is not None else sorted(WORKFLOWS.glob("*.yml"))
    sources = cmake_paths if cmake_paths is not None else cmake_sources()
    forced = forced_names(sources)
    problems: list[str] = []
    inspected = 0
    for path in candidates:
        inspected += 1
        text = path.read_text(encoding="utf-8", errors="replace")
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        for line_number, line in enumerate(text.split("\n"), start=1):
            if line.lstrip().startswith("#"):
                continue
            for flag, reason in REJECTED.items():
                if flag in line and (relative, flag) not in pinned:
                    problems.append(f"{relative}:{line_number}: passes {flag} - {reason}")
            for name in FLAG.findall(line):
                flag = f"-D{name}="
                if (relative, flag) in pinned or name not in forced:
                    continue
                problems.append(
                    f"{relative}:{line_number}: passes {flag} but {forced[name]} sets {name} with FORCE, so "
                    f"the request is computed away and the step does not do what it says - use "
                    f"-DKCONFIG_OVERRIDES=\"{name}=...\"")
    if inspected == 0:
        problems.append("no workflows were inspected - the walk is broken")
    if not sources:
        problems.append("no CMake sources were inspected - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("ci_build_flags failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"ci_build_flags ok ({len(list(WORKFLOWS.glob('*.yml')))} workflows against "
          f"{len(cmake_sources())} CMake files, no flag the build overrides or rejects)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
