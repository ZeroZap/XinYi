# openCH I2S2 SysTick-source HIL

The WCH I2S blocking timeout conversion now follows the active SysTick clock source. It previously assumed HCLK/8 unconditionally; it now uses HCLK when STCLK is selected and HCLK/8 otherwise.

WCH-Link program, verify and reset passed. The same image completed 64 four-sample SPI2/I2S master-TX calls with SysTick sourced from HCLK, then switched SysTick to HCLK/8 and completed another 64 calls. Every call returned the expected sample count and delivered one TX-complete callback.

This is bounded I2S controller/TXE and software-callback evidence. It does not establish physical I2S waveforms, sample-rate accuracy, ES8388 audio data reception, analog power paths, speakers, headphones, actual audio output, DMA/IRQ operation or endurance.
