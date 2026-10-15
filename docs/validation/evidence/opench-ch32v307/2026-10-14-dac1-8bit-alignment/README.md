# openCH DAC 8-bit alignment HIL

The WCH DAC HAL previously accepted left alignment for 8-bit resolution, but the CH32V30x DAC SDK only provides an 8-bit right-aligned holding register. The HAL now rejects 8-bit/left-aligned initialization and rejects `set_value()` alignment that disagrees with the initialized configuration.

WCH-Link program, verify and reset passed. The smoke initialized DAC1 channel 1 for 8-bit right alignment, verified that a left-aligned write returned `XY_HAL_ERROR_INVALID_PARAM`, then completed 32 DAC-to-ADC comparisons using values 32 and 224.

This proves supported 8-bit right-aligned register selection, configuration enforcement and the bounded on-chip DAC-to-ADC path. It does not establish calibrated voltage accuracy, external loading, linearity, noise or waveform quality.
