# openCH GPIO setter validation HIL

WCH GPIO setters now validate their requested mode, pull, output type, speed, or alternate value before checking pin ownership. Invalid values consistently return `XY_HAL_ERROR_INVALID_PARAM`; a valid request on an uninitialized pin returns `XY_HAL_ERROR_NOT_INIT`.

WCH-Link program, verify and reset passed. Both uninitialized and initialized PC4 rejected all invalid setter values. For initialized PC4, CFGLR, OUTDR, cached mode, and cached pull remained unchanged after every rejection.

This proves deterministic setter error precedence and failure atomicity.
