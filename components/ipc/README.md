# XinYi IPC Component

## Status

IPC is a host-guarded, partially mature component for local inter-domain communication.
The active, verified submodules are:

| Area | Public header | Implementation | Host CTest | Status |
| --- | --- | --- | --- | --- |
| Pipe ring buffer | `pipe/xy_pipe.h` | `pipe/xy_pipe.c` | `ipc_pipe` | Guarded |
| Broker | `xy_broker/xy_broker.h` | `xy_broker/xy_broker.c` | `ipc_broker` | Guarded |
| Message queue | `inc/xy_mq.h` | `src/xy_mq.c` | `ipc_mq` | Guarded |

Dormant or design-stage areas remain outside the active contract until they receive
focused design and tests. In particular, event groups are still a roadmap item; do
not infer event-group behavior from the broker, pipe, or message-queue tests.

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
cmake --build build/tests/unit --target test_ipc_pipe test_ipc_broker test_ipc_mq -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(ipc_pipe|ipc_broker|ipc_mq)$'

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

## Backlog

1. Decide whether IPC should receive a root `COMPONENT_IPC` Kconfig symbol or stay
   as an always-discoverable component library. This should be a config proposal
   before changing defaults.
2. Add an event-group proposal before implementing `components/ipc/src/xy_event.c`
   or public event APIs.
3. If `observer/` is promoted to the active contract, add a focused Unity/CTest
   target first and document its relationship to broker pub/sub.
4. Keep any future CMake/Kconfig changes path-limited to IPC and re-run the
   focused IPC CTests plus `make test-unit`.
