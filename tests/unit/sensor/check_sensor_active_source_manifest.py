#!/usr/bin/env python3
"""Guard the canonical Sensor active-source ownership manifest."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "docs" / "validation" / "sensor-active-source-manifest.md"
TRACKER = ROOT / "docs" / "plans" / "SPRINT_TRACKER.md"
AUDIT_PLAN = ROOT / "docs" / "plans" / "2026-08-17-component-audit-sprint-plan.md"
SENSOR_CMAKE = ROOT / "components" / "sensor" / "CMakeLists.txt"
DRIVERS_CMAKE = ROOT / "components" / "drivers" / "CMakeLists.txt"
UNIT_CMAKE = ROOT / "tests" / "unit" / "CMakeLists.txt"
STALE_BMP280 = ROOT / "components" / "sensor" / "drivers" / "pressure" / "xy_sensor_bmp280.c"
STALE_BH1750 = ROOT / "components" / "sensor" / "drivers" / "light" / "xy_sensor_bh1750.c"
STALE_MPU6050 = ROOT / "components" / "sensor" / "drivers" / "motion" / "xy_sensor_mpu6050.c"
STALE_SHT30 = ROOT / "components" / "sensor" / "drivers" / "temperature" / "xy_sensor_sht30.c"
STALE_HDC1080 = ROOT / "components" / "sensor" / "drivers" / "temperature" / "xy_sensor_hdc1080.c"
STALE_TSL2561 = ROOT / "components" / "sensor" / "drivers" / "light" / "xy_sensor_tsl2561.c"
STALE_INA226 = ROOT / "components" / "sensor" / "drivers" / "power" / "xy_sensor_ina226.c"
STALE_BQ25620 = ROOT / "components" / "sensor" / "drivers" / "power" / "xy_sensor_bq25620.c"
STALE_MAX17043 = ROOT / "components" / "sensor" / "drivers" / "power" / "xy_sensor_max17043.c"
STALE_ADXL362 = ROOT / "components" / "sensor" / "drivers" / "motion" / "xy_sensor_adxl362.c"
STALE_BME280 = ROOT / "components" / "sensor" / "drivers" / "pressure" / "xy_sensor_bme280.c"
STALE_ICM20608 = ROOT / "components" / "sensor" / "drivers" / "motion" / "xy_sensor_icm20608.c"
STALE_SGP40_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_sgp40.c"
STALE_SGP30_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_sgp30.c"
STALE_ENS160_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_ens160.c"
STALE_IM69D_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_im69d.c"
STALE_MAX30102_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_max30102.c"
STALE_AEAT8800_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_aeat8800.c"
STALE_MLX90393_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_mlx90393.c"
STALE_IIS2ICLP_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_iis2iclp.c"
STALE_DMP6100_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_dmp6100.c"
STALE_VL53L1X_LEGACY = ROOT / "components" / "sensor" / "sensors" / "sensor_vl53l1x.c"
STALE_AHT20_SOURCE = ROOT / "components" / "sensor" / "src" / "xy_aht20.c"
STALE_AHT20_HEADER = ROOT / "components" / "sensor" / "inc" / "xy_aht20.h"
STALE_SHT40_SOURCE = ROOT / "components" / "sensor" / "src" / "xy_sht40.c"
STALE_SHT40_HEADER = ROOT / "components" / "sensor" / "inc" / "xy_sht40.h"
STALE_SHT40_PROTOTYPE = (ROOT / "components" / "sensor" / "drivers" / "temperature" /
                         "xy_sensor_sht40.c")
SMART_HYGROMETER_CMAKE = ROOT / "projects" / "examples" / "smart_hygrometer" / "CMakeLists.txt"
SMART_HYGROMETER_MAIN = ROOT / "projects" / "examples" / "smart_hygrometer" / "main.c"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def false_owner_candidates(paths: list[Path]) -> list[str]:
    """Find tiny legacy owners that publish a literal scalar without transport."""
    literal_output = re.compile(
        r"value\.val_(?:float|int32|uint32)\s*=\s*[-+]?(?:\d+(?:\.\d*)?|\.\d+)[fFuUlL]*\s*;"
    )
    transport_call = re.compile(
        r"\b(?:sensor_(?:i2c|spi)_[a-z_]+|hal_(?:i2c|spi|adc)_[a-z_]+|xy_hal_[a-z_]+|"
        r"xy_[a-z0-9]+_(?:read|write|init|deinit)|[a-z0-9_]+_reg_(?:read|write))\s*\("
    )
    candidates: list[str] = []
    for path in paths:
        source = path.read_text(encoding="utf-8")
        executable = re.sub(r"^\s*extern\b[^;]*;", "", source, flags=re.MULTILINE)
        if literal_output.search(executable) and not transport_call.search(executable):
            candidates.append(path.name)
    return candidates


def main() -> int:
    errors: list[str] = []
    require(MANIFEST.is_file(), "sensor active-source manifest is missing", errors)
    if errors:
        print("sensor_active_source_manifest failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    manifest = MANIFEST.read_text(encoding="utf-8")
    tracker = TRACKER.read_text(encoding="utf-8")
    audit_plan = AUDIT_PLAN.read_text(encoding="utf-8")
    sensor_cmake = SENSOR_CMAKE.read_text(encoding="utf-8")
    drivers_cmake = DRIVERS_CMAKE.read_text(encoding="utf-8")
    unit_cmake = UNIT_CMAKE.read_text(encoding="utf-8")
    smart_hygrometer_cmake = SMART_HYGROMETER_CMAKE.read_text(encoding="utf-8")
    smart_hygrometer_main = SMART_HYGROMETER_MAIN.read_text(encoding="utf-8")

    legacy = sorted((ROOT / "components" / "sensor" / "sensors").glob("sensor_*.c"))
    experimental = sorted((ROOT / "components" / "sensor" / "src").glob("xy_*.c"))
    device = sorted((ROOT / "components" / "drivers" / "sensor").glob("**/xy_*.c"))

    canonical_names = {path.stem.removeprefix("xy_") for path in device}
    legacy_names = {path.stem.removeprefix("sensor_") for path in legacy}
    experimental_names = {path.stem.removeprefix("xy_") for path in experimental}
    prototype = sorted((ROOT / "components" / "sensor" / "drivers").glob("**/xy_sensor_*.c"))
    prototype_names = {path.stem.removeprefix("xy_sensor_") for path in prototype}

    require(len(legacy) == 45, f"expected 45 legacy active sources, found {len(legacy)}", errors)
    require(len(experimental) == 17,
            f"expected 17 experimental xy_* sources, found {len(experimental)}", errors)
    require(len(device) == 12, f"expected 12 Device-model sources, found {len(device)}", errors)
    false_owners = false_owner_candidates(legacy)
    require(not false_owners,
            f"legacy constant-output/zero-transport false owners found: {false_owners}", errors)
    require(canonical_names & legacy_names == {"sht30", "mpu6050", "bmp280", "bh1750", "aht20"},
            "canonical/legacy overlap must contain only approved compatibility wrappers", errors)
    require(not (canonical_names & experimental_names),
            "canonical Device owners must not reappear in experimental src/xy_*", errors)
    require(not (canonical_names & prototype_names),
            "canonical Device owners must not reappear as xy_sensor_* prototypes", errors)

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
        "AHT30",
        "L3G4200D",
        "BME680",
        "BH1750",
        "HMC5883L",
        "AHT20",
        "SHT40",
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
    require("| 2026-08-17 | D-001 | 决策 |" in tracker and
            "canonical API 已确定为 Device model" in tracker and
            "| CLOSED |" in tracker,
            "D-001 must be closed after the canonical Device-model decision", errors)
    require("17 个 `src/xy_*.c`" in audit_plan,
            "audit plan must use the current experimental source count", errors)
    require("Device-model canonical owner" in audit_plan,
            "audit plan must record the resolved Sensor ownership direction", errors)
    require(not STALE_BMP280.exists(),
            "retired xy_sensor_bmp280 lifecycle must not reappear", errors)
    require(not STALE_BH1750.exists(),
            "retired xy_sensor_bh1750 lifecycle must not reappear", errors)
    require(not STALE_MPU6050.exists(),
            "retired xy_sensor_mpu6050 lifecycle must not reappear", errors)
    require(not STALE_SHT30.exists(),
            "retired xy_sensor_sht30 lifecycle must not reappear", errors)
    require(not STALE_HDC1080.exists(),
            "retired xy_sensor_hdc1080 lifecycle must not reappear", errors)
    require(not STALE_TSL2561.exists(),
            "retired xy_sensor_tsl2561 lifecycle must not reappear", errors)
    require(not STALE_INA226.exists(),
            "retired xy_sensor_ina226 lifecycle must not reappear", errors)
    require(not STALE_BQ25620.exists(),
            "retired xy_sensor_bq25620 lifecycle must not reappear", errors)
    require(not STALE_MAX17043.exists(),
            "retired xy_sensor_max17043 lifecycle must not reappear", errors)
    require(not STALE_ADXL362.exists(),
            "retired xy_sensor_adxl362 lifecycle must not reappear", errors)
    require(not STALE_BME280.exists(),
            "retired xy_sensor_bme280 lifecycle must not reappear", errors)
    require(not STALE_ICM20608.exists(),
            "retired xy_sensor_icm20608 lifecycle must not reappear", errors)
    require(not STALE_SGP40_LEGACY.exists(),
            "retired stub sensor_sgp40 lifecycle must not reappear", errors)
    require(not STALE_SGP30_LEGACY.exists(),
            "retired stub sensor_sgp30 lifecycle must not reappear", errors)
    require(not STALE_ENS160_LEGACY.exists(),
            "retired stub sensor_ens160 lifecycle must not reappear", errors)
    require(not STALE_IM69D_LEGACY.exists(),
            "retired stub sensor_im69d lifecycle must not reappear", errors)
    require(not STALE_MAX30102_LEGACY.exists(),
            "retired stub sensor_max30102 lifecycle must not reappear", errors)
    require(not STALE_AEAT8800_LEGACY.exists(),
            "retired stub sensor_aeat8800 lifecycle must not reappear", errors)
    require(not STALE_MLX90393_LEGACY.exists(),
            "retired stub sensor_mlx90393 lifecycle must not reappear", errors)
    require(not STALE_IIS2ICLP_LEGACY.exists(),
            "retired unsupported sensor_iis2iclp lifecycle must not reappear", errors)
    require(not STALE_DMP6100_LEGACY.exists(),
            "retired ungrounded sensor_dmp6100 lifecycle must not reappear", errors)
    require(not STALE_VL53L1X_LEGACY.exists(),
            "retired legacy sensor_vl53l1x lifecycle must not reappear", errors)
    require(not STALE_AHT20_SOURCE.exists() and not STALE_AHT20_HEADER.exists(),
            "retired experimental AHT20 lifecycle must not reappear", errors)
    require(not STALE_SHT40_SOURCE.exists() and not STALE_SHT40_HEADER.exists(),
            "retired experimental SHT40 lifecycle must not reappear", errors)
    require(not STALE_SHT40_PROTOTYPE.exists(),
            "retired xy_sensor_sht40 lifecycle must not reappear", errors)
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

    print("sensor_active_source_manifest_ok legacy_active=45 experimental_test_only=17 "
          "device_active=12 approved_wrappers=5 overlap_duplicates=0 false_owners=0 "
          "hardware=mixed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
