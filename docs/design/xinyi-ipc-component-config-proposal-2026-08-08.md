# XinYi IPC Component Config Proposal

**Date:** 2026-08-08
**Status:** Accepted guardrail / post-contract sync
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
| Observer/subject | `components/ipc/observer/xy_observer.h` | `components/ipc/observer/xy_observer.c` | `ipc_observer` |
| Event group | `components/ipc/inc/xy_event_group.h` | `components/ipc/src/xy_event_group.c` | `ipc_event_group` |

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
not needed for the guarded pipe/broker/message-queue/observer/event-group contracts. A future generated
config slice should still prove disabled/enabled root-build behavior explicitly instead of inferring it
from the always-discoverable component library.

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

`components/ipc` has since promoted both previously design-stage areas into the guarded contract:

1. `xy_event_group.{h,c}` is a thin OSAL event-flags wrapper with timeout, bit-clear,
   wait-any/wait-all, no-clear, reserved-error-bit, and deinit guard coverage in `ipc_event_group`.
2. `xy_observer.{h,c}` is a local in-process observer helper, separate from broker pub/sub, with
   init/attach/notify/detach/capacity/reentrant attach-detach coverage in `ipc_observer`.

These promotions do **not** change the root config policy. They only mean future IPC config work no
longer needs to treat event group or observer as unverified dormant APIs. ISR safety, cross-thread
blocking semantics, and backend-specific scheduling behavior remain out of scope until OSAL/backend or
hardware evidence exists.

## Enablement criteria for a future root config slice

A future root `COMPONENT_IPC` or `XY_IPC_ENABLE` integration should only happen after:

1. The desired default is explicit: core always-on, default-on selectable, or default-off optional.
2. Root `Kconfig`, `components/ipc/Kconfig`, and generated `autoconf.h` / `config.cmake` agree on symbol
   names.
3. Root CMake behavior is proven for both the default path and the explicitly disabled/enabled path.
4. `ipc_pipe`, `ipc_broker`, `ipc_mq`, `ipc_observer`, and `ipc_event_group` remain passing in
   `make test-unit`.
5. Any newly added IPC submodule is either excluded from the promised contract or has its own focused
   host CTest before it is listed as guarded.

## Suggested next slice

Do not change IPC build defaults yet. The next low-risk IPC slice should be one of:

1. **Config implementation slice:** only if a real consumer needs generated IPC feature symbols; keep it
   path-limited to root `Kconfig`, `components/ipc/Kconfig`, IPC CMake/README, and generated-config probes.
2. **Threaded/ISR event evidence:** only after an OSAL backend or board-level need exists; extend
   `ipc_event_group` with real backend evidence rather than host fake assumptions.
3. **Observer regression slice:** only for a concrete callback ownership/reentrancy failure; add the
   smallest focused regression to `ipc_observer` instead of widening broker/pub-sub scope.

This keeps IPC progression reversible and prevents a broad config switch from implying readiness of
unverified future submodules.
