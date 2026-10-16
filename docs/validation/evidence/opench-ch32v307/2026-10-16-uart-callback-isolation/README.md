# openCH UART callback isolation HIL

WCH UART error callbacks are now stored per UART instance rather than in one global slot. Registration and synthetic error dispatch require an initialized instance. Deinit clears only that instance's callback and argument.

WCH-Link program, verify and reset passed. USART1 and UART6 registered different callbacks and arguments; each synthetic error reached only its own callback. UART6 deinit suppressed dispatch, reinit did not restore its stale callback, and USART1 remained registered and dispatchable.

Runtime markers were emitted through physical USART1/WCH-Link VCP. UART6 callback checks exercise HAL dispatch state and do not claim an external peer or hardware framing error.
