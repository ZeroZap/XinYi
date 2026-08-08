# XinYi IPC Event-Group Proposal

**Date:** 2026-08-08
**Status:** Draft / design-stage guardrail
**Scope:** `components/ipc`, OSAL event flags, IPC README/backlog
**Decision type:** proposal only; no implementation or default build-policy change in this slice

## Background

The IPC component is now host-guarded for the mature local communication primitives:

- `pipe/xy_pipe.{h,c}` via the `ipc_pipe` CTest
- `xy_broker/xy_broker.{h,c}` via the `ipc_broker` CTest
- `inc/xy_mq.h` + `src/xy_mq.c` via the `ipc_mq` CTest
- `observer/xy_observer.{h,c}` via the `ipc_observer` CTest

Older roadmap material still lists `components/ipc/src/xy_event.c` as a planned event-group
implementation. That file and public API do not currently exist. The repository already has
OSAL event-flag APIs in `components/kernel/osal/xy_os.h` (`xy_os_event_flags_*`), so adding
an IPC event-group implementation without a boundary decision would risk duplicating OSAL
synchronization semantics or confusing it with broker pub/sub events.

## Decision

Treat IPC event groups as a **thin optional convenience layer over OSAL event flags**, not as
another broker/topic system and not as a new standalone scheduler primitive.

Recommended ownership:

| Concern | Owner |
| --- | --- |
| Thread/task synchronization bits | OSAL `xy_os_event_flags_*` |
| IPC-facing naming and guard-friendly wrapper API | Future `components/ipc/inc/xy_event_group.h` |
| Message payload delivery / pub-sub | `xy_broker` |
| Callback fan-out within one address space | `xy_observer` |
| ISR-safe deferred signaling policy | OSAL/backend-specific contract; IPC wrapper only exposes it after backend proof |

## Non-goals

The first event-group slice must not:

1. Replace broker topics or observer callbacks.
2. Introduce dynamic string topic/event registries.
3. Add a root `COMPONENT_IPC` or change IPC default build discoverability.
4. Edit vendor RTOS or MCU SDK event-group implementations.
5. Claim ISR safety until PC plus MCU/backend behavior is explicitly tested or documented.
6. Implement a second wait scheduler in `components/ipc` when OSAL already owns blocking semantics.

## Proposed API shape

If/when implementation is needed, add a small public header:

```c
/* components/ipc/inc/xy_event_group.h */
typedef struct xy_ipc_event_group xy_ipc_event_group_t;

typedef uint32_t xy_ipc_event_bits_t;

#define XY_IPC_EVENT_WAIT_ANY   XY_OS_FLAGS_WAIT_ANY
#define XY_IPC_EVENT_WAIT_ALL   XY_OS_FLAGS_WAIT_ALL
#define XY_IPC_EVENT_NO_CLEAR   XY_OS_FLAGS_NO_CLEAR

int xy_ipc_event_group_init(xy_ipc_event_group_t *group, const char *name);
int xy_ipc_event_group_deinit(xy_ipc_event_group_t *group);
int xy_ipc_event_group_set(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                           xy_ipc_event_bits_t *after_set);
int xy_ipc_event_group_clear(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                             xy_ipc_event_bits_t *before_clear);
int xy_ipc_event_group_get(xy_ipc_event_group_t *group, xy_ipc_event_bits_t *current);
int xy_ipc_event_group_wait(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                            uint32_t options, uint32_t timeout_ms,
                            xy_ipc_event_bits_t *matched);
```

Implementation notes:

- Store only an OSAL event-flags handle plus small lifecycle metadata in the IPC wrapper.
- Return project-style integer status codes consistently with IPC neighbors; do not leak raw
  backend pointer errors into callers.
- Make output pointer writes deterministic: preserve caller outputs on failure and write them only
  on success, unless a future API explicitly documents otherwise.
- Reject zero-bit waits/sets/clears as invalid input unless the OSAL contract later defines a useful
  no-op behavior.

## Required host CTest before promotion

Before adding the wrapper to the active IPC contract, wire a focused Unity/CTest target named
`test_ipc_event_group` with registered CTest name `ipc_event_group`.

Minimum contracts to cover:

1. `init` rejects NULL group and handles NULL/default name.
2. `set` rejects NULL/uninitialized groups and zero-bit masks.
3. `set` then `get` returns the newly set bits.
4. `clear` returns the pre-clear bits and clears only the requested mask.
5. `wait` with `XY_IPC_EVENT_WAIT_ANY` returns when any requested bit is present.
6. `wait` with `XY_IPC_EVENT_WAIT_ALL` requires the full requested mask.
7. `wait` without `XY_IPC_EVENT_NO_CLEAR` clears matched bits after success.
8. `wait` with `XY_IPC_EVENT_NO_CLEAR` preserves matched bits.
9. Timeout/no-match returns a timeout status and preserves the caller output.
10. `deinit` deletes the OSAL object and makes later operations fail without side effects.

Because PC/bare-metal OSAL event flags may have single-thread semantics, the first host test should
drive already-set/no-match contracts synchronously. Blocking waits from another thread should remain
out of scope until the OSAL PC backend has an explicit threaded wait contract.

## CMake/config plan

Implementation slice should be path-limited to:

- `components/ipc/inc/xy_event_group.h`
- `components/ipc/src/xy_event_group.c` (prefer this explicit name over stale `xy_event.c`)
- `components/ipc/CMakeLists.txt` only if source filtering needs an explicit guard
- `tests/unit/ipc/test_ipc_event_group.c`
- `tests/unit/CMakeLists.txt`
- `components/ipc/README.md`

Do not add root Kconfig symbols in the same slice. If an application later needs IPC event groups to
be feature-gated, handle that as a separate generated-config slice after the API is host-guarded.

## Acceptance gates

A future implementation should pass:

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_ipc_event_group -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^ipc_event_group$'
make test-unit
git diff --check
```

If the wrapper changes root component build behavior, also run:

```bash
make
make HAL_PLATFORM=STM32U5 -j$(nproc)
```

## Open questions

1. Should the wrapper expose ISR/deferred-set APIs, or should ISR users call OSAL directly after backend
   support is proven?
2. Should event-group names be stored in the IPC wrapper for diagnostics, or delegated entirely to OSAL
   event-flags attributes?
3. Does any real consumer need event groups now, or should this remain a documented design boundary until
   a concrete module requests it?

## Suggested next slice

Implement `xy_ipc_event_group` only when a real consumer or test-driven component need appears. The
lowest-risk code slice would be a wrapper backed by local test fakes for `xy_os_event_flags_*`, followed
by focused `ipc_event_group` CTest coverage and full `make test-unit`. Until then, keep the current IPC
README wording that event groups are roadmap/design-stage material.
