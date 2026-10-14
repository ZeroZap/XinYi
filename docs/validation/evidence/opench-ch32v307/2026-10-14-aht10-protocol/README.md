# openCH AHT10 protocol HIL

WCH-Link program, verify and reset passed. The smoke used the AHT10 command protocol over the canonical WCH I2C HAL: direct status read, conditional `E1 08 00` calibration initialization, `AC 33 00` measurement command, 80 ms conversion delay, and one six-byte measurement frame. The returned frame was not busy, retained its calibration bit, and decoded inside the protocol's physical output ranges.

This is bounded communication and one-sample plausibility evidence. It does not establish humidity/temperature accuracy, calibration traceability, dynamic response, recovery, long-duration stability or endurance.
