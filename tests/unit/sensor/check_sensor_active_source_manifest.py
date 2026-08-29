#!/usr/bin/env python3
"""Guard the canonical Sensor active-source ownership manifest."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "docs" / "validation" / "sensor-active-source-manifest.md"
SENSOR_CMAKE = ROOT / "components" / "sensor" / "CMakeLists.txt"
DRIVERS_CMAKE = ROOT / "components" / "drivers" / "CMakeLists.txt"
UNIT_CMAKE = ROOT / "tests" / "unit" / "CMakeLists.txt"
STALE_BMP280 = ROOT / "components" / "sensor" / "drivers" / "pressure" / "xy_sensor_bmp280.c"
SMART_HYGROMETER_CMAKE = ROOT / "projects" / "examples" / "smart_hygrometer" / "CMakeLists.txt"
SMART_HYGROMETER_MAIN = ROOT / "projects" / "examples" / "smart_hygrometer" / "main.c"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    errors: list[str] = []
    require(MANIFEST.is_file(), "sensor active-source manifest is missing", errors)
    if errors:
        print("sensor_active_source_manifest failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    manifest = MANIFEST.read_text(encoding="utf-8")
    sensor_cmake = SENSOR_CMAKE.read_text(encoding="utf-8")
    drivers_cmake = DRIVERS_CMAKE.read_text(encoding="utf-8")
    unit_cmake = UNIT_CMAKE.read_text(encoding="utf-8")
    smart_hygrometer_cmake = SMART_HYGROMETER_CMAKE.read_text(encoding="utf-8")
    smart_hygrometer_main = SMART_HYGROMETER_MAIN.read_text(encoding="utf-8")

    legacy = sorted((ROOT / "components" / "sensor" / "sensors").glob("sensor_*.c"))
    experimental = sorted((ROOT / "components" / "sensor" / "src").glob("xy_*.c"))
    device = sorted((ROOT / "components" / "drivers" / "sensor").glob("**/xy_*.c"))

    require(len(legacy) == 55, f"expected 55 legacy active sources, found {len(legacy)}", errors)
    require(len(experimental) == 20,
            f"expected 20 experimental xy_* sources, found {len(experimental)}", errors)
    require(len(device) == 4, f"expected 4 Device-model sources, found {len(device)}", errors)

    for token in (
        "legacy-active-root",
        "experimental-test-only",
        "device-active-root",
        "禁止第四套生命周期",
        "Host 测试不等于根产品链接",
        "hardware-pending",
        "SHT30",
        "MPU6050",
        "ADS1115",
        "BMP280",
    ):
        require(token in manifest, f"manifest must preserve policy token: {token}", errors)

    require('file(GLOB SENSOR_DRIVERS "${CMAKE_CURRENT_LIST_DIR}/sensors/sensor_*.c")'
            in sensor_cmake, "legacy root source glob changed without manifest update", errors)
    require("src/xy_" not in sensor_cmake,
            "experimental xy_* sources must not silently enter sensor_component", errors)
    require('file(GLOB_RECURSE COMPONENT_SOURCES "*.c")' in drivers_cmake,
            "Device driver root source ownership changed without manifest update", errors)
    require("sensor_active_source_manifest" in unit_cmake,
            "sensor_active_source_manifest CTest must remain registered", errors)
    require(not STALE_BMP280.exists(),
            "retired xy_sensor_bmp280 lifecycle must not reappear", errors)
    require("components/drivers/sensor/pressure/bmp280" in smart_hygrometer_cmake,
            "smart_hygrometer must include the canonical BMP280 owner", errors)
    require("components/drivers/sensor/pressure/bmp280/xy_bmp280.c" in smart_hygrometer_cmake,
            "smart_hygrometer must compile the canonical BMP280 owner", errors)
    require("components/sensor/src/xy_bmp280.c" not in smart_hygrometer_cmake,
            "smart_hygrometer must not reference the removed experimental BMP280 source", errors)
    require("xy_bmp280_init_addr(&g_bmp280, NULL, BMP280_ADDR_DEFAULT)" in smart_hygrometer_main,
            "smart_hygrometer must use the canonical explicit-address BMP280 API", errors)

    if errors:
        print("sensor_active_source_manifest failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("sensor_active_source_manifest_ok legacy_active=55 experimental_test_only=20 "
          "device_active=4 hardware=pending")
    return 0


if __name__ == "__main__":
    sys.exit(main())
