# XinYi IPC Component Config Proposal

**Date:** 2026-08-08
**Status:** Draft / design-stage guardrail
**Scope:** `components/ipc/Kconfig`, root `Kconfig`, root CMake auto-discovery, `components/ipc/README.md`
**Decision type:** proposal only; no default build-policy change in this slice

## Background

The IPC component is already active in the root firmware build because the root `CMakeLists.txt`
auto-discovers top-level `components/*` directories that contain a `CMakeLists.txt`. IPC therefore builds
as `xy_ipc` / `ipc_component` without requiring a root Kconfig symbol.

The currently guarded IPC submodules are:

| Area | Public header | Implementation | Host CTest |
| --- | --- | --- | --- |
| Pipe ring buffer | `components/ipc/pipe/xy_pipe.h` | `components/ipc/pipe/xy_pipe.c` | `ipc_pipe` |
| Broker | `components/ipc/xy_broker/xy_broker.h` | `components/ipc/xy_broker/xy_broker.c` | `ipc_broker` |
| Message queue | `components/ipc/inc/xy_mq.h` | `components/ipc/src/xy_mq.c` | `ipc_mq` |

`components/ipc/Kconfig` still contains legacy local symbols:

- `XY_IPC_ENABLE`
- `XY_IPC_TEST`
- `XY_IPC_EXAMPLE`

These symbols are not currently the root Kconfig source of truth. Adding a root `COMPONENT_IPC` symbol
without a clear policy would be misleading because the library is already auto-discovered and its active
submodules are host-guarded.

## Current risk

There are two different decisions that should not be conflated:

1. **Build discoverability:** whether the root build sees `components/ipc/CMakeLists.txt`.
2. **Feature enablement:** whether application code should treat IPC as an enabled runtime feature with
   generated config symbols and conditional public API expectations.

Today the first decision is already effectively `on` through root auto-discovery. The second decision is
not needed for the guarded pipe/broker/message-queue contracts, and changing it prematurely could imply
that dormant event/observer APIs are also production-ready.

## Proposed policy

Keep IPC as an always-discoverable core component library for now.

Recommended near-term policy:

1. Do **not** add root `COMPONENT_IPC` yet.
2. Treat `xy_ipc` / `ipc_component` as a core local-communication library built when the component tree is
   included by the root build.
3. Keep `XY_IPC_ENABLE` in `components/ipc/Kconfig` as a legacy/local candidate until a broader generated
   config model needs IPC to be optional.
4. Do not use `XY_IPC_ENABLE` to hide pipe, broker, or message-queue sources in the current root build.
5. If root config ownership is needed later, add it as a separate reversible slice with generated-config
   verification and explicit README wording.

## Event-group and observer boundary

`components/ipc` contains dormant or design-stage material beyond the three guarded submodules. Those
areas should not become enabled by a generic IPC config switch without their own contracts.

Before promoting event groups:

1. Write a focused event-group proposal that defines whether it maps to OSAL event flags, broker topics,
   or a standalone IPC primitive.
2. Add a host Unity/CTest target for the public event-group API before wiring it into the active contract.
3. Document timeout, bit-clear, wait-any/wait-all, and ISR-safety expectations explicitly.

Before promoting observer:

1. Add a focused Unity/CTest target for `observer/xy_observer.{h,c}`.
2. Document its relationship to broker pub/sub so both mechanisms do not compete for the same use case.
3. Keep observer examples/docs separate from broker until the lifecycle and callback ownership contract is
   verified.

## Enablement criteria for a future root config slice

A future root `COMPONENT_IPC` or `XY_IPC_ENABLE` integration should only happen after:

1. The desired default is explicit: core always-on, default-on selectable, or default-off optional.
2. Root `Kconfig`, `components/ipc/Kconfig`, and generated `autoconf.h` / `config.cmake` agree on symbol
   names.
3. Root CMake behavior is proven for both the default path and the explicitly disabled/enabled path.
4. `ipc_pipe`, `ipc_broker`, and `ipc_mq` remain passing in `make test-unit`.
5. Dormant event/observer APIs are either still excluded from the promised contract or have their own
   focused host tests.

## Suggested next slice

Do not change IPC build defaults yet. The next low-risk IPC slice should be one of:

1. **Event-group proposal:** define the intended API and OSAL relationship before implementation.
2. **Observer focused CTest:** if observer is to be promoted, add host coverage first and then update the
   README status table.
3. **Config implementation slice:** only if a real consumer needs generated IPC feature symbols; keep it
   path-limited to root `Kconfig`, `components/ipc/Kconfig`, IPC CMake/README, and generated-config probes.

This keeps IPC progression reversible and prevents a broad config switch from implying readiness of
unverified submodules.
