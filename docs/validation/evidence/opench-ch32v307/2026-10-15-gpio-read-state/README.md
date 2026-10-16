# openCH GPIO read-state HIL

WCH single-pin reads now require the pin to have been initialized. Invalid port/pin values remain invalid-parameter errors; a valid but unowned or deinitialized pin returns `XY_HAL_ERROR_NOT_INIT`.

WCH-Link program, verify and reset passed. PC4 rejected reads before initialization, returned a valid binary level after input initialization, and rejected reads again after deinitialization.

This proves HAL ownership and lifecycle checks around INDR access. It does not identify or certify the external voltage present on PC4.
