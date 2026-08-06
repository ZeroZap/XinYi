# XinYi Net LTE Hardware Validation Record Template

**Date:** 2026-08-06  
**Status:** Template / no modem result recorded  
**Scope:** real-board validation record for default-off LTE HAL UART binding  
**Related design:** `docs/design/xinyi-net-lte-hardware-validation-plan-2026-08-06.md`

## Purpose

This document is the required evidence format before LTE can move beyond direct opt-in. It intentionally separates real modem evidence from compile-only and host-fake results so the project does not accidentally treat adapter tests as board validation.

Do not fill success fields with assumed values. If hardware is unavailable, leave the result as `pending` and record the blocker.

## Run identity

| Field | Value |
| --- | --- |
| Operator | pending |
| Git commit under test | pending |
| Board / revision | pending |
| MCU / build target | pending |
| Modem model | pending |
| Modem firmware | pending |
| SIM/network provider | pending |
| Validation location | pending |

## Firmware/software under test

| Item | Required value |
| --- | --- |
| `XY_NET_ENABLE_LTE` default | must remain `0` unless explicitly overridden for the smoke |
| LTE binding used | `xy_lte_hal_uart_adapter` |
| LTE core entrypoints exercised | at minimum `xy_lte_check()`, `xy_lte_check_sim()`, `xy_lte_get_signal_quality()` |
| Host guard status | `test_lte`, `test_lte_uart_adapter`, `test_lte_hal_uart_adapter`, `test_net_feature_gated_umbrella`, `make test-unit` |
| Target compile status | STM32U5 build/probe result and log pointer |

## Board wiring and UART configuration

| Field | Value |
| --- | --- |
| UART instance | pending |
| TX pin / mux | pending |
| RX pin / mux | pending |
| RTS/CTS pins and mode | pending (`none` is acceptable only if observed stable) |
| Baudrate | pending |
| Data bits / parity / stop bits | pending |
| RX buffer size | pending |
| Adapter default timeout | pending |

## Modem power/reset sequence

| Step | GPIO / action | Active level | Delay | Observed result |
| --- | --- | --- | --- | --- |
| Regulator / enable | pending | pending | pending | pending |
| PWRKEY | pending | pending | pending | pending |
| RESET | pending | pending | pending | pending |
| Boot wait before first `AT` | n/a | n/a | pending | pending |

## Required command transcript

Record exact bytes or sanitized logs for each step. Include timeout and retry count; do not record only a final boolean.

### 1. Modem absent / powered-off negative check

| Field | Value |
| --- | --- |
| Setup | pending |
| API call | `xy_lte_check()` through HAL UART adapter |
| Expected | timeout or transport error, no crash |
| Actual | pending |
| Raw TX | pending |
| Raw RX | pending |
| Retry count | pending |

### 2. First `AT` response

| Field | Value |
| --- | --- |
| API call | `xy_lte_check()` |
| Expected | `XY_LTE_OK` with `OK` response |
| Actual | pending |
| Raw TX | pending |
| Raw RX | pending |
| Timeout / retries | pending |

### 3. SIM query

| Field | Value |
| --- | --- |
| API call | `xy_lte_check_sim()` |
| Expected | valid SIM-ready or documented SIM error |
| Actual | pending |
| Raw AT response | pending |

### 4. Signal quality

| Field | Value |
| --- | --- |
| API call | `xy_lte_get_signal_quality()` |
| Expected | RSSI/BER parsed from modem response |
| Actual | pending |
| Raw AT response | pending |
| RSSI / BER mapping | pending |

### 5. Attach/PDP boundary

| Field | Value |
| --- | --- |
| Attach status API | pending |
| PDP activation attempted? | pending / intentionally skipped |
| Raw response | pending |
| Reason if skipped | pending |

## Result classification

Choose exactly one:

- `pending`: no real modem run has been performed.
- `compile-only`: target build/probe passed, but no board UART/modem evidence exists.
- `hardware-failed`: real board run failed; blocker and raw logs are captured.
- `hardware-passed-basic`: real board run proved AT, SIM, signal, and modem-absent timeout behavior.
- `hardware-passed-attach`: basic validation plus attach/PDP behavior proved.

Current result: `pending`

## Blockers / notes

- pending

## Enablement decision

LTE remains default-off and direct-opt-in until this record reaches at least `hardware-passed-basic` and the flow-control choice is documented with reproducible logs.
