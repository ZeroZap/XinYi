# openCH Delay SysTick-source HIL

The WCH blocking delay HAL now derives its tick scale from the active SysTick source. It previously assumed HCLK/8 unconditionally; it now uses HCLK when STCLK is selected and HCLK/8 otherwise.

WCH-Link program, verify and reset passed. The same image emitted eight markers separated by `xy_hal_delay_ms(250)` under HCLK and another eight under HCLK/8. Host monotonic timing produced qualified HCLK intervals of 250.452–251.632 ms (median 250.508 ms) and HCLK/8 intervals of 250.153–251.533 ms (median 250.974 ms), within the 240–270 ms acceptance window.

The first two HCLK host timestamps were excluded because USB/VCP recovery released buffered startup lines together; later HCLK intervals and all HCLK/8 intervals were captured normally. This is bounded blocking-delay evidence, not calibrated oscillator accuracy, microsecond-resolution instrumentation, interrupt-latency characterization, low-power behavior or long-duration drift evidence.
