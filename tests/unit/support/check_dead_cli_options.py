#!/usr/bin/env python3
"""An argparse option nothing reads is a knob that does not exist.

`tools/program_opench_ch32v307.py` declared `--mrs-root` and never referenced `args.mrs_root` once. It looked
like an override for a Windows-only install path, and it was a pin I had added to justify exactly that: a
reviewer reading the option list believes a knob exists, and the option list is what a tool's interface *is*.

The check is deliberately shallow and its blind spot is stated rather than hidden: a file that reaches its
arguments dynamically - `vars(args)`, `getattr(args, ...)` - cannot be judged by reading names, so it is
skipped and counted. If every file were skipped the run would look identical to a clean one, so the count is
reported and an empty walk is a failure.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SCAN_ROOTS = ("tools", "boards")
SKIP_PREFIXES = ("tools/tests", "tools/.venv", "boards/third_party", "tools/third_party")

#: (file, option) -> why the option exists without being read yet. Empty on purpose.
PINNED: dict[tuple[str, str], str] = {}

ADD_ARGUMENT = re.compile(r"add_argument\(\s*[\"']--([a-z0-9][a-z0-9-]*)[\"'](.*?)\)\s*\n", re.S)
DEST = re.compile(r"dest\s*=\s*[\"'](\w+)[\"']")
DYNAMIC = re.compile(r"\bvars\(|getattr\(\s*args\b|__dict__")


def tool_sources() -> list[Path]:
    found: list[Path] = []
    for root in SCAN_ROOTS:
        for path in sorted((ROOT / root).rglob("*.py")):
            relative = path.relative_to(ROOT).as_posix()
            if any(relative.startswith(prefix) for prefix in SKIP_PREFIXES):
                continue
            found.append(path)
    return found


def options_in(text: str) -> list[tuple[str, str, str]]:
    """(option, dest, whole call text) for every long option declared in the file."""
    declared: list[tuple[str, str, str]] = []
    for match in ADD_ARGUMENT.finditer(text):
        option = f"--{match.group(1)}"
        dest = DEST.search(match.group(2))
        declared.append((option, dest.group(1) if dest else match.group(1).replace("-", "_"), match.group(0)))
    return declared


def findings(paths: list[Path] | None = None, pinned: dict[tuple[str, str], str] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    candidates = paths if paths is not None else tool_sources()
    problems: list[str] = []
    inspected = dynamic = 0
    for path in candidates:
        text = path.read_text(encoding="utf-8", errors="replace")
        if "add_argument" not in text:
            continue
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        if DYNAMIC.search(text):
            dynamic += 1
            continue
        inspected += 1
        for option, dest, call in options_in(text):
            if (relative, option) in pinned:
                continue
            body = text.replace(call, "", 1)
            if not re.search(rf"\b{re.escape(dest)}\b", body):
                problems.append(
                    f"{relative}: declares {option} (dest={dest}) and never reads it - either wire it up or "
                    f"delete it, because an option that does nothing is still part of the interface")
    if inspected == 0:
        problems.append(f"no tool with a readable argument parser was inspected ({dynamic} were skipped as "
                        f"dynamic) - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("dead_cli_options failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"dead_cli_options ok ({len(tool_sources())} tool sources, every declared option is read)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
