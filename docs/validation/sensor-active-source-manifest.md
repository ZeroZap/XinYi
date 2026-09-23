# XinYi Sensor active-source ownership manifest

**Date**: 2026-08-28  
**Status**: Host-governed ownership baseline; mixed hardware evidence
**Scope**: first-party Sensor implementations under `components/sensor` and
`components/drivers/sensor`

> This manifest defines source/build ownership. It does not upgrade Host tests to board,
> performance, safety, or product evidence.

## Policy

- `legacy-active-root`: `components/sensor/sensors/sensor_*.c` plus the explicitly inventoried
  top-level `sensor_adt7420.c` are the current `sensor_component` compatibility source set. New
  chips must not be added to this lifecycle;
  migration of these owners to the Device model is now the next major Sensor workstream after the
  current Pandora I2C2 Device owners are completed and stabilized.
- `experimental-test-only`: `components/sensor/src/xy_*.c` is compiled directly by focused
  Host tests where wired, but is not linked into the root `sensor_component`. **Host 测试不等于根产品链接**.
- `device-active-root`: `components/drivers/sensor/**/xy_*.c` is collected by the root
  `xy_drivers` target and is the canonical destination for migrations using the Device model.
- Existing public compatibility wrappers may remain during migration, but a chip must have one
  active implementation owner. **禁止第四套生命周期**.
- Hardware evidence is tracked per owner: the legacy and experimental inventories remain generally
  `hardware-pending`, while the Pandora I2C2 Device owners below have bounded B1 evidence. Source
  ownership and Host contracts alone never prove accuracy, timing, recovery, or calibration.

## Current inventory

| Track | Build ownership | Sources | Public/lifecycle status | Focused evidence | Hardware |
|---|---|---:|---|---|---|
| legacy `sensor_*` | `sensor_component`; `components/sensor/CMakeLists.txt` globs 38 `sensors/sensor_*.c` files and explicitly lists top-level `sensor_adt7420.c` | 39 | `legacy-active-root`; frozen for new drivers | broad legacy Sensor Unity CTests | `hardware-pending` |
| new `components/sensor/src/xy_*` | excluded from root `sensor_component`; tests link selected source files directly | 6 | `experimental-test-only`; no product-root claim | selected driver contracts | `hardware-pending` |
| Device-model drivers | `xy_drivers`; recursive source collection under `components/drivers` | 47 | `device-active-root`; canonical migration destination | AHT10/AP3216C/ICM20608/AHT30/L3G4200D/BME680/HMC5883L basic-chain verified; AK09918/LSM9DS1/VL53L0X/LIS2DW12 and other non-board owners remain `hardware-pending` |

The Device-model root set is currently exactly:

- SHT30: `components/drivers/sensor/temperature/sht30/xy_sht30.c`
- MPU6050: `components/drivers/sensor/motion/mpu6050/xy_mpu6050.c`
- ADS1115: `components/drivers/sensor/adc/ads1115/xy_ads1115.c`
- BMP280: `components/drivers/sensor/pressure/bmp280/xy_bmp280.c`
- AHT30: `components/drivers/sensor/temperature/aht30/xy_aht30.c`
- L3G4200D: `components/drivers/sensor/motion/l3g4200d/xy_l3g4200d.c`
- BME680: `components/drivers/sensor/environment/bme680/xy_bme680.c`
- BH1750: `components/drivers/sensor/light/bh1750/xy_bh1750.c`
- HMC5883L: `components/drivers/sensor/magnetic/hmc5883l/xy_hmc5883l.c`
- SC7A22H: `components/drivers/sensor/motion/sc7a22h/xy_sc7a22h.c`
- AHT20: `components/drivers/sensor/temperature/aht20/xy_aht20.c`
- SHT40: `components/drivers/sensor/temperature/sht40/xy_sht40.c`
- INA219: `components/drivers/sensor/adc/ina219/xy_ina219.c`
- BMP390: `components/drivers/sensor/pressure/bmp390/xy_bmp390.c`
- AHT10: `components/drivers/sensor/temperature/aht10/xy_aht10.c`
- AP3216C: `components/drivers/sensor/light/ap3216c/xy_ap3216c.c`
- ICM20608: `components/drivers/sensor/motion/icm20608/xy_icm20608.c`
- HDC1080: `components/drivers/sensor/temperature/hdc1080/xy_hdc1080.c`
- TSL2561: `components/drivers/sensor/light/tsl2561/xy_tsl2561.c`
- INA226: `components/drivers/sensor/adc/ina226/xy_ina226.c`
- MLX90614: `components/drivers/sensor/temperature/mlx90614/xy_mlx90614.c`
- VL53L1X: `components/drivers/sensor/distance/vl53l1x/xy_vl53l1x.c`
- LPS22HB: `components/drivers/sensor/pressure/lps22hb/xy_lps22hb.c`
- SGP40: `components/drivers/sensor/environment/sgp40/xy_sgp40.c`
- LTC2945: `components/drivers/sensor/adc/ltc2945/xy_ltc2945.c`
- BMI088: `components/drivers/sensor/motion/bmi088/xy_bmi088.c`
- BMI270: `components/drivers/sensor/motion/bmi270/xy_bmi270.c`
- BNO055: `components/drivers/sensor/motion/bno055/xy_bno055.c`
- AS5048B: `components/drivers/sensor/angle/as5048b/xy_as5048b.c`
- AS5600: `components/drivers/sensor/angle/as5600/xy_as5600.c`
- MAX44009: `components/drivers/sensor/light/max44009/xy_max44009.c`
- VCNL4040: `components/drivers/sensor/proximity/vcnl4040/xy_vcnl4040.c`
- QMC5883L: `components/drivers/sensor/magnetic/qmc5883l/xy_qmc5883l.c`
- AK09918: `components/drivers/sensor/magnetic/ak09918/xy_ak09918.c`
- IST8310: `components/drivers/sensor/magnetic/ist8310/xy_ist8310.c`
- BMA400: `components/drivers/sensor/motion/bma400/xy_bma400.c`
- KX023: `components/drivers/sensor/motion/kx023/xy_kx023.c`
- ADXL362: `components/drivers/sensor/motion/adxl362/xy_adxl362.c`
- LSM6DSO: `components/drivers/sensor/motion/lsm6dso/xy_lsm6dso.c`
- LSM6DSL: `components/drivers/sensor/motion/lsm6dsl/xy_lsm6dsl.c`
- LSM6DSR: `components/drivers/sensor/motion/lsm6dsr/xy_lsm6dsr.c`
- LSM9DS1: `components/drivers/sensor/motion/lsm9ds1/xy_lsm9ds1.c`
- LIS2DH12: `components/drivers/sensor/motion/lis2dh12/xy_lis2dh12.c`
- APDS9960: `components/drivers/sensor/proximity/apds9960/xy_apds9960.c`
- CCS811: `components/drivers/sensor/environment/ccs811/xy_ccs811.c`
- LIS2DW12: `components/drivers/sensor/motion/lis2dw12/xy_lis2dw12.c`

