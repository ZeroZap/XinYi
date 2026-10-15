# openCH System SysTick-source HIL

The WCH System HAL tick count and microsecond timestamp now derive their scale from the active SysTick clock source. They previously assumed HCLK/8 unconditionally; they now use HCLK when STCLK is selected and HCLK/8 otherwise.

WCH-Link program, verify and reset passed. The same image measured a raw-counter-defined 20 ms window under each clock source. `xy_hal_sys_get_timestamp_us()` returned 19,000–21,000 us and `xy_hal_sys_get_tick_count()` returned 19–21 ms for both HCLK and HCLK/8.

This is bounded free-running counter conversion evidence. It does not establish oscillator accuracy, long-duration drift, interrupt-driven ticks, low-power retention, cross-clock continuity while the source is changed mid-interval or wall-clock timekeeping.
