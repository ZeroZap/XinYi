# openCH GPIO batch atomic HIL

WCH batch writes now validate the entire pin mask before modifying hardware. Every selected pin must be initialized as output or alternate-function. Empty masks are rejected. A valid batch is applied through one BSHR write, avoiding the prior OUTDR read-modify-write race.

WCH-Link program, verify and reset passed. PC4/PC5 were set and cleared in opposite patterns and verified through OUTDR. Batches containing an input or uninitialized pin were rejected without changing OUTDR.

This proves failure-atomic validation and output-latch updates. It is not external electrical or simultaneous-edge instrumentation.