BMP390 public read/deinit operations require both the nested Device lifecycle and a live I2C
handle. Successful deinit clears that handle, so a stale outer lifecycle bit cannot authorize
Bosch SensorAPI transport callbacks. The adapter callbacks now fail closed before touching the bus
when nested transport is invalid. This hardening adds no pressure/temperature accuracy,
timing, recovery, or hardware claim.

HDC1080 and INA219 public transport operations likewise require both lifecycle layers and a live
I2C handle. Successful deinit clears the handle; missing-handle rejection performs no bus I/O and
preserves cached/caller output. This adds no measurement accuracy, timing, recovery, or hardware
claim.

SHT40 cached serial and precision operations now require the same live nested Device transport as
measurement and teardown operations. Losing either nested lifecycle state or the I2C handle rejects
the operation without exposing cached identity or changing precision state. This adds no sensor
accuracy, timing, recovery, or hardware claim.

VL53L1X cached offset/xtalk setters and last-result access now require the same live nested Device
transport as measurement operations. Losing either nested lifecycle state or the I2C handle rejects
cache mutation and stale result exposure. This adds no ranging accuracy, timing, recovery, or
hardware claim.

SGP40 cached offset, warm-up/uptime status and last-data access now require the same live nested
Device transport as measurement operations. Losing either nested lifecycle state or the I2C handle
rejects cache mutation and stale state exposure. This adds no VOC accuracy, warm-up timing,
recovery, or hardware claim.

BMI270 public register/configuration/sample/power/lifecycle operations now require a live bus
handle in addition to the driver lifecycle flag. Successful deinit clears the handle; losing it
rejects operations before I2C/SPI access and preserves caller output and cached configuration. This
adds no IMU accuracy, timing, recovery, or hardware claim.

BNO055 public register/configuration/fused/raw/status/power/lifecycle operations now require a live
bus handle in addition to the driver lifecycle flag. Successful deinit clears the handle; losing it
rejects operations before I2C access and preserves caller outputs and cached configuration. This
adds no fusion quality, IMU accuracy, timing, recovery, or hardware claim.

### LIS2DW12 migration status

