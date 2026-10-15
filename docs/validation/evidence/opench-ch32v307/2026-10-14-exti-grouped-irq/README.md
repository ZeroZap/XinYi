# openCH EXTI grouped-IRQ HIL

The WCH EXTI HAL now keeps a shared NVIC IRQ group enabled while any EXTI line in that group remains enabled. Disabling line 5 no longer disables `EXTI9_5_IRQn` when line 6 is still active.

WCH-Link program, verify and reset passed. Lines 5 and 6 were mapped, configured and software-triggered. The line-5 callback fired once for its trigger. Line 5 was then disabled while line 6 remained enabled; a second line-6 software trigger did not change the line-5 callback count, proving the shared IRQ remained active without invoking the disabled line's callback.

This proves grouped NVIC ownership and software-trigger dispatch. It is not physical GPIO edge evidence; the board pin edge path remains a separate HIL item.
