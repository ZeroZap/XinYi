# Pandora HIL state snapshot / verify / restore harness — board run 2026-10-28

## Why

`2026-10-28-fota-boot-flow` flashed the resident bootloader without first capturing the
application region, so the bootloader's own `install_once()` rewrote `0x08008000` and the
previous application image could not be restored byte-for-byte. The whole boot decision state
is flash plus a volatile RTC-backup handshake, so the missing step is cheap: read those
windows before touching the board, and make the cycle reversible.

## Tool

`tools/pandora_hil_state.py` (host tests: `tools/tests/test_pandora_hil_state.py`, 34 cases;
`make test-tools`).

| Region | Address | Size | Restorable | Owner |
| --- | --- | --- | --- | --- |
| `bootloader` | `0x08000000` | 32 KiB | yes | `STM32L475VETX_BOOTLOADER.ld` FLASH |
| `application` | `0x08008000` | 472 KiB | yes | `STM32L475VETX_FOTA_APP.ld` FLASH |
| `fota_metadata` | `0x0807E000` | 4 KiB | yes | `STM32L475VETX_FLASH.ld` FOTA_METADATA |
| `board_reserved` | `0x0807F000` | 4 KiB | yes | `STM32L475VETX_FLASH.ld` BOARD_RESERVED |
| `rtc_backup` | `0x40002850` | 128 B | **no** | `RTC->BKP0R..BKP31R`, slots 1/2 = FOTA handshake |

The four flash regions tile `0x08000000..0x08080000` exactly; `check-layout` and the host test
compare them against the board linker scripts plus `pandora_fota_flash.h`,
`pandora_fota_install_flash.h` and `pandora_fota_boot_flow.h`, so a layout move cannot leave the
harness reading the wrong window. The RTC block is captured (to prove it did not change) but never
written back: restoring it needs PWR write access plus an enabled RTC clock, which a passive
debugger read must not do.

`snapshot` fails closed — a hard region (anything but `rtc_backup`) that cannot be read or returns
a short file leaves no manifest behind, so a partial capture can never look restorable.

## Board run

Probe: ST-Link V2.1 `V2J24S11`, serial `066AFF313933554D43244015`, chip id `0x415`
(STM32L47x_L48x, 512 KiB flash). Full 528 KiB capture takes ~5 s; a four-region restore ~28 s.

| Step | Transcript | Result |
| --- | --- | --- |
| Snapshot (`hil-state-harness`) | `00-snapshot.txt` | 5/5 regions captured, per-region SHA-256 |
| Second independent read pass | `01-determinism.txt` | identical hashes |
| Clean verify + restore plan | `02-verify-clean-and-plan.txt` | `verify_ok regions=5`, `plan_ok steps=4` (no writes) |
| Inject 1 byte (`0xff`→`0x5a` at `board_reserved+0x400`) | `03-drift-injection.txt` | `st-flash write` verified |
| Verify after drift | `04-verify-drift.txt` | `board_reserved: DIFFERS first_diff=+1024`, rc=1, other four regions still identical |
| Restore | `05-restore.txt` | `restore_ok regions=4`, every read-back byte-identical |
| Verify after restore | `06-verify-final.txt` | `verify_ok regions=5`, rc=0 |

Captured state (also in `manifest.json`; that manifest's `git_head` is the pre-commit HEAD of this
slice):

```text
bootloader     0x08000000   32768 B nonzero= 30432 sha256=762c4a757863f1da…
application    0x08008000  483328 B nonzero=480591 sha256=d35e2e9ab2fe7249…
fota_metadata  0x0807E000    4096 B nonzero=  4096 sha256=f47a8ec3e9aff231…
board_reserved 0x0807F000    4096 B nonzero=  4051 sha256=118f9c6f2d3787d9…
rtc_backup     0x40002850     128 B nonzero=     0 sha256=38723a2e5e8a17aa…
```

`bootloader` reads back `762c4a75…a142`, the hash independently established when the previous run
restored that image — the harness agrees with a hash it did not produce.

`fota_metadata` is 100 % non-zero, i.e. erased `0xFF`: the journal is still blank from the previous
run's cleanup, and `rtc_backup` is all zero, so no confirm/restage request is pending. The board was
**not** reset after this run, so the boot decision state seen here is the state left in place.

## What this proves, and what it does not

Proves: on this board, `snapshot` reads all five windows deterministically across three
independent passes (including one that spans a full erase/rewrite of all four flash regions);
`verify` detects a single injected byte at the exact offset and exits non-zero; `restore` writes
the four flash regions back and its own read-back comparison is byte-identical; `plan` performs no
writes.

Does not prove: RTC backup retention across a real power cycle (the block was read, never
power-cycled); that a restore survives a subsequent bootloader run — the bootloader still re-runs
`install_once()` on the next boot if a candidate is pending, so a restored application region can be
clobbered again by a reset, not by this tool; flash endurance or interrupted-write behaviour (the
writes here completed and verified); that `st-flash` never resets the target — it did not during
these writes, but a future version may, which would race the read-back.

## Pitfalls found

- `st-flash` 1.8.0 reads as `st-flash read [path] [addr] [size]` — the path comes first, unlike
  `write [path] [addr]`. The first live snapshot failed with `Invalid value for size` because the
  host test only asserted the argv the implementation builds, not the CLI's real grammar; the board
  run is what caught it.
- Erased flash reads as `0xFF`, so `nonzero_bytes` cannot mean "region is blank" — it is there to
  catch a region silently blanked to `0x00` by a failed write.
- Seven source mutations of the tool are each caught by the host suite: ignoring `--yes`, restoring
  the RTC block, skipping the length-mismatch compare, moving the application window, making an
  optional region fatal, reverting the `st-flash` read argument order, and dropping the short-capture
  check.
- The mutation harness must snapshot the file under test at run time. An earlier pass restored a
  stale copy and silently reverted the `st-flash` argument fix; the read-only `verify` against the
  board is what surfaced it, which is why the final code was re-verified on hardware after the last
  edit.
- Raw region files stay under `~/xinyi-hw-evidence/2026-10-28/pandora-state/`; only the manifest and
  transcripts are versioned here (528 KiB of flash dump does not belong in git).
- `manifest.json` was captured while HEAD was `87daee16`; this slice is committed on top of
  `74f76cc7` (a parallel session landed the openCH OSAL work in between). The captured board state
  itself is unchanged by that.
