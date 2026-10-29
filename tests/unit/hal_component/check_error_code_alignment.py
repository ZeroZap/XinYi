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


# The device layer renames a few codes as it re-exports them. The table is short and
# explicit because the naming difference is the interesting part: a reader has to know that
# XY_DEVICE_NOT_SUPPORT and XY_HAL_ERROR_NOT_SUPPORTED mean the same thing.
DEVICE_ALIASES = {
    "OK": "OK",
    "ERROR": "ERROR",
    "INVALID_PARAM": "ERROR_INVALID_PARAM",
    "NOT_SUPPORT": "ERROR_NOT_SUPPORTED",
    "TIMEOUT": "ERROR_TIMEOUT",
    "BUSY": "ERROR_BUSY",
    "NO_MEM": "ERROR_NO_MEMORY",
    "IO_ERROR": "ERROR_IO",
    "NOT_INIT": "ERROR_NOT_INIT",
    "ALREADY_INIT": "ERROR_ALREADY_INIT",
    "NO_RESOURCE": "ERROR_NO_RESOURCE",
    "FAIL": "ERROR_FAIL",
    "NOT_FOUND": "ERROR_NOT_FOUND",
    "EINVAL": "ERROR_INVALID_PARAM",
    "ENOMEM": "ERROR_NO_MEMORY",
    "ENODEV": "ERROR_NOT_FOUND",
    "EAGAIN": "ERROR_NOT_READY",  # the HAL has no equivalent, clib does
}


def parse_device_aliases() -> dict[str, str]:
    """Read `#define XY_DEVICE_X <symbol>` pairs; these alias other families, not numbers."""
    text = HEADERS["device"][0].read_text(encoding="utf-8")
    aliases: dict[str, str] = {}
    for name, target in re.findall(r"#define\s+XY_DEVICE_([A-Z0-9_]+)\s+(XY_[A-Z0-9_]+)\s*$",
                                   text, flags=re.M):
        aliases[name] = target
    return aliases


def resolve(symbol: str, families: dict[str, dict[str, int]],
            aliases: dict[str, str] | None = None, depth: int = 0) -> int | None:
    """Resolve a symbol to a number, following aliases of aliases (EINVAL → INVALID_PARAM)."""
    if symbol.startswith("XY_HAL_"):
        return families["hal"].get(symbol[len("XY_HAL_"):])
    if symbol.startswith("XY_DEVICE_"):
        name = symbol[len("XY_DEVICE_"):]
        if aliases and name in aliases and depth < 4:
            return resolve(aliases[name], families, aliases, depth + 1)
        return None
    if symbol.startswith("XY_"):
        return families["clib"].get(symbol[len("XY_"):])
    return None


def check_device_aliases(families: dict[str, dict[str, int]]) -> list[str]:
    """Every XY_DEVICE_* alias must resolve, and to the number the HAL uses.

    The device layer defines these as aliases rather than numbers, which is how it stayed
    out of the mismatch the other two headers had - but only while the alias points at the
    right symbol. XY_DEVICE_NO_RESOURCE → XY_ERROR_NO_RESOURCE was a different number from
    XY_HAL_ERROR_NO_RESOURCE until the families were aligned, so a driver checking it
    against a HAL status was checking something else.
    """
    failures = []
    aliases = parse_device_aliases()
    if not aliases:
        return ["device: no XY_DEVICE_* aliases parsed - the parser is looking at the "
                "wrong shape, which is not the same as the aliases being fine"]

    defined = set(aliases)
    expected = {"OK", "ERROR", "INVALID_PARAM", "NOT_SUPPORT", "TIMEOUT", "BUSY", "NO_MEM",
                "IO_ERROR", "NOT_INIT", "ALREADY_INIT", "NO_RESOURCE", "FAIL", "NOT_FOUND",
                "EINVAL", "ENOMEM", "ENODEV", "EAGAIN"}
    for name in sorted(expected - defined):
        failures.append(f"device: XY_DEVICE_{name} is gone; device code referring to it "
                        f"will not compile and the removal was not deliberate here")

    for name, symbol in sorted(aliases.items()):
        value = resolve(symbol, families, aliases)
        if value is None:
            failures.append(f"device: XY_DEVICE_{name} → {symbol}, which no family defines")
            continue
        target = DEVICE_ALIASES.get(name)
        if target is None:
            continue
        hal_value = families["hal"].get(target)
        if hal_value is not None and value != hal_value:
            failures.append(f"device: XY_DEVICE_{name} → {symbol} = {value} but "
                            f"XY_HAL_{target} = {hal_value}")
    return failures


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

    failures.extend(check_device_aliases(families))

    if failures:
        print("error_code_alignment failed:")
        for failure in failures:
            print(f"- {failure}")
        return 1
    shared = len(names)
    print(f"error_code_alignment ok families={len(families)} shared_names={shared} "
          f"device_aliases={len(DEVICE_ALIASES)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
