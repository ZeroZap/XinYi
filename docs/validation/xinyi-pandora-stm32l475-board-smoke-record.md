# Pandora STM32L475VE Board Smoke Record

**Date**: 2026-09-02
**Source commit**: `00cc9ee97256c5114c5e218717fc12253ad6e0f9`
**Board**: Pandora STM32L475VE
**Status**: `BLOCKED_VCP_UNSTABLE`
**Evidence classification**: `PROGRAMMING_VERIFIED`; `BOARD_RUNTIME_PENDING`

This record separates a verified ST-Link flash operation from unobserved firmware runtime. A
successful programmer verify does not prove the UART banner, LED, key, or AHT10 paths executed.

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

## Runtime observation blocker

Before programming, Linux exposed `/dev/ttyACM0` and the probe was visible. Opening the VCP for a
six-second 115200-8-N-1 capture did not return data and did not terminate normally; the capture was
stopped by the 20-second command timeout (`CAPTURE_BYTES=0`). Immediately afterward:

- `/dev/ttyACM0` was absent;
- `lsusb` no longer listed `0483:374b`;
- `st-info --probe` reported `Found 0 stlink programmers`.

This matches an unstable ST-Link V2.1 VCP/USB path, so no UART banner was captured. LED, KEY0, AHT10
ACK/measurement, and NACK recovery were not independently observed and remain
`BOARD_RUNTIME_PENDING`. No B1/B2 status is granted by this record.

## Unblock and rerun

1. Power-cycle/reconnect the board and upgrade the ST-Link firmware, or connect an independent
   USB-TTL adapter to PA9 (USART1 TX), PA10 (USART1 RX), and GND.
2. Reflash the same committed image and save a bounded raw UART log containing the banner and AHT10
   measurement.
3. Observe the PE7 LED and KEY0 path, then force an AHT10 NACK/recovery cycle and retain the log.
4. Only then update this record and the HAL evidence matrix to B1/B2 as supported by the observations.