# I2C2 Sensor Validation Evidence

Pandora STM32L475 I2C2 uses `PB10=SCL` and `PB11=SDA` at 100 kHz.
SC7A22H at `0x18` is intentionally paused: ACK is observed, but identity/register layout is not confirmed.

## Confirmed slave addresses

| Device | 7-bit address | Evidence |
|---|---:|---|
| HMC5883L | `0x1E` | `"H43"` identity, gain `0xE0`, DRDY `PA4=1`, continuous XYZ and field conversion |
| BH1750/GY-30 | `0x23` | continuous illuminance samples |
| AHT30 | `0x38` | CRC-valid temperature/humidity samples |
| MPU6050/GY-521 | `0x68` | continuous raw accelerometer/gyro samples |
| L3G4200D | `0x69` | `WHO_AM_I=0xD3`, continuous angular-rate samples |
| BME680 | `0x77` | `CHIP_ID=0x61`, compensated pressure/gas samples |

## Latest bounded HMC5883L evidence

The Pandora probe configured HMC5883L gain to `8.10 Ga`, read back `CONFIG_B=0xE0`, observed `PA4=1`, and produced stable converted output around `266, 98, -208 mgauss` in one continuous capture. This is basic-chain and static evidence, not calibration or dynamic stimulus qualification.

## Evidence boundary

No SC7A22H identity, register-layout, data-format, or PB8/PB9 interrupt claim is made. No manual hardware action was required for the latest capture.
