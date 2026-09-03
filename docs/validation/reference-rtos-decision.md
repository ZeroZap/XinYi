# XinYi Sprint 5 Reference RTOS Decision

**Date**: 2026-08-29
**Owner**: Zero
**Status**: `BOARD_RUNTIME_PARTIAL`
**Reference backend**: **FreeRTOS**
**Integration**: `root-selected-pandora-task-sync-smoke-verified`
**Hardware**: `pandora-thread-semaphore-queue-b1`; `isr-to-task/stress-pending`

## Decision

Sprint 5 uses **FreeRTOS** as XinYi's single reference RTOS for the first OSAL concurrency
vertical slice. This selects the next integration target; it does not enable FreeRTOS in the
default build and does not claim the adapter or kernel is runtime-qualified.

RT-Thread remains an available source candidate but is **not selected** for this Sprint. Its
current adapter still contains explicitly unsupported thread-flag and simplified enumeration
paths, and the checked-in kernel tree has no identified STM32U5 Cortex-M33 port/config owned by
a XinYi reference board. Maintaining two concurrent RTOS integration paths would violate the
single-backend Sprint scope.

## Repository facts behind the choice

- The FreeRTOS kernel source is present at `third_party/freertos/FreeRTOS` and its upstream
  headers are available locally.
- The XinYi adapter exists at
  `components/kernel/osal/backend/freertos/xy_os_freertos.c` and covers the OSAL API surface,
  while documenting unsupported join/enumeration behavior.
- XinYi now owns a bounded reference `FreeRTOSConfig.h` plus a pinned upstream V10.4.6
  Cortex-M33 non-secure port outside `third_party/`; provenance and file hashes are recorded
  beside the port.
- `freertos_stm32u5_compile` compiles the adapter, required kernel modules, heap and port into
  nine Arm Cortex-M33 objects with `-Werror`.
- Root Kconfig/CMake now exposes target-gated `OSAL_BACKEND_FREERTOS` opt-in for STM32U5 and
  STM32L4, maps it to the canonical OSAL adapter and matching pinned kernel/config/port, and builds
  `xy_osal` plus `freertos_kernel`. An explicit FreeRTOS request on PC fails closed. The root
  default remains bare-metal; STM32U5 remains compile-only while Pandora has the bounded runtime
  evidence recorded below.
- The same pinned FreeRTOS V10.4.6 integration now has an STM32L4 Cortex-M4F port/config and a
  Pandora STM32L475VE runtime image. On 2026-09-03, `46499b33e332f2b4111e1c0149366b5c86064909`
  was programmed and verified with ST-Link; a six-second independent WCH-Link UART capture retained
  the exact embedded commit plus interleaved 500 ms/1000 ms `OSAL_TASK_FAST`/`OSAL_TASK_SLOW`
  markers. This is bounded B1 thread
  scheduling evidence only, not ISR-to-task, synchronization, stress, shutdown/re-init, STM32U5,
  or product RTOS qualification.
- Follow-up images retained task-context semaphore B1 and a depth-2 `uint32_t` message queue B1.
  The queue image `4ebf46dacd8c906455fb541ca70c25b692cc51d8` completed 12 ordered
  `FAST → QUEUE_SEND → SEM_TAKE → QUEUE_RECV → SLOW` cycles with no mismatch or timeout marker.
  The consumer verifies a monotonic sequence before printing `QUEUE_RECV`; this does not prove ISR
  safety, event flags, resource exhaustion, long-duration stress, or another target.
- RT-Thread has a larger source tree, but current CMake paths and the STM32U5 adapter/port
  assumptions are also stale or incomplete; source volume is not evidence of readiness.

## Next bounded integration slice

1. Add runtime coverage for mutex, event flags, timeout, resource exhaustion, ISR-to-task wakeup,
   and shutdown/re-init; scheduler, task-context semaphore, and message queue normal paths are now
   bounded B1 on Pandora.
2. Extend the runtime to IPC MQ/broker, Trace multi-task behavior, and Device registry/PM
   concurrency before S5-01 can become `DONE`.

## Evidence boundary

The STM32U5 gate remains source/static-library compile-only. Pandora now supplies a bounded real-board
scheduler/thread plus task-context semaphore/message-queue smoke for bounded STM32L4 images; it
**不构成 ISR-to-task、event/mutex、压力、性能、STM32U5 或完整 RTOS 产品证据**. S5-01 remains
in progress until the required runtime/stress matrix exists.

Retained UART evidence: [Pandora OSAL/FreeRTOS capture](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-freertos.txt),
SHA-256 `6bde99f52beda6b5b30b3bd7bc655dc8eda116662eae0a96502a36b06264d627`.
