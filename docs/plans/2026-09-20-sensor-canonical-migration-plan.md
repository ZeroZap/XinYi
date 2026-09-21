# Sensor canonical Device migration execution plan

**Date:** 2026-09-20
**Status:** Active
**Scope:** Migrate legacy/test-only Sensor implementations to one canonical Device owner without requiring immediate board validation.

## Policy correction

- Missing hardware evidence is **not** a deletion criterion. It limits the evidence state to `hardware-pending`.
- Chips with a traceable public datasheet or vendor implementation are rebuilt or promoted directly under `components/drivers/sensor/**`.
- An incorrect legacy implementation is used only as an API/consumer inventory; register protocol and engineering-unit conversion are rebuilt from primary documentation.
- A legacy `sensor_device_t` API remains only when a checked consumer exists. It becomes a thin compatibility wrapper and delegates all transport, protocol, conversion, configuration and lifecycle work to the canonical owner.
- A chip remains `unsupported` only when its exact identity or protocol cannot be established. Required evidence is full part number, vendor, datasheet revision, bus/ID/address and preferably board BOM/schematic provenance.
- Each chip is one bounded slice and one path-limited commit. Bulk migration and a fourth lifecycle are prohibited.

## Canonical slice template

Each migrated chip must provide:

1. `components/drivers/sensor/<class>/<chip>/xy_<chip>.{h,c}` with an embedded `xy_i2c_device_t` or the appropriate Device transport helper.
2. A chip-local `CMakeLists.txt` and category inclusion so `xy_drivers` owns the source in a root build.
3. A typed API covering explicit-address initialization, deinit, identity/configuration and meaningful measurements/configuration.
4. Strict lifecycle checks on both outer state and nested helper state; invalid state performs zero I/O.
5. First-error stop and staged commit for caller output, sample/config cache and lifecycle state.
6. A focused canonical test that links the real owner and mocks only Device/HAL seams.
7. A compatibility-wrapper test when a legacy consumer remains. The wrapper may retain metadata/unit/API shape but no register protocol.
8. Manifest and ownership guards preventing legacy/experimental/prototype implementation duplication.
9. Evidence recorded as `hardware-pending`, B1 or B2 independently of source migration.

## Acceptance gate per chip

- focused identity/init/config/sample/deinit/re-init tests;
- every transport failure point and nested-helper invalidation covered;
- output/cache/lifecycle atomicity verified;
- canonical source linked by root `xy_drivers` and, when required, by `sensor_component` through the wrapper path;
- manifest checker passes with one implementation owner;
- full Host suite passes;
- PC, STM32L4 and STM32U5 Release builds pass;
- `git diff --check` and `git diff --cached --check` pass;
- hardware claim remains `hardware-pending` unless separately observed.

## Execution order

### M0 — ownership gate correction (`DONE`)

1. Resolve APDS9960 duplicate root ownership:
   - `components/sensor/sensor_apds9960.c`
   - `components/sensor/sensors/sensor_apds9960.c`
2. Expand the ownership manifest to scan explicit top-level `components/sensor/sensor_*.c` sources as well as the `sensors/` glob.
3. Preserve the stronger checked implementation while preparing APDS9960 for canonical migration; do not treat duplicate cleanup as product validation.

Closed by retaining the focused-test-backed `sensors/sensor_apds9960.c`, removing the weaker
top-level source and duplicate header, and adding a manifest guard for explicit top-level owners and
top-level/subdirectory name overlap. APDS9960 remains a legacy owner pending its M4 Device migration.

### M1 — restore recently retired, mature digital protocols

Order is fixed:

1. **INA219** (`P0`, rebuild)
   - Explicit shunt resistance, current-LSB and calibration ownership.
   - Separate shunt voltage, bus voltage, current and power APIs.
   - Restore legacy factory only as a wrapper if a consumer requires it.
   - State: `hardware-pending`.
2. **BMP390** (`P0`, rebuild)
   - Use Bosch BMP3 SensorAPI or a pinned datasheet implementation.
   - Read NVM calibration and perform temperature-first pressure compensation.
   - State: `hardware-pending`.
   - Implementation: canonical owner uses pinned Bosch BMP3 SensorAPI `bmp3_v2.0.6` with
     repository-recorded commit/license/file hashes; focused compensation and failure contracts are
     complete, while board evidence remains pending.
