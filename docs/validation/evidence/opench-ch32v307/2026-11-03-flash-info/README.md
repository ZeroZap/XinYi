# openCH CH32V307 internal flash: info + sector boundary + reversible page R/W

Board: openCH CH32V307, WCH-Link CH549F. Image built from the tracked `MCU/wch` SDK,
programmed through WCH OpenOCD (`program <elf> 0x00000000 verify reset`, adapter 1000), UART
captured on `/dev/ttyACM0`.

## What was verified

`xy_hal_flash_*` on the real board, in one image (`flash_info_main.c`):

- `get_info`: flash_size 288 KiB, sector_count 72, page_size 4096, write_alignment 4;
- `get_sector_info(71)`: last page at `0x08047000`, size 4096;
- `is_valid_address`: `0x08000000` and `0x08047FFF` valid, `0x08048000` rejected;
- `unlock` → `erase`(last page) → `write`(16-byte pattern) → `lock` → `read` → read-back
  `memcmp` equal, repeated forever with `OPENCH_FLASH_PAGE_RW_OK`.

Capture shows `OPENCH_FLASH_PAGE_RW_OK` (×2). The intervening
`OPENCH_DMA1_SYSTICK8_*` lines are ring residue from the previously programmed image, flushed
out on the same read.

## Boundary

Controller-level proof of the WCH internal-flash HAL contract (geometry, address validation,
erase/program/read round-trip). Not proof of: write endurance, power-loss safety, or read-protect
behavior (`set_read_protect`/`get_read_protect` return `NOT_SUPPORTED` on this backend).
