# openCH CH32V307 blocking delay runtime evidence

The smoke calls `xy_hal_delay_us(1000)` and `xy_hal_delay_ms(250)` between UART markers. WCH-Link program/verify/reset succeeded. Host monotonic timestamps around complete UART lines provide bounded interval evidence; UART transmission and scheduling add overhead, so this is not a precision calibration. Internal Flash was considered first but deliberately deferred because the current image/linker layout has no reserved destructive-test partition.
