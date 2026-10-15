# openCH DAC wave-generation register HIL

The WCH DAC HAL previously reported `XY_HAL_ERROR_NOT_SUPPORTED` for triangle and noise generation despite the CH32V30x SDK exposing both hardware modes. The HAL now maps supported triangle amplitudes (1..4095 power-of-two-minus-one values) to the SDK encoding and maps noise masks 0..11 to the SDK LFSR unmask field. Unsupported triangle amplitudes return `XY_HAL_ERROR_INVALID_PARAM`.

WCH-Link program, verify and reset passed. Runtime checked DAC1 triangle amplitude 255, rejected amplitude 256, then checked DAC2 noise mask 5 directly in the DAC CTLR register.

This proves HAL-to-SDK register mapping and parameter rejection. It does not prove analog waveform amplitude, frequency, spectral noise behavior, external trigger timing or oscilloscope-level output quality.
