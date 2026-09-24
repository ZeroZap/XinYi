#!/usr/bin/env python3
"""Guard PM charger hardware-hook ownership."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
PM_CHARGER = ROOT / "components" / "pm" / "src" / "xy_charger.c"
PM_PLATFORM = ROOT / "components" / "pm" / "src" / "xy_pm_platform.c"
UNIT_CMAKE = ROOT / "tests" / "unit" / "CMakeLists.txt"
PANDORA_CMAKE = ROOT / "boards" / "pandora_stm32l475" / "CMakeLists.txt"


def definitions(text: str, symbol: str) -> int:
    return len(re.findall(rf"^\s*int\s+{symbol}\s*\(", text, re.MULTILINE))


def main() -> int:
    errors = []
    charger = PM_CHARGER.read_text(encoding="utf-8")
    platform = PM_PLATFORM.read_text(encoding="utf-8")
    unit_cmake = UNIT_CMAKE.read_text(encoding="utf-8")
    pandora_cmake = PANDORA_CMAKE.read_text(encoding="utf-8")

    for symbol in ("xy_charger_hw_enable", "xy_charger_hw_disable"):
        if definitions(charger, symbol) != 0:
            errors.append(f"{symbol} must not be implemented by PM policy")
        if definitions(platform, symbol) != 1:
            errors.append(f"{symbol} must have exactly one platform owner")

    if definitions(platform, "xy_charger_hw_init") != 1:
        errors.append("xy_charger_hw_init must have exactly one platform owner")
    if "XY_PM_PLATFORM_OWNS_CHARGER_HW" in unit_cmake or \
       "XY_PM_PLATFORM_OWNS_CHARGER_HW" in pandora_cmake:
        errors.append("obsolete charger ownership bypass macro must not return")
    if "XY_PM_ERROR_NOT_SUPPORTED" not in platform:
        errors.append("platform charger hooks must preserve fail-closed unsupported behavior")

    if errors:
        print("pm_charger_ownership failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("pm_charger_ownership_ok policy=consumer platform=single-owner default=fail-closed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
