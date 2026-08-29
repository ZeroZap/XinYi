# XinYi Sprint 5 Reference RTOS Decision

**Date**: 2026-08-29
**Owner**: Zero
**Status**: `REFERENCE_SELECTED`
**Reference backend**: **FreeRTOS**
**Integration**: `integration-pending`
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
- The repository does **not** contain a project-owned `FreeRTOSConfig.h` or an STM32U5
  Cortex-M33 FreeRTOS port. The checked-in kernel currently exposes only a RISC-V GCC port.
- `components/kernel/osal/CMakeLists.txt` still points to stale third-party include paths and
  only links a kernel target if one already exists. The root default must therefore remain
  bare-metal until the integration gate is implemented.
- RT-Thread has a larger source tree, but current CMake paths and the STM32U5 adapter/port
  assumptions are also stale or incomplete; source volume is not evidence of readiness.

## Next bounded integration slice

1. Add a XinYi-owned minimal `FreeRTOSConfig.h` for the selected reference target.
2. Add or pin a Cortex-M33 non-secure FreeRTOS kernel port without editing vendor source.
3. Make the Kconfig/CMake selection fail closed when config, port, or kernel target is absent.
4. Compile the FreeRTOS adapter and kernel with the real ARM toolchain.
5. Add runtime coverage for thread scheduling, mutex/semaphore, queue, event flags, timeout,
   resource exhaustion, ISR-to-task wakeup, and shutdown/re-init.
6. Extend the runtime to IPC MQ/broker, Trace multi-task behavior, and Device registry/PM
   concurrency before S5-01 can become `DONE`.

## Evidence boundary

This decision is a repository-grounded selection and policy guard only. It **不构成 RTOS runtime、ISR、并发或实板证据**. Host bare-metal tests, source inspection, and the presence of
third-party kernel files do not satisfy C1/Q1/B1/B2. S5-01 remains in progress until a real
reference configuration compiles and the required runtime/stress evidence exists.
