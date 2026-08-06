# XinYi Fuel Gauge SMBus Hardware Validation Record Template

**Date:** 2026-08-06  
**Status:** Template / no SMBus hardware result recorded  
**Scope:** real-board validation record for standalone `components/fuel_gauge` SMBus/I2C drivers  
**Related design:** `docs/design/xinyi-fuel-gauge-smbus-hardware-validation-plan-2026-08-06.md`

## Purpose

This document is the required evidence format before Fuel Gauge SMBus/I2C behavior is described as hardware-qualified. It intentionally separates host fake-I2C coverage from real board evidence so retry, clock-stretching, discharge-time NACK, and snapshot-preservation behavior are not inferred from synthetic tests.

Do not fill success fields with assumed values. If hardware is unavailable, leave the result as `pending` and record the blocker.

## Run identity

| Field | Value |
| --- | --- |
| Operator | pending |
| Git commit under test | pending |
| Board / revision | pending |
| MCU / build target | pending |
| Fuel gauge chip | pending |
| Battery pack | pending |
| Validation location | pending |

## Firmware/software under test

| Item | Required value |
| --- | --- |
| Fuel Gauge component line | standalone `components/fuel_gauge`, not PM/Sensor-local |
| Driver under test | `bq40z50`, `bq27z746`, `bq27z561`, `max17043`, or documented board-specific driver |
| Host guard status | `fg_bq40z50`, `fg_bq27z746`, `fg_bq27z561`, `fg_max17043`, `fuel_gauge_core`, `make test-unit` |
| Target compile status | STM32U5 or board-specific build/probe result and log pointer |
| Hardware threshold programming | pending / intentionally not exercised / exercised with raw logs |

## Board wiring and bus configuration

| Field | Value |
| --- | --- |
| I2C/SMBus instance | pending |
| SDA pin / mux | pending |
| SCL pin / mux | pending |
| Gauge address | pending |
| Bus speed | pending |
| Pull-up voltage / resistance | pending |
| Level shifter present? | pending |
| Clock stretching expected/enabled? | pending |
| PEC required/enabled? | pending |

## Required command transcript

Record exact logs or sanitized traces for each step. Include return codes, retry/NACK counts, and timestamp behavior; do not record only a final boolean.

### 1. Init/register smoke

| Field | Value |
| --- | --- |
| Setup | pending |
| API sequence | `xy_fuel_gauge_register()` -> `xy_fuel_gauge_init()` |
| Expected | success for wired gauge, documented error for absent gauge |
| Actual | pending |
| Raw log / trace | pending |
| Retry/NACK counters | pending |

### 2. First successful fetch snapshot

| Field | Value |
| --- | --- |
| API sequence | `xy_fuel_gauge_fetch()` plus voltage/current/SOC/status helpers |
| Expected | timestamp advances only after complete successful fetch |
| Actual | pending |
| Voltage/current/SOC/SOH | pending |
| Status/balance flags | pending |
| Raw log / trace | pending |

### 3. Transient read failure / snapshot preservation

| Field | Value |
| --- | --- |
| Setup | pending |
| API sequence | forced/observed failed `xy_fuel_gauge_fetch()` after one success |
| Expected | previous committed snapshot and timestamp are preserved |
| Actual | pending |
| Caller output preservation evidence | pending |
| Retry/NACK counters | pending |

### 4. Discharge/load or charger-transition loop

| Field | Value |
| --- | --- |
| Load condition | pending |
| Duration / sample interval | pending |
| Total fetch attempts | pending |
| Successful fetches | pending |
| Failed fetches | pending |
| Maximum consecutive failures | pending |
| Bus recovered without power-cycle? | pending |
| Snapshot contract held? | pending |

### 5. Logic analyzer / timing trace (if available)

| Field | Value |
| --- | --- |
| Trace tool / sample rate | pending |
| Clock stretching observed? | pending |
| PEC bytes observed? | pending |
| Retry transaction pattern | pending |
| Trace artifact path | pending |

## Result classification

Choose exactly one:

- `pending`: no real board run has been performed.
- `compile-only`: target build/probe passed, but no gauge bus evidence exists.
- `hardware-failed`: real board run failed; blocker and raw logs are captured.
- `hardware-passed-smoke`: init/fetch/basic helpers passed on a real board/gauge.
- `hardware-passed-stress`: smoke plus discharge/load or charger-transition loop passed.
- `hardware-passed-trace`: stress/smoke plus electrical timing trace reviewed.

Current result: `pending`

## Blockers / notes

- pending

## Enablement decision

Fuel Gauge remains host-guarded but not SMBus hardware-qualified until this record reaches at least `hardware-passed-smoke`. Clock stretching, discharge-time transient NACK behavior, and hardware alert-threshold programming must not be claimed from fake-I2C host tests alone.
