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

#: A query answered with a constant looks exactly like a real answer: NULL is "no owner" or "not
#: found", 0 is "no flags" or "zero bytes". Only a stub can answer a query with nothing else.
BARE_NULL = re.compile(r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)*return\s+NULL\s*;\s*\}$")
BARE_CONST_ZERO = re.compile(r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)*return\s+0U?\s*;\s*\}$")

#: A function returning a status may refuse an argument without reading it - "this backend cannot do
#: this, whatever you passed" is a real answer. A function returning a value has no such excuse: it was
#: asked a question about its argument and did not look. Three thread queries answered about the main
#: thread whatever id they were given.
VALUE_RETURNING = re.compile(r"^([A-Za-z_][\w \t\*]*?)\b(\w+)\s*\(([^;{]*)\)\s*$", re.M)

#: version->api is the version of this adapter's contract, so it has to come from the adapter's own
#: macros (or from a vendor query when the backend delegates). A literal drifts the moment the contract
#: moves, and one backend reported 2.0 there - its own kernel's version in the wrong field.
API_VERSION_LINE = re.compile(r"version\s*->\s*api\s*=([^;]*);")

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

#: A query answered with a constant looks exactly like a real answer: NULL is "no owner" or "not
#: found", 0 is "no flags" or "zero bytes". Only a stub can answer a query with nothing else.
BARE_NULL = re.compile(r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)*return\s+NULL\s*;\s*\}$")
BARE_CONST_ZERO = re.compile(r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)*return\s+0U?\s*;\s*\}$")

#: A function returning a status may refuse an argument without reading it - "this backend cannot do
#: this, whatever you passed" is a real answer. A function returning a value has no such excuse: it was
#: asked a question about its argument and did not look. Three thread queries answered about the main
#: thread whatever id they were given.
VALUE_RETURNING = re.compile(r"^([A-Za-z_][\w \t\*]*?)\b(\w+)\s*\(([^;{]*)\)\s*$", re.M)

#: version->api is the version of this adapter's contract, so it has to come from the adapter's own
#: macros (or from a vendor query when the backend delegates). A literal drifts the moment the contract
#: moves, and one backend reported 2.0 there - its own kernel's version in the wrong field.
API_VERSION_LINE = re.compile(r"version\s*->\s*api\s*=([^;]*);")

VENDOR_CALL = re.compile(r"\b(?:x[A-Z]\w+|os[A-Z]\w+|rt_\w+|vTask\w+|HAL_\w+)")
LOCAL_IF = re.compile(r"\bif\s*\(([^)]*)\)")
BARE_RETURN = re.compile(r"return\s+XY_OS_ERROR\s*;")

#: How far past an if() to look for the rejection. Enough for `{ unlock(); return X; }`, short enough
#: not to attribute a later, unrelated rejection to this check.
WINDOW = 200

#: Sites that reject a local check with the unspecified error and are still right to. Each needs a
#: reason that would survive review, not a shrug.
#: Queries that must answer a constant because that is the honest answer, with the reason.
PINNED_CONSTANT_QUERIES = {
    ("baremetal", "xy_os_thread_new"):
        "there is one context and no scheduler, so failing to create a thread is the correct answer and "
        "NULL is how this API reports it - a refusal, not a placeholder",
    ("baremetal", "xy_os_thread_get_stack_size"):
        "this backend has no per-thread stack bookkeeping, and the OSAL has no portable way to ask the "
        "main stack's size; the header now says 0 means unknown",
    ("baremetal", "xy_os_thread_get_stack_space"):
        "same: nothing to measure without per-thread stacks; the header says 0 means unknown",
    ("rtthread", "xy_os_thread_get_stack_space"): "backend compiled by no target in this build",
    ("freertos", "xy_os_thread_get_count"):
        "deliberate: FreeRTOS offers kernel-global task inventory, which would include tasks this "
        "adapter does not own; counting them would report threads the caller never created",
    ("freertos", "xy_os_thread_enumerate"):
        "deliberate: enumerating kernel-global handles would publish tasks this adapter never made, and "
        "NULL placeholders for the ones it cannot resolve",
    ("cmsis_rtx", "xy_os_thread_get_count"): "backend compiled by no target in this build",
}

#: Value-returning functions that legitimately ignore an argument.
PINNED_UNREAD_PARAMETERS = {
    ("baremetal", "xy_os_thread_new"):
        "returns NULL because there is no scheduler to create a thread on; the work function and its "
        "argument are not merely ignored, they can never run",
    ("baremetal", "xy_os_thread_get_stack_size"):
        "no per-thread stacks exist here, so the id cannot change the answer - 0 means unknown and the "
        "header says so",
    ("baremetal", "xy_os_thread_get_stack_space"):
        "same: nothing to measure per thread, so 0 = unknown for any id",
}

#: Backends whose api version cannot be corrected by compiling them here.
PINNED_API_VERSIONS = {
    ("rtthread", "version->api"):
        "unreachable from every build in this tree, so an edit would be unverifiable; it reports 0x20000 "
        "(RT-Thread 2.0) in the field meant for the adapter's API version while already putting RT_VERSION "
        "in version->kernel - the fix is the same XY_OSAL_VERSION_ expression the others use",
}

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


