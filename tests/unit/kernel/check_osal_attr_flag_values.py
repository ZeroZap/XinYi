#!/usr/bin/env python3
"""The OSAL attribute flags are CMSIS-RTOS2 bit values, and one backend depends on that being true.

`backend/cmsis_rtx/xy_os_cmsis_rtx.c` forwards them verbatim - `mutex_attr.attr_bits = attr->attr_bits`
- so if 0x02 ever meant something else on either side, a caller asking for priority inheritance would
silently get a different mutex. Nothing checked the correspondence until now; it was a coincidence that
looked like a design.

The values are not read from the tree. The vendor `MCU/` directories that carry `cmsis_os2.h` are
untracked - zero of them are in git - so a clean checkout has none of them, and the first version of this
guard failed there with "the walk is broken" while passing in a developer tree. The CMSIS-RTOS2 numbers
are a published standard, so they are stated here, and any vendor copy that happens to be on disk is
cross-checked against them: a copy that disagrees is worth knowing about, but its absence is not an error.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]

#: CMSIS-RTOS2 attribute values (from the standard, cross-checked against the copies under MCU/ during
#: the audit). These are the authority; the tree is not.
CMSIS_VALUES = {
    "osThreadDetached": 0x00000000,
    "osThreadJoinable": 0x00000001,
    "osMutexRecursive": 0x00000001,
    "osMutexPrioInherit": 0x00000002,
    "osMutexRobust": 0x00000008,
}

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


def findings(root: Path = ROOT, pairs: dict[str, str] | None = None,
             cmsis: dict[str, int] | None = None) -> list[str]:
    pairs = PAIRS if pairs is None else pairs
    cmsis = CMSIS_VALUES if cmsis is None else cmsis
    problems: list[str] = []

    # Bonus: if vendor copies are present, they must agree with the standard values used above. They are
    # untracked, so their absence says nothing.
    for path in headers(root):
        values = defines(path)
        for name, value in values.items():
            if name in cmsis and cmsis[name] != value:
                problems.append(
                    f"{path.relative_to(root).as_posix()}: defines {name} as {hex(value)} but CMSIS-RTOS2 "
                    f"says {hex(cmsis[name])}")
    if problems:
        return problems

    osal = osal_flags(root)
    checked = 0
    for flag, name in sorted(pairs.items()):
        if flag not in osal:
            problems.append(f"{flag} is no longer defined in xy_os.h - the pin here is stale")
            continue
        if name not in cmsis:
            problems.append(f"{name} has no value stated in this guard - the pin is stale")
            continue
        checked += 1
        if osal[flag] != cmsis[name]:
            problems.append(
                f"{flag} is {hex(osal[flag])} but {name} is {hex(cmsis[name])}: the cmsis_rtx backend "
                f"forwards this value verbatim, so the two meanings have come apart")
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
    vendor = len(headers())
    print(f"osal_attr_flag_values ok ({len(PAIRS)} attribute flags match the CMSIS-RTOS2 values that "
          f"cmsis_rtx forwards them to; vendor copies cross-checked: {vendor})")
    for flag, cmsis in sorted(PAIRS.items()):
        print(f"  {flag} = {hex(osal[flag])} = {cmsis}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
