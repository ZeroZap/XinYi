#!/usr/bin/env python3
"""The three error-code headers have to agree, because the code compares across them.

xy_hal_error.h, xy_clib/xy_error.h and xy_device_error.h each define the same semantic
error names and each says in its own comment that the values are aligned and callers can
compare across them. They were not aligned: thirteen of the fifteen shared names had
different numbers, so XY_ERROR_NOT_SUPPORTED (-3) was numerically the same as
XY_HAL_ERROR_TIMEOUT (-3). A caller checking for "not supported" against a HAL status was
checking for a timeout.

This guard is mechanical on purpose. "The comments claim alignment" is not alignment, and
the claim is exactly what makes the mismatch dangerous: it invites the comparison that is
silently wrong.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HEADERS = {
    "hal": (ROOT / "components" / "hal" / "inc" / "xy_hal_error.h", "XY_HAL_"),
    "clib": (ROOT / "components" / "clib" / "xy_clib" / "xy_error.h", "XY_"),
    "device": (ROOT / "components" / "device" / "inc" / "xy_device_error.h", "XY_"),
}


def parse(path: Path, prefix: str) -> dict[str, int]:
    text = path.read_text(encoding="utf-8")
    codes: dict[str, int] = {}
    for name, value in re.findall(rf"\b({prefix}[A-Z0-9_]+)\s*=\s*(-?\d+)", text):
        codes[name[len(prefix):]] = int(value)
    for name, value in re.findall(rf"#define\s+({prefix}[A-Z0-9_]+)\s+\(?(-?\d+)\)?", text):
        codes.setdefault(name[len(prefix):], int(value))
    return codes


def main() -> int:
    families = {name: parse(path, prefix) for name, (path, prefix) in HEADERS.items()}
    failures = []

    for name, codes in families.items():
        if not codes:
            failures.append(f"{name}: no error codes parsed - the parser is looking at the "
                            f"wrong shape, which is not the same as the header being fine")
            continue
        if codes.get("OK") != 0:
            failures.append(f"{name}: XY_OK is {codes.get('OK')}, not 0")
        for code_name, value in sorted(codes.items()):
            if code_name == "OK":
                continue
            if value >= 0:
                failures.append(f"{name}: {code_name} is {value}; error codes must be "
                                f"negative so a data value can never be mistaken for one")
        seen: dict[int, str] = {}
        for code_name, value in sorted(codes.items()):
            if value in seen and code_name != "OK":
                failures.append(f"{name}: {code_name} and {seen[value]} are both {value}")
            seen.setdefault(value, code_name)

    # Cross-family: every shared name must carry the same number.
    names = set(families["hal"]) & (set(families["clib"]) | set(families["device"]))
    for code_name in sorted(names):
        values = {family: codes[code_name] for family, codes in families.items()
                  if code_name in codes}
        if len(set(values.values())) > 1:
            detail = ", ".join(f"{family}={value}" for family, value in sorted(values.items()))
            failures.append(f"{code_name} disagrees across headers: {detail}")

    if failures:
        print("error_code_alignment failed:")
        for failure in failures:
            print(f"- {failure}")
        return 1
    shared = len(names)
    print(f"error_code_alignment ok families={len(families)} shared_names={shared}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