#: A flag-word entry that always answers the sentinel refuses everything, including the calls a working
#: backend would answer. That is the honest answer where there is no state to answer from - but it is also
#: indistinguishable from an entry nobody got round to implementing, so it has to be named here.
BARE_SENTINEL = re.compile(
    r"^\{\s*(?:\(void\)\s*\w+\s*;\s*)*return\s+(?:\(uint32_t\)\s*)?XY_OS_(?:THREAD|EVENT)_FLAGS_ERROR\s*;\s*\}$")

PINNED_FLAG_REFUSALS = {
    ("baremetal", "xy_os_thread_flags_set"):
        "no scheduler, so there is no per-thread notification word to set",
    ("baremetal", "xy_os_thread_flags_clear"):
        "no scheduler, so there is no per-thread notification word to clear",
    ("baremetal", "xy_os_thread_flags_get"):
        "no scheduler, so there is no per-thread notification word to read",
    ("baremetal", "xy_os_thread_flags_wait"):
        "no scheduler, so there is nothing to wait on and no word to wait for",
}


def flag_word_refusal_findings(source: str, name: str,
                               pins: dict[tuple[str, str], str] | None = None) -> list[str]:
    """An entry that refuses everything must be named as unimplemented, not pass as a refusal."""
    pins = PINNED_FLAG_REFUSALS if pins is None else pins
    problems: list[str] = []
    for match in DEFINITION.finditer(source):
        entry = match.group(1)
        if not FLAG_WORD_ENTRY.match(entry):
            continue
        body = braced_body(source[match.end():])
        if not body or not BARE_SENTINEL.match(re.sub(r"\s+", " ", body).strip()):
            continue
        if (name, entry) in pins:
            continue
        problems.append(
            f"{name}:{entry}: answers the sentinel whatever the input is - a refusal this broad is an "
            f"unimplemented entry and the caller cannot tell it from a rejected flag word; name it in "
            f"PINNED_FLAG_REFUSALS with the reason, or implement it")
    return problems


def api_version_findings(source: str, name: str,
                         sites: dict[tuple[str, str], str] | None = None) -> list[str]:
    """The reported API version must be the adapter's, not a number typed into the backend."""
    sites = PINNED_API_VERSIONS if sites is None else sites
    problems: list[str] = []
    for line in API_VERSION_LINE.finditer(source):
        expression = line.group(1)
        if "XY_OSAL_VERSION_" in expression or re.search(r"\b\w*[Gg]etVersion\s*\(", expression):
            continue
        if (name, "version->api") in sites:
            continue
        problems.append(
            f"{name}:version->api: assigned {expression.strip()!r}, which is not this adapter's version - "
            f"use XY_OSAL_VERSION_*, or take a pin with the reason")
    return problems


def unread_parameter_findings(source: str, name: str,
                              sites: dict[tuple[str, str], str] | None = None) -> list[str]:
    """A value-returning function that never reads one of its parameters answers about something else."""
    sites = PINNED_UNREAD_PARAMETERS if sites is None else sites
    problems: list[str] = []
    for match in VALUE_RETURNING.finditer(source):
        return_type, function, params = match.group(1).strip(), match.group(2), match.group(3)
        if not function.startswith("xy_os_") or "xy_os_status_t" in return_type:
            continue
        body = braced_body(source[match.end():])
        if not body:
            continue
        names = [part.strip().split()[-1].lstrip("*")
                 for part in params.split(",") if part.strip() and part.strip() != "void"]
        unread = [n for n in names if n and not re.search(r"\b%s\b" % re.escape(n), body)]
        if not unread or (name, function) in sites:
            continue
        problems.append(
            f"{name}:{function}: returns a value without reading {', '.join(unread)} - the answer cannot "
            f"depend on what the caller asked about; answer about the argument or take a pin")
    return problems


def constant_query_findings(source: str, name: str,
                            sites: dict[tuple[str, str], str] | None = None) -> list[str]:
    """A query whose whole body is a constant answer cannot be told from a working one."""
    sites = PINNED_CONSTANT_QUERIES if sites is None else sites
    problems: list[str] = []
    for match in DEFINITION.finditer(source):
        function = match.group(1)
        if not function.startswith("xy_os_"):
            continue
        body = braced_body(source[match.end():])
        if not body:
            continue
        flat = re.sub(r"\s+", " ", body).strip()
        if not (BARE_NULL.match(flat) or BARE_CONST_ZERO.match(flat)):
            continue
        if (name, function) in sites:
            continue
        problems.append(
            f"{name}:{function}: answers a constant and nothing else, which a caller cannot tell from a "
            f"real answer; either report the state or take a pin with the reason")
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
        problems.extend(flag_word_refusal_findings(source, path.parent.name))
        problems.extend(constant_query_findings(source, path.parent.name))
        problems.extend(unread_parameter_findings(source, path.parent.name))
        problems.extend(api_version_findings(source, path.parent.name))
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
          f"none a bare unspecified return, no unpinned local rejection, no false flag success, no constant-only query, no value query ignoring its argument, no invented api version)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
