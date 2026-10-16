# openCH CH32V307 official-SDK FreeRTOS port HIL

This smoke consumes the pinned WCH SDK's FreeRTOS kernel, `portable/GCC/RISC-V` PFIC port, startup, system file, and linker script directly from `MCU/wch/CH32V307EVT/EXAM/FreeRTOS/FreeRTOS_Core`. Vendor files were not copied or edited. Board-local code supplies configuration and the validation application.

WCH-Link program, verify, and reset passed. Two tasks ran under the official scheduler and SysTick port. TIM2 used `__attribute__((interrupt("WCH-Interrupt-fast")))`, explicitly switched to the FreeRTOS interrupt stack with `GET_INT_SP/FREE_INT_SP`, called `xSemaphoreGiveFromISR`, requested `portYIELD_FROM_ISR`, and woke a higher-priority task. Ten ISR-to-task transfers completed while the independent worker task continued running; no timeout/error marker appeared.

This is a direct FreeRTOS scheduler and fast external ISR-to-task validation on openCH. It is not yet XinYi OSAL-on-FreeRTOS integration, long-duration stress, nested-IRQ qualification, or a claim that every existing HAL ISR is RTOS-safe.
