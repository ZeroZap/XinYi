# openCH ADC prescaler HIL

The WCH ADC initializer previously ignored `xy_hal_adc_config_t.clock_div` and always selected PCLK2/8. It now validates dividers 2, 4, 6 and 8 and applies the requested divider during initialization. Runtime `set_prescaler()` uses the same mapping helper.

WCH-Link program, verify and reset passed. The same image initialized ADC1 at PCLK2/6, checked both the HAL value and RCC ADCPRE field, and completed 16 DAC1-to-ADC1 level comparisons. It then switched to PCLK2/8, checked the HAL value and register field again, and completed another 16 comparisons.

This proves divider configuration and the bounded on-chip conversion path. It does not establish ADC clock accuracy, calibrated analog accuracy, linearity, noise, external loading or sampling-rate performance.
