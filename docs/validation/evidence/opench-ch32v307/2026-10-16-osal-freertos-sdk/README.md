# openCH XinYi OSAL over official WCH FreeRTOS HIL

The board target links XinYi's canonical FreeRTOS OSAL adapter directly with the pinned WCH SDK FreeRTOS kernel, RISC-V/PFIC port, startup, system file, and linker script. Vendor SDK files remain unmodified.

WCH-Link program, verify, and reset passed. The application used only `xy_os_*` APIs to initialize/start the kernel, create two threads, delay the worker, create/acquire a semaphore, and release it from TIM2 ISR. TIM2 used `WCH-Interrupt-fast` plus the official port's interrupt-stack switch. Ten `xy_os_semaphore_release_from_isr` wakeups completed while the independent OSAL worker continued running; no timeout or ISR-release error marker appeared.

This proves a bounded openCH XinYi OSAL/FreeRTOS scheduler, task synchronization, and fast external ISR-to-task path. It does not yet prove root `-DOSAL_BACKEND=freertos` selection for WCH, every OSAL primitive, all HAL IRQ handlers, nested IRQs, or long-duration stability.
