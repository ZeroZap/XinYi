# Pandora bounded boot attempts → ROLLBACK_REQUIRED on silicon (2026-10-28)

Slice goal: S6-12 proved the attempt threshold with the pre-refactor bootloader, and
S6-131 moved the decision sequence into the board-owned `pandora_fota_boot_flow.c`
with a 17-case host contract for the rollback branch — but that branch had never run on
the board since the refactor, and the `PANDORA_BOOT_*` marker stream had no validator in
the repository at all (the only FOTA validator, `validate_fota_boot_contract.py`, is
pinned to the older application-level `FOTA_*` stream). Both gaps are closed here.

## Tooling added with this run

`boards/pandora_stm32l475/validate_fota_bootloader_capture.py`:

- `capture` — open the UART before the reset, capture a bounded window, then judge it.
- `analyze` — judge an existing capture.
- `journal` — decode both `xy_fota_boot` journal slots from a flash dump and verify each
  record CRC.

`analyze_capture()` splits a capture into boot frames (`READY` → `BOOTLOADER_COMMIT` →
optional restage → install → decision → optional `JUMP_APP`) and rejects the orders the
flow owner cannot produce: decision before commit, install after the decision, duplicate
decision, jump after a halting decision, jump missing after a confirm/attempt decision,
unknown `PANDORA_BOOT_*` marker. Application output is expected and counted — the
capture is armed while the previous firmware is still printing, and the image the
bootloader jumps into keeps printing afterwards — but foreign output *inside* a boot
frame is a failure. `--expect attempt|confirm|rollback|install_error` makes the caller
state the outcome it predicted, so the judgement is not just internal consistency.

Host: `tests/unit/fota/test_pandora_fota_bootloader_capture.py`, 43 cases, registered as
CTest `pandora_fota_bootloader_capture_validator` (suite 297 → 298). A real pty is used
so the termios capture path is exercised, not only the parser.

## Images and board state

| Item | Value |
| --- | --- |
| Source commit (embedded, `BOOTLOADER_COMMIT`) | `7db31c06c6c5032017e2ceabc5dfd1542ef5f64d` |
| Bootloader artifact | `pandora_stm32l475_fota_bootloader.bin`, 15548 B |
| Bootloader SHA-256 | `24cdd9dfe6c4dc300045f88db3ab415aa155afc7cbe7762a057b9d47e1301f27` |
| Linker budget | `FLASH 15548 B / 32 KB (47.45%)`, `RAM 2864 B / 96 KB (2.91%)` |
| Candidate in W25Q128 (untouched) | `image_version=3`, `image_size=39424`, CRC32 `0xA81835BB` |
| Journal baseline | both slots all-`0xFF`, `JOURNAL_INVALID`, no active record |
| Pre-run state (`s0`) | bootloader `762c4a75…`, application `d35e2e9a…`, `fota_metadata f47a8ec3…`, `board_reserved 118f9c6f…`, RTC backup all-zero |
| Program | snapshot-gated `tools/pandora_hil_program.py run --yes` (no `--reset`), `span_identical=true erased_tail=836` |
| Probe / UART | ST-Link V2.1 `V2J24S11` `066AFF313933554D43244015` `0x415`; CP2102 `/dev/ttyUSB0` 115200 8N1 |

## Runs

### Run 0 — a capture window that missed the boot (kept, not hidden)

`run0-write-only-uart.txt` (5130 B, SHA-256 `4c53723a…`): capture armed 1 s before the
gate write, 14 s window. It contains only the firmware that was already running
(`ICM20608_*` lines) and no bootloader marker, so
`BOOTLOADER_CHAIN_VALIDATION_FAILED chain=INVALID boots=0`. The gate's pre-check
re-reads all 528 KiB before writing, so the write landed after the window closed. A
`verify` straight afterwards showed `application` and `fota_metadata` byte-identical to `s0` —
that cycle's write did not start a boot decision. (That `verify` transcript was a session
observation and is not archived here; the run-1 journal dump below is the archived proof of the
cycle that *did* boot.)

Why it matters: in the *next* cycle the same gate write did start one (the journal held
`attempts=1` before the caller issued any reset, and my explicit reset produced
`attempts=2`). So a write may or may not run the boot decision depending on what the core
was doing — the attempt count must be read from the journal, never assumed from the number
of captures or resets. The bootloader was restored and re-flashed from `s0` before run 1.

### Run 1 — install + attempt through the flow owner

`run1-uart.txt` (188 B, SHA-256 `898d56f4…`):
`BOOTLOADER_CHAIN_REVIEW_CANDIDATE chain=ATTEMPT terminal=jump boots=1 attempts=1`.

```text
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 7db31c06c6c5032017e2ceabc5dfd1542ef5f64d
PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED
PANDORA_BOOT_ATTEMPT_COMMITTED
PANDORA_BOOT_JUMP_APP
```

