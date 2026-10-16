# openCH GPIO init validation HIL

WCH GPIO init now fully validates a configuration before checking pin ownership. Malformed configurations return `XY_HAL_ERROR_INVALID_PARAM`, including on an already-owned pin; only a valid second init returns `XY_HAL_ERROR_ALREADY_INIT`. Validation and duplicate rejection are side-effect free.

WCH-Link program, verify and reset passed. On owned PC4, null and invalid mode/pull/otype/speed/alternate configurations were rejected as invalid parameters. A valid duplicate was rejected as already initialized. CFGLR, OUTDR, and cached mode remained unchanged throughout.

This proves deterministic error precedence and failure atomicity for GPIO initialization.
