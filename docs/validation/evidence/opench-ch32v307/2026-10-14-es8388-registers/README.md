# openCH ES8388 register HIL

WCH-Link program, verify and reset passed. The smoke read a bounded snapshot of codec control, power, clock, serial-format, mixer and volume registers through the canonical WCH I2C HAL. It then modified both DAC digital-volume registers with one-bit reversible values, verified both readbacks, and restored and verified both original values.

This is bounded register transport and reversible-state evidence. The smoke deliberately avoids reset, analog power-path switching and speaker/headphone activation. It does not establish I2S sample transfer, DAC/ADC conversion, microphone/headphone audio, gain accuracy, noise, recovery or endurance.
