# XinYi Net LTE Board Flow-Control Design

**Date:** 2026-08-06  
**Status:** Proposal / board evidence pending  
**Scope:** board-owned UART flow-control and modem power sequencing contract for the default-off LTE HAL UART adapter  
**Related design:** `docs/design/xinyi-net-lte-hardware-validation-plan-2026-08-06.md`

## Background

The LTE software path is now guarded by host tests and an STM32U5 compile probe, but there is still no real modem evidence. The next risk is not AT parser behavior; it is board integration:

- which UART instance and pins are used;
- whether RTS/CTS are physically routed and enabled;
- whether the modem boot/power timing is reproducible;
- whether absent-modem and powered-modem behavior are distinguishable in logs.

This document fixes the design boundary before any real-board smoke harness fills the validation record. It does **not** enable LTE by default and does **not** add board GPIO control to `components/net`.

## Ownership boundary

| Concern | Owner | Rule |
| --- | --- | --- |
| AT command semantics | `components/net/src/xy_lte.c` | Transport-neutral; no UART pins, GPIOs, power rails, or modem model assumptions. |
| HAL UART byte I/O | `xy_lte_hal_uart_adapter` | Converts public HAL UART send/recv status into LTE transport return values only. |
| UART instance/pin mux | Board/project smoke code | Selected outside `components/net`; must be recorded in validation evidence. |
| RTS/CTS choice | Board/project smoke code | Start with documented `none` only if the first real smoke proves stable responses. |
| Modem PWRKEY/RESET/ENABLE | Board/project smoke code | Use board hooks with explicit polarity and timing; never hide this in LTE core. |
| SIM/regulator sequencing | Board/project smoke code | Treat as board/platform responsibility; record delays and observed result. |

## Flow-control decision tree

1. **Inventory hardware first**
   - Record whether the board routes modem RTS/CTS pins to MCU pins.
   - If not routed, mark flow control as `not available` rather than `none by preference`.

2. **Baseline with no hardware flow control only for small command responses**
   - Allowed baseline commands: `AT`, SIM status, signal quality, attach status.
   - The smoke must log raw TX/RX byte counts and timeouts.
   - If responses are truncated, interleaved, or timeout-prone, do not tune LTE core retries first; move to RTS/CTS or board UART buffering analysis.

3. **Enable RTS/CTS before PDP or large-response work**
   - PDP activation, operator scans, or large modem info responses should require either RTS/CTS or a documented reason why the board cannot support it.
   - The flow-control mode must be visible in the validation record's UART table.

4. **Keep adapter ABI stable until evidence says otherwise**
   - Do not add modem wake/sleep, GPIO, or flow-control fields to `xy_lte_hal_uart_adapter_t` without a new proposal.
   - If HAL RX availability/interrupt APIs are later needed, add a separate adapter extension; do not overload `recv` semantics silently.

## Board smoke harness shape

The first board-local smoke should be outside `components/net` and should look like this sequence:

```text
board_uart_pinmux_config()
board_modem_power_enable()
board_modem_reset_or_pwrkey_sequence()
wait documented boot delay
xy_lte_hal_uart_adapter_init(...)
xy_lte_bind_transport(...)
xy_lte_check()
xy_lte_check_sim()
xy_lte_get_signal_quality()
optional attach-status query
modem-absent/powered-off negative run
```

The smoke is allowed to live under a board/project/example path, but the result must be summarized in `docs/validation/` using the existing hardware validation record template.

## Minimum log requirements before default enablement

Before any `XY_NET_ENABLE_LTE` default change is reconsidered, the checked-in validation record must include:

- Git commit under test.
- UART instance and pins.
- Flow-control mode and whether RTS/CTS are physically routed.
- Modem power/reset GPIO names, polarity, and delays.
- Raw `AT` TX/RX transcript with timeout/retry count.
- SIM query transcript.
- Signal quality transcript and parsed RSSI/BER.
- Modem-absent or powered-off negative transcript.
- Clear result classification: `hardware-failed`, `hardware-passed-basic`, or `hardware-passed-attach`.

Compile-only STM32U5 output and host fake CTests may be referenced as prerequisites, but they must not be copied into hardware evidence fields.

## Next implementation slice

If real hardware is still unavailable, keep the validation record pending. The next code slice should be a board-local smoke entrypoint only when the board target, UART pins, modem power hooks, and logging path are known. Otherwise continue with documentation/validation templates rather than inventing hardware behavior.
