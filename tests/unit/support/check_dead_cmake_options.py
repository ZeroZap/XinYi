#!/usr/bin/env python3
"""A CMake option nothing reads is a switch that does not switch.

`examples/fota_example/CMakeLists.txt` declared `option(BUILD_EXAMPLES "Build FOTA examples" ON)` and never
looked at it: configuring with -DBUILD_EXAMPLES=OFF built every example anyway. That is the same defect as an
argparse option nothing reads - the interface is what the option list says it is - and it is worse in CMake,
because `-DBUILD_EXAMPLES=OFF` is the kind of thing a reader copies without checking.

The corpus a reference may legitimately appear in is deliberately wide, because an option can be read by a
preset file, a workflow, a Kconfig default, a Makefile or a helper script and still be doing its job. What it
cannot be is decoration.

The second half is the same defect one step further in: an option that is read is still a lie if the name it
reads can never hold the value the reader believes. `components/net/smbus/CMakeLists.txt` opened with

    if(NOT XY_COMPONENT_SMBUS AND NOT XY_CONFIG_SMBUS_ENABLED)
        return()
    endif()

and both names were undefined in CMake, so the condition was `TRUE AND TRUE` and the file returned before it
declared `xy_smbus` - from every configure, always. `XY_CONFIG_SMBUS_ENABLED` is a *C preprocessor macro* in
`components/xy_config.h`; CMake never sees it. `XY_COMPONENT_SMBUS`/`XY_PROTO_*`/`XY_DRIVER_*` do exist as
CMake variables, but only because `cmake/Kconfig.cmake` emits one per Kconfig symbol, so a name whose suffix
is not a declared symbol is undefined in exactly the same way. Those two namespaces are therefore checkable
against their source of truth instead of being guessed at: `XY_CONFIG_*` may not appear in a condition at all,
and a generated name in a condition must have a Kconfig symbol behind it.
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


#: The `XY_CONFIG_*` namespace belongs to `components/xy_config.h`, a C header. CMake has no such variable, so
#: a condition naming one is constant - and `if(NOT XY_CONFIG_X)` is constantly *true*, which is the direction
#: that silently skips work.
C_ONLY_NAME = re.compile(r"\bXY_CONFIG_[A-Z0-9_]+")
#: Variables CMake only ever has because `cmake/Kconfig.cmake` emitted them, one per Kconfig symbol.
GENERATED_NAME = re.compile(r"\bXY_(?:COMPONENT|PROTO|DRIVER)_[A-Z0-9_]+")
CONDITION = re.compile(r"\b(?:if|elseif|while)\s*\(([^)]*)\)")
KCONFIG_SYMBOL = re.compile(r"^\s*(?:menu)?config\s+([A-Za-z0-9_]+)", re.M)


def _kconfig_symbols(kconfigs: list[Path] | None = None) -> set[str]:
    """Every first-party Kconfig symbol name, without the `CONFIG_`/`XY_` decorations."""
    files = kconfigs if kconfigs is not None else _walk(("Kconfig", "Kconfig.*"))
    symbols: set[str] = set()
    for path in files:
        try:
            symbols.update(KCONFIG_SYMBOL.findall(path.read_text(encoding="utf-8", errors="replace")))
        except OSError:
            continue
    return symbols


def condition_findings(cmake: list[Path] | None = None,
                       kconfigs: list[Path] | None = None) -> list[str]:
    """Conditions that test a name CMake cannot have.

    Only `if`/`elseif`/`while` conditions are read: a `list(FILTER ... REGEX "XY_CONFIG_")` is a string, not a
    decision. The two namespaces are treated differently on purpose. `XY_CONFIG_*` is rejected outright because
    it has one owner - the C header - and mixing it into CMake is always the same mistake. A generated name is
    accepted when its suffix is a Kconfig symbol name, which is the only way it can come to exist.
    """
    files = cmake if cmake is not None else _walk(CMAKE_GLOBS)
    symbols = _kconfig_symbols(kconfigs)
    problems: list[str] = []
    for path in files:
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        text = re.sub(r"#[^\n]*", "", text)
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        for match in CONDITION.finditer(text):
            expression = " ".join(match.group(1).split())
            for name in sorted(set(C_ONLY_NAME.findall(expression))):
                problems.append(
                    f"{relative}: `if({expression})` tests {name}, which is a preprocessor macro in "
                    f"components/xy_config.h and never a CMake variable - the branch it guards is decided "
                    f"before anything can set it")
            for name in sorted(set(GENERATED_NAME.findall(expression))):
                if name[3:] in symbols:
                    continue
                problems.append(
                    f"{relative}: `if({expression})` tests {name}, but Kconfig declares no symbol "
                    f"{name[3:]} - nothing generates that variable, so the test is always false")
    return problems


def main() -> int:
    problems = findings() + condition_findings()
    if problems:
        print("dead_cmake_options failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"dead_cmake_options ok ({len(_walk(CMAKE_GLOBS))} CMake files, every option is read somewhere and "
          f"every gated name exists)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