3. **QMA6100P** (`P1`, rebuild)
   - Confirm exact public part identity before coding.
   - Rebuild chip ID, output width, ODR/range and sensitivity contracts; do not reuse the old mixed enum/physical cache.
4. **IIS2ICLX** (`P1`, rebuild)
   - Correct formal model and SPI-only transport.
   - Do not restore the invalid `IIS2ICLP` I2C owner.

### M2 — migrate owners with existing Pandora evidence

1. AHT10
2. AP3216C
3. ICM20608

Create canonical owners first, then replace each legacy implementation with a thin wrapper. Existing B1 evidence remains bounded to the already observed board path; migration itself does not upgrade it.

AHT10 is complete: the canonical Device owner now owns `0x38` initialization, measurement
transactions and both converted channels; the legacy root source is a humidity-only compatibility
wrapper. Focused Device/wrapper contracts preserve existing Pandora evidence without upgrading it.

AP3216C is complete: the canonical Device owner now owns `0x1E` reset/mode lifecycle and ALS,
proximity and IR register decoding; the legacy three-factory API delegates all transport and
conversion work. Existing Pandora B1 evidence remains bounded and is not upgraded by this migration.

ICM20608 is complete: the canonical Device owner now owns identity, default configuration,
I2C/SPI transport dispatch and accel/gyro/temperature conversion; the legacy three-factory API is a
compatibility wrapper. Existing static/basic-chain Pandora evidence remains bounded.

QMA6100P remains blocked on exact part identity/documentation. IIS2ICLX reconstruction was deferred
in this run because the primary ST datasheet fetch failed; no register constants were guessed.

### M3 — promote substantive experimental typed owners

Preferred order:

1. HDC1080
2. TSL2561
3. INA226
4. VL53L1X
5. SGP40
6. LPS22HB
7. LTC2945
8. MLX90614
9. BMI088 / BMI270 / BNO055

HDC1080 is complete: the tested typed source/header were moved into the Device root, the fixed
address/lifecycle boundary was tightened, and the focused target now compiles the canonical owner.

TSL2561 is complete: the tested typed source/header were moved into the Device root, all three
documented addresses are validated before Device initialization, nested-helper lifecycle rejection
performs zero I/O, and channel/lux/timestamp cache publication is staged until both reads succeed.

INA226 is complete: the Device-root owner explicitly models shunt resistance/current-LSB and
datasheet calibration/current/power scaling. The old INA229 alias was removed because INA229 does
not share INA226's register protocol. Hardware and metrology remain pending.

VL53L1X is complete: the typed owner moved into the Device root, its private 16-bit I2C callback
lifecycle was replaced by nested `xy_i2c_device_t` transactions, and existing identity/result/
timeout/configuration/calibration Host contracts were retained. Hardware remains pending.

LPS22HB is complete: the focused typed source/header moved into the Device root and the root
`xy_drivers` target now owns the implementation. Existing identity/configuration/pressure/
temperature/FIFO/interrupt/error-preservation Host contracts remain unchanged. Hardware remains
pending; the legacy callback-shaped transport is retained inside this bounded API migration and is
the next transport-boundary hardening target.

SGP40 is complete: the typed owner moved into the Device root and its private command/read callback
lifecycle was replaced by nested `xy_i2c_device_t` transactions. Feature-set, serial-ID, self-test,
CRC and measurement contracts remain focused-test backed; VOC accuracy and hardware remain pending.

LTC2945 is complete as a datasheet rebuild rather than a source move. The experimental owner used
incorrect register addresses and modeled nonexistent charge/energy accumulators. The Device-root
owner now uses the documented CONTROL/ALERT/STATUS/FAULT, POWER, SENSE and VIN register map with
explicit shunt resistance and staged sample publication. Hardware and metrology remain pending.

AS5048B is complete as a canonical angle Device owner. The owner models the I2C `0x40` address,
datasheet `0xFE/0xFF` 14-bit angle read, staged sample publication and nested lifecycle boundary;
angle accuracy and hardware remain pending.

AS5600 is complete as a canonical angle Device owner. The owner models the I2C `0x36` address,
datasheet `0x0E/0x0F` 12-bit angle read, reserved-bit masking, staged sample publication and
nested lifecycle boundary; angle accuracy and hardware remain pending.

