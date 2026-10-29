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
#: An if() that decides from values the backend already holds, rather than from a vendor call's
#: result. When such a check rejects the call, the reason is known and the code can say it.
#: Entries that return a flag word rather than a status code. Their failure has to be reported inside
#: that word, and 0 is a valid answer - "no flags are set" - so it cannot double as the failure.
FLAG_WORD_ENTRY = re.compile(r"^xy_os_thread_flags_")
BARE_ZERO = re.compile(r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)*return\s+0U?\s*;\s*\}$")

VENDOR_CALL = re.compile(r"\b(?:x[A-Z]\w+|os[A-Z]\w+|rt_\w+|vTask\w+|HAL_\w+)")
LOCAL_IF = re.compile(r"\bif\s*\(([^)]*)\)")
BARE_RETURN = re.compile(r"return\s+XY_OS_ERROR\s*;")

#: How far past an if() to look for the rejection. Enough for `{ unlock(); return X; }`, short enough
#: not to attribute a later, unrelated rejection to this check.
WINDOW = 200

#: Backends whose local rejections still answer with the unspecified error. Pinned per backend rather
#: than per function: neither of these compiles in any target, so the edits cannot be verified here,
#: and enumerating the sites would rot the moment one more is added elsewhere in the file.
PINNED_UNSPECIFIED = {
    "rtthread": "compiled by no target in this build (see check_osal_backend_advertise.py), so a "
                "change here would be an edit nothing can compile or run",
    "rtx5": "orphaned: no OSAL_BACKEND branch names this directory, as above",
}

DEFINITION = re.compile(r"^[A-Za-z_][\w \t\*]*\b(xy_os_\w+)\s*\([^;{]*\)\s*$", re.M)
#: An if() that decides from values the backend already holds, rather than from a vendor call's
#: result. When such a check rejects the call, the reason is known and the code can say it.
#: Entries that return a flag word rather than a status code. Their failure has to be reported inside
#: that word, and 0 is a valid answer - "no flags are set" - so it cannot double as the failure.
FLAG_WORD_ENTRY = re.compile(r"^xy_os_thread_flags_")
BARE_ZERO = re.compile(r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)*return\s+0U?\s*;\s*\}$")

VENDOR_CALL = re.compile(r"\b(?:x[A-Z]\w+|os[A-Z]\w+|rt_\w+|vTask\w+|HAL_\w+)")
LOCAL_IF = re.compile(r"\bif\s*\(([^)]*)\)")
BARE_RETURN = re.compile(r"return\s+XY_OS_ERROR\s*;")

#: How far past an if() to look for the rejection. Enough for `{ unlock(); return X; }`, short enough
#: not to attribute a later, unrelated rejection to this check.
WINDOW = 200

#: Sites that reject a local check with the unspecified error and are still right to. Each needs a
#: reason that would survive review, not a shrug.
#: Individual sites inside a backend that is compiled, where the unspecified answer is still right.
PINNED_SITES = {
    ("baremetal", "xy_os_mutex_release"):
        "releasing a mutex this context does not hold. FreeRTOS reaches the same answer through "
        "xSemaphoreGive returning pdFAIL, and no mapping separates that from its other failures - "
        "changing only this side would turn one honest uncertainty into two behaviours",
}
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


def local_check_findings(source: str, name: str,
                         pinned: dict[str, str] | None = None,
                         sites: dict[tuple[str, str], str] | None = None) -> list[str]:
    """A rejection decided from local state can name its reason; the unspecified code names nothing."""
    pinned = PINNED_UNSPECIFIED if pinned is None else pinned
    sites = PINNED_SITES if sites is None else sites
    problems: list[str] = []
    rejecting = [match.group(1) for match in DEFINITION.finditer(source)
                 if local_rejection(braced_body(source[match.end():]) or "")]
    rejecting = [function for function in rejecting if (name, function) not in sites]
    if name in pinned:
        if not rejecting:
            problems.append(f"{name}: pinned as answering local rejections with the unspecified error, "
                            f"but none is left - the pin is stale ({pinned[name]})")
        return problems
    for function in rejecting:
        problems.append(
            f"{name}:{function}: rejects a call after a check of its own state and answers with "
            f"XY_OS_ERROR, which says nothing about why")
    return problems


def false_success_findings(source: str, name: str) -> list[str]:
    """A flag-word entry that answers 0 whatever the input is claiming success it cannot deliver."""
    problems: list[str] = []
    for match in DEFINITION.finditer(source):
        if not FLAG_WORD_ENTRY.match(match.group(1)):
            continue
        body = braced_body(source[match.end():])
        if not body:
            continue
        if BARE_ZERO.match(re.sub(r"\s+", " ", body).strip()):
            problems.append(
                f"{name}:{match.group(1)}: answers 0 whatever the input is. Zero is what a working "
                f"backend returns when no flags are set, so callers cannot tell this from success; "
                f"return XY_OS_THREAD_FLAGS_ERROR")
    return problems


def local_rejection(body: str) -> bool:
    """True when a decision the backend makes on its own values is answered with a bare XY_OS_ERROR."""
    for match in LOCAL_IF.finditer(body):
        if VENDOR_CALL.search(match.group(1)):
            continue
        if BARE_RETURN.search(body[match.end():match.end() + WINDOW]):
            return True
    return False


def findings(backend_dir: Path = BACKENDS) -> list[str]:
    problems: list[str] = []
    inspected = 0
    for path in sorted(backend_dir.glob("*/xy_os_*.c")):
        source = read_code(path)
        inspected += len(DEFINITION.findall(source))
        problems.extend(stub_findings(source, path.parent.name))
        problems.extend(local_check_findings(source, path.parent.name))
        problems.extend(false_success_findings(source, path.parent.name))
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
          f"{len(list(BACKENDS.glob('*/xy_os_*.c')))} backends, "
          f"none a bare unspecified return, no unpinned local rejection, no false flag success)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
