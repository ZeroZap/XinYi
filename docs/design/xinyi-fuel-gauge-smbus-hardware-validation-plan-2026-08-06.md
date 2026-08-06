# XinYi Fuel Gauge SMBus Hardware Validation Plan

**Date**: 2026-08-06  
**Status**: proposal / pending real hardware evidence  
**Scope**: `components/fuel_gauge` SMBus/I2C driver behavior, especially BQ40Z50-style packs during discharge.

## Background

Fuel Gauge host CTests now cover standalone core behavior and the driver contracts for BQ27Z746, BQ40Z50, MAX17043, and BQ27Z561. The host fakes verify API guards, cached snapshots, output preservation, transient NACK retry, and alert-threshold cache behavior, but they cannot prove real SMBus electrical timing.

The remaining high-value gap is board-level validation of SMBus clock stretching, discharge-time transient NACKs, and real gauge/modem-equivalent bus recovery behavior. Until that evidence exists, host coverage should be treated as contract coverage rather than hardware qualification.

## Default Policy

- Keep `components/fuel_gauge` host-tested and buildable, but do not claim SMBus hardware qualification from host fakes alone.
- Do not move Fuel Gauge back under PM or Sensor while this validation is pending; it remains a standalone component line.
- Do not add vendor-SDK-specific I2C calls to chip drivers as a shortcut. Real board wiring belongs in HAL/board/project smoke layers.
- Hardware validation records must use real board logs or captured bus traces, not synthetic fake-I2C output.

## Required Evidence Levels

| Level | Meaning | Acceptable evidence |
| --- | --- | --- |
| `pending` | No real board run yet | This proposal plus host CTest results |
| `compile-only` | Firmware/library compiles for target MCU | `make HAL_PLATFORM=STM32U5 -j$(nproc)` or equivalent |
| `hardware-failed` | Board run attempted and failed | UART/log excerpt, I2C/SMBus error counters, failure conditions |
| `hardware-passed-smoke` | One board/gauge can init/fetch/read status | Real log with device type, voltage/current/SOC, retry counters |
| `hardware-passed-stress` | Discharge/load or long-loop run is stable | Timestamped loop log, NACK/retry counts, no stale-snapshot violation |
| `hardware-passed-trace` | Electrical timing is inspected | Logic-analyzer trace showing clock stretching/PEC/retry behavior |

## Minimum Board Smoke Flow

1. Build target firmware with Fuel Gauge enabled through normal Kconfig/CMake flow.
2. Boot board with known battery pack/gauge wiring and record:
   - board/project name;
   - gauge chip and I2C/SMBus address;
   - pull-up voltage and approximate bus speed;
   - whether clock stretching is expected/enabled.
3. Run a simple init/fetch loop:
   - register device;
   - call `xy_fuel_gauge_init()`;
   - call `xy_fuel_gauge_fetch()` at a bounded interval;
   - read voltage/current/SOC/SOH/status helpers;
   - report error code and retry count for every failed read.
4. Confirm failed reads preserve the previous committed snapshot:
   - no timestamp advance on failed `fetch()`;
   - cached voltage/current/SOC/status helper values remain the last successful values;
   - caller output parameters are not overwritten on failed direct helper calls.
5. Record whether status helpers are cache-only or direct-read helpers:
   - `xy_fuel_gauge_bq40z50_get_balance_status()` is cache-only;
   - `xy_fuel_gauge_bq40z50_read_balance_status()` performs a bounded direct read without updating the fetch snapshot.

## Stress/Discharge Flow

Run the same loop under a real discharge load or charger transition and capture:

- total fetch attempts;
- total successful fetches;
- total failed fetches;
- per-register transient retry counts if available;
- maximum consecutive NACK/read failures;
- whether the snapshot/timestamp contract held after failures;
- whether the bus recovered without power-cycling the board.

## Suggested Validation Record Template

Create a record under `docs/validation/` when real hardware is available, for example:

```text
docs/validation/xinyi-fuel-gauge-smbus-hardware-validation-YYYY-MM-DD.md
```

The record should include:

```markdown
# XinYi Fuel Gauge SMBus Hardware Validation Record

- Status: pending | compile-only | hardware-failed | hardware-passed-smoke | hardware-passed-stress | hardware-passed-trace
- Board/project:
- MCU/platform:
- Fuel gauge chip:
- Battery pack:
- I2C/SMBus address:
- Bus speed:
- Pull-ups / level shifting:
- Firmware commit:
- Build command + result:
- Hardware command/procedure:
- Init/fetch log excerpt:
- Retry/NACK counters:
- Snapshot-preservation evidence:
- Known failures / follow-up:
```

## Host Tests That Guard the Intended Contract

Current host gates that should pass before any hardware run:

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_fg_bq40z50 test_fg_bq27z746 test_fg_bq27z561 test_fg_max17043 -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(fg_bq40z50|fg_bq27z746|fg_bq27z561|fg_max17043)$'
make test-unit
git diff --check
```

## Non-goals For This Slice

- No HAL I2C API redesign.
- No Fuel Gauge/PM/Sensor directory migration.
- No hardware-threshold programming implementation without board evidence.
- No claim that host fake retry tests prove SMBus clock stretching on real hardware.
