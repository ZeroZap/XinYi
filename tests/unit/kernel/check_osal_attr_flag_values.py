#!/usr/bin/env python3
"""The OSAL attribute flags are CMSIS-RTOS2 bit values, and one backend depends on that being true.

`backend/cmsis_rtx/xy_os_cmsis_rtx.c` forwards them verbatim - `mutex_attr.attr_bits = attr->attr_bits`
- so if 0x02 ever meant something else on either side, a caller asking for priority inheritance would
silently get a different mutex. Nothing checked the correspondence until now; it was a coincidence that
looked like a design.

The check reads every `cmsis_os2.h` in the tree rather than naming one, because the vendor trees each
carry a copy: if they disagree with each other, the pass-through has no single meaning and that is worth
knowing before comparing it to anything.
"""

from __future__ import annotations

import re
import sys
from functools import lru_cache
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]

#: OSAL flag -> the CMSIS constant the cmsis_rtx backend forwards it to.
PAIRS = {
    "XY_OS_THREAD_JOINABLE": "osThreadJoinable",
    "XY_OS_THREAD_DETACHED": "osThreadDetached",
    "XY_OS_MUTEX_RECURSIVE": "osMutexRecursive",
    "XY_OS_MUTEX_PRIO_INHERIT": "osMutexPrioInherit",
    "XY_OS_MUTEX_ROBUST": "osMutexRobust",
}

DEFINE = re.compile(r"#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)")


def headers(root: Path = ROOT) -> list[Path]:
    return sorted(p for p in root.rglob("cmsis_os2.h") if "/docs/" not in p.as_posix())


def defines(path: Path) -> dict[str, int]:
    # read_code, not read_text: a #define that has been commented out does not define anything, and this
    # guard exists to compare live values.
    text = read_code(path)
    # Join continuation lines first: XY_OS_THREAD_DETACHED is written as "#define ... \" plus the value,
    # and a scan that misses it reports a live flag as stale.
    text = text.replace("\\\n", " ")
    return {name: int(value, 0) for name, value in DEFINE.findall(text)}


def osal_flags(root: Path = ROOT) -> dict[str, int]:
    return defines(root / "components" / "kernel" / "osal" / "xy_os.h")


def findings(root: Path = ROOT, pairs: dict[str, str] | None = None) -> list[str]:
    pairs = PAIRS if pairs is None else pairs
    problems: list[str] = []

    header_files = headers(root)
    if not header_files:
        return ["no cmsis_os2.h was found in the tree - the walk is broken"]

    # What does each vendor copy say? They are all supposed to describe the same standard.
    consensus: dict[str, int] = {}
    disagreeing: dict[str, set[int]] = {}
    for path in header_files:
        values = defines(path)
        for cmsis in set(pairs.values()):
            if cmsis in values:
                consensus.setdefault(cmsis, values[cmsis])
                disagreeing.setdefault(cmsis, set()).add(values[cmsis])
    for cmsis, seen in sorted(disagreeing.items()):
        if len(seen) > 1:
            problems.append(
                f"{cmsis}: the cmsis_os2.h copies in this tree disagree ({sorted(hex(v) for v in seen)}), "
                f"so a verbatim pass-through has no single meaning")
    if problems:
        return problems

    osal = osal_flags(root)
    checked = 0
    for flag, cmsis in pairs.items():
        if flag not in osal:
            problems.append(f"{flag} is no longer defined in xy_os.h - the pin here is stale")
            continue
        if cmsis not in consensus:
            problems.append(f"{cmsis} is not defined by any cmsis_os2.h in this tree - the pin is stale")
            continue
        checked += 1
        if osal[flag] != consensus[cmsis]:
            problems.append(
                f"{flag} is {hex(osal[flag])} but {cmsis} is {hex(consensus[cmsis])}: the cmsis_rtx "
                f"backend forwards this value verbatim, so the two meanings have come apart")
    if checked == 0 and not problems:
        problems.append("no flag pair was compared - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("osal_attr_flag_values failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    osal = osal_flags()
    print(f"osal_attr_flag_values ok ({len(PAIRS)} attribute flags match the CMSIS values that "
          f"cmsis_rtx forwards them to; read from {len(headers())} cmsis_os2.h copies)")
    for flag, cmsis in sorted(PAIRS.items()):
        print(f"  {flag} = {hex(osal[flag])} = {cmsis}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
