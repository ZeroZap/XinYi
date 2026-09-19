# I2C2 Sensor Validation Evidence

Pandora STM32L475 I2C2 uses `PB10=SCL` and `PB11=SDA` at 100 kHz.
SC7A22H at `0x18` is confirmed on Pandora hardware using the vendor demo configuration. The
captured identity is `WHO_AM_I=0x18`; `COM_CFG=0x50`, `ACC_CONF=0x07`, and `ACC_RANGE=0x01`
read back correctly. `DATA_STAT=0x03` matches the vendor demo's ready condition
`(DATA_STAT & 0x03) == 0x03`; it must not be rejected as a configuration failure without newer
vendor clarification.

## Confirmed slave addresses

| Device | 7-bit address | Evidence |
|---|---:|---|
| SC7A22H | `0x18` | `WHO_AM_I=0x18`, vendor-demo configuration readback, `PB8=1`, continuous XYZ and mg conversion |
| HMC5883L | `0x1E` | `"H43"` identity, gain `0xE0`, DRDY `PA4=1`, continuous XYZ and field conversion |
| BH1750/GY-30 | `0x23` | continuous illuminance samples |
| AHT30 | `0x38` | CRC-valid temperature/humidity samples |
| MPU6050/GY-521 | `0x68` | continuous raw accelerometer/gyro samples |
| L3G4200D | `0x69` | `WHO_AM_I=0xD3`, continuous angular-rate samples |
| BME680 | `0x77` | `CHIP_ID=0x61`, compensated pressure/gas samples |

## Latest bounded HMC5883L evidence

The Pandora probe configured HMC5883L gain to `8.10 Ga`, read back `CONFIG_B=0xE0`, observed `PA4=1`, and produced stable converted output around `266, 98, -208 mgauss` in one continuous capture. This is basic-chain and static evidence, not calibration or dynamic stimulus qualification.

## Latest bounded SC7A22H evidence

With the board stationary, the vendor-demo `±4g` profile produced approximately
`803, -125, -523 mg`, with a vector magnitude of `963–968 mg`. Manual rotation produced clear
changes on all axes. `INT_CFG1=0x01` drove `PB8=1`; EXTI8 counting increased strictly from 19 to
153 across nine captured samples, with increments of 13–19 edges per sample interval. This proves
the routed DRDY level and bounded EXTI edge delivery. The vendor demo explicitly waits for
`(DATA_STAT & 0x03) == 0x03`, so the
observed status is accepted as ready for this silicon/document set despite the PDF text labeling
bit 1 as `CONF_ERR`.
