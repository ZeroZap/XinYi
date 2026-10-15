# openCH DMA1 SysTick/8 HIL

The WCH DMA blocking completion timeout now follows the active SysTick clock source: HCLK when STCLK is selected and HCLK/8 otherwise. This replaces the unconditional HCLK assumption.

WCH-Link program, verify and reset passed with SysTick explicitly configured for HCLK/8. DMA1 channel 1 completed 128 memory-to-memory transfers of 256 bytes. Each round used a changing source pattern, required the remaining counter to reach zero, and compared every destination byte.

This is bounded DMA memory-to-memory polling evidence. It does not establish peripheral request routing, circular mode, half-transfer/error interrupts, callback delivery, bus-contention behavior, injected timeout handling or endurance.
