# XY OSAL Backend Comparison

## Evidence boundary

This document is a **source/compile inventory only**. XinYi currently selects FreeRTOS as the
Sprint 5 reference backend and has a guarded STM32U5 Cortex-M33 source/static-library compile
path. The integration state is `runtime-pending`.

XinYi has **no project-owned runtime benchmark evidence** for context-switch latency, interrupt
latency, memory footprint, power, scheduler behavior, ISR-to-task wakeups, or concurrent stress.
The tables below describe visible adapter mappings; they do not claim runtime, hardware,
performance, safety, or production qualification. Upstream ecosystem features are not XinYi
integration evidence.

## Current backend status

| Backend | Sprint role | Root selection | Current evidence | Known gaps |
|---|---|---|---|---|
| Bare-metal | Default fallback | Default on supported configurations | Host contract + PC build | No scheduler; thread and synchronization semantics are limited |
| FreeRTOS | Single Sprint 5 reference | Explicit STM32U5-only opt-in | Pinned V10.4.6 config/port; Arm GNU `-Werror` compile; root static-library build | No runnable image, scheduler/ISR/concurrency test, resource-exhaustion test, shutdown/re-init, or B1/B2 |
| RT-Thread | Not selected this Sprint | Not a canonical root selection | Source candidate only | Adapter has unsupported/simplified paths; no XinYi-owned STM32U5 config/port/runtime gate |

The authoritative selection and evidence boundary are in
[`docs/validation/reference-rtos-decision.md`](../../../docs/validation/reference-rtos-decision.md).

## Adapter mapping inventory

A mapped call means that the adapter references an RTOS primitive. It does **not** mean the path
has been executed or qualified.

### Kernel and thread mapping

| OSAL call | Bare-metal adapter | FreeRTOS adapter | RT-Thread adapter |
|---|---|---|---|
| `xy_os_kernel_start()` | Local state transition | `vTaskStartScheduler()` | Kernel assumed externally started |
| `xy_os_kernel_lock()` | Local nesting counter | `vTaskSuspendAll()` | `rt_enter_critical()` |
| `xy_os_thread_new()` | Unsupported | `xTaskCreate()` | `rt_thread_create()` |
| `xy_os_thread_terminate()` | Unsupported | `vTaskDelete()` | `rt_thread_delete()` |
| `xy_os_thread_suspend()` | Unsupported | `vTaskSuspend()` | `rt_thread_suspend()` |
| `xy_os_thread_set_priority()` | Unsupported | `vTaskPrioritySet()` | `rt_thread_control()` |
| `xy_os_thread_yield()` | No-op | `taskYIELD()` | `rt_thread_yield()` |

### Synchronization and communication mapping

| Capability | FreeRTOS adapter | RT-Thread adapter | Evidence note |
|---|---|---|---|
| Mutex | FreeRTOS semaphore/mutex API | `rt_mutex_*` | Source mapping only |
| Semaphore | FreeRTOS semaphore API | `rt_sem_*` | ISR behavior not yet tested by XinYi |
| Event flags | FreeRTOS event groups | RT-Thread event API | Timeout/clear semantics need runtime coverage |
| Message queue | FreeRTOS queue API | RT-Thread message queue API | Queue full/empty and concurrency need runtime coverage |
| Memory pool | Adapter-owned free list over RTOS allocation | RT-Thread memory pool API | FreeRTOS adapter allocation ownership and exhaustion need runtime coverage |
| Software timer | FreeRTOS timer daemon | RT-Thread timer API | Callback context and shutdown/re-init need runtime coverage |

### Known API limitations

- Bare-metal remains the default but cannot provide real concurrent thread scheduling.
- The FreeRTOS adapter explicitly does not implement thread join or enumeration; stack-size
  reporting is incomplete.
- The RT-Thread adapter contains unsupported or simplified thread-flag/enumeration paths.
- Application portability is not yet proven: backend timeout units, ISR-safe entry points,
  lifecycle, priority mapping, and resource limits still require tests.
- FreeRTOS is the only reference backend in this Sprint. Do not maintain two simultaneous
  runtime integration tracks.

## Selection guidance

Use the backend selected by the product's validated configuration, not this file as a performance
ranking:

- Keep bare-metal for simple/default builds that do not require concurrency.
- Use the FreeRTOS opt-in only for continued STM32U5 integration work until runtime evidence is
  complete.
- Do not select RT-Thread for the current Sprint unless the reference decision is deliberately
  reopened and the same config/port/runtime evidence is supplied.

No backend is currently approved by XinYi for safety-critical use, hardware timing claims, power
claims, or production readiness.

## Required runtime gate

Before FreeRTOS can move beyond `runtime-pending`, a runnable and repeatable fixture must cover:

1. scheduler start and at least two task context switches;
2. mutex, semaphore, queue, event flags, thread flags, and timeout semantics;
3. resource exhaustion with explicit error propagation;
4. ISR-to-task wakeup through supported ISR-safe primitives;
5. timer callback behavior and tick progression;
6. shutdown/re-init or an explicit documented unsupported contract;
7. IPC MQ/broker, Trace multi-task behavior, and Device registry/PM concurrency;
8. board-owned STM32U5 B1/B2 logs before any hardware claim.

Performance or memory tables may be added only with a reproducible XinYi record containing the
exact board/QEMU model, CPU frequency, toolchain, optimization, config, sample method, raw output,
and commit SHA.
