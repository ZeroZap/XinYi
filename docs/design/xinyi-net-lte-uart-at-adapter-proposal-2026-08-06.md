# XinYi Net LTE UART/AT Adapter Proposal

**Date:** 2026-08-06
**Status:** Draft / design-stage compile-probe target
**Scope:** `components/net/inc/xy_lte.h`, `components/net/src/xy_lte.c`, future LTE adapter source, `tests/unit/net/test_lte.c`
**Decision type:** proposal only; no HAL/UART implementation change in this slice

## Background

The current LTE component has now crossed two guardrail steps:

1. `XY_NET_ENABLE_LTE` remains default-off and only direct consumers or explicit feature-gated probes include `xy_lte.h`.
2. `test_lte` has fake transport coverage for the internal command seam: command writes, queued responses, AT/CSQ/SIM/attach/PDP/send failures, and output/state preservation.

The remaining risk is not LTE API shape anymore; it is the first real transport adapter. A direct jump from fake transport to target UART code would risk importing platform/vendor details into `components/net` and making host tests brittle.

## Design goal

Add a reversible UART/AT adapter layer that can be compile-probed on PC and STM32U5 before LTE is enabled through the broader Net umbrella.

The adapter should satisfy three constraints:

- LTE core owns modem state and AT command semantics.
- Adapter owns byte transport, timeout, and optional line buffering.
- Platform HAL/vendor details stay outside `xy_lte.c` and do not leak into `xy_lte.h` unless the public ABI is explicitly accepted.

## Proposed adapter shape

Keep `xy_lte_transport_t` as the stable seam and add a small adapter helper around it:

```c
typedef struct {
    void *uart;
    uint32_t default_timeout_ms;
    uint8_t *rx_buffer;
    size_t rx_buffer_len;
} xy_lte_uart_adapter_t;

int xy_lte_uart_adapter_init(xy_lte_uart_adapter_t *adapter,
                             void *uart,
                             uint8_t *rx_buffer,
                             size_t rx_buffer_len,
                             uint32_t default_timeout_ms);

int xy_lte_uart_adapter_get_transport(xy_lte_uart_adapter_t *adapter,
                                      xy_lte_transport_t *transport);
```

Recommended placement for a future implementation slice:

- Header: `components/net/inc/xy_lte_uart_adapter.h`
- Source: `components/net/src/xy_lte_uart_adapter.c`
- Tests: `tests/unit/net/test_lte_uart_adapter.c` or a focused section in `test_lte.c` if the adapter stays tiny

## Host-testable contract

Before binding to real HAL UART, the first implementation slice should use test-local UART callbacks or weak shims so host tests can prove behavior without hardware:

1. `xy_lte_uart_adapter_init()` rejects missing adapter, UART handle, RX buffer, or zero buffer length.
2. `xy_lte_uart_adapter_get_transport()` rejects missing output and returns a transport whose `context` points to the adapter.
3. Transport `write()` forwards exactly the requested bytes and returns either `XY_LTE_OK`, the byte count accepted by the underlying UART, or a negative LTE error without partial success ambiguity.
4. Transport `read()` zero-terminates only in LTE command helper buffers, not in the adapter byte stream itself.
5. Transport `flush()` is optional but, if present, must propagate HAL/underlying flush errors.
6. Timeout arguments from LTE command calls must be forwarded to the backend rather than replaced unconditionally by the adapter default.

## HAL boundary options

Two implementation options are acceptable; choose the narrower one during coding:

### Option A: callback-backed adapter first

Use adapter-local callback function pointers for host coverage:

```c
typedef int (*xy_lte_uart_write_fn)(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms);
typedef int (*xy_lte_uart_read_fn)(void *uart, uint8_t *data, size_t len, uint32_t timeout_ms);
typedef int (*xy_lte_uart_flush_fn)(void *uart);
```

This is the safest first slice because it compiles on PC with no HAL dependency and allows STM32U5 to compile the source as long as no vendor header is included.

### Option B: direct HAL adapter after callback proof

Only after Option A passes should a direct HAL binding be considered. That binding must include only canonical public HAL headers and must be compile-probed for PC and STM32U5. Vendor SDK headers under `MCU/` must not be included from `components/net`.

## Enablement criteria

Do not change `XY_NET_ENABLE_LTE` default until all are true:

1. Existing `lte_component` fake-transport tests still pass.
2. New UART adapter focused CTest passes on PC.
3. `test_net_feature_gated_umbrella` still proves default-off and explicit opt-in include behavior.
4. `make test-unit` and `git diff --check` pass.
5. STM32U5 compile either includes the adapter cleanly or documents the exact missing public HAL dependency as a blocker.

## Suggested next implementation slice

Implement Option A only:

- paths: `components/net/inc/xy_lte_uart_adapter.h`, `components/net/src/xy_lte_uart_adapter.c`, `tests/unit/net/test_lte_uart_adapter.c`, `tests/unit/CMakeLists.txt`;
- focused build: `cmake --build build/tests/unit --target test_lte_uart_adapter -j$(nproc)`;
- focused run: `cd build/tests/unit && ctest --output-on-failure -R '^lte_uart_adapter$'`;
- broader gate: `make test-unit && git diff --check`.

This keeps LTE transport progress concrete while avoiding a premature Net umbrella or vendor-HAL migration.
