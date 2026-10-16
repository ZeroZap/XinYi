# openCH GPIO deinit-state HIL

WCH GPIO deinit now requires the pin to be initialized. A valid but unowned pin and a repeated deinit return `XY_HAL_ERROR_NOT_INIT`; invalid ports or pins remain invalid-parameter errors.

WCH-Link program, verify and reset passed. PC4 rejected deinit before initialization, accepted deinit after output initialization, reported its cached mode as not initialized, and rejected the repeated deinit.

This proves HAL lifecycle and ownership behavior around hardware reset-to-floating-input configuration.
