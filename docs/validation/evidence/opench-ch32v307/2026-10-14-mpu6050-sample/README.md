# openCH MPU6050 sample HIL

WCH-Link program, verify and reset passed. The smoke verified WHO_AM_I `0x68`, saved PWR_MGMT_1, cleared only the sleep bit, waited 100 ms, read the contiguous 14-byte accelerometer/temperature/gyroscope frame through the canonical WCH I2C HAL, checked a protocol-plausible temperature and nonzero motion fields, then restored and verified the original PWR_MGMT_1 value.

This is bounded identity, configuration, frame transport and reversible-state evidence. It does not establish acceleration/gyro/temperature accuracy, calibration, alignment, noise, interrupt behavior, dynamic response, recovery or endurance.