The legacy LIS2DW12 API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/lis2dw12`. The owner verifies WHO_AM_I `0x44` at 7-bit address
`0x18`, applies the documented soft-reset, address-increment and low-power 100 Hz configuration,
and stages left-aligned 14-bit XYZ samples behind the nested I2C lifecycle. Initialization clears
the complete owner when Device-helper setup or any identity/config transaction fails. Public
read/deinit operations require the outer lifecycle, nested lifecycle and live I2C handle; rejected
or failed operations preserve caller/cache state, and successful teardown clears the transport.
Acceleration accuracy, low-power current, timing and hardware endurance remain `hardware-pending`.

PA122 is intentionally unsupported and retired from the active source set. The repository has no
authoritative identity/register documentation beyond a legacy fixed address and threshold heuristic;
keeping that implementation would manufacture a Device owner from an unverified protocol.

The top-level APDS9960 implementation was a weaker duplicate of
`components/sensor/sensors/sensor_apds9960.c`: both exported the same legacy factories, while the
top-level copy lacked public guards, proximity/gesture factories, FIFO-level bounds and transport
error propagation already covered by the focused Host contract. It and its byte-identical duplicate
header were removed. The manifest checker now inventories explicit top-level implementation owners
and rejects any chip name that also exists under `sensors/`; this ownership correction does not
promote APDS9960 to the canonical Device model or establish hardware evidence.

### Pandora I2C2 hardware status

The detailed current evidence is maintained in `docs/validation/i2c2-sensor-evidence.md`. SC7A22H at `0x18` is now hardware-confirmed with `WHO_AM_I=0x18`, vendor-demo configuration readback, continuous XYZ, corrected mg conversion, an approximately 1g stationary vector, manual-rotation response, and monotonically increasing DRDY EXTI edge counts on `PB8`. The vendor demo treats `(DATA_STAT & 0x03) == 0x03` as ready; the PDF bit-label conflict remains documented. HMC5883L at `0x1E` has bounded basic-chain evidence: `"H43"`, gain readback `0xE0`, `PA4` DRDY high, continuous XYZ, converted field output and manual magnetic stimulus response. BH1750 `0x23`, AHT30 `0x38`, MPU6050 `0x68`, L3G4200D `0x69`, and BME680 `0x77` remain in the joint I2C2 sample path.

The Pandora STM32L475 I2C2 target (`PB10/PB11`, 100 kHz) identifies and samples the three new
owners at `0x38`, `0x69`, and `0x77`. A reset-synchronized UART capture proved AHT30 CRC-valid
temperature/humidity frames, L3G4200D identity `0xD3` and three-axis output, and BME680 identity
`0x61` with Bosch-compensated temperature, pressure, humidity, and heater-stable gas resistance.
The BME680 Bosch SensorAPI adapter callbacks now independently validate nested Device lifecycle and
the live I2C handle before bus access, so invalid callback context fails closed.
L3G4200D internal register helpers now independently enforce the same nested lifecycle and live
handle contract, including during initialization before the outer lifecycle is published.
HMC5883L internal register helpers now also reject missing nested lifecycle or transport context
before Device I2C access, including the pre-publication initialization phase.
The AHT30 owner now additionally requires a live nested I2C handle for public read/deinit and clears
that handle after successful teardown; this lifecycle hardening does not expand hardware evidence.
This is basic-chain hardware evidence; it does not claim calibrated accuracy, environmental chamber
qualification, long-run reliability, or general I2C fault recovery.

### Pandora I2C2 Device-owner stabilization gate

The current-device-first gate is complete for AHT30, L3G4200D, BME680, BH1750, MPU6050,
HMC5883L, and SC7A22H. Each owner is root-linked through `xy_drivers`, uses the Device/I2C helper
lifecycle, has focused normal and failure contracts, passes the Host suite and PC/L4/U5 compile
gates, and remains represented by the Pandora I2C2 integration target. The latest hardening covers
nested-helper rejection, first-error propagation, output/cache atomicity, and lifecycle cleanup.

This gate authorizes the next Sensor workstream—incremental migration of legacy
`sensor_device_t` owners. It does not authorize deleting compatibility wrappers in bulk, changing
legacy public APIs without a migration contract, or claiming B2 recovery for the I2C2 devices.

## Admission and migration contract

A Sensor implementation may become canonical active product source only when all of the following
are explicit in one reviewed slice:

1. one implementation owner and one lifecycle (`xy_device_t`/typed Device adapter preferred);
2. public header and Kconfig/CMake switch ownership;
3. root target inclusion proven from a clean configure/build, not only a test-local source list;
4. focused normal/error/output-preservation/re-init Host coverage;
5. legacy wrapper marked compatibility-only when retained;
6. board evidence remains separately recorded as `hardware-pending`, B1, or B2.

The first migration candidates were SHT30, MPU6050, ADS1115, and BMP280. All four Device-model
sources now have explicit root Sensor ownership; migration must remove duplicate active ownership
rather than merely add a new copy.

### SHT30 migration status

The Device-model source is the single canonical implementation. The root-linked
`sensor_sht30.c` is now a compatibility-only legacy lifecycle wrapper: it delegates init/read and
CRC/error/output-preservation behavior to `components/drivers/sensor/temperature/sht30/xy_sht30.c`,
and `sensor_component` explicitly links that owner. The canonical driver accepts both documented
addresses through `xy_sht30_init_addr()`, while the existing two-argument initializer remains the
0x44 compatibility API. The duplicate test-local `components/sensor/src/xy_sht30.c` implementation,
header, and CTest were removed; tracked examples select the Device owner. Focused wrapper, Device,
and integration tests plus the root `sensor_component` build prove one implementation owner with
one compatibility boundary. No hardware status is upgraded.

The unreferenced `components/sensor/drivers/temperature/xy_sensor_sht30.c` prototype has also been
removed and is covered by the manifest guard, preventing a fourth SHT30 lifecycle from returning.
The canonical owner requires both nested `base.initialized` and `i2c_handle` for deinit/read;
successful deinit clears the complete device state. Existing temperature/humidity accuracy and
long-run evidence remains bounded.

### ADS1115 migration status

The Device-model source is the single canonical implementation and is explicitly linked into the
root `sensor_component`. Its public API preserves the established channel, differential, PGA/data
rate, voltage, I/O error, and output-preservation contracts. The duplicate test-local
`components/sensor/src/xy_ads1115.c` implementation and header were removed, and the existing
focused ADC/power-monitor target now compiles the Device owner directly. This remains Host/PC source
ownership evidence only; conversion accuracy, timing, recovery, and board support remain pending.

There is no root-linked `sensor_device_t` ADS1115 factory to preserve: the compatibility boundary is
the typed `xy_ads1115_*` API itself. Audit also confirmed no `components/sensor/drivers/**` ADS1115
prototype remains. All public lifecycle/read/config operations now reject a missing nested I2C
helper without I/O or public-state changes.
Successful deinit also clears the nested I2C handle, preventing stale transport reuse.

LIS2DH12 initialization now clears the complete owner when Device-helper setup or any identity/config
transaction fails. Public read/deinit operations require the outer driver lifecycle, nested Device
lifecycle, and live I2C handle; rejected or failed operations perform no unauthorized I/O and preserve
caller/cache state. This adds no acceleration accuracy, power, timing, recovery, or hardware claim.

### BH1750 migration status

The Device-model source under `components/drivers/sensor/light/bh1750` is now the single active
implementation owner. Root-linked `sensor_bh1750.c` is a compatibility-only `sensor_device_t`
wrapper that delegates init/read/deinit to the typed Device owner and preserves the legacy lux
conversion at the wrapper boundary. The unreferenced prototype
`components/sensor/drivers/light/xy_sensor_bh1750.c` was removed, preventing a fourth BH1750
protocol owner. All public operations now require both nested `base.initialized` and `i2c_handle`;
a stale lifecycle bit cannot authorize bus access.
lifecycle from returning. Focused wrapper and Device tests plus root builds prove source/lifecycle
ownership only; existing Pandora B1 remains bounded and no B2 recovery claim is added.

### MPU6050 migration status

The Device-model source is the single canonical implementation and is explicitly linked into the
root `sensor_component`. Root-linked `sensor_mpu6050.c` is now a compatibility-only
`sensor_device_t` wrapper: its accel/gyro factories preserve legacy metadata and units while
init/read/deinit delegate to the typed Device owner with explicit error mapping and output
preservation. The Device owner absorbs the richer range, calibration, converted-output and I/O
failure contracts from the former test-local implementation while preserving the existing
default-address initializer for Device consumers. The duplicate
`components/sensor/src/xy_mpu6050.c` implementation/header and the unreferenced
`components/sensor/drivers/motion/xy_sensor_mpu6050.c` fourth lifecycle have been removed. Focused
wrapper and Device tests plus root builds prove one implementation owner with one compatibility
boundary. This is Host/compile ownership evidence only; IMU accuracy, calibration quality, timing
and board recovery remain pending.

### BMP280 migration status

The Device-model source is now the single active implementation owner and carries the
Bosch integer compensation, both documented I2C addresses, init/deinit I/O propagation, cached
output preservation, and status-returning getter contracts. The root-linked `sensor_bmp280.c` is a
compatibility-only pressure/temperature wrapper that delegates lifecycle and sampling to this
owner. The unreferenced fourth-lifecycle prototype
`components/sensor/drivers/pressure/xy_sensor_bmp280.c` has been retired, and the tracked smart
hygrometer example now compiles the canonical owner through its explicit-address API instead of a
missing experimental source. The ownership policy guard prevents both stale paths from returning.
Public read/getter/deinit paths now require a live nested I2C handle, and successful deinit clears
that handle so stale lifecycle state cannot authorize access. Focused Device, legacy and heterogeneous registry tests prove Host/source ownership only; accuracy,
timing, recovery and board status remain pending.

### AHT20 migration status

The former experimental typed implementation is now the canonical Device-model owner under
`components/drivers/sensor/temperature/aht20`. The root-linked `sensor_aht20.c` temperature and
humidity factories are compatibility-only wrappers: lifecycle and sampling delegate to the typed
owner while preserving legacy metadata, units, error mapping, and caller output on failure. The
experimental source/header pair was moved rather than copied, and both focused targets now compile
the canonical source. Host/source ownership only; no AHT20 board, accuracy, timing, or recovery
status is upgraded.

All AHT20 public bus operations now require the outer lifecycle plus nested `base.initialized` and
`i2c_handle`; successful deinit clears the nested handle, so stale transport state fails closed.

SHT40 now applies the same nested transport contract to public reads and deinit, clearing its I2C
handle after successful teardown; no hardware accuracy or recovery claim is added.

### AHT10 migration status

The Device-model source under `components/drivers/sensor/temperature/aht10` is now the single
protocol implementation owner.

AHT10 public read and deinit paths also require a live nested `i2c_handle`; successful teardown
clears the handle so stale transport state cannot authorize bus access. It uses the Device I2C helper for the fixed `0x38` address, stages
both temperature and humidity before committing output/cache, preserves state on trigger/read/busy
failures, and requires both outer and nested lifecycle state. Root-linked `sensor_aht10.c` remains a
compatibility-only humidity wrapper and delegates all transport, conversion and lifecycle work to
the canonical owner. Existing Pandora AHT10 B1 evidence remains bounded to the previously observed
board path; this migration adds no new accuracy, timing, NACK-recovery or long-run claim.

### AP3216C migration status

The Device-model source under `components/drivers/sensor/light/ap3216c` now owns reset/mode
configuration and ALS, proximity and IR register decoding. Root-linked `sensor_ap3216c.c` remains
only as a compatibility wrapper for the legacy factories and contains no direct I2C protocol.
The owner also requires both nested `base.initialized` and `i2c_handle` before any public operation;
stale lifecycle state cannot authorize bus access. Existing Pandora AP3216C B1 evidence remains
bounded; this migration adds no accuracy, threshold, NACK-recovery or long-run claim.

### ICM20608 migration status

`components/drivers/sensor/motion/icm20608` now owns identity, reset,
default range/filter configuration, I2C/SPI transport dispatch and converted acceleration,
gyroscope and temperature outputs. Root-linked `sensor_icm20608.c` retains the three legacy
factories as compatibility wrappers only. Existing Pandora ICM20608 static/basic-chain evidence
remains bounded; migration does not establish dynamic response, accuracy, calibration or recovery.
Internal writes now enforce the same I2C/SPI transport validity as reads. Successful deinit clears
the active I2C handle or SPI callback/context set, preventing stale transport reuse.

### HDC1080 migration status

The former focused-test-backed typed source/header pair was moved from the experimental Sensor tree
to `components/drivers/sensor/temperature/hdc1080` and is now root-linked by `xy_drivers`. The
canonical owner enforces the fixed `0x40` address, outer+nested lifecycle checks, staged temperature
and humidity publication, and fail-closed heater controls. There is no legacy `sensor_device_t`
factory to preserve. Host/source ownership only; accuracy, timing, heater behavior and hardware
recovery remain `hardware-pending`.

### TSL2561 migration status

The former focused-test-backed typed source/header pair was moved from the experimental Sensor tree
to `components/drivers/sensor/light/tsl2561` and is now root-linked by `xy_drivers`. The canonical
owner accepts only the three documented addresses, rejects an invalid nested Device/I2C lifecycle
without I/O, and publishes channel/lux/timestamp cache only after both channel reads succeed. There
is no legacy `sensor_device_t` factory to preserve. Host/source ownership only; optical accuracy,
integration timing, saturation behavior and hardware recovery remain `hardware-pending`.

### INA226 migration status

The experimental source/header pair was replaced by a canonical INA226-only Device owner under
`components/drivers/sensor/adc/ina226`. Configuration now explicitly carries shunt resistance and
current LSB, calibration is derived with integer units, current is read from the calibrated current
register, and power uses the documented `25 * current-LSB` scale. The previous unsupported INA229
alias was removed rather than sharing INA226's incompatible register model. Measurement publication
is staged across bus, shunt, current and power reads. Public read, alert and deinit paths require
both outer and nested transport lifecycle; successful deinit clears the nested I2C handle. Host/source
ownership only; metrology, alert
thresholds and hardware recovery remain `hardware-pending`.

### SHT40 migration status

The former experimental typed implementation is now the canonical Device-model owner under
`components/drivers/sensor/temperature/sht40`. Its public API and focused lifecycle, CRC,
precision, transport-error, cache/output-preservation contracts are unchanged; the source/header
pair was moved rather than copied, and the focused target compiles the canonical source directly.
The unreferenced `components/sensor/drivers/temperature/xy_sensor_sht40.c` prototype was removed;
there is no root-linked legacy SHT40 factory to preserve. Host/source ownership only; no SHT40
board, accuracy, timing, recovery, or durability status is upgraded.

### MLX90614 migration status

The focused-test-backed MLX90614 source/header pair was moved from the experimental Sensor tree to
`components/drivers/sensor/temperature/mlx90614` and is now root-linked by `xy_drivers`. The
canonical owner keeps PEC-checked temperature and emissivity reads, fixed-address validation,
output/cache preservation, and the explicit unsupported EEPROM-write boundary. Public operations
now also require the nested Device/I2C lifecycle, and deinit clears both driver and Device readiness.
Host/source ownership only; infrared accuracy, emissivity calibration, SMBus electrical behavior,
timing, recovery, and board status remain `hardware-pending`.

### VL53L1X migration status

The focused typed source/header pair was moved into
`components/drivers/sensor/distance/vl53l1x` and is now root-linked by `xy_drivers`. Its private
callback-shaped I2C owner was replaced by a nested `xy_i2c_device_t`; 16-bit register transactions
are issued through Device write/read operations, and every operational API fails closed when the
outer or nested lifecycle is invalid. Identity, staged result/cache publication, timeout, ROI,
interrupt and calibration Host contracts remain covered. Ranging accuracy, full ST tuning data,
optical calibration and hardware recovery remain `hardware-pending`.

### LPS22HB migration status

The focused typed source/header pair was moved from the experimental Sensor tree to
`components/drivers/sensor/pressure/lps22hb` and is now root-linked by `xy_drivers`. Existing
identity, reset, pressure/temperature conversion, FIFO, interrupt, timeout, cache-preservation and
configuration-failure contracts remain covered. Every operational and configuration API now requires
the outer interface plus initialized nested I2C Device state and live bus handle; transport loss
therefore fails before I/O, delay, or configuration-cache mutation. Host/source ownership only;
accuracy, waterproofing, timing, interrupt behavior and hardware endurance remain
`hardware-pending`.

### MAX44009 migration status

The legacy MAX44009 owner is now represented by `components/drivers/sensor/light/max44009`.
The canonical owner validates the two documented 7-bit addresses (`0x4A`/`0x4B`), reads the
`0x03`/`0x04` exponent-mantissa registers, converts lux to integer milli-lux, and stages the
sample/timestamp behind nested I2C lifecycle checks. Optical accuracy and hardware endurance
remain `hardware-pending`.

### VCNL4040 migration status

The legacy VCNL4040 owner is now represented by `components/drivers/sensor/proximity/vcnl4040`.
The canonical owner uses the fixed 7-bit address `0x60`, reads the documented little-endian
proximity output at `0x08/0x09`, and stages raw proximity/timestamp publication behind nested I2C
lifecycle checks. Optical response, distance calibration and hardware endurance remain
`hardware-pending`.

### QMC5883L migration status

The legacy QMC5883L API is now backed by the canonical Device owner at
`components/drivers/sensor/magnetic/qmc5883l`. The owner verifies chip ID `0xFF`, performs the
documented reset/period/continuous-mode sequence, checks DRDY before reading little-endian XYZ,
and stages raw axes/timestamp behind nested I2C lifecycle checks. Magnetic calibration, accuracy,
interrupt behavior and hardware endurance remain `hardware-pending`.

### AK09918 migration status

The legacy AK09918 API is now backed by the canonical Device owner at
`components/drivers/sensor/magnetic/ak09918`. The owner verifies AKM WIA1/WIA2 (`0x48/0x09`),
resets through CNTL3, enters continuous 100 Hz mode through CNTL2, gates reads on ST1 DRDY and
stages little-endian XYZ/timestamp output behind nested I2C lifecycle checks. Magnetic calibration,
accuracy and hardware endurance remain `hardware-pending`.

### IST8310 migration status

The legacy IST8310 API is now backed by the canonical Device owner at
`components/drivers/sensor/magnetic/ist8310`. The owner verifies WHO_AM_I `0x10`, configures
continuous 100 Hz operation, reads little-endian XYZ data, and stages sample/timestamp publication
behind nested I2C lifecycle checks. Magnetic calibration, accuracy and hardware endurance remain
`hardware-pending`.

### BMA400 migration status

The legacy BMA400 API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/bma400`. The owner verifies CHIP_ID `0x90`, performs the
documented soft reset and low-power/±2g/25 Hz configuration, and stages little-endian XYZ raw
samples and timestamps behind nested I2C lifecycle checks. Motion accuracy, power characterization,
interrupt/FIFO behavior and hardware endurance remain `hardware-pending`.

### KX023 migration status

The legacy KX023 API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/kx023`. The owner verifies WHO_AM_I `0x15`, performs the
documented soft reset and standby/12.5 Hz/low-power configuration, and stages little-endian XYZ
raw samples and timestamps through register helpers that independently validate nested I2C
lifecycle and handle state. Invalid transport fails closed without bus access or state mutation;
successful teardown clears the nested handle. Motion accuracy, power, interrupt
behavior and hardware endurance remain `hardware-pending`.

### ADXL362 migration status

The legacy ADXL362 API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/adxl362`. The owner uses the SPI Device helper, verifies device
ID `0xAD`, configures 100 Hz/±2g measurement mode, decodes signed 12-bit XYZ data, and stages
sample/timestamp publication behind nested SPI lifecycle checks. Motion accuracy, SPI timing,
interrupt/FIFO behavior and hardware endurance remain `hardware-pending`.

### LSM6DSO migration status

The legacy LSM6DSO API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/lsm6dso`. The owner verifies WHO_AM_I `0x6C`, performs the
documented reset/I3C-disable/104 Hz/±2g/±250 dps configuration, reads staged accelerometer and
gyroscope XYZ data through register helpers that independently validate the nested I2C lifecycle,
preserves output/cache on transport failure, and clears the nested handle after successful teardown.
Invalid transport fails closed without bus access or lifecycle mutation. Motion accuracy, ODR timing,
interrupt/FIFO behavior and hardware endurance remain `hardware-pending`.

### LSM6DSL migration status

The legacy LSM6DSL API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/lsm6dsl`. The owner verifies WHO_AM_I `0x6A`, performs the
documented reset/I3C-disable/104 Hz/±2g/±250 dps configuration, reads staged accelerometer and
gyroscope XYZ data through register helpers that independently validate the nested I2C lifecycle,
preserves output/cache on transport failure, and clears the nested handle after successful teardown.
Invalid transport fails closed without bus access or lifecycle mutation. Motion accuracy, ODR timing,
interrupt/FIFO behavior and hardware endurance remain
`hardware-pending`.

### LSM6DSR migration status

The legacy LSM6DSR API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/lsm6dsr`. The owner verifies WHO_AM_I `0x69`, performs the
documented reset/I3C-disable/104 Hz/±2g/±250 dps configuration, reads staged accelerometer and
gyroscope XYZ data through register helpers that independently validate the nested I2C lifecycle,
preserves output/cache on transport failure, and clears the nested handle after successful teardown.
Invalid transport fails closed without bus access or lifecycle mutation. Motion accuracy, ODR timing,
interrupt/FIFO behavior and hardware endurance remain `hardware-pending`.

### LSM9DS1 migration status

The legacy LSM9DS1 API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/lsm9ds1`. The owner verifies IMU WHO_AM_I `0x68` and magnetometer
WHO_AM_I `0x3D` at the documented 7-bit addresses `0x6A` and `0x1C`, performs reset and 104 Hz
configuration, reads staged accelerometer, gyroscope and magnetometer XYZ data through two nested
I2C lifecycles, and preserves output/cache on transport failure. Motion and magnetic accuracy,
timing, interrupt behavior and hardware endurance remain `hardware-pending`.

### LIS2DH12 migration status

The legacy LIS2DH12 API is now backed by the canonical Device owner at
`components/drivers/sensor/motion/lis2dh12`. The owner verifies WHO_AM_I `0x33` at the documented
7-bit address `0x18`, configures 10 Hz/XYZ/±2g high-resolution mode and temperature enable, and
publishes staged left-aligned 12-bit XYZ samples through the nested I2C lifecycle. Acceleration
accuracy, low-power behavior, timing and hardware endurance remain `hardware-pending`.

### APDS9960 migration status

The legacy APDS9960 API is now backed by the canonical Device owner at
`components/drivers/sensor/proximity/apds9960`. The owner verifies the documented ID values
`0xAB`/`0x9C` at 7-bit address `0x39`, enables the requested sensor functions, stages RGBC and
proximity samples, and bounds gesture FIFO reads to the documented 32-entry limit behind the nested
I2C lifecycle. Optical response, gesture classification, timing and hardware endurance remain
`hardware-pending`.

### CCS811 migration status

The legacy CCS811 API is now backed by the canonical Device owner at
`components/drivers/sensor/environment/ccs811`. The owner verifies HW_ID `0x81` at 7-bit address
`0x5A`, starts the application, configures one-second measurement mode, gates reads on DATA_READY,
and stages eCO2/TVOC results behind the nested I2C lifecycle. Gas response, baseline behavior,
accuracy, warm-up and hardware endurance remain `hardware-pending`.

### AS5600 migration status

The legacy AS5600 owner is now represented by `components/drivers/sensor/angle/as5600`. The
canonical owner uses fixed 7-bit address `0x36`, reads the documented `0x0E/0x0F` raw angle,
masks the reserved upper bits, and stages output/timestamp publication behind nested I2C
lifecycle checks. Magnet installation, angle accuracy and hardware endurance remain
`hardware-pending`.

### BMI088 migration status

The focused-test-backed BMI088 source/header pair moved from the experimental Sensor tree to
`components/drivers/sensor/motion/bmi088` and is now root-linked by `xy_drivers`. The canonical
owner retains the explicit dual-chip-select SPI seam and its identity, configuration, staged raw
publication, physical conversion, calibration, range-cache atomicity, and lifecycle contracts.
Public operations, including the cached calibration setter, now require initialized state and a live
SPI handle, reject detached transports without I/O or state publication, and clear the transport
pointer only after successful deinitialization. Host/source ownership only; SPI electrical behavior,
timing, interrupts, FIFO, calibration quality, motion accuracy and hardware recovery remain
`hardware-pending`.

### AS5048B migration status

The legacy AS5048B I2C owner is now represented by the canonical Device owner at
`components/drivers/sensor/angle/as5048b`. The owner uses the documented `0xFE/0xFF` 14-bit
angle transaction, staged output/timestamp publication, fixed 7-bit address `0x40`, and nested
I2C lifecycle checks. Angle accuracy, magnet installation and hardware endurance remain
`hardware-pending`.

### BNO055 migration status

The focused-test-backed BNO055 source/header pair moved from the experimental Sensor tree to
`components/drivers/sensor/motion/bno055` and is now root-linked by `xy_drivers`. The canonical
owner retains its explicit I2C transport and unsupported-UART boundary, plus initialization,
mode/power/unit configuration, raw/fused output, axis-remap, failure-preservation, and lifecycle
contracts. Host/source ownership only; fusion quality, calibration, timing, interrupt behavior,
motion accuracy, UART transport, and hardware recovery remain `hardware-pending`.

## Guard and update rule

The manifest checker also scans the legacy root for small owners that publish a literal scalar
value while issuing no recognized I2C, SPI, ADC, HAL, typed-owner, or register-helper call. This
fails the Host gate instead of allowing another constant-output/zero-transport placeholder to be
counted as an active driver. The check is intentionally narrow: derived constants inside real
transport drivers remain valid and substantive protocol review is still required.

The current 34 canonical names have exactly thirteen approved legacy filename overlaps:
`sht30`, `mpu6050`, `bmp280`, `bh1750`, `aht20`, `aht10`, `ap3216c`, `icm20608`, `as5600`, `max44009`, `vcnl4040`, `qmc5883l`, and `ak09918`. Each overlap is a compatibility wrapper
documented above, not an implementation owner. The other twenty-one canonical owners (`ads1115`, `aht30`,
`bme680`, `hmc5883l`, `l3g4200d`, `sc7a22`, `sht40`, `ina219`, `bmp390`, `hdc1080`, `tsl2561`, `ina226`, `mlx90614`, `vl53l1x`, `lps22hb`, `sgp40`, `ltc2945`, `bmi088`, `bmi270`, `bno055`, and `as5048b`) have no legacy-root counterpart. No canonical name overlaps
the 6 experimental `src/xy_*` files or the remaining `xy_sensor_*` prototypes.

HDC1080 was first retained as the stronger test-only owner while an unchecked prototype was
removed. It has now been promoted to the canonical Device root as documented above; the stale
experimental and prototype paths remain forbidden.

TSL2561 was first retained as the stronger test-only owner while an unchecked prototype was removed.
It has now been promoted to the canonical Device root as documented above; both stale paths remain
forbidden. This source migration does not claim hardware, accuracy, timing, or recovery evidence.

SGP40 first replaced a constant-output legacy stub with a substantive test-only typed owner. That
owner is now promoted to `components/drivers/sensor/environment/sgp40`, uses nested Device I2C
transactions, and keeps feature-set/serial/self-test/CRC/measurement contracts. This establishes no
VOC algorithm accuracy, compensation behavior, burn-in, timing, recovery or hardware evidence.

LTC2945 is rebuilt in `components/drivers/sensor/adc/ltc2945` from the Analog Devices register and
scaling contract. The previous experimental source incorrectly assigned STATUS/POWER/VIN/SENSE
registers and exposed nonexistent charge/energy accumulators. The canonical owner now publishes
VIN, SENSE, current, power, status and fault only after all transactions succeed. Address strapping,
metrology, alert thresholds and hardware recovery remain `hardware-pending`.

SGP30 had only a legacy-root placeholder that returned a fixed `100.0` gas value without issuing an
I2C command or implementing the documented eCO2/TVOC measurement protocol. It had no second
substantive owner or checked consumer outside the grouped stub test, so the source/header and their
placeholder assertions were removed instead of preserving a false product capability. The manifest
guard prevents this lifecycle from returning. SGP30 now has no active implementation and remains a
future Device-model driver candidate; this cleanup establishes no gas measurement or hardware
evidence.

ENS160 had the same false-owner shape: init/deinit performed no device operation and read always
published `100.0` without checking part ID, validity status, AQI, TVOC, or eCO2 registers. With no
second implementation or consumer outside the grouped stub test, its source/header and fake
measurement assertions were removed. ENS160 is unsupported until a real Device-model owner exists.

IM69D was also a false I2C owner: it assigned a fabricated `0x30` address to a digital microphone,
performed no transport or sample acquisition, and always published `0.0`. Its source/header and
grouped stub assertions were removed. IM69D is unsupported until a correctly modelled audio/PDM or
I2S component exists; this cleanup establishes no acoustic or hardware evidence.

MAX30102 completed the grouped-stub retirement. Its init/deinit performed no register operation and
read always published a fabricated `72.0` heart rate without part-ID, FIFO, LED, sampling, signal
quality, or algorithm contracts. The source/header and now-empty grouped stub test were removed.
MAX30102 is unsupported until a real Device owner and explicitly bounded biosignal algorithm exist.

AEAT-8800 was a false SPI owner: its private state contained no chip-select or transport contract,
init/deinit performed no operation, and read always published `0.0` degrees. The source/header and
its fake focused assertions were removed while the independent MLX90393 test target remains. AEAT-
8800 is unsupported until a real SPI/SSI Device owner is implemented.

MLX90393 was the remaining false angle owner. It declared an unused raw HAL read symbol but init,
read, and deinit performed no transport; read always published `0.0` and did not model the chip's
magnetic field outputs or an angle derivation contract. Its source/header and now-empty focused
target were removed. MLX90393 is unsupported until a real Device owner and explicit angle model exist.

The legacy `IIS2ICLP` owner was also retired after protocol audit. The source modeled an I2C/SPI
accelerometer with register map `0x0F/0x20/0x22/0x28`, identity `0x6D`, and address `0x23`, but ST's
IIS2ICLX/IIS2ICL family is an SPI-only inclinometer whose identity and control/output register map do
not match that contract. The existing fake-register test therefore characterized repository fiction,
not a supported chip protocol. The source/header and its focused assertions were removed instead of
preserving a misidentified active driver. IIS2ICLX remains unsupported until a datasheet-grounded
Device owner is implemented; no board, accuracy, transport, or product capability is claimed.

DMP6100 was retired after provenance audit found no repository datasheet, vendor source, board
consumer, or independent implementation supporting its declared `0x12` address, `0x0C` control,
`0x18` data base, fixed `0x56` configuration, or unconditional raw×16 conversion. Although its fake
I2C tests exercised error propagation, they only mirrored those ungrounded constants. DMP6100 is
unsupported until a traceable datasheet and Device-model contract are available.

CMS and HS-ADS1100 were retired together as template clones. Their sources were 94% text-similar
and used the same `0x18` address, `0x20=0x57` initialization, `0x28..0x2D` raw layout and direct raw-
to-mg publication despite different vendor/model labels; neither init read the WHO_AM_I constants
declared in its header. No datasheet, vendor source, board consumer or independent implementation
grounded either contract. Both remain unsupported pending traceable documentation and Device owners.

QMA6100 closed the legacy stub-accelerometer target. Repository-only review found contradictory
state semantics: init cached physical range `2`, `set_range()` cached register enums `0..3`, and
read interpreted both domains as physical g before multiplying raw values by `16/31/63/127` mg.
Thus the same 4G enum (`1`) fell into the 16G/default scale path, while tests only repeated the
implementation's constants. With no local datasheet/vendor source/board consumer, the owner and
now-empty grouped target were retired pending a traceable Device-model implementation.

The former BMP390 owner was retired because it published uncompensated pressure ADC counts as hectopascals using
`raw / 256`. A BMP390 measurement requires temperature and pressure calibration coefficients from
NVM plus Bosch compensation; the legacy owner read no calibration registers, discarded the three
temperature bytes it fetched, and even aliased OSR/ODR constants onto pressure-data addresses
`0x1C/0x1D`. Its fake-I2C tests therefore validated fabricated engineering units rather than a
pressure contract. BMP390 has now been rebuilt as a canonical Device owner using Bosch BMP3
SensorAPI `bmp3_v2.0.6` at commit `db4cf8e4140c593b8c3d85f8c6c07335c7ffa9dc` (BSD-3-Clause),
with pinned file hashes and an unmodified imported source copy. The XinYi adapter owns I2C lifecycle,
strict BMP390 identity, default pressure/temperature configuration, transport-error mapping, and
atomic compensated temperature/pressure publication. Focused Host tests exercise identity,
calibration/configuration, a pinned compensated sample, failure preservation and deinit recovery.
This restores root source ownership only; board accuracy, timing, interrupt/FIFO behavior and B1/B2
remain `hardware-pending`.

INA219 was retired because its init performed no configuration or calibration transaction, while
read treated the signed shunt-voltage register (`0x01`) as current and multiplied it by a fixed
`0.1`. INA219 current register scaling depends on the programmed calibration/current-LSB and board
shunt resistance; none existed in private state or the factory contract. The focused test merely
repeated the fabricated conversion. Restoration requires explicit shunt/calibration ownership and
separate shunt-voltage, bus-voltage, current and power transaction contracts.

GD30DF was retired after the same protocol-provenance audit. It appeared only in the original bulk
sensor import, with no datasheet, vendor source, board consumer, or independent implementation in
the repository. Its `0x18` address, `0x20=0x57` control write, and `0x28..0x2D` output layout mirror
the LIS2DH-family register shape, while the declared `0x1E` identity was read but never validated and
the raw words were published directly as milli-g. The fake-I2C test only repeated those constants.
GD30DF remains unsupported pending traceable documentation and a Device-model owner.

The AS5048 angle owner is explicitly bounded to the I2C AS5048B variant. Its former Host contract
and implementation decoded angle registers `0xFE/0xFF` as a little-endian word, but the AS5048B
serial contract places angle bits `[13:6]` in `0xFE` and `[5:0]` in `0xFF`. The AS5048A is SPI-only
and is not represented by this owner. Public address/register constants and model identity now say
AS5048B, and the decoder follows that byte layout while preserving caller output on read failure.
This is protocol/Host evidence only; diagnostics, magnetic accuracy, timing, and hardware remain
pending.

VL53L1X follows the same bounded retirement rule. The removed legacy owner only reset register
`0x2D` through an 8-bit register helper and read two bytes from `0x6E`; it had no checked consumer
outside its dedicated test. The retained `components/sensor/src/xy_vl53l1x.c` implementation has
the substantive device identity, 16-bit register addressing, configuration, measurement, cache,
calibration, interrupt and failure contracts. It remains `experimental-test-only`: this cleanup
does not promote it into the canonical Device root or establish ranging accuracy or hardware
evidence.

INA226 first retained the stronger test-only owner after its unchecked prototype was removed. It has
now been rebuilt and promoted to the canonical Device root as documented above; stale experimental
and prototype paths remain forbidden.

BQ25620 follows the same retirement rule with an additional product boundary: the focused
sensor-side `components/sensor/src/xy_bq25620.c` contract remains `experimental-test-only`, while
the unreferenced `components/sensor/drivers/power/xy_sensor_bq25620.c` prototype was removed. That
prototype published fixed example voltage/current/SOC values after reading only charge status and
had no checked root consumer. The standalone canonical charger owner remains
`components/charger/src/xy_bq25620.c`; Pandora has no charger IC, so this cleanup does not promote
charger hardware, battery state, safety, or recovery evidence. The manifest guard prevents the
prototype lifecycle from returning.

MAX17043 also had an unreferenced `xy_sensor_*` singleton prototype beside two better-defined
owners: the focused sensor-side `components/sensor/src/xy_max17043.c` contract and the standalone
Fuel Gauge driver `components/fuel_gauge/drivers/xy_fg_max17043.c`. The prototype ignored its
configuration-write result, had no checked CMake consumer, and duplicated fuel-gauge ownership, so
it was removed and guarded against return. The sensor-side source remains
`experimental-test-only`; the standalone Fuel Gauge component remains the product-facing owner.
This cleanup does not promote MAX17043 hardware, accuracy, alert, battery-state, or recovery
evidence.

ADXL362 had an unreferenced `xy_sensor_*` singleton prototype beside the root-linked legacy owner
`components/sensor/sensors/sensor_adxl362.c`. The prototype's SPI helpers were placeholders that
never called the HAL, its sample path consumed an uninitialized stack buffer, and no checked CMake
consumer referenced its registration entry point. It was removed and is guarded against return;
the existing legacy owner and its focused low-power-accelerometer contract remain unchanged. This
cleanup does not promote ADXL362 hardware, motion accuracy, SPI transport, interrupt, or recovery
evidence.

BME280 had an unreferenced `xy_sensor_*` singleton prototype without a matching public header or
checked CMake consumer. Its calibration reader ignored six I2C failures and reused bytes across
different humidity coefficients, while initialization ignored reset/configuration write failures.
It was removed and is guarded against return. The root-linked legacy `sensor_bme280.c` owner and its
existing Sensor contracts remain unchanged. This cleanup does not promote BME280 hardware,
environmental accuracy, calibration, transport, or recovery evidence.

ICM20608 also had an unreferenced `xy_sensor_*` singleton prototype beside the root-linked legacy
owner used by the focused Host target and Pandora board image. The prototype had no public header or
checked CMake consumer, expected the wrong `WHO_AM_I` value (`0xAF` instead of the observed `0xAE`),
ignored every configuration-write failure, and exposed no deinit path. It was removed and is guarded
against return. The existing `sensor_icm20608.c` owner and its bounded Pandora evidence remain
unchanged; this cleanup does not promote dynamic response, calibration, transport recovery, or
long-run hardware evidence.

`tests/unit/sensor/check_sensor_active_source_manifest.py` fails when source counts or root ownership
shape change without this manifest and its CTest being updated. Any addition, deletion, root-source
selection change, or lifecycle decision must update this file, the Sprint tracker, and the component
evidence matrix in the same path-limited slice.
The canonical BME680 owner now requires both nested `base.initialized` and `i2c_handle` for
deinit/read, and clears the nested handle on successful deinit. A stale lifecycle bit cannot
authorize bus access; existing environmental accuracy and endurance evidence remains bounded.
The MLX90614 owner clears the nested I2C handle together with lifecycle state on successful
deinit, preventing stale transport reuse. Existing temperature accuracy, PEC robustness and
long-run evidence remain bounded.
The HMC5883L owner now requires a valid nested I2C handle for every public operation and clears
that handle on successful deinit; stale outer lifecycle state cannot authorize bus access.
The TSL2561 owner also clears its nested I2C handle on successful deinit, preserving the
fail-closed ready contract for subsequent operations.
The MPU6050 owner applies the same nested-handle guard to public operations and clears the handle
on successful deinit; existing motion accuracy and calibration evidence remains bounded.
Successful deinit clears the nested I2C handle as well as lifecycle state, preventing stale
transport reuse; existing board evidence remains bounded.
The canonical owner now requires both nested `base.initialized` and `i2c_handle` throughout its
public and internal bus paths. Invalid nested transport fails closed without bus access; successful
deinit still clears the complete nested helper. Hardware accuracy and recovery remain pending.
