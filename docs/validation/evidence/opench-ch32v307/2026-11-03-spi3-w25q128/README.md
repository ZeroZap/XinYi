# openCH SPI3 + onboard W25Q128: JEDEC identity and reversible read/write/restore

Board: openCH CH32V307, WCH-Link CH549F. UART evidence is captured over the CH549F virtual
COM port (`/dev/ttyACM0`, by-id `usb-wch.cn_WCH-Link_BC5C0A1DC679-if01`). Both images were
programmed through the WCH OpenOCD fork (`program <elf> 0x00000000 verify reset`, adapter speed
1000) — every image reported `Programming Finished` / `Verified OK` / `Resetting Target`.

## What was asked

The openCH board carries an onboard W25Q128 QSPI-flash-like part on SPI3, but no image had ever
exercised it through `xy_hal_spi_*`: the `spi3_w25q128_smoke` and `w25q128_rw_restore_smoke`
targets existed only as sources. Two questions, both answerable without extra lab equipment:

1. Does the SPI3 HAL talk to the real part and read its JEDEC identity (`0x9F` → `EF 40 18`)?
2. Does a full erase / program / read-back / restore cycle leave the part intact?

## Images and results

| image | elf sha256 | observed marker | verdict |
|---|---|---|---|
| `opench_ch32v307_spi3_w25q128_smoke` | `ea86c59d…` | `OPENCH_SPI3_W25Q128_JEDEC_OK EF4018` (repeated) | `B1_REVIEW_CANDIDATE` |
| `opench_ch32v307_w25q128_rw_restore_smoke` | `478207ef…` | `OPENCH_W25Q128_RW_BOOT_20261012_B` → `OPENCH_W25Q128_RW_RESTORE_OK_B` | `B1_REVIEW_CANDIDATE` |

JEDEC `EF 40 18` is the Winbond W25Q128 identification; the identity read therefore returned the
correct vendor/manufacturer byte sequence, not a coincidental value.

The second image walks the full reversible sequence against the last 4 KiB sector
(`0x00EFF000`, chosen to avoid disturbing any other data): read 4096 B into `backup`, sector-erase
(`0x20`), write-enable (`0x06`) + page-program (`0x02`) a 256 B `index ^ 0xA5` pattern, wait-ready
(`0x05`), read back and `memcmp` the pattern, then erase again and rewrite the backup, and finally
read back and `memcmp` the backup. Its terminal marker `OPENCH_W25Q128_RW_RESTORE_OK_B` is only
reached when both the pattern round-trip and the restore round-trip compare equal.

## What the capture shows (and what it does not)

The two captures are in `uart-spi3-jedec.txt` and `uart-rw-restore.txt`. The second capture's
head contains a few `OPENCH_SPI3_W25Q128_JEDEC_OK` lines left in the bridge ring by the previous
image — expected ring residue, not a defect; the boot marker `…_BOOT_20261012_B` and the terminal
`…_RW_RESTORE_OK_B` are the new image's own output and arrive in order.

## Boundary

This proves SPI3 clock/data/CS wiring to the onboard part, JEDEC identity read, sector erase,
page program, read-back and restore all work through `xy_hal_spi_*` on the real board. It does
**not** prove SPI electrical timing/margin, QSPI quad mode (the part is driven in single-line
SPI here), or clock rates beyond the `SPI_BaudRatePrescaler_8` used by both images.
