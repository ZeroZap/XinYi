# XinYi Sensor active-source ownership manifest

**Date**: 2026-08-28  
**Status**: Host-governed ownership baseline; hardware-pending  
**Scope**: first-party Sensor implementations under `components/sensor` and
`components/drivers/sensor`

> This manifest defines source/build ownership. It does not upgrade Host tests to board,
> performance, safety, or product evidence.

## Policy

- `legacy-active-root`: `components/sensor/sensors/sensor_*.c` is the current
  `sensor_component` product source set. New chips must not be added to this lifecycle.
- `experimental-test-only`: `components/sensor/src/xy_*.c` is compiled directly by focused
  Host tests where wired, but is not linked into the root `sensor_component`. **Host 测试不等于根产品链接**.
- `device-active-root`: `components/drivers/sensor/**/xy_*.c` is collected by the root
  `xy_drivers` target and is the canonical destination for migrations using the Device model.
- Existing public compatibility wrappers may remain during migration, but a chip must have one
  active implementation owner. **禁止第四套生命周期**.
- All three tracks remain `hardware-pending`; source ownership and Host contracts do not prove
  sensor accuracy, timing, bus recovery, calibration, or board support.

## Current inventory

| Track | Build ownership | Sources | Public/lifecycle status | Focused evidence | Hardware |
|---|---|---:|---|---|---|
| legacy `sensor_*` | `sensor_component`; `components/sensor/CMakeLists.txt` globs `sensors/sensor_*.c` | 55 | `legacy-active-root`; frozen for new drivers | broad legacy Sensor Unity CTests | `hardware-pending` |
| new `components/sensor/src/xy_*` | excluded from root `sensor_component`; tests link selected source files directly | 23 | `experimental-test-only`; no product-root claim | selected SHT30/MPU6050/ADS1115/BMP280 and other driver contracts | `hardware-pending` |
| Device-model drivers | `xy_drivers`; recursive source collection under `components/drivers` | 4 | `device-active-root`; canonical migration destination | SHT30 integration, transaction/CRC/error contract, and heterogeneous four-driver test | `hardware-pending` |

The Device-model root set is currently exactly:

- SHT30: `components/drivers/sensor/temperature/sht30/xy_sht30.c`
- MPU6050: `components/drivers/sensor/motion/mpu6050/xy_mpu6050.c`
- ADS1115: `components/drivers/sensor/adc/ads1115/xy_ads1115.c`
- BMP280: `components/drivers/sensor/pressure/bmp280/xy_bmp280.c`

## Admission and migration contract

A Sensor implementation may become canonical active product source only when all of the following
are explicit in one reviewed slice:

1. one implementation owner and one lifecycle (`xy_device_t`/typed Device adapter preferred);
2. public header and Kconfig/CMake switch ownership;
3. root target inclusion proven from a clean configure/build, not only a test-local source list;
4. focused normal/error/output-preservation/re-init Host coverage;
5. legacy wrapper marked compatibility-only when retained;
6. board evidence remains separately recorded as `hardware-pending`, B1, or B2.

The first migration candidates are SHT30, MPU6050, and ADS1115; BMP280 remains the fourth existing
Device-model reference. Migration must remove duplicate active ownership rather than merely add a
new copy.

### SHT30 migration status

The Device-model source is the selected canonical destination and now has a focused transaction
contract covering helper/reset failure propagation, initialization state, CRC rejection, cached
output preservation, conversion, and uninitialized access. The legacy `sensor_sht30.c` and
test-local `components/sensor/src/xy_sht30.c` remain compatibility/experimental implementations in
this intermediate slice; therefore SHT30 duplicate lifecycle removal is not yet complete and no
hardware status is upgraded.

## Guard and update rule

`tests/unit/sensor/check_sensor_active_source_manifest.py` fails when source counts or root ownership
shape change without this manifest and its CTest being updated. Any addition, deletion, root-source
selection change, or lifecycle decision must update this file, the Sprint tracker, and the component
evidence matrix in the same path-limited slice.
