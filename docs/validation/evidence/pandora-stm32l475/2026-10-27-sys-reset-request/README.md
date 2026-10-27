# Pandora STM32L475VE SYS reset-request retention HIL

`xy_sys_reset(reset_by)` on the Pandora board now records the reason in RTC backup
register slot 3 (`PANDORA_RESET_REQUEST_MAGIC | reason`, reason in 0..255) before
issuing the software reset. The next boot decodes the record in `xy_sys_init()`,
reports it through the new `xy_sys_reset_request()` accessor as
`SYS_RESET_REQUEST <n>`, and consumes the record one-shot so a later watchdog or
pin reset cannot re-report a stale reason.

Backup slots 0..2 remain owned by the W25Q128 reset marker and the FOTA
confirm/restage flows.

## Result

Both captures show the same three-boot contract on one programmed image
(`0c6d3be0…`):

| boot | `SYS_RESET_CSR` | `SYS_RESET_KIND` | `SYS_RESET_REQUEST` | meaning |
|------|-----------------|------------------|---------------------|---------|
| 1 | `0x00000000` / `0x00000001` | `POWER_ON` / `EXTERNAL_PIN` | `0` | no retained record |
| 2 | `0x00000005` | `SOFTWARE` | `1` | `XY_SYS_RESET_REQUEST_APP_RESTART` survived the reset |
| 3 | `0x00000009` | `WATCHDOG` | `0` | record consumed one-shot |

- `uart.txt` — capture armed before `st-flash write` + verify; first boot reports
  `POWER_ON` (CSR `0x0`, the probe's core reset does not set RCC reset flags).
- `uart-external-pin.txt` — capture armed before `st-flash reset` on the same
  image; first boot reports `EXTERNAL_PIN` (CSR `0x1`) and the same
  `0 → 1 → 0` request chain.
- `FOTA_FIRMWARE_COMMIT 0c6d3be0…`, `FOTA_BOOT_CONTRACT_OK`, and the AHT10
  ACK/measurement loop continue afterwards, so the retention change did not
  disturb the existing boot contract.

## Verification

- Machine parse: `python3 boards/pandora_stm32l475/validate_sys_reset_request_capture.py
  --capture … --firmware-commit 0c6d3be0aaa5ad685f63f97382b410f4b2cb8efa`
  → `PASS` for both captures (`reset_contract.json`,
  `reset_contract-external-pin.json`).
- Negative controls: the pre-change `41a11c70` capture fails with
  `observed 0 SYS reset record(s)`, and a wrong `--firmware-commit` fails the
  identity binding.
- Program: `st-flash --serial 066AFF313933554D43244015 write
  pandora_stm32l475_smoke.bin 0x08000000` → `Flash written and verified`.
  Independent read-back of the 14296-byte span is byte-identical
  (SHA-256 `d96fee89…2025`, `cmp` passed).
- ST-Link V2.1 `V2J24S11`, chipid `0x415`, 524288 bytes Flash.
- UART: CP2102 `/dev/ttyUSB0`, 115200 8N1.
- Regression: Host `280/280` (including the new
  `test_pandora_sys` retention cases), `PC` / `STM32L4` / `STM32U5` builds,
  Pandora board contract guard, `git diff --check`.

## Boundary

This proves the board-owned retention contract for slot 3 on one Pandora
STM32L475VE with one committed image: the reason survives a real software reset,
is reported once, and is not carried into a watchdog boot. The capture also
shows that a debug-port core reset boots with `SYS_RESET_CSR 0x0`, so reset-kind
evidence still depends on how the reset was triggered.

It does not prove backup-domain retention across power removal/brownout, RTC
tamper/backup-domain-reset behavior, retention while another writer uses slots
0..2 concurrently, or any product/release qualification.
