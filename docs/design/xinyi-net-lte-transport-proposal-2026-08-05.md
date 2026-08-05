# XinYi Net LTE Transport Proposal

**Date:** 2026-08-05  
**Status:** Draft / design-stage guardrail; fake transport host contracts present
**Scope:** `components/net/inc/xy_lte.h`, `components/net/src/xy_lte.c`, active Net host tests  
**Decision type:** proposal only; no UART/HAL implementation change in this slice

## Background

The Net component currently treats LTE as an explicitly disabled, direct-opt-in module:

- `XY_NET_ENABLE_LTE` defaults to `0` in `components/net/inc/xy_net_config.h`.
- `xy_lte.h` exposes a broad modem API for attach, PDP, socket-like send/recv, AT command helpers, signal and SIM queries.
- `xy_lte.c` preserves caller-visible lifecycle/callback state but still uses an internal `lte_send_cmd()` placeholder rather than a real UART/AT transport.
- `tests/unit/net/test_lte.c` guards lifecycle, callback registration, parameter validation,
  fake transport command success/failure, state preservation, read-style output preservation,
  and the placeholder no-transport recv zero-fill behavior.
- `tests/unit/net/test_net_core.c` documents the current umbrella contract: LTE headers remain directly includable, but LTE is not auto-exported/enabled by `xy_net`.

This means LTE should not be switched on through `xy_net` until transport ownership is explicit and host-testable.

## Problem

The LTE driver has two responsibilities mixed together:

1. High-level modem state machine/API semantics (`attach`, PDP active flag, link validation, callback registration).
2. Low-level command transport (`AT` write/read, command timeout, prompt handling for `CIPSEND`, URC dispatch).

Without a narrow transport boundary, a future implementation is likely to either:

- depend directly on one HAL UART implementation and become hard to test on PC;
- hide command failures behind optimistic state updates;
- duplicate AT-client logic already present in Net;
- enable LTE in the umbrella before error and timeout behavior is stable.

## Proposed boundary

Introduce a small LTE transport vtable in a future implementation slice. Keep it local to LTE until at least one real modem profile is validated.

```c
typedef struct {
    void *context;
    int (*write)(void *context, const uint8_t *data, size_t len, uint32_t timeout_ms);
    int (*read)(void *context, uint8_t *data, size_t len, uint32_t timeout_ms);
    int (*flush)(void *context);
} xy_lte_transport_t;
```

Recommended integration path:

1. Add transport storage to `xy_lte_t` only after deciding whether the public ABI can change, or add a separate `xy_lte_bind_transport()` helper if ABI churn should be minimized.
2. Keep `xy_lte_init()` behavior compatible for existing tests: default baudrate remains `115200`, default APN remains `cmnet`, and LTE remains initialized only when a valid handle/transport is provided.
3. Make `lte_send_cmd()` route through the injected transport in host tests and through a UART adapter in target builds.
4. Keep URC parsing as a separate helper from command/response transactions so unsolicited callbacks do not corrupt synchronous responses.
5. Do not enable `XY_NET_ENABLE_LTE` by default until focused tests prove command failure propagation and state preservation.

## State and failure contracts to lock with host tests

Before real UART wiring, extend `test_lte` with a fake transport queue and guard these contracts:

- `xy_lte_check()` returns `XY_LTE_OK` only when `AT` receives an `OK` response; empty/`ERROR` responses fail.
- `xy_lte_get_signal()` preserves caller output when `AT+CSQ` transport or parse fails.
- `xy_lte_attach()` sets `attached = true` only after both network-mode and attach commands succeed.
- `xy_lte_detach()` clears `attached` only after the detach command succeeds, or explicitly documents best-effort deinit semantics if deinit must always clear it.
- `xy_lte_set_pdp_context()` copies `ctx` into `lte->pdp` only after the command succeeds; failed commands preserve the previous cache.
- `xy_lte_activate_pdp()` and `xy_lte_deactivate_pdp()` update `pdp_active` only on successful command flow.
- `xy_lte_send()` first validates the prompt command result before accepting payload bytes.
- `xy_lte_recv()` should either consume queued URC payload bytes or return zero without modifying unrelated caller state; zero-fill remains a placeholder contract only until transport buffering exists.

## Relationship with existing Net/AT components

Short-term recommendation: keep LTE transport internal and fakeable. Do not immediately couple `xy_lte.c` to the larger AT client/server trees because those trees have separate lifecycle assumptions and are only partially included by default Net builds.

Longer-term recommendation: if the lightweight AT client becomes the canonical modem transport layer, write a dedicated migration proposal that maps:

- AT client send/recv callbacks to LTE command transactions;
- URC registration to LTE `urc_callback` / `recv_callback` dispatch;
- timeout and error-code domains between `XY_AT_RESP_*` and `XY_LTE_*`.

## Enablement criteria

LTE can move from `XY_NET_ENABLE_LTE=0` to an opt-in or default-on policy only after:

1. A fake-transport `lte_component` CTest covers command success/failure paths.
2. Header direct-include tests still pass with `XY_NET_ENABLE_LTE=0`.
3. A UART adapter compiles on PC and STM32U5 without importing vendor-specific code into the component layer.
4. `xy_net` umbrella policy explicitly states whether LTE is exported by `xy_net.h` or included directly through `xy_lte.h`.
5. README status changes from "stub" to "transport-backed" with exact supported modem command subset.

## 2026-08-06 status

The first fake-transport host coverage slice is now present in `test_lte`: it injects a
queued write/read transport, checks command strings, verifies `AT`/`CSQ`/SIM/attach/PDP/send
failure paths, and preserves caller-visible state/outputs on transport failures. LTE should
still remain `XY_NET_ENABLE_LTE=0` by default; this test coverage proves the internal
transport seam, not a target UART adapter.

## Suggested next slice

After the fake-transport host contracts, keep LTE disabled globally and add the next reversible
slice:

- paths: `components/net/inc/xy_lte.h`, `components/net/src/xy_lte.c`, `tests/unit/net/test_lte.c`,
  plus a narrowly scoped adapter/proposal path if UART behavior is still design-stage;
- focused verification: `cmake --build build/tests/unit --target test_lte -j$(nproc)` and `cd build/tests/unit && ctest --output-on-failure -R '^lte_component$'`;
- full gate: `make test-unit && git diff --check`.

This keeps the design reversible and prevents a broad Net API migration before a real UART/AT
adapter is compile-probed on PC and STM32U5.
