# XinYi IPC Component

## Status

IPC is a host-guarded, partially mature component for local inter-domain communication.
The active, verified submodules are:

| Area | Public header | Implementation | Host CTest | Status |
| --- | --- | --- | --- | --- |
| Pipe ring buffer | `pipe/xy_pipe.h` | `pipe/xy_pipe.c` | `ipc_pipe` | Guarded |
| Broker | `xy_broker/xy_broker.h` | `xy_broker/xy_broker.c` | `ipc_broker` | Guarded |
| Message queue | `inc/xy_mq.h` | `src/xy_mq.c` | `ipc_mq` | Guarded |
| Observer/subject | `observer/xy_observer.h` | `observer/xy_observer.c` | `ipc_observer` | Guarded |
| Event group | `inc/xy_event_group.h` | `src/xy_event_group.c` | `ipc_event_group` | Guarded |

Dormant or design-stage areas remain outside the active contract until they receive
focused design and tests. Event groups are intentionally limited to a thin OSAL
event-flags wrapper; do not infer broker/topic or scheduler behavior from them.

## Build and configuration ownership

- Root CMake auto-discovers `components/ipc/CMakeLists.txt` and builds the
  component as `xy_ipc` / `ipc_component` when the component directory is included
  in a root firmware build.
- `components/ipc/Kconfig` contains legacy local symbols (`XY_IPC_ENABLE`,
  `XY_IPC_TEST`, `XY_IPC_EXAMPLE`). These are documentation/config candidates,
  not currently the root `Kconfig` source of truth.
- The canonical host tests live in `tests/unit/CMakeLists.txt`; do not revive
  component-local test runners for the same contracts.

## Verification commands

From the repository root:

```bash
# Focused IPC checks
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_ipc_pipe test_ipc_broker test_ipc_mq test_ipc_observer test_ipc_event_group -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(ipc_pipe|ipc_broker|ipc_mq|ipc_observer|ipc_event_group)$'

# Full PC unit gate
make test-unit

git diff --check
```

## Current contract boundaries

### Pipe

`xy_pipe` is a caller-supplied-buffer ring buffer. The host test covers
initialization guards, clear/deinit, read/write/peek, empty/full behavior, partial
writes, and wraparound ordering.

### Broker

`xy_broker` is a fixed-ID in-process message broker. The host test covers
lifecycle, server registration, direct queue delivery, pub/sub, unsubscribe, queue
limits/statistics, request/response helpers, timeout, and debug-name helpers.

### Message queue

`xy_mq` is an allocated-slot message queue. The host test covers init/deinit
validation, FIFO ordering, priority/urgent drop behavior, overwrite-old policy,
timeout/delay behavior, stats, null-payload guards, and metadata preservation when
the caller receive buffer is smaller than the stored payload.

### Observer/subject

`xy_observer` is a small local observer-pattern helper. The host test covers
observer/subject init guards, name truncation, attach idempotency, notify data and
user-data dispatch, detach/not-found behavior, capacity limits, clear, and deinit.

### Event group

`xy_ipc_event_group` is an optional convenience wrapper over OSAL event flags, not
a broker/topic replacement. The host test covers init/name guards, set/get/clear,
wait-any, wait-all, clear-on-success, no-clear, timeout output preservation, and
post-deinit not-initialized behavior using the host-safe bare-metal OSAL backend.

## Backlog

1. IPC config ownership is now captured in
   `docs/design/xinyi-ipc-component-config-proposal-2026-08-08.md`: keep IPC as an
   always-discoverable core component for now; do not add root `COMPONENT_IPC`
   unless a future generated-config slice proves the disabled/enabled paths.
2. Event-group ownership from
   `docs/design/xinyi-ipc-event-group-proposal-2026-08-08.md` is now implemented as
   `xy_event_group.{h,c}` with focused host coverage. Keep it thin over OSAL event
   flags; defer ISR/threaded wait extensions until backend evidence exists.
3. Keep any future CMake/Kconfig changes path-limited to IPC and re-run the
   focused IPC CTests plus `make test-unit`.
