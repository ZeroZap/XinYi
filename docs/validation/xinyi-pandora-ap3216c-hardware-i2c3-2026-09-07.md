# Pandora AP3216C hardware-I2C3 validation — 2026-09-07

## Result

Pandora STM32L475VE hardware I2C3 now has a bounded board B1 path through the project layers:

```text
STM32L4 I2C3 peripheral (PC0 SCL / PC1 SDA)
  -> xy_hal_i2c_master_transmit/receive
  -> canonical xy_i2c_device helpers
  -> existing AP3216C driver
```

This closes the previously pending hardware-I2C peripheral normal path for AP3216C. It does not
claim NACK recovery, controlled optical response, accuracy, calibration, or general I2C HAL
qualification.

## Bounded NACK recovery extension

Clean committed firmware `b1c2429c34743070fb0fa883c9f86a346de41f93` first issued a one-byte
hardware-I2C3 receive to unused 7-bit address `0x7F` and required the HAL boundary to return
`XY_HAL_ERROR_IO`. It then initialized the canonical Device helper at AP3216C address `0x1E`, read
back mode `0x03`, and continued sampling. The reset-synchronized capture retained 7449 bytes and
the fail-closed validator found the ordered markers `NACK_OBSERVED→NACK_RECOVERED→CONFIG`, 59
in-range samples, 27 unique tuples, and no firmware error marker. The 11856-byte programmed image
and ST-Link read-back both have SHA-256 `d493618000c69182b656e35edb3d2c6691c61ee85ccc99f0fa25437d767f2f56`.

This grants a bounded address-NACK→next-transaction recovery B2 only. It does not cover SDA/SCL
stuck-low, hot unplug, physical line faults, arbitration, timeout recovery, or general I2C HAL
qualification.

Retained extension files:

- `evidence/pandora-stm32l475/2026-09-07/ap3216c-hw-i2c3-nack-b1c2429c.raw`
- `evidence/pandora-stm32l475/2026-09-07/ap3216c-hw-i2c3-nack-b1c2429c.json`

## Firmware and programming

- Source commit embedded in ELF/UART: `4873bd914b2f595eb6715f3a6021539d55fe34f1`.
- Target: `pandora_stm32l475_ap3216c`, Arm GNU 15.2.1, Release.
- ELF text/data/bss: `11596/92/2852` bytes; BIN size: `11688` bytes.
- ELF SHA-256: `e61274bbcb1413d9ae746771a6e979a9d6d71a98c0a8f12ac26cd3b828c9dbe6`.
- BIN/read-back SHA-256: `70abf49eb646dbb5a892606953ef5a07e99799fa2db73d6e4219a54143ad6a04`.
- Probe: ST-Link V2J24S11, STM32L47x/L48x chip ID `0x415`.
- `st-flash --reset write` reported `Flash written and verified`; 11688-byte read-back was
  byte-identical.

## Runtime evidence

Independent WCH-Link UART capture retained 7107 bytes. The exact committed firmware cycle contains:

- `PANDORA AP3216C SENSOR READY`;
- exact `FIRMWARE_COMMIT` identity;
- `AP3216C_BUS=HW_I2C3 SCL=PC0 SDA=PC1 ADDR=0x1E`;
- `AP3216C_HW_I2C_READY`;
- configuration read-back `0x03` (`ALS_PS`);
- 29 in-range samples after the matching boot banner, 17 unique value tuples;
- zero AP3216C firmware error markers.

One non-ASCII byte occurred at a reset boundary (`PAND\xffPANDORA`); the validator records it and
permits at most four such boundary bytes while still requiring the complete subsequent ordered
identity/config/sample chain.

Retained files:

- `evidence/pandora-stm32l475/2026-09-07/ap3216c-hw-i2c3-4873bd91.raw`
- `evidence/pandora-stm32l475/2026-09-07/ap3216c-hw-i2c3-4873bd91.json`

## Verification

- focused board policy: `pandora_stm32l475_board` passed;
- focused capture contract: `pandora_ap3216c_hw_i2c_capture_validator` passed;
- ELF symbols include `Reset_Handler`, `SysTick_Handler`, `HAL_I2C_Init`,
  `HAL_I2C_Master_Transmit`, and `HAL_I2C_Master_Receive`;
- broader Host/PC/L4/U5 gates are recorded with the closing commit.
