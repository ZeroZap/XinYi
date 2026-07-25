# XinYi Sensor Tail Host Coverage Closure — 2026-07-25

## Scope

This note closes the current design loop for the legacy tail drivers under
`components/sensor/sensors/`. The goal is to keep future scheduled slices from
re-scanning the same tail set as if it were still unwired.

Inputs checked in this slice:

- `components/sensor/sensors/sensor_*.c`
- `tests/unit/sensor/test_*.c`
- `tests/unit/CMakeLists.txt`
- `docs/design/unit-test-inventory.md`

## Current coverage shape

All 55 legacy `sensor_*.c` files currently have a corresponding host CTest
entry, either as a dedicated target or as part of a grouped legacy target.
Grouped targets are intentional where drivers share the same bus/fixture shape.

Representative tail groups:

| CTest name | Unit file | Legacy drivers guarded |
| --- | --- | --- |
| `sensor_magnetic_i2c_sensors` | `tests/unit/sensor/test_magnetic_i2c_sensors.c` | QMC5883L, IST8310, AK09918, CMM905 |
| `sensor_low_power_accel_sensors` | `tests/unit/sensor/test_low_power_accel_sensors.c` | BMA400, KX023, ADXL362, LIS2DW12, IIS2ICLP |
| `sensor_stub_accel_sensors` | `tests/unit/sensor/test_stub_accel_sensors.c` | DMP6100, CMS, GD30DF, HS-ADS1100, QMA6100 |
| `sensor_stub_i2c_sensors` | `tests/unit/sensor/test_stub_i2c_sensors.c` | SGP30, SGP40, ENS160, IM69D, MAX30102 |
| `sensor_st_silan_accel_sensors` | `tests/unit/sensor/test_st_silan_accel_sensors.c` | LIS2DH12, LIS2DW12, SC7A20, Silan SC7A20 |
| `sensor_lsm6_imu_sensors` | `tests/unit/sensor/test_lsm6_imu_sensors.c` | LSM6DSL, LSM6DSO, LSM6DSR |
| `sensor_light_uv_sensors` | `tests/unit/sensor/test_light_uv_sensors.c` | MAX44009, GUVA-S12SD |
| `sensor_env_i2c_sensors` | `tests/unit/sensor/test_env_i2c_sensors.c` | AHT10, BMP390 |
| `sensor_analog_misc_sensors` | `tests/unit/sensor/test_analog_misc_sensors.c` | ACS712, FSR, MG811 |
| `sensor_mq_gas_sensors` | `tests/unit/sensor/test_mq_gas_sensors.c` | MQ3, MQ7, MQ135 |
| `sensor_angle_encoders` | `tests/unit/sensor/test_angle_encoders.c` | AS5600, AS5048 |
| `sensor_magnetic_angle_sensors` | `tests/unit/sensor/test_magnetic_angle_sensors.c` | MLX90393, AEAT-8800 |
| `sensor_proximity_sensors` | `tests/unit/sensor/test_proximity_sensors.c` | PA122, VCNL4040 |

Dedicated legacy tail targets include INA219, GPS, LSM9DS1, APDS9960,
AP3216C, CCS811, VL53L0X, BMP280, ICM20608, BH1750 legacy, and SHT30 legacy.

## Design conclusion

The low-risk “add another missing legacy sensor host target” stream is now at a
closure point. Future sensor work should avoid blindly adding more tail targets
and instead choose one of these narrower follow-up modes:

1. **Contract hardening inside an existing target** — add missing failure,
   output-preservation, or cache-preservation assertions to a known CTest.
2. **Inventory/doc sync** — update `docs/design/unit-test-inventory.md` only
   when target names or coverage grouping actually changes.
3. **Public API cleanup proposal** — if a legacy driver exposes behavior that
   conflicts with the newer `components/sensor/src/xy_*` driver family, write a
   proposal before implementation.

## Next recommended slice

Prefer `sensor_magnetic_i2c_sensors` hardening only if a concrete MM905/CMM905
hardware contract is confirmed. Without a datasheet-backed status/WHOAMI
contract, do not invent a status-read sequence just because `CMM905_REG_STATUS`
exists in the header.

If no new sensor-specific contract is available, move the continuous component
loop to the next low-risk non-sensor backlog item, such as PID or a small Net
protocol documentation/test slice.

## Verification notes

This slice intentionally changes documentation only. Verification should still
run the focused tail target plus the full unit suite and whitespace gate:

```bash
cmake --build build/tests/unit --target test_magnetic_i2c_sensors -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^sensor_magnetic_i2c_sensors$'
make test-unit
git diff --check
```
