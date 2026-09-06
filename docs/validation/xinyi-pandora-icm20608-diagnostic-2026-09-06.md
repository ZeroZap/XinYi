# Pandora ICM20608 movement diagnostic — 2026-09-06

## Result

The Pandora ICM20608 at software-I2C3 address `0x68` has real identity/static-sampling B1 evidence,
but no observed movement/orientation response. Dynamic B1 remains pending.

Two user-operated captures were analyzed without promoting them beyond their evidence:

- `icm20608-user-motion-final.raw`: 224 samples; accel ranges X `-25..-10`, Y `-12..-2`,
  Z `999..1003` mg; gyro fixed at `1,0,0` dps.
- `icm20608-user-orientation-final.raw`: 269 samples; accel ranges X `-30..28`, Y `-25..2`,
  Z `998..1008` mg; gyro fixed at `1,0,0` dps, despite reported approximately 90-degree holds.

This is consistent with a stationary sensor and does not demonstrate that the moved object was the
ICM20608-bearing assembly.

## Driver/config diagnosis

The board application creates accel and gyro views but calls `init()` only through the accel view.
Therefore the suspected accel-then-gyro double reset is not present in this runtime path. The
diagnostic image explicitly emits `ICM20608_INIT_PATH=ACCEL_ONLY RESET_COUNT=1`.

Committed firmware `e3c5f87d69db8b2f04611779dbdbd7f32d73a965` logs:

- each 14-byte burst from `ACCEL_XOUT_H` through the gyro data registers;
- `PWR_MGMT_1`, `PWR_MGMT_2`, `ACCEL_CONFIG`, `GYRO_CONFIG`, `CONFIG`, `ACCEL_CONFIG2`, and
  `INT_STATUS` on every cycle.

An unattended reset-synchronized 8-second capture produced 58 bursts and all 58 raw byte strings
were unique. Register snapshots stayed at `01,00,08,08,04,04`, while `INT_STATUS` was `01` or `05`.
There were no I/O error markers. This demonstrates that the burst transaction advances across all
14 bytes and that samples/data-ready state update; it rules out a software-side frozen register
read in this path. It does not prove physical movement response because no movement was performed
during this unattended capture.

## Build/flash identity

- ELF SHA-256: `c23fe466e4559c1681696ea9899095aa751dc437f2677b95905828d05809d69f`
- BIN/read-back SHA-256: `fc1e25595b3cbb37e8832ddf9b75e0f95cb73841bf2bfcf2114b801da03cb535`
- Symbols: `Reset_Handler=0x08000e30`, `SysTick_Handler=0x08000294`
- ST-Link: STM32L47x/L48x, `Flash written and verified`; byte-for-byte read-back passed.
- Diagnostic UART SHA-256: `36ca65f3521e3c484951bb7919747d32e46177311a077b949fd55ae78754a150`

## Evidence files

All paths are relative to `docs/validation/evidence/pandora-stm32l475/2026-09-06/`:

- `icm20608-user-motion-final.raw` — SHA-256 `3b04bdae509ac76db1c6b51689554ccb6af808c96831ebc985ba19779ef34a89`
- `icm20608-user-orientation-final.raw` — SHA-256 `e73eecb9c2f28385c8501fa3a64119e059c9bdd225a192fd4c40a94368734f9f`
- `icm20608-raw-diagnostic-e3c5f87d.raw` — SHA-256 `36ca65f3521e3c484951bb7919747d32e46177311a077b949fd55ae78754a150`
- `uart-wchlink-icm20608-997f46f8.txt` — prior identity/static capture, SHA-256
  `6d0b59ec6af5ef62a2226f1d1f122ec104253c6c88962794ea62bbbf86bd119f`

The ignored binary read-back is intentionally identified by hash rather than committed.

## Next evidence boundary

The checked-in Pandora V2.4 schematic identifies the axis sensor as `U11 ICM-20608`, connected to
`IIC_SCL1`/`IIC_SDA1`. The vendor's official product page identifies its photograph as the front of
the Pandora IoT development board, but the available 350×243 image does not make the `U11` silkscreen
legible. Therefore the safe unambiguous object to move is the complete Pandora STM32L475VE main PCB,
not the ST-Link, WCH-Link, LCD, Wi-Fi module, or another attached/daughter PCB. The main PCB should be
held by its edges with both USB/UART leads slack, then moved through flat, left-edge-down, and
right-edge-down 90-degree holds while briefly rotating between holds.

A fresh reset-synchronized 30-second capture was armed against the independent WCH-Link UART, but no
operator action could be coordinated from the unattended cron run. It retained 222 samples, all 222
raw bursts unique, and the exact `e3c5f87d69db8b2f04611779dbdbd7f32d73a965` firmware identity.
Accelerometer ranges remained X `-42..-37`, Y `-21..-17`, Z `998..1003` mg; gyro ranges remained X
`0..1`, Y `0`, Z `0` dps. This is a valid reset/capture-path recheck but not a movement attempt and
does not upgrade dynamic evidence. The raw capture is
`evidence/pandora-stm32l475/2026-09-06/icm20608-main-pcb-orientation-30s.raw`, SHA-256
`d8fd721bd8a423163297e9ef0ac1aae8fff409941a393d895e0a5eb388eb9293`.

Dynamic acceptance still requires substantial accel-axis redistribution during those held
orientations and a non-static gyro transient during rotation. If moving the complete main PCB is not
safe, stop this validation rather than repeat stationary captures and proceed to the board's `U8`
AP3216C at the positively scanned `0x1E` address; cover/proximity response still requires a separately
coordinated optical stimulus.