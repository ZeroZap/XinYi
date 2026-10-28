# Pandora FOTA bootloader boot-sequence owner — board run 2026-10-28

Refactor slice: the resident bootloader's decision sequence moved into the host-testable
board owner `boards/pandora_stm32l475/pandora_fota_boot_flow.c` (see
`tests/unit/fota/test_pandora_fota_boot_flow.c`). `fota_bootloader_main.c` now supplies the
RTC-backup / W25Q128 install+confirm+attempt / application-vector operations and halts on
`PANDORA_FOTA_BOOT_OUTCOME_HALT`. Marker text and ordering are unchanged.

## Image under test

| Item | Value |
| --- | --- |
| Source commit (embedded, `BOOTLOADER_COMMIT`) | `2511ca8a0c2aac0983839f0f990e31e2af23ed7b` |
| Artifact | `pandora_stm32l475_fota_bootloader.bin`, 15548 bytes |
| Artifact SHA-256 | `e615e62eb296df6f2900ffbdc91849899de1d64a88b3d7f4d720c190cd32d0c8` |
| Linker budget | `FLASH 15548 B / 32 KB (47.45%)`, `RAM 2864 B / 96 KB (2.91%)` |
| Program | ST-Link V2.1 (`V2J24S11`, serial `066AFF313933554D43244015`), `st-flash write 0x08000000` → `Flash written and verified` |
| Read-back | `st-flash read 0x08000000 0x4000`, first 15548 bytes `cmp` identical to the BIN |
| UART | CP2102 `/dev/ttyUSB0`, 115200 8N1, capture armed before `st-flash reset` |

## Runs

Run 1 (`uart-run1.txt`, 180 bytes; capture SHA-256 `21a61f0c…ac583c`; metadata `boot_flow_run1.json`):

```text
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 2511ca8a0c2aac0983839f0f990e31e2af23ed7b
PANDORA_BOOT_CANDIDATE_INSTALLED
PANDORA_BOOT_ATTEMPT_COMMITTED
PANDORA_BOOT_JUMP_APP
```

Run 2, same image, no reflash (`uart-run2.txt`, 188 bytes; capture SHA-256 `df97f0ff…6a4c628`; metadata `boot_flow_run2.json`):

```text
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 2511ca8a0c2aac0983839f0f990e31e2af23ed7b
PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED
PANDORA_BOOT_ATTEMPT_COMMITTED
PANDORA_BOOT_JUMP_APP
```

Both runs show the refactored order on silicon: install decision → attempt recorded → handoff,
with the fresh-install and already-installed markers both exercised, no error marker present, and
the embedded commit matching the artifact's source commit.

## What this does and does not prove

- Proves: one bootloader image built from `2511ca8a` installs/attempts/jumps in the expected order
  and reports the documented markers, in two consecutive boots (fresh install, then idempotent
  install).
- Does **not** prove the confirm branch (`CONFIRM_REQUEST_MAGIC` → confirm → ACK) on hardware; that
  path is covered by the 17-case Host contract only, and no application on the board set the RTC
  confirm-request slot during this run.
- Does not prove power-loss/interrupted-write behaviour, journal durability/endurance, RTC backup
  retention across a real power cycle, or vector-handoff correctness beyond the observed jump
  marker (the application then produced no further UART output in the 9 s window).

## Board state changes (recorded, not silently reverted)

- The bootloader region was restored to the pre-run image (`0x08000000..0x08007FFF`, 32768 bytes,
  SHA-256 `762c4a757863f1dab00a7854c736f371358e3a2598c4ae7bd0ff878a21dda142`), read back and
  `cmp`-identical.
- Run 1 performed `xy_fota_boot_candidate_install_once()`, i.e. the bootloader erased and
  reprogrammed the application region from the resident W25Q128 candidate. The pre-run application
  bytes were **not** snapshotted before the write, so the previous application image cannot be
  restored byte-for-byte from this record — that is a process defect in this run, documented here
  rather than papered over.
- Two boots recorded journal attempts at `0x0807E000`. The two journal slots were erased
  (`st-flash erase 0x0807E000 0x1000` → both 2 KB pages, read back all-`0xFF`), returning the
  blank-journal precondition the first run observed. Post-run journal snapshot before erasing:
  `34ef0454c8337eb091bbf3313148db01ab71771d58e88fcbc369928092fa658e`.
- RTC backup slots were not modified (neither the confirm-request nor the restage slot was set), so
  the next boot behaves exactly like run 1.
