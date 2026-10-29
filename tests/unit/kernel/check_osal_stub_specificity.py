#!/usr/bin/env python3
"""A stub must say *why* it cannot do the thing, and "unspecified RTOS error" is not why.

The bare-metal backend's six thread operations - set priority, suspend, resume, detach, join,
terminate - were each a function body containing one line: `return XY_OS_ERROR;`. XY_OS_ERROR is
documented as "Unspecified RTOS error", so a caller got the same answer for "this backend has no
thread table" as for "the driver just failed", and could act on neither. The existing host test even
asserted that value, which turned the defect into the contract.

They return XY_OS_ERROR_NOT_SUPPORTED now, and this rule is what stops the next stub from settling for
the unspecified code. It judges only the exact shape - a body that is nothing but a return of
XY_OS_ERROR - because anything longer is a decision with input behind it, and second-guessing those
would produce arguments rather than findings.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import braced_body, read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKENDS = ROOT / "components" / "kernel" / "osal" / "backend"

DEFINITION = re.compile(r"^[A-Za-z_][\w \t\*]*\b(xy_os_\w+)\s*\([^;{]*\)\s*$", re.M)
BARE_UNSPECIFIED = re.compile(r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)?return\s+XY_OS_ERROR\s*;\s*\}$")


def stub_findings(source: str, name: str) -> list[str]:
    """Functions whose entire body is `return XY_OS_ERROR;`."""
    problems: list[str] = []
    for match in DEFINITION.finditer(source):
        body = braced_body(source[match.end():])
        if not body:
            continue
        if BARE_UNSPECIFIED.match(re.sub(r"\s+", " ", body).strip()):
            problems.append(
                f"{name}:{match.group(1)}: returns XY_OS_ERROR - 'unspecified RTOS error' - as its "
                f"whole body; say what actually prevents it (NOT_SUPPORTED, PARAMETER, a state check)")
    return problems


def findings(backend_dir: Path = BACKENDS) -> list[str]:
    problems: list[str] = []
    inspected = 0
    for path in sorted(backend_dir.glob("*/xy_os_*.c")):
        source = read_code(path)
        inspected += len(DEFINITION.findall(source))
        problems.extend(stub_findings(source, path.parent.name))
    if not inspected:
        problems.append("no function definitions were found - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    inspected = sum(len(DEFINITION.findall(read_code(path)))
                    for path in sorted(BACKENDS.glob("*/xy_os_*.c")))
    if problems:
        print("osal_stub_specificity failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"osal_stub_specificity ok ({inspected} definitions across "
          f"{len(list(BACKENDS.glob('*/xy_os_*.c')))} backends, none a bare unspecified return)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
