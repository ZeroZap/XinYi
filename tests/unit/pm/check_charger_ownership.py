#!/usr/bin/env python3
"""Guard canonical Charger ownership after power-driver migration."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
DRIVER = ROOT / "components" / "drivers" / "power" / "charger"
LEGACY = ROOT / "components" / "charger"
README = DRIVER / "README.md"
SOURCE = DRIVER / "xy_bq25620.c"
HEADER = DRIVER / "xy_bq25620.h"
FRAMEWORK = DRIVER / "xy_charger_device.h"
RETIRED_COLLIDING_HEADER = DRIVER / "xy_charger.h"
CMAKE = DRIVER / "CMakeLists.txt"
UNIT_CMAKE = ROOT / "tests" / "unit" / "CMakeLists.txt"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    errors: list[str] = []
    for path in (README, SOURCE, HEADER, FRAMEWORK, CMAKE, UNIT_CMAKE):
        require(path.is_file(), f"required ownership input is missing: {path.relative_to(ROOT)}", errors)
    require(not LEGACY.exists(), "legacy components/charger must remain removed", errors)
    require(not RETIRED_COLLIDING_HEADER.exists(),
            "power-driver xy_charger.h must remain retired to avoid PM public-header collision",
            errors)
    if errors:
        print("charger_ownership failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    readme = README.read_text(encoding="utf-8")
    unit_cmake = UNIT_CMAKE.read_text(encoding="utf-8")
    framework = FRAMEWORK.read_text(encoding="utf-8")
    require("${DRV}/power/charger/xy_bq25620.c" in unit_cmake,
            "charger_bq25620 CTest must exercise the canonical power-driver owner", errors)
    require("charger_ownership" in unit_cmake,
            "charger_ownership policy CTest must remain registered", errors)
    require("xy_charger_device_t" in framework and "xy_charger_init(" not in framework,
            "Device charger contract must stay namespaced away from PM charger symbols", errors)
    require("int xy_charger_device_" not in framework,
            "Device charger header must not advertise generic APIs without an implementation owner",
            errors)
    require("*config;" not in framework and "xy_charger_device_status_t status;" not in framework,
            "Device charger contract must not retain ownerless config/status cache fields", errors)
    require("xy_bq25620_write_reg" not in HEADER.read_text(encoding="utf-8") and
            "hw_write_reg" not in framework,
            "raw register writes must not bypass the validated Charger control API", errors)
    require("xy_bq25620_configure" in HEADER.read_text(encoding="utf-8"),
            "BQ25620 full configuration must have a chip-owner public API", errors)
    require("charger_header_coexistence" in unit_cmake,
            "PM/Device charger public-header coexistence gate must remain registered", errors)
    for token in ("canonical implementation owner", "Host-guarded", "hardware-pending",
                  "components/drivers/power/charger/xy_bq25620.c"):
        require(token in readme, f"README must preserve ownership/evidence token: {token}", errors)

    if errors:
        print("charger_ownership failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("charger_ownership_ok owner=components/drivers/power/charger "
          "evidence=Host-guarded hardware=hardware-pending legacy=removed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