Journal (`run1-journal.json`, dump SHA-256 `34ef0454…`): slot 0 `INSTALLED generation=3
attempts=1`, slot 1 `INSTALLED generation=4 attempts=2`, both `record_crc_ok=true`,
`image_version=3`, `image_size=39424`, `image_crc32=0xA81835BB`. `ALREADY_INSTALLED`
plus two attempt records is the write-triggered boot installing and recording attempt 1,
then the explicit reset recording attempt 2.

That dump is byte-identical to the journal the earlier install-drift run ended with
(`journal-after-write-reset-test.bin`, same `34ef0454…`), i.e. the journal encoding for
`INSTALLED/3/1` + `INSTALLED/4/2` is reproducible across sessions and builds.

### Run 2 — attempt 3 exhausts the threshold

`run2-rollback-uart.txt` (165 B, SHA-256 `6d6fa21d…`):
`BOOTLOADER_CHAIN_REVIEW_CANDIDATE chain=ROLLBACK terminal=halt_rollback boots=1 attempts=0`.

```text
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 7db31c06c6c5032017e2ceabc5dfd1542ef5f64d
PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED
PANDORA_BOOT_ROLLBACK_REQUIRED
```

No `PANDORA_BOOT_JUMP_APP` — the flow returned `HALT` and `stop()` disabled interrupts
instead of handing off. Journal (`run2-journal.json`, dump SHA-256 `5df7237b…`):
slot 0 `ROLLED_BACK generation=5 attempts=3` (active), slot 1 keeps `INSTALLED
generation=4 attempts=2`; both CRCs valid. The rollback is a durable journal state, not
just a marker.

### Run 3 — the next boot refuses the rolled-back candidate and writes nothing

`run3-install-error-uart.txt` (119 B, SHA-256 `1abfa3ea…`):
`chain=HALT_ERROR terminal=halt_error`.

```text
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 7db31c06c6c5032017e2ceabc5dfd1542ef5f64d
PANDORA_BOOT_INSTALL_ERROR
```

Journal dump after this boot is byte-identical to run 2's (`5df7237b…`, both slots
unchanged) — `xy_fota_boot_candidate_install_once()` returned `XY_FOTA_VERSION_ERROR`
before any journal write or erase. Fail-closed at the flash level, not only in the marker
stream. The host contract now pins that too: the rollback test asserts journal bytes,
write count and erase count are unchanged by the refused fourth boot.

## Restore

`restore --snapshot s0 --yes` then `verify`: `verify_ok regions=5` — bootloader
`762c4a75…` (original resident image), application `d35e2e9a…`, journal `f47a8ec3…`
(blank again), `board_reserved 118f9c6f…`, RTC backup `38723a2e…`. The board is
byte-identical to its pre-run state.

## What this establishes

1. On this board, a bootloader built from `7db31c06` drives the refactored flow to
   `ROLLBACK_REQUIRED` at exactly `PANDORA_FOTA_FLOW_MAX_ATTEMPTS` (3) and halts without
   a vector handoff.
2. The rollback is recorded durably (`ROLLED_BACK`, `attempts=3`) and every later boot is
   refused at the install step with `INSTALL_ERROR`, without touching the journal.
3. `PANDORA_BOOT_*` captures are now judged by a repo-owned validator with an explicit
   expected outcome, and the journal is decoded (with CRC) by the same tool.

## What this does not establish

- No power-loss, brownout, interrupted-write or journal-endurance evidence; all four
  boots are software resets with the probe attached.
- The confirm branch (`CONFIRM_REQUEST_MAGIC` → confirm + ACK) still has not run on
  hardware; RTC backup stayed all-zero throughout.
- After a rollback the application region still holds the rejected candidate and the
  bootloader halts: recovery needs a reflash or the reviewed-restage build option
  (`PANDORA_FOTA_AUTHORIZE_RESTAGE`, host-tested only). That is the intended fail-closed
  behaviour, not a known-good-slot rollback.
- No runtime evidence for the image the bootloader jumps into: runs 1 shows `JUMP_APP`
  and then no further output inside the window.
- Attempt accounting depends on how many times the boot decision actually ran; a write
  may start one (run 1) or not (run 0), so the journal is the only trustworthy counter.

## Artifacts

In this directory: `run0-write-only-uart.txt` / `.json`, `run1-uart.txt` / `.json`,
`run1-journal.json`, `run2-rollback-uart.txt` / `.json`, `run2-journal.json`,
`run3-install-error-uart.txt` / `.json`, `run3-journal.json`, `program.json` (gate log).

Outside the repo: `~/xinyi-hw-evidence/2026-10-28-bootloader-rollback/` holds the `s0`
snapshot (manifest plus region payloads) and the three journal dumps (`j1`, `j2`, `j3`).
