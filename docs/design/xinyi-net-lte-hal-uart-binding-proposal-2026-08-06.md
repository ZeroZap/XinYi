# XinYi Net LTE HAL UART Binding Proposal

**Date:** 2026-08-06
**Status:** Draft / design-stage only
**Scope:** future `components/net/inc/xy_lte_hal_uart_adapter.h`, future `components/net/src/xy_lte_hal_uart_adapter.c`, future `tests/unit/net/test_lte_hal_uart_adapter.c`
**Decision type:** proposal only; no `XY_NET_ENABLE_LTE` default change

## Background

The LTE component now has three guardrails in place:

1. `xy_lte_transport_t` is the LTE core's AT command seam.
2. `xy_lte_uart_adapter.{h,c}` proves a callback-backed byte transport adapter with focused host coverage.
3. `XY_NET_ENABLE_LTE` remains default-off; explicit consumers can include LTE headers directly, and the feature-gated umbrella probe verifies opt-in behavior.

The next risky step is binding the callback seam to the project HAL UART API. This must stay separate from `xy_lte.c` so modem state/AT semantics do not absorb platform I/O details.

## Design goal

Add a narrow HAL UART binding layer that converts canonical XinYi HAL UART calls into the already-tested `xy_lte_transport_t` contract.

The binding should be compile-probed on PC and STM32U5 before any runtime enablement decision:

- no vendor SDK includes under `components/net`;
- no direct references to `MCU/` headers;
- no change to `XY_NET_ENABLE_LTE=0` default;
- no changes to LTE AT command parsing in the same slice;
- no automatic export from `xy_net.h` unless a feature-gated probe covers it.

## Proposed public shape

Keep this as a separate optional binding instead of extending the callback adapter ABI:

```c
typedef struct {
    void *uart;
    uint32_t default_timeout_ms;
    uint8_t *rx_buffer;
    size_t rx_buffer_len;
} xy_lte_hal_uart_adapter_t;

int xy_lte_hal_uart_adapter_init(xy_lte_hal_uart_adapter_t *adapter,
                                 void *uart,
                                 uint8_t *rx_buffer,
                                 size_t rx_buffer_len,
                                 uint32_t default_timeout_ms);

int xy_lte_hal_uart_adapter_get_transport(xy_lte_hal_uart_adapter_t *adapter,
                                          xy_lte_transport_t *transport);
```

The `void *uart` type should only be narrowed after direct compile probes confirm the current public HAL UART handle type and include roots. If the public HAL API already exposes a stable UART handle typedef, the implementation slice can adopt it with a focused public-header probe.

## Implementation boundaries

The HAL binding implementation may call only canonical public HAL functions, for example the current equivalents of:

- UART transmit/write with timeout;
- UART receive/read with timeout;
- optional UART flush/drain if the public HAL exposes one.

If the current HAL API lacks a flush primitive, `transport.flush()` should return `XY_LTE_OK` and the missing primitive should be documented rather than emulated with vendor calls.

The binding should normalize backend return values to LTE transport semantics:

| HAL/backend result | LTE transport result |
| --- | --- |
| full byte count accepted/read | byte count or `XY_LTE_OK` only if the underlying HAL is status-only |
| timeout | `XY_LTE_TIMEOUT` |
| invalid arguments / missing handle | `XY_LTE_INVALID_PARAM` |
| other HAL error | `XY_LTE_ERROR` |

Avoid treating a short positive write as success unless the current HAL explicitly documents partial-write success. Host tests should make this behavior visible.

## Host-testable contract

A first implementation slice should add `test_lte_hal_uart_adapter` with local HAL fakes/stubs and cover:

1. init rejects missing adapter, UART handle, RX buffer, or zero RX buffer length;
2. exported transport points back to the adapter and rejects corrupted adapter state;
3. write forwards exact bytes and timeout to the HAL stub;
4. read forwards buffer length and timeout without zero-terminating byte streams;
5. zero timeout falls back to `default_timeout_ms`;
6. backend timeout/error values are normalized to `XY_LTE_TIMEOUT` / `XY_LTE_ERROR`;
7. optional flush succeeds as no-op if the public HAL has no flush primitive;
8. binding into `xy_lte_check()` still sends `AT` and reads `OK` through the HAL-backed transport.

## Verification gates for the implementation slice

```bash
cmake --build build/tests/unit --target test_lte_hal_uart_adapter -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^lte_hal_uart_adapter$'
make test-unit
git diff --check
```

If the public HAL include roots are stable locally, also compile-probe STM32U5:

```bash
make HAL_PLATFORM=STM32U5 -j$(nproc)
```

If STM32U5 fails because of missing local toolchain or SDK checkout, report the exact blocker and keep the binding default-off.

## Enablement rule

This proposal does **not** justify enabling LTE through `xy_net.h` by default. Default enablement can be reconsidered only after:

1. callback adapter tests still pass;
2. HAL UART binding tests pass;
3. `test_net_feature_gated_umbrella` still proves default-off / explicit opt-in behavior;
4. PC unit tests pass;
5. STM32U5 compile either passes or has a documented environment-only blocker;
6. at least one hardware validation note records real UART/modem behavior.

Until then, LTE remains a direct-opt-in, host-guarded module.
