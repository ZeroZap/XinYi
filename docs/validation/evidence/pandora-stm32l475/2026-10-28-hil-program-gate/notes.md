# Pandora snapshot-gated programming — board run 2026-10-28

## Why

`2026-10-28-hil-state-harness` made a Pandora flash cycle *reversible* by adding
`tools/pandora_hil_state.py`. It stays a tool you have to remember to run, and the
`2026-10-28-fota-boot-flow` run is the proof that remembering is the weak link: the
bootloader was written, its own `install_once()` rewrote `0x08008000`, and the previous
application image was destroyed before anyone captured it.

This slice moves the missing step from a habit into a precondition of the write.

## Tool

`tools/pandora_hil_program.py` (host tests `tools/tests/test_pandora_hil_program.py`,
36 cases; run by `make test-tools`). It reuses the state harness for every read, region
definition and comparison; it adds only the write contract and its ordering.

| Gate | Rule | Failure mode |
| --- | --- | --- |
| 1. Snapshot required | both `plan` and `run` load an existing manifest | exit 2, nothing written |
| 2. Snapshot must be current | every restorable region is re-read and compared byte-for-byte **before** the write | exit 2, nothing written, drift named |
| 3. Span must be legal | `.bin` only, non-empty, page-aligned start, inside one restorable flash region, fits, may not cross the region end | exit 2, nothing written |
| 4. Image re-hashed | the file is hashed again immediately before the write | exit 2, nothing written |
| 5. Read-back | the containing region is re-read; the image span must match byte-for-byte | exit 1, `program_failed`, restore command printed |
| 6. Outside-span changes | changes inside the image's erase page are reported as `erased_tail`; anything beyond it is a hard failure | exit 1 |
| 7. `--reset` drift scan | after one boot decision, every captured region is compared with the snapshot | reported per region |

`program_ok` is printed only when the read-back matches. The RTC backup block is never
written: `_flash_region_for()` rejects it, and programming at its address is a refusal,
not a silently-clamped write.

The tool cannot prevent a *runtime* rewrite (the bootloader installing a candidate is a
legitimate flash write by the firmware). What it guarantees is that such a rewrite is
detected by the drift scan and that the bytes it replaced are recoverable, because the
pre-write snapshot is the only accepted authorization to write at all.

## Host coverage

36 cases (`tools/tests/test_pandora_hil_program.py`), 141 total for `make test-tools`:
span legality (`.elf`, empty, RTC, unmapped, unaligned, oversized, region-crossing,
exact-fill), read-back classification (span mismatch offset, page-erase tail, changes
beyond the erase page, short read), pre-check refusal paths (drift, unreadable
restorable region, unreadable *optional* RTC must not veto), ordering (all pre-check
reads precede the write), write-once-at-address, no RTC write, image-mutation refusal,
optional reset + drift, program log contents, and CLI exit codes.

10 source mutations, each caught by its specific test, with a green baseline:

```text
skip_confirm / ignore_precheck_drift / skip_precheck_reads / force_span_identical
accept_elf / drop_page_alignment / drop_unexpected_change_check / drop_region_end_check
drop_rehash_before_write / drop_empty_image_check
mutations=10 caught=10 survived=0
```

The first mutation harness reported 10/10 but its baseline also failed — discovery was
importing nothing, so every non-zero exit looked like a catch. Re-run with an explicit
module target and `BASELINE_GREEN=True` before the numbers above were accepted.

## Board run

Probe: ST-Link V2.1 `V2J24S11`, serial `066AFF313933554D43244015`, chip id `0x415`.
Image: `pandora_stm32l475_smoke.bin`, 14360 B, SHA-256 `e12440bfee1e2ef6…`, written to
`application` at `0x08008000`.

| Step | Transcript | Result |
| --- | --- | --- |
| Snapshot (`before-program-gate`) | `00-snapshot.txt` | 5/5 captured; matches the S6-134 capture exactly |
| `plan` | `01-plan.txt` | `plan_ok writes=0`, 8 erase pages, no target access |
| `run` without `--yes` | `02-refused-without-yes.txt` | `refused: … (--yes)`, exit 2 |
| `run --yes --reset --settle 3` | `03-program-and-reset.txt` | `span_identical=true`, `erased_tail=2024`, `unexpected_changes=0`, drift `application: CHANGED first_diff=+4`, all four other regions unchanged, `program_ok`, exit 0 (19 s) |
| Second `run --yes` on the now-stale snapshot | `04-precheck-refuses-stale-snapshot.txt` | `precheck_failed: … application: DIFFERS first_diff=+4`, exit 2, nothing written |
| `restore --snapshot … --yes` | `05-restore.txt` | `restore_ok regions=4`, every read-back identical (28 s) |
| `verify` | `06-verify-final.txt` | `verify_ok regions=5`, board byte-identical to the pre-run state |

`manifest.json` and `program.json` are the machine records; the region payloads stay in
`~/xinyi-hw-evidence/2026-10-28-hil-program-gate/pandora-state/`.

## What the board actually established

- **Page erase is real and the tool models it correctly.** The image is 14360 B and
  `erased_tail=2024`; the containing 2 KiB pages end at `0x0800C000`, i.e.
  `16384 - 14360 = 2024` bytes after the image were erased to `0xFF` by the programming
  operation. This was a modelling assumption in the host fixture and is now an observed
  property of `st-flash` on this part.
- **The erase stops at the page boundary.** `unexpected_changes=0` means nothing beyond
  the last touched page moved, so a program operation is confined to its page window —
  which is why bytes outside it are a hard failure rather than a note.
- **The boot decision did not rewrite the application this time.** After `--reset`, the
  only changed region was `application`, and its first difference is `+4` — the reset
  vector of the image that was just written. `bootloader`, `fota_metadata`,
  `board_reserved` and `rtc_backup` were all unchanged, so at this board state the
  bootloader found nothing to install. The drift scan is therefore not yet proven
  against an *actual* bootloader-install rewrite; that case is exercised on the host
  fixture (`test_reset_reports_regions_that_drifted_after_the_boot_decision`).
- **The gate refuses on a real board, not only on a fake.** The second run against the
  same snapshot aborted before touching flash and named the region and offset.

## Boundary

- No power-loss, brownout, or interrupted-write evidence; no flash endurance.
- The restore path re-proves recoverability, but a program that is interrupted *during*
  `st-flash write` is outside what this tool can observe.
- No runtime/UART evidence is claimed for the smoke image; only flash-level bytes.
- The gate is only as strong as the snapshot being of the state you mean to preserve: it
  refuses stale snapshots, but it cannot know that a *fresh* snapshot was taken after a
  change you did not intend.
