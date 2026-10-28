# Pandora AHT10 canonical owner — board runtime (2026-10-28)

## Scope

Turn U7 AHT10 into a canonical Device owner on the Pandora reference board, the
same way AP3216C and ICM20608 were owner-migrated (S6-116), and take the first
machine-judged board runtime for that owner.

Before this slice the only AHT10 board path was `main.c`, which built an
`xy_i2c_device_t` inline, sent `0xE1 0x08 0x00` and `0xAC 0x33 0x00` by hand and
re-implemented the 20-bit humidity/temperature decode. That code was outside the
driver and outside every host test, and the resulting capture was only judged by
`capture_uart.py`, which is welded to that smoke image's banner.

Source commit: `8deb91735ec221186fb0e49497c5769fa1f4de3c` (this evidence commit
follows it; the image embeds the source commit it was built from).

## Artifacts

| Path | Content |
| --- | --- |
| `aht10-uart.log` | Accepted capture, 1629 B, SHA-256 `b218f24dc36fa120b975265b5e99eb050b187bf95183f53d558669676c627bba` |
| `aht10-capture.json` | Validator verdict `AHT10_SOFT_I2C4_B1` plus sample statistics |
| `attempt2-app-slot.log` / `.json` | Negative control: the same image written to `0x08008000` never ran (see below) |
| `program.json` | Snapshot-gated write log of the accepted run |

Image: `pandora_stm32l475_aht10.bin`, 9120 B, SHA-256
`bf6f70cc28f3a03a6e995e8ef89ed4db422f2ed0814f3d54b7be23e0c8f9b84f`
(ELF text 9076 / data 44 / bss 2860; `FLASH 9120/504 KB`, `RAM 2904/96 KB`).
Board: STM32L475VE, ST-Link V2.1 `V2J24S11` / `066AFF313933554D43244015`, chipid
`0x415`. UART: CP2102 `/dev/ttyUSB0`, 115200 8N1.

## Procedure and results

1. Snapshot `s0` (`tools/pandora_hil_state.py`): bootloader `762c4a75…`,
   application `d35e2e9a…`, `fota_metadata` `f47a8ec3…`, `board_reserved`
   `118f9c6f…`, RTC backup all-zero.
2. Gated write of the image to `0x08000000` (`pandora_hil_program.py run
   --address 0x08000000`): `span_identical=true`, `erased_tail=1120`,
   `unexpected_changes=0`.
   Why `0x08000000`: `STM32L475VETX_FLASH.ld` links these validation images at
   `0x08000000`; the application slot at `0x08008000` belongs to the FOTA
   bootloader (`STM32L475VETX_FOTA_APP.ld`).
3. Capture with `validate_aht10_capture.py capture --reset-command "st-flash
   reset"` (UART opened and flushed *before* the reset): 1629 B, verdict
   `AHT10_SOFT_I2C4_B1`.
   - ordered markers: `PANDORA AHT10 SENSOR READY` → `FIRMWARE_COMMIT
     8deb9173…` → `AHT10_BUS=SOFT_I2C4 SCL=PD6 SDA=PC1 ADDR=0x38` →
     `PANDORA_I2C4_INIT_DONE` → `I2C4_ACK=0x38` → `PANDORA_I2C4_SCAN_DONE
     count=1` → `AHT10_DRIVER_READY ADDR=0x38`
   - 31 samples, 21 unique, no malformed lines, no error markers, 0 non-ASCII
   - humidity 5458–5465 centi-%RH (54.58–54.65 %RH), temperature 3053–3068
     centi-C (30.53–30.68 °C), both inside the AHT10 public range
   - only `0x38` answered the 0x08..0x77 scan on this bus
4. Post-run `verify`: only `bootloader` differed (the image just written);
   `application`, `fota_metadata`, `board_reserved` and `rtc_backup` identical —
   the image touched nothing but its own slot.
5. `restore` + `verify_ok regions=5`: the board is byte-identical to `s0`
   (resident bootloader `762c4a75…`, application `d35e2e9a…`, blank journal
   restored, RTC untouched).

## Negative control: the application slot is not a durable place to run a validation image

The first attempt in this session wrote the same image to `0x08008000` and
captured 6168 B that contained only `PANDORA ICM20608 SENSOR READY` /
`FIRMWARE_COMMIT 2a596d90…` lines. `verify` showed `application` had changed to
`d27a6a26…`: the resident bootloader installed its W25Q128 candidate over the
freshly written image during the reset that `st-flash write` performs, so the
validator's `AHT10_SOFT_I2C4_VALIDATION_FAILED` verdict was correct — the AHT10
image never ran.

`attempt2-app-slot.log`/`.json` are the reproduction of that failure with its
raw bytes retained (6164 B, same verdict, same eight failures). Consequence for
future unattended runs on this board: a validation image written to
`0x08008000` is silently replaced by the bootloader's pending candidate, so
either write to `0x08000000` (validation image, bootloader temporarily gone) or
make the image the candidate itself.

## Boundaries

- One board, one bounded 6 s window, one image. No long-run, soak, thermal or
  accuracy claim; humidity/temperature ranges are what the part reported in a
  still room, not a calibrated reference.
- No controlled stimulus (no breath/humidity step), no response time, no
  absolute accuracy, no condensation behaviour.
- The capture proves the canonical driver's init/measure/decode chain and the
  bus presence of `0x38`; it does not prove soft-I2C timing margins, external
  pull-ups, or bus recovery.
- `main.c` still carries the legacy inline AHT10 path for the smoke image; this
  slice did not remove it (that image's markers and `capture_uart.py` contract
  are the subject of a separate decision).
- The bootloader was temporarily replaced by the validation image and restored
  byte-exactly; this is a flash byte-level statement, not evidence about the
  bootloader's own behaviour in this run.
