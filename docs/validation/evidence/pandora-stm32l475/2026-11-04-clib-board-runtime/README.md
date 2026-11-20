# Clib on the Pandora board — first runtime evidence (2026-11-04)

## Why this slice

The evidence matrix lists **Clib** with a Board column of `pending`. The component has 40 Host
test cases across four headers (`xy_frame`, `xy_time_civil`, `xy_bcd`, `xy_rtc_alarm_mask`), but
nothing had ever run it on a target. That is not a formality: the parts most likely to differ
between host and board are exactly the parts that are pure computation here —

- the calendar math is 64-bit on a 32-bit Cortex-M4, so a division or a truncation bug would not
  show up on the host;
- the frame codec indexes fixed-size buffers and refuses over-long payloads, so a size check that
  is off by one behaves differently under a different `size_t`;
- the CRC-8/ATM is a bit-at-a-time loop whose result depends on integer width and promotion rules.

The Host tests already pin the published values (CRC-8/ATM check value `0xF4`, `2000-03-01` =
day `11017`, epoch weekday `4`). The board image re-checks the same values, so a host/board
divergence shows up as a failed marker rather than as a quietly different answer.

## What ran

Image `pandora_stm32l475_clib` (new, `boards/pandora_stm32l475/clib_main.c`), 7948 B,
SHA-256 `7eee11ac63a99f9a73f4b01c241584536ef5373b5bbad41d256d72e1275b6fc8`, linked at
`0x08000000`.

Board: STM32L475VE, ST-Link V2.1 `V2J24S11` / `066AFF313933554D43244015`, chipid `0x415`.
UART: CP2102 `/dev/ttyUSB0`, 115200 8N1.

Write was snapshot-gated (`tools/pandora_hil_program.py run --address 0x08000000`):
`span_identical=true`, `unexpected_changes=0`.

### Marker sequence

```
PANDORA CLIB BOARD READY
FIRMWARE_COMMIT da9175f1ec1f97e48349a538776262becc4989d9
PANDORA_CLIB_FRAME_OK_A
PANDORA_CLIB_CIVIL_OK_A
PANDORA_CLIB_BCD_OK_A
PANDORA_CLIB_MASK_OK_A
B1_CLIB_BOARD_RUNTIME_PASS
```

| check | what it proves on the board |
|---|---|
| frame | CRC-8/ATM check value `0xF4`; a transposition changes the checksum; encode/decode round-trip keeps type and payload; the wire checksum recomputed independently over type+payload matches; a corrupted checksum is rejected; a start byte inside the payload does not split the frame; an undersized buffer and an over-long payload are refused; noise before a frame is ignored and the next frame still decodes; an impossible length is rejected |
| civil | epoch is day 0 and midnight is 0 s; `2000-03-01` = 11017 d, `2026-11-03` = 20760 d; the time of day is included; February has 28 days in 2026, 29 in 2024, and 1900 has 28 while 2000 has 29; March has 31 days and April 30; `from_days` round-trips for 20760, 0 and 11017; weekday 4 (Thursday) at the epoch, 2 (Tuesday) on 2026-11-03 |
| bcd | 8-bit known values 0/9/10/99; `0x42` decodes to 42; a nibble above nine is refused; 16-bit `1234` and decode of `0x5678`; an invalid 16-bit nibble is refused; the RTC year is a full year (`0x26` BCD and `0x1A` binary both decode to 2026, `0x1A` read as BCD is refused as −1); encode produces the register spelling (`0x26` BCD, 26 binary); a year outside the register's century is refused rather than truncated |
| mask | requiring every field skips nothing, requiring none skips everything; the inversion is per field; every representable mask round-trips through skip and back; a mask asking for a field the backend cannot compare is refused |

## Restart cycle

8 further reset-and-run cycles, each reporting all four checks and the terminal
`B1_CLIB_BOARD_RUNTIME_PASS`, with no error marker in any capture. The board therefore reaches
the same verdict from a cold reset every time.

## Board state restored

The image was written over the `bootloader` region, so the pre-write snapshot was restored
afterwards: `restore_ok regions=4` — `bootloader`, `application`, `fota_metadata` and
`board_reserved` all byte-identical to the pre-slice state.

## Two defects the board run found

Both were in the new image, not in the component, and both are the reason the board run is worth
doing:

1. `xy_hal_uart_init()` needs `handle->Instance` set to `USART1` before the call — `HAL_UART_Init`
   dereferences it to reach the registers. The image initially left it unset, so the HAL failed and
   the image halted **before** its first UART write: zero bytes on the wire, no error marker. The
   capture looked like a dead board until the image was compared against a known-good one.
2. The month-length assertion was written as `2026-04-01 − 2026-03-01 == 30`. March has 31 days,
   so the difference is 31; April's 30 days show up between `2026-05-01` and `2026-04-01`. The
   board rejected the wrong value exactly as it should.

## Boundary

This is runtime proof that the clib computations produce the documented values on a Cortex-M4
target. It is not a performance measurement, not a fuzz result (the frame decoder was exercised
with the specific cases listed, not with random input), and it says nothing about the parts of the
component that are not pure computation — the printf sink, the allocator-backed containers and the
string/ctype helpers, which remain Host-only.
