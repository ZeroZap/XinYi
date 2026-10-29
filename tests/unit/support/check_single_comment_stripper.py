#!/usr/bin/env python3
"""There is one comment stripper, and guards do not implement their own.

source_scan.code_only is a character walk with explicit states: it knows a `//` or `/*`
inside a string literal is not a comment. The regular-expression version it replaced does
not:

    re.sub(r"/\\*.*?\\*/", " ", text, flags=re.S)    # "/*" in a literal opens a comment
    re.sub(r"//[^\\n]*", " ", text)                  # "http://x" loses the rest of the line

Seven guards had their own copy, four of them the regex one. The failure is silent and cuts
both ways: a literal like "/*" can swallow real code (a contract asks for a function that is
there, and the guard calls it missing), or swallow the forbidden call it exists to find (the
guard passes over a real violation).

A duplicated stripper is how this came back the first time, so this checks for the
duplication itself rather than re-checking each guard's behaviour.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SUPPORT = Path(__file__).resolve().parents[1] / "support"

# Patterns that mean "this file strips comments with a regular expression".
REGEX_STRIPPERS = (
    re.compile(r're\.sub\(\s*r?"/\\\*.*?\*/"'),
    re.compile(r're\.sub\(\s*r?"//\[\^'),
    re.compile(r're\.sub\(\s*r"//[^"]*"'),
)
LOCAL_DEFINITION = re.compile(r"^def (code_only|strip_comments)\([^)]*\)", re.M)


def guard_files(root: Path = ROOT) -> list[Path]:
    files = [path for path in (root / "tests").rglob("*.py")]
    files += [path for path in (root / "tools").rglob("*.py")
              if ".venv" not in path.parts and "site-packages" not in path.parts]
    return sorted(files)


def findings(root: Path = ROOT, support: Path = SUPPORT) -> list[str]:
    problems: list[str] = []
    for path in guard_files(root):
        if support in path.parents:
            continue  # the one implementation lives here
        text = path.read_text(encoding="utf-8", errors="replace")
        relative = path.relative_to(root).as_posix()
        for pattern in REGEX_STRIPPERS:
            if pattern.search(text):
                problems.append(
                    f"{relative}: strips comments with a regular expression; use "
                    f"source_scan.code_only, which knows a comment marker inside a string "
                    f"literal is not a comment")
                break
        if LOCAL_DEFINITION.search(text) and "source_scan" not in text:
            problems.append(
                f"{relative}: defines its own comment stripper instead of importing the "
                f"shared one")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("single_comment_stripper failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print("single_comment_stripper ok (one implementation, in support/source_scan.py)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
