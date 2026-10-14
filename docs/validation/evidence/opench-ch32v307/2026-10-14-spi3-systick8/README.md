# openCH SPI3 SysTick/8 HIL

The WCH SPI timeout conversion now follows the active SysTick clock source: HCLK when STCLK is selected and HCLK/8 otherwise. This replaces the unconditional HCLK assumption and matches the established CH32V307 timer behavior.

WCH-Link program, verify and reset passed with SysTick explicitly configured for HCLK/8. The smoke completed 64 consecutive W25Q128 JEDEC reads through `xy_hal_spi_transmit()` plus `xy_hal_spi_receive()` and verified `EF4018` each time. It is read-only and does not alter flash contents.

This is bounded SPI3 blocking-transfer and timeout-clock evidence. It does not establish timeout behavior under an electrically stalled peripheral, signal integrity, maximum clock rate, DMA/IRQ operation, write endurance or long-duration reliability.
