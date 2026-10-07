# openCH CH32V307 DAC1 to ADC1 runtime evidence

DAC1 channel 1 drives PA4 at codes 512 and 3072. ADC1 channel 4 reads the same physical pin after each transition and the smoke requires bounded low/high windows plus a minimum delta. WCH-Link program/verify/reset and UART markers provide B1 tracking-path evidence. This does not establish calibrated voltage accuracy, DAC linearity, settling performance, waveform/DMA behavior, load drive, or endurance.
