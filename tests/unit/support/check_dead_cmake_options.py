#!/usr/bin/env python3
"""A CMake option nothing reads is a switch that does not switch.

`examples/fota_example/CMakeLists.txt` declared `option(BUILD_EXAMPLES "Build FOTA examples" ON)` and never
looked at it: configuring with -DBUILD_EXAMPLES=OFF built every example anyway. That is the same defect as an
argparse option nothing reads - the interface is what the option list says it is - and it is worse in CMake,
because `-DBUILD_EXAMPLES=OFF` is the kind of thing a reader copies without checking.

The corpus a reference may legitimately appear in is deliberately wide, because an option can be read by a
preset file, a workflow, a Kconfig default, a Makefile or a helper script and still be doing its job. What it
cannot be is decoration.
"""

from __future__ import annotations

import fnmatch
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CMAKE_GLOBS = ("**/CMakeLists.txt", "**/*.cmake")
SKIP_PREFIXES = ("build/", "tools/.venv/", "MCU/", "boards/third_party/", "tools/third_party/")
#: files whose text counts as a reference, even though they declare nothing
# Documentation is deliberately absent: a mention in a README is not a consumer, and counting it would let a
# decorative option pass because somebody wrote about it. The rest of the list is where a real reader lives.
REFERENCE_GLOBS = ("Kconfig*", "**/Kconfig*", "Makefile", "**/Makefile", ".github/workflows/*.yml",
                   "**/CMakePresets.json", "tools/*.py", "boards/**/*.py")

OPTION = re.compile(r"^\s*option\(\s*([A-Za-z_][A-Za-z0-9_]*)", re.M)

#: name -> why it is declared and intentionally unread. Empty on purpose.
PINNED: dict[str, str] = {}


#: directories never descended into, so the filesystem walk does not pay for the build tree or vendors
PRUNE_DIRS = {"build", ".git", ".venv", "MCU", "third_party", "node_modules", "__pycache__"}


def _walk(globs: tuple[str, ...]) -> list[Path]:
    """Files matching the globs, without descending into directories that can only waste time.

    Globbing the whole tree and filtering afterwards is what made this take 40 seconds: build/ and the vendor
    trees are pruned here instead, before they are read.
    """
    # Every glob here ends in a basename pattern, so matching on the basename is exact. Filtering patterns by
    # "does not start with **" silently dropped CMakeLists.txt and left the guard scanning .cmake files only -
    # a run that reported ok while never looking at the file the defect was in.
    patterns = [Path(pattern).name for pattern in globs]
    found: list[Path] = []
    for directory, subdirectories, filenames in os.walk(ROOT):
        subdirectories[:] = sorted(d for d in subdirectories if d not in PRUNE_DIRS)
        for filename in sorted(filenames):
            path = Path(directory) / filename
            relative = path.relative_to(ROOT).as_posix()
            if any(relative.startswith(prefix) for prefix in SKIP_PREFIXES):
                continue
            if any(fnmatch.fnmatch(filename, pattern) for pattern in patterns):
                found.append(path)
    return found


def findings(declaring: list[Path] | None = None, referencing: list[Path] | None = None,
             pinned: dict[str, str] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    declarations = declaring if declaring is not None else _walk(CMAKE_GLOBS)
    corpus = referencing if referencing is not None else _walk(CMAKE_GLOBS + REFERENCE_GLOBS)
    if not declarations:
        return ["no CMake file was inspected - the walk is broken"]
    texts = {path: path.read_text(encoding="utf-8", errors="replace") for path in set(declarations) | set(corpus)}
    problems: list[str] = []
    for path in declarations:
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        for name in OPTION.findall(texts[path]):
            if name in pinned:
                continue
            references = sum(len(re.findall(rf"\b{re.escape(name)}\b", text)) for text in texts.values())
            if references <= 1:  # the declaration is the only mention
                problems.append(
                    f"{relative}: declares option({name}) and nothing reads it - a reader who passes "
                    f"-D{name}=... changes nothing; wire it up or delete it")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("dead_cmake_options failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"dead_cmake_options ok ({len(_walk(CMAKE_GLOBS))} CMake files, every option is read somewhere)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
