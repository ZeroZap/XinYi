#!/usr/bin/env python3
"""The unit of `timeout` - one word the whole wait family depended on and nobody wrote down.

Seven wait entries in xy_os.h take a `timeout`. Every one of them documented it as "Timeout value or 0
(no wait) or XY_OS_WAIT_FOREVER", which says what the special values are and nothing about the scale.
Four backends then read that same argument three ways:

    bare-metal      deadline = xy_tick_get() + timeout      -> ticks
    FreeRTOS        vTaskDelay/timeouts take it unchanged   -> ticks
    RT-Thread       rt_mutex_take(mutex, timeout)           -> ticks
    CMSIS-RTOS2     osMutexAcquire(id, timeout)             -> MILLISECONDS

At the common 1 kHz tick the two are identical, which is how the ambiguity lived this long. At 100 Hz a
caller passing 1000 for "one second" waits ten.

Two rules, both provable from the tree:

1. Every documented `timeout` parameter has to name its unit. The header now says ticks, with the
   reasoning, and states the conversion callers are expected to use.
2. No backend may hand the public `timeout` straight to a CMSIS-RTOS2 `os*` call. Those take
   milliseconds, so a raw pass silently redefines the argument. The known sites are pinned with a
   reason - the CMSIS backend is compiled by no target, so converting it would be an edit nothing here
   can verify - and the pin fails in both directions.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
PUBLIC_HEADER = ROOT / "components" / "kernel" / "osal" / "xy_os.h"
BACKENDS = ROOT / "components" / "kernel" / "osal" / "backend"

#: Timeout parameters whose documentation does not name a unit.
UNIT_WORDS = ("tick", "Tick", "TICK")

#: Raw passes of the public timeout into a CMSIS-RTOS2 call, which takes milliseconds. Keyed by
#: backend directory; the value is the reason the site is still there.
PINNED_RAW_MS = {
    "cmsis_rtx": "compiled by no target in this build (see check_osal_backend_advertise.py), so a "
                 "conversion written here would be an edit nothing can compile or run - the decision "
                 "to wire this backend in comes first",
    "rtx5": "orphaned as a whole: no OSAL_BACKEND branch names this directory, so it compiles nowhere "
            "(see check_osal_backend_advertise.py). Same CMSIS milliseconds, same reason not to edit "
            "it blind",
}

WAIT_PARAM = re.compile(r"@param\[in\]\s+timeout\s+(.*)")
TIMEOUT_DECL = re.compile(r"^\s*[A-Za-z_][\w \t\*]*\bxy_os_\w+\([^;]*\buint32_t\s+timeout\b", re.M)
CMSIS_RAW = re.compile(r"\bos[A-Z][A-Za-z]*\s*\([^;]*\btimeout\b[^;]*\)")
CONVERTED = re.compile(r"(ticks?_to_ms|ms_to_ticks)\s*\(")


def undocumented_timeouts(header_text: str) -> list[str]:
    """Every `timeout` parameter line must say what it is measured in."""
    problems: list[str] = []
    matches = WAIT_PARAM.findall(header_text)
    if not matches:
        return ["no `@param[in] timeout` line was found - a parser that finds nothing is not the same "
                "as every parameter being documented"]
    for text in matches:
        if not any(word in text for word in UNIT_WORDS):
            problems.append(f"a timeout parameter is documented as {text.strip()!r} without naming its "
                            f"unit - ticks and milliseconds differ at any tick rate but 1 kHz")
    return problems


def wait_entries_unmentioned(header_text: str) -> list[str]:
    """An entry that takes a timeout but has no `@param` for it cannot have a documented unit."""
    problems: list[str] = []
    declarations = TIMEOUT_DECL.findall(header_text)
    documented = len(WAIT_PARAM.findall(header_text))
    if len(declarations) != documented:
        problems.append(
            f"{len(declarations)} entries take a timeout but {documented} documentation lines describe "
            f"one - the count is a fact, and a gap in it is an undocumented parameter")
    return problems


def raw_ms_passes(source: str) -> list[str]:
    """Direct passes of the public timeout into a CMSIS-RTOS2 call, which reads milliseconds."""
    hits: list[str] = []
    for line in source.splitlines():
        match = CMSIS_RAW.search(line)
        if match and not CONVERTED.search(match.group(0)):
            hits.append(line.strip())
    return hits


def backend_findings(backend_dir: Path, pinned: dict[str, str] | None = None) -> list[str]:
    pinned = PINNED_RAW_MS if pinned is None else pinned
    problems: list[str] = []
    inspected = 0
    for path in sorted(backend_dir.glob("*/xy_os_*.c")):
        source = read_code(path)
        if "timeout" not in source:
            continue
        inspected += 1
        hits = raw_ms_passes(source)
        name = path.parent.name
        if hits and name not in pinned:
            problems.append(
                f"{name}: {len(hits)} wait call(s) pass the public timeout straight to a CMSIS-RTOS2 "
                f"function, which reads milliseconds - the same argument means ticks everywhere else "
                f"(first: {hits[0]})")
        if not hits and name in pinned:
            problems.append(
                f"{name}: pinned as passing milliseconds through raw, but no such call is left - the "
                f"pin is stale ({pinned[name]})")
    if not inspected:
        problems.append("no backend mentioning a timeout was found - the walk is broken")
    return problems


def header_findings(header_path: Path = PUBLIC_HEADER) -> list[str]:
    """Reads the header's documentation, so it reads the raw text on purpose.

    source_scan strips comments before scanning, which is right for every behavioural guard and
    exactly wrong for this one: the thing under test is a sentence in a comment.
    """
    text = read_raw(header_path)
    return undocumented_timeouts(text) + wait_entries_unmentioned(text)


def main() -> int:
    header_text = read_raw(PUBLIC_HEADER)
    problems = undocumented_timeouts(header_text) + wait_entries_unmentioned(header_text)
    problems += backend_findings(BACKENDS)

    documented = len(WAIT_PARAM.findall(header_text))
    declarations = len(TIMEOUT_DECL.findall(header_text))
    if problems:
        print("osal_timeout_units failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1

    print(f"osal_timeout_units ok ({declarations} wait entries, {documented} documented in ticks; "
          f"pinned raw-ms backends={sorted(PINNED_RAW_MS)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
