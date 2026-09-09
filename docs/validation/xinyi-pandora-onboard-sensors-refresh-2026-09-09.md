# Pandora onboard Sensor validation refresh — 2026-09-09

## Scope

Bounded unattended runtime refresh for the three sensor ICs present on Pandora STM32L475VE V2.4:

- U7 AHT10 temperature/humidity
- U8 AP3216C ALS/proximity/IR
- U11 ICM-20608 accelerometer/gyroscope/temperature

Source commit: `e9489ec78fe472b2f0243cf3f8aa30634a093171`.
Board probe: ST-Link V2.1 serial `066AFF313933554D43244015`, firmware `V2J24S11`.
UART: WCH-Link stable path `/dev/serial/by-id/usb-wch.cn_WCH-Link_B49C8F0639CE-if01`, 115200 8N1.

## Build and image identity

All three board images were configured for `HAL_PLATFORM=STM32L4` and
`STM32L4_BOARD=pandora_stm32l475`, then built successfully.

| Image | Bytes | SHA-256 |
|---|---:|---|
| `pandora_stm32l475_smoke.bin` (AHT10 path) | 13772 | `c698e7c147c3520df33b0a22d5d9453965ad3b3e8d92632b661eb443b80d2914` |
| `pandora_stm32l475_ap3216c.bin` | 12716 | `a62c25eff555e145580eef881ee453eb3f5e5b723631eb0d190ceaa4645ca6d7` |
| `pandora_stm32l475_icm20608.bin` | 11336 | `207d075900e2c0a4f4fb7524cf2db98f8963677fe1628279c73f7490559b6b15` |

Each image was programmed with ST-Link verification. AP3216C and ICM20608 were also read back over SWD for their exact image length; `cmp` succeeded and read-back SHA-256 matched the source BIN.

## AHT10 result

Raw UART: `/tmp/pandora-aht10-e9489ec7-clean.log`

- bytes: 4158
- UART SHA-256: `358e6a7ccce12ffb70f001975ec0c9190704c31b6da0df75977103a272b31f4a`
- exact source commit observed: yes
- complete banner/ACK/measurement cycles: 24
- NACK: 0
- humidity: 64241–64379 milli-percent, 23 unique values
- temperature: 30232–30253 milli-degrees C, 15 unique values

Result: existing bounded static AHT10 B1 is refreshed at the current source commit. This does not prove calibrated accuracy, controlled stimulus response, condensation behavior, or long-duration stability.

## AP3216C result

Raw UART: `/tmp/pandora-ap3216c-e9489ec7-identity.log`
Read-back: `/tmp/pandora-ap3216c-e9489ec7-readback.bin`

- bytes: 7603
- UART SHA-256: `997fb631c3140725f82c5b43430d779599857e22ece06998ef1d408121843b09`
- exact source commit observed: yes
- hardware I2C3 ready: yes
- unused-address `0x7F` NACK observed, then access to `0x1E` recovered: yes
- config: `0x03` continuous ALS+PS
- error markers: 0
- bounded samples: 106
- unique raw samples: 63
- ALS: 9–10 lux
- proximity: 0–13 raw
- IR: 0–514 raw

Result: current-commit identity/config/static sampling plus software NACK→subsequent-device-access recovery is confirmed. Prior bounded manual IR stimulus evidence remains the authority for response; this unattended run does not add a new controlled stimulus or quantitative accuracy claim.

## ICM20608 result

Raw UART: `/tmp/pandora-icm20608-e9489ec7-clean.log`
Read-back: `/tmp/pandora-icm20608-e9489ec7-readback.bin`

- bytes: 12251
- UART SHA-256: `e6ca3a30f8959783c218f8f97c250de72bfabac24730154752ea880d0c56791f`
- exact source commit observed: yes
- I2C scan ACKs include `0x68`; WHO_AM_I: `0xAE`
- init contract: accel-only init, reset count 1
- raw 14-byte bursts: 88/88 unique
- register snapshots: `01,00,08,08,04,04,01` and `01,00,08,08,04,04,05`; the last status byte varies with data-ready state
- acceleration: X -111..-102 mg, Y 19..28 mg, Z 990..1000 mg
- gyroscope: X 0..1 dps, Y 0 dps, Z 0 dps
- error markers: 0

Result: current-commit identity/config/fresh static sampling B1 is confirmed. Dynamic orientation/motion response, calibration, accuracy and long-duration stability remain pending and require a controlled movement fixture or operator action.

## Limitations

- No BQ25620 work was performed.
- No controlled optical, proximity, humidity, temperature, or motion stimulus was applied.
- Runtime ranges are observations from one bounded run, not sensor accuracy specifications.
- Temporary raw logs/read-backs remain under `/tmp`; this record retains hashes and exact paths but does not archive those large artifacts in Git.
