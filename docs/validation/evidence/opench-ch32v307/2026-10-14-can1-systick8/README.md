# openCH CAN1 SysTick/8 HIL

The WCH CAN blocking send/receive timeout and receive timestamp conversion now follow the active SysTick clock source: HCLK when STCLK is selected and HCLK/8 otherwise. This replaces the unconditional HCLK assumption.

WCH-Link program, verify and reset passed with SysTick explicitly configured for HCLK/8. Sixty-four internal-loopback rounds transmitted and received a standard CAN1 data frame with ID `0x123`, DLC 4, and a per-round changing payload byte; ID, type, DLC and all payload bytes were compared.

This is bounded CAN controller internal-loopback evidence. It does not establish the external transceiver, termination, arbitration on a physical bus, dominant/recessive electrical levels, error injection, bus-off recovery, timing margin, EMI behavior or multi-node interoperability.
