# openCH FreeRTOS OSAL timer/thread-flags HIL

The official WCH FreeRTOS configuration now enables the timer daemon with a bounded queue and stack. A periodic XinYi OSAL timer callback signaled a monitor through OSAL thread flags exactly 20 times while an independent lower-priority worker remained live.

WCH-Link program, verify, and reset passed. Periodic callback accounting, active-state observation, stop command, post-stop stability, thread-flag timeout, and worker liveness passed without error markers. The standalone official-SDK port smoke was updated to compile `timers.c`, so the full WCH root build remains valid when timers are enabled. Four platform/root builds and 278 host tests passed.

This is bounded software-timer daemon and thread-flag coverage. It does not prove ISR-origin thread flags, timer queue exhaustion, callback overrun handling, timing accuracy, or long-duration stability.
