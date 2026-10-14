# openCH UART6/UART7 SysTick/8 HIL

The WCH UART blocking timeout conversion now follows the active SysTick clock source: HCLK when STCLK is selected and HCLK/8 otherwise. This replaces the unconditional HCLK assumption.

WCH-Link program, verify and reset passed with SysTick explicitly configured for HCLK/8. Eight consecutive rounds each sent `AT` to the onboard ESP8266 over UART6 and observed `OK`, then sent `AT+TPL?` to the onboard CH9141 BLE module over UART7 and observed `TPL`.

This is bounded UART blocking TX/RX and timeout-clock evidence. It proves basic local command exchange only; it does not establish Wi-Fi/BLE RF links, throughput, framing-error recovery, flow control, DMA/IRQ operation or endurance.
