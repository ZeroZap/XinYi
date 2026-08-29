# XinYi Sprint 5 Reference RTOS Decision

**Date**: 2026-08-29
**Owner**: Zero
**Status**: `REFERENCE_SELECTED`
**Reference backend**: **FreeRTOS**
**Integration**: `root-selected-compile-guarded-runtime-pending`
**Hardware**: `hardware-pending`

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
- Root Kconfig/CMake now exposes an STM32U5-only `OSAL_BACKEND_FREERTOS` opt-in, maps it to both
  the canonical OSAL adapter and matching pinned kernel/config/port, and builds `xy_osal` plus
  `freertos_kernel`. An explicit FreeRTOS request on PC fails closed. The root default remains
  bare-metal because this gate still does not link a runnable image or provide scheduler evidence.
- RT-Thread has a larger source tree, but current CMake paths and the STM32U5 adapter/port
  assumptions are also stale or incomplete; source volume is not evidence of readiness.

## Next bounded integration slice

1. Add runtime coverage for thread scheduling, mutex/semaphore, queue, event flags, timeout,
   resource exhaustion, ISR-to-task wakeup, and shutdown/re-init.
2. Extend the runtime to IPC MQ/broker, Trace multi-task behavior, and Device registry/PM
   concurrency before S5-01 can become `DONE`.

## Evidence boundary

This slice adds a clean source-level STM32U5 Cortex-M33 compile gate. It **不构成 RTOS runtime、ISR、并发或实板证据**. The gate does not link startup/vector/HAL code or execute a scheduler,
so S5-01 remains in progress until a link/runtime fixture and the required runtime/stress evidence
exist.
