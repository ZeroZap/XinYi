#!/usr/bin/env python3
"""Guard the standalone Charger ownership and evidence boundary."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
CHARGER = ROOT / "components" / "charger"
README = CHARGER / "README.md"
DEPRECATED = CHARGER / "DEPRECATED.md"
CHARGER_CMAKE = CHARGER / "CMakeLists.txt"
UNIT_CMAKE = ROOT / "tests" / "unit" / "CMakeLists.txt"
DRIVER_REPLACEMENT = ROOT / "components" / "drivers" / "power" / "charger"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    errors: list[str] = []
    for path in (README, DEPRECATED, CHARGER_CMAKE, UNIT_CMAKE):
        require(path.is_file(), f"required ownership input is missing: {path.relative_to(ROOT)}", errors)
    if errors:
        print("charger_ownership failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    readme = README.read_text(encoding="utf-8")
    deprecated = DEPRECATED.read_text(encoding="utf-8")
    charger_cmake = CHARGER_CMAKE.read_text(encoding="utf-8")
    unit_cmake = UNIT_CMAKE.read_text(encoding="utf-8")

    require(not DRIVER_REPLACEMENT.exists(),
            "unexpected replacement owner exists; update the ownership decision and guard", errors)
    require("src/xy_bq25620.c" in charger_cmake,
            "standalone Charger CMake must keep the BQ25620 implementation owner", errors)
    require("components/charger/src/xy_bq25620.c" in unit_cmake,
            "charger_bq25620 CTest must exercise the standalone implementation owner", errors)
    require("charger_ownership" in unit_cmake,
            "charger_ownership policy CTest must remain registered", errors)

    for token in (
        "canonical implementation owner",
        "legacy-maintained",
        "Host-guarded",
        "hardware-pending",
        "components/charger/src/xy_bq25620.c",
    ):
        require(token in readme, f"README must preserve ownership/evidence token: {token}", errors)

    for token in (
        "historical notice",
        "不存在",
        "components/charger/",
        "不得迁移",
    ):
        require(token in deprecated, f"DEPRECATED notice must preserve correction token: {token}", errors)

    require("drivers/power/charger/xy_bq25620.h" not in deprecated,
            "deprecated notice must not recommend a nonexistent header", errors)
    require("drivers/power/fuel_gauge/xy_fg_max17043.h" not in deprecated,
            "deprecated notice must not recommend a nonexistent fuel-gauge header", errors)

    if errors:
        print("charger_ownership failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("charger_ownership_ok owner=components/charger maturity=legacy-maintained "
          "evidence=Host-guarded hardware=hardware-pending")
    return 0


if __name__ == "__main__":
    sys.exit(main())
