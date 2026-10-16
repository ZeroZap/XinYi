# openCH GPIO reinit boundary HIL

WCH GPIO initialization now rejects a second init of an owned pin with `XY_HAL_ERROR_ALREADY_INIT`. This prevents accidental ownership takeover and unrequested register/cache replacement. Runtime reconfiguration remains available through the explicit setters, while deinit followed by init remains the ownership-transfer path.

WCH-Link program, verify and reset passed. PC4 was initialized as output; a second input/pull-up init was rejected without changing CFGLR or cached mode/pull. Explicit set-mode/set-pull succeeded, then deinit followed by init succeeded.

This proves HAL ownership, failure atomicity, and explicit reconfiguration paths.
