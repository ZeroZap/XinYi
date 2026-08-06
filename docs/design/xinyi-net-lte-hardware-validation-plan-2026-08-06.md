# XinYi Net LTE Hardware Validation Plan

**Date:** 2026-08-06
**Status:** Proposal / hardware execution pending
**Scope:** LTE modem validation after host-guarded UART adapters
**Decision type:** validation plan only; no `XY_NET_ENABLE_LTE` default change

## Background

LTE now has three software guardrails:

1. `xy_lte_transport_t` isolates AT command exchange from physical I/O.
2. `xy_lte_uart_adapter` proves callback-backed byte transport behavior on host.
3. `xy_lte_hal_uart_adapter` proves the default-off HAL UART binding through focused host tests and an STM32U5 compile probe.

The remaining gap is not another fake transport layer. The next milestone is a real modem validation record that proves board UART wiring, modem boot timing, flow control, and AT response behavior without changing the default-off feature policy.

## Non-goals

- Do not enable `XY_NET_ENABLE_LTE` by default.
- Do not export LTE from `xy_net.h` without an explicit opt-in probe.
- Do not add vendor SDK or modem-specific headers under `components/net`.
- Do not fold board power/reset sequencing into `xy_lte.c`.
- Do not replace host fake tests with hardware-only validation.

## Proposed validation fixture

Use a board- or project-local smoke entrypoint outside the LTE core that can bind the already-implemented HAL UART adapter:

```text
board init / app smoke
  -> configure UART pins and baudrate
  -> power/reset modem through board code
  -> xy_lte_hal_uart_adapter_init(...)
  -> xy_lte_bind_transport(...)
  -> xy_lte_check()
  -> query signal/SIM/attach status only after AT check succeeds
```

Board code owns all physical concerns:

- UART instance selection;
- TX/RX pin mux;
- RTS/CTS enablement if present;
- modem PWRKEY/RESET/ENABLE GPIO timing;
- SIM voltage and modem regulator sequencing;
- boot delay before the first `AT` probe.

`components/net` should continue to own only transport-neutral LTE command behavior and the HAL UART adapter seam.

## Minimum hardware validation log

A useful hardware validation record should capture:

| Item | Required evidence |
| --- | --- |
| Board/modem identity | board revision, modem model, firmware version if available |
| UART settings | UART instance, baudrate, parity/stop bits, flow-control mode |
| Power/reset sequence | GPIO names, active polarity, timing delays |
| First AT response | sent bytes, received bytes, timeout used, retry count |
| SIM query | `xy_lte_check_sim()` result and raw AT response if exposed by smoke tooling |
| Signal query | `xy_lte_get_signal_quality()` result and RSSI/BER mapping |
| Attach/PDP boundary | attach status and whether PDP activation was attempted or intentionally skipped |
| Failure mode | timeout/error behavior when modem is absent or powered off |

Use `docs/validation/xinyi-net-lte-hardware-validation-record-template-2026-08-06.md` as the checked-in evidence format. The template deliberately starts at `pending` and must not be promoted to `compile-only` or `hardware-passed-*` without real command logs. Every completed record must label compile-only results separately from real modem results.

## Flow-control decision rule

Keep the HAL UART adapter unchanged until hardware evidence requires otherwise:

1. If the modem works reliably with no hardware flow control, document `XY_HAL_UART_FLOWCTRL_NONE` as the board smoke baseline.
2. If large responses or attach/PDP traffic drop bytes, enable RTS/CTS in board UART config first.
3. If the HAL exposes RX availability/interrupt callbacks later, add a separate adapter extension proposal before changing `xy_lte_hal_uart_adapter_t`.
4. If modem-specific wake/sleep pins are needed, model them as board power-management hooks, not as LTE core fields.

## Verification before default enablement

Default enablement remains blocked until all of the following are true:

1. `test_lte`, `test_lte_uart_adapter`, `test_lte_hal_uart_adapter`, `test_lte_hal_uart_smoke_example`, and `test_net_feature_gated_umbrella` pass.
2. `make test-unit` passes.
3. STM32U5 build or compile probe passes in the target environment.
4. A real hardware validation record exists with at least `AT`, SIM, signal, and modem-absent timeout evidence.
5. The board flow-control choice is documented and reproducible.

Until then, LTE remains direct-opt-in and default-off.

## Next small slice

Create a board-local or example smoke harness that binds `xy_lte_hal_uart_adapter` without enabling LTE globally. If no modem hardware is present, add only the smoke skeleton/proposal and keep the validation record pending rather than fabricating modem output.