MAX44009 is complete as a canonical light Device owner. The owner models the `0x4A`/`0x4B`
addresses, datasheet `0x03`/`0x04` exponent-mantissa registers, integer milli-lux conversion,
staged sample publication and nested lifecycle boundary; optical accuracy and hardware remain
pending.

VCNL4040 is complete as a canonical proximity Device owner. The owner models fixed 7-bit address
`0x60`, the datasheet little-endian proximity registers `0x08/0x09`, staged raw sample publication
and nested lifecycle boundary; optical response, distance calibration and hardware remain pending.

- QMC5883L is complete as a canonical magnetic Device owner. The owner models fixed 7-bit address
`0x0D`, chip ID `0xFF`, reset/period/continuous configuration, DRDY gating, little-endian XYZ and
staged sample publication; magnetic calibration, accuracy and hardware remain pending.

AK09918 is complete as a canonical magnetic Device owner. The owner models fixed 7-bit address
`0x0C`, AKM WIA identity, CNTL3 reset, continuous 100 Hz CNTL2 configuration, ST1 DRDY gating,
little-endian XYZ and staged sample publication; magnetic calibration, accuracy and hardware remain
pending.

IST8310 is complete as a canonical magnetic Device owner. The owner models fixed 7-bit address
`0x0C`, WHO_AM_I `0x10`, continuous 100 Hz configuration, little-endian XYZ and staged sample
publication; magnetic calibration, accuracy and hardware remain pending.

BMA400 is complete as a canonical motion Device owner. The owner models fixed 7-bit address
`0x14`, CHIP_ID `0x90`, soft reset, low-power/±2g/25 Hz configuration, little-endian XYZ and
staged sample publication; motion accuracy, power and hardware remain pending.

KX023 is complete as a canonical motion Device owner. The owner models fixed 7-bit address `0x1E`,
WHO_AM_I `0x15`, soft reset, standby/12.5 Hz/low-power configuration, little-endian XYZ and staged
sample publication; motion accuracy, power and hardware remain pending.

ADXL362 is complete as a canonical SPI motion Device owner. The owner models device ID `0xAD`,
100 Hz/±2g measurement configuration, signed 12-bit XYZ decode and staged sample publication behind
the nested SPI lifecycle; motion accuracy, SPI timing and hardware remain pending.

LSM6DSO is complete as a canonical I2C motion Device owner. The owner models fixed 7-bit address
`0x6A`, WHO_AM_I `0x6C`, reset/I3C-disable/104 Hz/±2g/±250 dps configuration, staged accelerometer
and gyroscope XYZ publication and nested lifecycle failure boundaries; motion accuracy, timing and
hardware remain pending.

LSM6DSL is complete as a canonical I2C motion Device owner. The owner models fixed 7-bit address
`0x6A`, WHO_AM_I `0x6A`, reset/I3C-disable/104 Hz/±2g/±250 dps configuration, staged accelerometer
and gyroscope XYZ publication and nested lifecycle failure boundaries; motion accuracy, timing and
hardware remain pending.

LSM6DSR is complete as a canonical I2C motion Device owner. The owner models fixed 7-bit address
`0x6A`, WHO_AM_I `0x69`, reset/I3C-disable/104 Hz/±2g/±250 dps configuration, staged accelerometer
and gyroscope XYZ publication and nested lifecycle failure boundaries; motion accuracy, timing and
hardware remain pending.

LSM9DS1 is complete as a canonical dual-I2C motion/magnetic Device owner. The owner models IMU
address `0x6A`, magnetometer address `0x1C`, WHO_AM_I values `0x68`/`0x3D`, reset and 104 Hz
configuration, staged accelerometer/gyroscope/magnetometer XYZ publication and independent nested
I2C lifecycle failure boundaries; accuracy, timing and hardware remain pending.

LIS2DH12 is complete as a canonical I2C motion Device owner. The owner models address `0x18`,
WHO_AM_I `0x33`, 10 Hz/XYZ/±2g high-resolution configuration, temperature enable, left-aligned
12-bit XYZ decode and staged sample publication behind the nested I2C lifecycle; acceleration
accuracy, timing and hardware remain pending.

