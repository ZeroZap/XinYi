# openCH FreeRTOS OSAL fast-ISR stress HIL

TIM2 generated 2,000 bounded `WCH-Interrupt-fast` events. Each event entered through the official WCH interrupt-stack macros and called XinYi `xy_os_semaphore_release_from_isr`; the higher-priority consumer acquired exactly 2,000 tokens while an independent lower-priority worker remained live.

WCH-Link program, verify, and reset passed. ISR count and task receive count matched the target, release failures were zero, the worker ran at least 100 iterations, and no timeout/error marker was emitted. Four platform/root builds and 278 host tests passed.

This validates a bounded sustained fast-ISR-to-task chain. It is not nested-interrupt, worst-case latency, stack-watermark, overload/drop-policy, or long-duration qualification.
