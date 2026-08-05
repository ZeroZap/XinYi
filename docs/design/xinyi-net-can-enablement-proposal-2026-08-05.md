# XinYi Net CAN Enablement Proposal

**Date:** 2026-08-05  
**Status:** Draft / design-stage guardrail; component hardening contracts present
**Scope:** `components/net/inc/xy_can.h`, `components/net/src/xy_can.c`, `components/net/CMakeLists.txt`, `tests/unit/net/test_can.c`  
**Decision type:** proposal only; no default `xy_net` library enablement in this slice

## Background

The Net component currently keeps CAN in a direct-opt-in state:

- `components/net/inc/xy_net_config.h` defines `XY_NET_ENABLE_CAN` as `0`.
- `components/net/CMakeLists.txt` still comments `src/xy_can.c` out of the `xy_net` static library with the note `disabled - incomplete implementation`.
- `components/net/inc/xy_can.h` and `components/net/src/xy_can.c` are nevertheless active enough to be covered by `tests/unit/net/test_can.c`.
- `test_can` currently guards init/deinit, start/stop, callback registration, direct mode,
  FIFO RX/TX, timeout polling, FIFO usage, invalid one-slot FIFO rejection, timeout
  output/counter preservation, FIFO overflow accounting, oversized-frame rejection, and
  unregister callback suppression.

This means CAN is no longer an untested placeholder, but it is also not ready to become a default `xy_net` dependency without tightening ownership and platform behavior.

## Current risk

CAN mixes three concerns that should be kept separate before default enablement:

1. Protocol/component-level FIFO and callback semantics.
2. Platform/HAL CAN controller start/stop/send/receive integration.
3. `xy_net` library export policy and build footprint.

The current implementation contains platform-specific hooks only under `MCU_CH32` and otherwise uses host-friendly direct-mode placeholders. If `xy_can.c` is added to `xy_net` unconditionally now, downstream builds may interpret a PC-tested FIFO shim as a real target-backed CAN driver.

## Proposed enablement policy

Keep CAN disabled by default in `xy_net` until a feature-gated integration slice proves both host and STM32U5 behavior.

Recommended sequence:

1. Keep `XY_NET_ENABLE_CAN=0` as the default in `xy_net_config.h`.
2. Add a CMake/config gate later so `src/xy_can.c` is compiled into `xy_net` only when the generated Net CAN option is explicitly enabled.
3. Preserve direct include support for `xy_can.h` regardless of `xy_net` umbrella policy.
4. Treat `test_can` as the current contract owner for component-level behavior, not proof of target HAL integration.
5. Add a separate HAL-adapter test/probe before declaring CAN default-on.

## Contracts locked before feature-gated build integration

`test_can` now covers the component-level edge contracts that were needed before touching
`xy_net` feature-gated build policy:

- reject invalid FIFO sizes that make the ring-buffer unusable;
- preserve `tx_count` when FIFO send times out;
- preserve caller RX output when FIFO receive times out;
- record `error_count` for full FIFO ISR drops, send timeouts, and receive timeouts;
- make `xy_can_isr_receive()` count invalid/overflowed frames instead of silently dropping them;
- verify callback unregister prevents both direct-mode and FIFO receive callback delivery;
- compile-probe the public header with only canonical public include roots via `test_can_public_header`;
- compile-probe the platform adapter path for PC and STM32U5 before enabling `xy_can.c` through `xy_net`.

## Relationship with `xy_net`

Short term, `xy_net` should continue to document CAN as a direct component API guarded by `test_can`, not an umbrella-exported default protocol.

Longer term, once the feature gate exists, `xy_net` can expose CAN in one of two reversible ways:

1. **Opt-in library member:** `xy_net` links `xy_can.c` only when `XY_NET_ENABLE_CAN=1`, while users still include `xy_can.h` directly.
2. **Umbrella export:** `xy_net.h` includes `xy_can.h` only under the same generated option, and README/Kconfig state the extra dependency clearly.

Do not choose option 2 until the component map and README explain the added build footprint and platform requirements.

## Enablement criteria

CAN can move out of the `disabled - incomplete implementation` bucket only after:

1. A feature-gated CMake/Kconfig slice compiles `xy_can.c` into `xy_net` when explicitly enabled.
2. `test_can` covers timeout/output-preservation and FIFO-overflow accounting contracts.
3. A focused compile probe proves `xy_can.h` is self-contained for public consumers.
4. PC `make test-unit` passes with CAN still default-disabled.
5. STM32U5 build either compiles the gated CAN path or explicitly documents the missing HAL adapter as a blocker.

## 2026-08-06 status

The CAN hardening test batch is now represented in the active `can_component` CTest, so the
next CAN slice should not add another component-edge test unless a new real failure appears.
The remaining design risk is feature-gated `xy_net` integration versus target HAL adapter
readiness.

## Suggested next slice

Add a feature-gated build/probe slice before changing default build policy:

- paths: `components/net/CMakeLists.txt`, `components/net/inc/xy_net_config.h`,
  `components/net/inc/xy_net.h` or a small compile-probe test if the umbrella policy changes;
- focused verification: keep `can_component` and `can_public_header` passing, then configure a
  temporary explicit `XY_NET_ENABLE_CAN=1`/CMake-gated build if the slice adds the gate;
- full gate: `make test-unit && git diff --check`, plus `make HAL_PLATFORM=STM32U5` if the gated
  source enters the target build.

This keeps CAN progression reversible and prevents the Net library from implying target-ready
CAN support before the adapter path is compile-probed.
