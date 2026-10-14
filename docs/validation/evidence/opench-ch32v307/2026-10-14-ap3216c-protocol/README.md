# openCH AP3216C protocol HIL

WCH-Link program, verify and reset passed. The smoke saved SYSTEM_CONFIG, enabled ALS+PS/IR mode, waited 120 ms, read the contiguous six-byte IR/ALS/PS frame through the canonical WCH I2C HAL, checked IR/PS overflow flags and decoded field widths, then restored and verified the original SYSTEM_CONFIG value.

This is bounded communication, data-frame and reversible-configuration evidence. It does not establish optical accuracy, calibration, interrupt behavior, dynamic response, cover-glass performance, recovery, long-duration stability or endurance.
