# openCH EXTI lifecycle HIL

`xy_hal_exti_init()` now clears stale EXTI configuration and disables all WCH EXTI NVIC groups before rebuilding the HAL handle table. This prevents a prior EXTI session from leaking pending or enabled interrupt state into a new initialization.

WCH-Link program, verify and reset passed. Runtime configured line 5, dispatched one software interrupt, deinitialized, confirmed clean not-initialized state, then initialized and configured line 5 again. The second software interrupt produced the expected callback.

This proves EXTI lifecycle cleanup and reinitialization. It does not prove a physical GPIO edge path.
