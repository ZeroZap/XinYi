# Pandora STM32L475VE Board Smoke Record

**Date**: 2026-09-03
**Source commit**: `9b50ec38c4e32f9e37c93a0f3f70379453c9622f`
**Board**: Pandora STM32L475VE
**Status**: `B1_BOARD_SMOKE_VERIFIED`; `B2_PENDING`
**Evidence classification**: `PROGRAMMING_VERIFIED`; `BOARD_RUNTIME_B1`

This record separates verified programming, observed normal-path board runtime, and still-pending
negative/recovery evidence. B1 does not imply AHT10 NACK/recovery verification.

## Environment

- ST-Link USB identity: `0483:374b` (`ST-LINK/V2.1`)
- ST-Link firmware: `V2J24S11`
- Probe serial: `066AFF313933554D43244015`
- Target detected by `st-info --probe`: STM32L47x/L48x, chip ID `0x415`, 512 KiB Flash,
  96 KiB SRAM
- Toolchain: Arm GNU Toolchain 15.2.Rel1, GCC `15.2.1 20251203`
- Programmer: `st-flash 1.8.0`

## Build and programming evidence

The following focused source contract passed:

```text
ctest --test-dir build/tests/unit -R '^pandora_stm32l475_board$' --output-on-failure
1/1 passed
```

The board target cleanly configured, linked, and produced:

```text
text=7236 data=12 bss=2700 dec=9948
FLASH used=7248 bytes (1.38%)
RAM used=2712 bytes (2.76%)
```

The exact image was programmed at `0x08000000`. `st-flash` reported:

```text
Attempting to write 7248 bytes
Flash written and verified
```

## Historical ST-Link VCP blocker

Before programming, Linux exposed `/dev/ttyACM0` and the probe was visible. Opening the VCP for a
six-second 115200-8-N-1 capture did not return data and did not terminate normally; the capture was
stopped by the 20-second command timeout (`CAPTURE_BYTES=0`). Immediately afterward:

- `/dev/ttyACM0` was absent;
- `lsusb` no longer listed `0483:374b`;
- `st-info --probe` reported `Found 0 stlink programmers`.

This matches an unstable ST-Link V2.1 VCP/USB path, so no UART banner was captured. LED, KEY0, AHT10
ACK/measurement, and NACK recovery were not independently observed and remain
`BOARD_RUNTIME_PENDING` at that time. The independent-UART result below supersedes that B1 blocker;
the board-local ST-Link VCP limitation remains.

## 2026-09-03 independent-UART B1 result

The current committed image was rebuilt with embedded identity
`9b50ec38c4e32f9e37c93a0f3f70379453c9622f`, programmed once with `st-flash --reset write`, and
reported `Flash written and verified`. PE7 was then visually observed toggling at the firmware's
500 ms interval.

An independent WCH-Link UART connected to PA9/PA10/GND enumerated as
`/dev/serial/by-id/usb-wch.cn_WCH-Link_B49C8F0639CE-if01`. A bounded six-second 115200-8-N-1
capture retained 1560 bytes and was classified `B1_REVIEW_CANDIDATE`; review confirmed the real
adapter path, exact flashed commit, and ordered runtime content. The log contains ten complete
normal-path cycles, each with the Pandora banner, matching firmware identity, AHT10 ACK, and a
plausible measurement. Observed ranges were 77153–77180 milli-percent RH and 28583–28601
milli-degrees C.

Retained evidence:

- [raw UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-b1.txt), SHA-256
  `93d4dc22669b26b8b666f4bc4d25968f9b2aa02959968f476fbfe4191730a658`
- [capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-b1.json), SHA-256
  `54f357183833ace492f2382e591fda3e4f75dc90002d625c7ae4d0f676652532`

This closes the normal-path board smoke at B1 for this exact board/image/wiring.

A second eight-second capture retained 2052 bytes while KEY0 was held for about two seconds. It
contains 13 matching firmware cycles and four `KEY0` events, verifying the PD10 active-low key path:

- [KEY0 UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-key0.txt), SHA-256
  `57e69784ce8a436f969fd9562826cf667aca34c3561523a99544e8e33b720f1b`

No retained capture contains an AHT10 NACK, so negative/recovery behavior remains pending and B2 is
not granted.

## Remaining B2 work

1. Force an AHT10 NACK followed by reconnection, ACK, and a plausible measurement in one retained
   capture.
2. Grant B2 only after reviewing the real device path and ordered negative/recovery bytes.

Use the bounded capture helper after the board or independent USB-TTL adapter appears:

```text
python3 boards/pandora_stm32l475/capture_uart.py \
  --device /dev/ttyACM0 \
  --timeout 6 \
  --firmware-commit <exact-flashed-commit> \
  --output build/pandora-runtime/uart.log \
  --metadata build/pandora-runtime/capture.json
```

The helper exits nonzero and records `NO_DATA_TIMEOUT`, `DEVICE_OPEN_FAILED`, `CAPTURE_IO_FAILED`, or
`CAPTURE_CONTENT_MISMATCH` rather than creating false runtime evidence. Captured bytes only receive
`CAPTURED` and `B1_REVIEW_CANDIDATE` when they contain the exact firmware banner
`PANDORA STM32L475VE XINYI SMOKE OK`, followed by the exact matching `FIRMWARE_COMMIT <sha>`
identity marker, the exact sensor acknowledgement `AHT10 0x38 ACK`, and an
AHT10 measurement within the sensor's plausible output range (0–100000 milli-percent RH and
-50000–150000 milli-degrees C); banner-only, missing-ACK, misplaced/mismatched identity, out-of-range,
and unrelated bootloader/noise bytes are retained but rejected. These markers must occur in firmware
order: banner, matching commit, ACK, then plausible measurement; reordered retained bytes are also
rejected. The candidate classification still
requires human review of the real device path, flashed commit, raw log and metadata before this record
may grant B1. A serial
EOF/disconnect is classified immediately as an I/O failure instead of being mislabeled as a no-data
timeout. Its PTY Host contract enforces banner-plus-ACK-plus-measurement eligibility, content mismatch,
bounded no-data timeout, disconnect, missing-device refusal, firmware commit binding, and metadata
generation. A successful PTY test is not board evidence; only retained bytes from the real
board/adapter may support B1/B2.

If the same retained capture also contains the firmware banner followed in order by `AHT10 0x38 NACK`,
a later ACK and a plausible measurement, the helper records `B2_REVIEW_CANDIDATE`. A NACK before the
firmware banner or after an earlier successful measurement remains only a B1 candidate. This only
identifies a recovery-log candidate;
human review must still confirm the real device path, ordering, flashed commit and raw bytes before the
record can grant B2.
