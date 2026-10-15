# openCH DAC GPIO ownership HIL

The generic WCH DAC HAL no longer enables GPIOA or forces both PA4 and PA5 into analog mode during `xy_hal_dac_init()`. Pin muxing is board/application ownership. The openCH DAC/ADC smoke now explicitly configures only PA4 as analog before initializing DAC1 channel 1 and ADC1 channel 4.

WCH-Link program, verify and reset passed. Four runtime rounds drove DAC values 512 and 3072, sampled PA4 through ADC1 channel 4, and required the low sample to be 100–1200, the high sample to be 2400–3800, and their difference to be at least 1800 counts.

This proves the bounded on-chip DAC-to-ADC path still works after ownership separation. It does not establish calibrated voltage accuracy, PA5/DAC channel 2, external loading, settling specifications, linearity, noise or waveform quality.
