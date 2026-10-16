# openCH UART auxiliary-state HIL

WCH UART `available`, `flush`, and `error` now require an initialized UART instance. Invalid handles still return `XY_HAL_ERROR_INVALID_PARAM`; valid but uninitialized instances return `XY_HAL_ERROR_NOT_INIT` without touching peripheral state.

WCH-Link program, verify and reset passed. USART1 auxiliary calls succeeded while initialized and returned NOT_INIT before init and after deinit. UART6 auxiliary calls returned NOT_INIT without initialization. USART1 was reinitialized after deinit to keep the physical VCP available for the final marker.

This validates lifecycle boundaries for the auxiliary UART APIs; it does not claim external peer traffic.
