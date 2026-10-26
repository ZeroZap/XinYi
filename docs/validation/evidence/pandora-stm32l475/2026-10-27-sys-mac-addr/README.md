# Pandora STM32L475VE SYS MAC getter HIL

`xy_sys_get_mac_addr()` on the Pandora board derives a deterministic unicast,
locally-administered MAC from the MCU 96-bit unique ID, instead of returning the
weak-default `XY_ERROR_NOT_SUPPORTED`. The smoke emits `SYS_MAC_ADDR` on every
reset path, and the same value was observed across external-pin, software, and
watchdog resets.

## Result

- `SYS_MAC_ADDR 2E001B001350` — identical across `EXTERNAL_PIN`, `SOFTWARE`,
  `WATCHDOG` reset kinds.
- `SYS_CHIP_ID 001B002E3647501320313556` — the 96-bit unique ID (matches HAL
  `xy_hal_sys_get_unique_id`).
- MAC derivation: low 6 bytes of UID `2E 00 1B 00 13 50`, bit 0 cleared
  (unicast), bit 1 set (locally administered) → `2E`.
- `FOTA_FIRMWARE_COMMIT 41a11c70…` — image binds to source commit `41a11c70`.
- `PANDORA STM32L475VE XINYI SMOKE OK` repeated; AHT10 sensor ACK/measurement
  loop alive.

## Verification

- Program: `st-flash --serial 066AFF313933554D43244015 write
  pandora_stm32l475_smoke.bin 0x08000000` → `Flash written and verified`.
- ST-Link V2.1 `V2J24S11`, chipid `0x415` (STM32L47x_L48x).
- UART capture via CP2102 (`ttyUSB0`, 115200 8N1); ST-Link VCP (`ttyACM1`)
  produced no application UART.
- Regression: Host `272/272`; `PC` / `STM32L4` / `STM32U5` / `WCH` builds pass;
  `git diff --check` clean.

## Boundary

This proves the Pandora `xy_sys_get_mac_addr()` contract and its deterministic
UID-derived value on real hardware. It is not networking/Ethernet qualification
(the Pandora L475 has no factory Ethernet MAC and this value is a device
identity, not a validated link-layer address), nor a proof of
reset-cause/watchdog electrical behavior beyond the observed CSR values.
