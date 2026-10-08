# openCH CH32V307 EXTI0 software IRQ evidence

Identity-gated UART evidence requires boot, READY and repeated callback markers after WCH-Link program/verify/reset. The WCH backend configures EXTI0, AFIO routing, PFIC/NVIC enable, software EXTI pending and explicit PFIC pending delivery, then dispatches through the linked WCH fast-interrupt ABI handler to the registered callback. This proves the software-triggered controller/IRQ/callback chain; it does not prove an external PA0 edge, debounce, GPIO electrical behavior, latency, nested IRQ behavior, recovery or endurance.