APDS9960 is complete as a canonical I2C optical/proximity Device owner. The owner models address
`0x39`, ID values `0xAB`/`0x9C`, function enable, staged RGBC/proximity samples and bounded gesture
FIFO reads behind the nested I2C lifecycle; optical response, gesture classification, timing and
hardware remain pending.

CCS811 is complete as a canonical I2C gas Device owner. The owner models address `0x5A`, HW_ID
`0x81`, application start, one-second measurement mode, DATA_READY gating and staged eCO2/TVOC
publication behind the nested I2C lifecycle; gas response, baseline, accuracy and hardware remain
pending.

- AK09918 is complete as a canonical magnetic Device owner. The owner models fixed 7-bit address
`0x0C`, AKM WIA identity, reset/continuous-100Hz configuration, data-ready gating, little-endian
XYZ and staged sample publication; magnetic calibration, accuracy and hardware remain pending.

BMI088 is complete as a root-ownership promotion: the focused-test-backed dual-chip-select SPI
source/header pair moved into `components/drivers/sensor/motion/bmi088`, `xy_drivers` now owns it,
and the focused target compiles that canonical path. Existing identity, configuration, conversion,
calibration, failure-preservation and lifecycle contracts remain unchanged. Hardware, timing,
interrupt, FIFO, calibration quality and motion accuracy remain pending.

BNO055 is complete as a root-ownership promotion: the focused-test-backed source/header pair moved
into `components/drivers/sensor/motion/bno055`, `xy_drivers` now owns it, and the focused target
compiles that canonical path. Existing initialization, I2C, fused/raw output, mode/power/unit,
axis-remap, unsupported-UART, failure-preservation and lifecycle contracts remain unchanged.
Fusion quality, calibration, timing, interrupts, motion accuracy, UART transport and hardware remain pending.

BMI270 is complete as a root-ownership promotion: the focused-test-backed source/header pair moved
into `components/drivers/sensor/motion/bmi270`, `xy_drivers` now owns it, and the focused target
compiles that canonical path. Existing initialization, I2C/SPI, range, raw/sensor-time atomicity,
failure-preservation and lifecycle contracts remain unchanged. Motion accuracy, timing, interrupts,
calibration and hardware remain pending.

Move rather than copy the implementation. Delete the experimental path only after the Device-root target and focused test use the canonical source.

Domain-owner corrections:

- Sensor BQ25620 becomes an adapter to `components/charger`, not another register owner.
- Sensor MAX17043 becomes an adapter to `components/fuel_gauge`, not another register owner.

### M4 — remaining mature digital protocols

Short protocols first:

- AS5048B, AS5600, MAX44009, VCNL4040;
- QMC5883L, AK09918, IST8310;
- ADXL362, BMA400, KX023, LIS2DH12, LIS2DW12;
- LSM6DSL, LSM6DSO, LSM6DSR, LSM9DS1;
- APDS9960, CCS811, VL53L0X. VL53L0X is now complete as a canonical Device owner: fixed `0x29` address, Model ID `0xEE`, staged range sample, nested lifecycle guards, and a focused Host contract are in place; distance accuracy and hardware remain pending.

Optional FIFO, gesture, interrupt and algorithm features may follow in later slices; they do not block ownership migration when identity, default configuration and basic sampling contracts are complete.

### M5 — identity/board-data dependent

Keep pending until exact documentation exists:

- CMM905/MM905;
- PA122 (identity/register protocol unresolved; remain unsupported);
- SC7A20 versus Silan SC7A20 identity/owner relationship;
- GPS module/transport ownership;
- ACS712, FSR, GUVA-S12SD, MG811, MQ135, MQ3 and MQ7 board analog front ends.

Analog migrations require board reference voltage, divider/load/sense resistance, zero point, temperature behavior and calibration curve. Do not promote empirical formulas without that context.

Remain `unsupported` with no-return guards until identity is established:

- GD30DF;
- CMS;
- HS-ADS1100;
- DMP6100.

## Progress accounting

Track three independent numbers after every slice:

- canonical Device active-root owners;
- legacy compatibility/active-root files;
- experimental test-only owners.

A migration is complete only when the canonical count/root ownership changes as intended and duplicate implementation count remains zero. Deleting an owner without creating/promoting the planned canonical owner does not count as migration progress.
