# openCH UART lifecycle HIL

The WCH UART backend now tracks initialization independently for USART1/2/3 and UART4/5/6/7/8. Duplicate initialization returns `XY_HAL_ERROR_ALREADY_INIT`; deinit, send, and receive on an uninitialized instance return `XY_HAL_ERROR_NOT_INIT`. Deinit clears only the selected instance state.

WCH-Link program, verify and reset passed. Runtime markers were emitted through USART1/WCH-Link VCP. USART1 pre-init send/receive/deinit and duplicate-init boundaries passed. UART6 init, duplicate init, deinit, post-deinit send/receive, and repeated deinit boundaries passed.

This proves HAL lifecycle isolation and USART1 physical TX. UART6 lifecycle checks do not claim an external peer transaction in this slice.
