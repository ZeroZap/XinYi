# openCH canonical root FreeRTOS selection HIL

The root Kconfig/CMake path now accepts `OSAL_BACKEND_FREERTOS=ON` for `HAL_PLATFORM=WCH`. It selects exactly one OSAL backend (`FREERTOS=y`, `BAREMETAL=n`), builds the official WCH SDK RISC-V/PFIC FreeRTOS kernel as canonical `freertos_kernel`, and links canonical `xy_osal` against it. The board runtime target now consumes those canonical targets instead of compiling duplicate kernel/adapter sources.

A clean root configure reported `Third Party: FreeRTOS enabled (port: WCH_RISCV_PFIC)` and `XY OSAL: Using FreeRTOS backend`. WCH-Link program, verify, reset, task synchronization, and fast TIM2 ISR through `xy_os_semaphore_release_from_isr` all passed with no error marker.

STM32L4 and STM32U5 FreeRTOS root builds, PC bare-metal root build, CH32V307 FreeRTOS root build, and 278 host tests passed. This establishes the canonical build selection and bounded runtime chain; it does not qualify every OSAL primitive, nested interrupts, all HAL IRQs, or long-duration operation.
