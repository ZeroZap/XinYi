# openCH GPIO write-state HIL

WCH GPIO write and toggle now reject uninitialized pins and pins configured in non-output modes. Output and alternate-function modes remain writable for board control and peripheral line setup.

WCH-Link program, verify and reset passed. PC4 rejected write/toggle before initialization, rejected both while configured as input, then accepted write-high and toggle-low as confirmed through OUTDR. After deinit, write again returned `XY_HAL_ERROR_NOT_INIT`.

This proves GPIO HAL state enforcement and output-latch behavior. It is not an externally instrumented voltage or edge-timing measurement.
