# openCH ADC1 SysTick/8 HIL

The WCH ADC blocking conversion timeout now follows the active SysTick clock source: HCLK when STCLK is selected and HCLK/8 otherwise. This replaces the unconditional HCLK assumption.

WCH-Link program, verify and reset passed with SysTick explicitly configured for HCLK/8. Thirty-two rounds drove DAC1 channel 1 on PA4 at codes 512 and 3072, read the same pad through ADC1 channel 4, and verified bounded low/high ranges plus at least 1800 counts of separation.

This is bounded ADC conversion completion and DAC-to-ADC response evidence. It does not establish absolute analog accuracy, calibrated reference voltage, INL/DNL, noise, external source impedance behavior, injected channels, DMA/IRQ operation or endurance.
