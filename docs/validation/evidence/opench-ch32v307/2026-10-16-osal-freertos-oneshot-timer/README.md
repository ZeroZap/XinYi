# openCH FreeRTOS OSAL one-shot timer lifecycle HIL

The first hardware run exposed a real backend ABI bug: `xy_os_timer_func_t` accepts the user argument, while FreeRTOS invokes callbacks with a timer handle. The prior cast passed the timer handle as the application argument. The adapter now owns a small callback context, uses a correctly typed FreeRTOS trampoline, forwards the original user argument, and frees that context after a successful delete command.

WCH-Link program, verify, and reset passed after the fix. One-shot fire, inactive state after expiry, restart with updated argument data, deletion of an active timer, no callback after deletion, timer recreation/deletion, and independent task liveness all passed. Four platform/root builds and 278 host tests passed.

This is bounded one-shot lifecycle/resource recovery. It does not prove timer-command queue exhaustion, deletion from inside callbacks, concurrent delete races, allocation failure, or long-duration heap behavior.
