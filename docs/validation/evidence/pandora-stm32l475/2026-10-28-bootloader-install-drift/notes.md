# Pandora bootloader-authored install vs the post-reset drift scan (2026-10-28)

Slice goal: S6-135's drift scan had only ever been exercised against a host fixture. The
2026-10-28 program-gate run ended with `application: unchanged` because at that board state
the resident bootloader found nothing to install. This run puts a bootloader that does
install on the board and makes the drift scan face it.

## Images

| Item | Value |
| --- | --- |
| Source commit (embedded, `BOOTLOADER_COMMIT`) | `85006528f29ab3266a7a8d02651784fe31d81fa3` |
| Bootloader artifact | `pandora_stm32l475_fota_bootloader.bin`, 15548 B |
| Bootloader SHA-256 | `48ad7cafc1b5a1962f0f27cdc7398b5130b95d40a249b54b195b28c5bce66c84` |
| Linker budget (rebuild) | `FLASH 15548 B / 32 KB (47.45%)`, `RAM 2864 B / 96 KB (2.91%)` |
| Marker image written to `0x08008000` | `pandora_pattern.bin`, 4096 B (2 pages), SHA-256 `1ce169fe9b624349d714a2407c1507e0d96b2a6d29d457d412a76b7ded930d01` |
| Candidate in W25Q128 (untouched) | `image_version=3`, `image_size=39424`, payload CRC32 `0xA81835BB` |
| Program probe | ST-Link V2.1 `V2J24S11`, serial `066AFF313933554D43244015`, chip id `0x415` |
| UART | CP2102 `/dev/ttyUSB0`, 115200 8N1, capture armed before the write and reset |

The image under test is a rebuild of the current HEAD, so the marker chain carries this
run's own commit rather than the `2511ca8a` of the earlier boot-flow evidence.

## Runs

Three full cycles. Each one starts from the same verified blank-journal state
(`bootloader 762c4a75…`, `application d35e2e9a…`, `fota_metadata f47a8ec3…` all-`0xFF`,
`board_reserved 118f9c6f…`), writes a 4 KiB marker image over the head of the application,
flashes the flow bootloader, lets the boot decision install, and restores.

### Run 1 — install through the pattern (evidence anchor)

`04-program-with-reset.txt`: marker image written and read back
(`span_identical=true erased_tail=0 unexpected_changes=0`), then `--reset --settle 12`.
Drift: `application: unchanged`, `fota_metadata: CHANGED first_diff=+0`, `bootloader:
unchanged`, `board_reserved`/`rtc_backup` unchanged. `program_ok`.

`application: unchanged` is the interesting line: the 4 KiB marker image was gone and the
application was byte-identical to the snapshot again, i.e. the boot decision erased and
reprogrammed the exact same candidate bytes. UART (`uart-install.txt`, 180 B,
`B1_REVIEW_CANDIDATE`, SHA-256 `2ba92333…`):

```text
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 85006528f29ab3266a7a8d02651784fe31d81fa3
PANDORA_BOOT_CANDIDATE_INSTALLED
PANDORA_BOOT_ATTEMPT_COMMITTED
PANDORA_BOOT_JUMP_APP
```

### Run 2 — the mid-install sample that motivated the sampler

While re-flashing for a repeat, the journal region was read 9 s after the bootloader
write and before any further command: it held a valid `INSTALLING`, `generation=1`,
`attempts=0` record (`run2-s1`, `fota_metadata 97f20d63…`), and the RTC read failed
(`st-flash read … 0x40002850 0x80 failed rc=255`). A fixed-settle drift scan sampling
there would have reported the pre-install state as if it were the boot's outcome.

The same read-back explains itself: `st-flash write` resets the MCU. Measured directly —
writing the byte-identical bootloader image to `0x08000000` with no reset command of any
kind advanced the journal from `generation=3, attempts=1` to `generation=4, attempts=2`
(`journal-before-write-reset-test.bin` / `journal-after-write-reset-test.bin`, 20 s
apart, nothing else issued). So the boot decision starts on its own after a write, and a
fixed sleep cannot distinguish "finished" from "has not started erasing yet".

### Run 3 — the sampler's own evidence

`run3-02-program-bootloader-quiescent.txt`, gate write of the flow bootloader with
`--reset --settle 2 --stable-for 3 --max-wait 30`:

```text
readback span_identical=true observed_sha256=48ad7cafc1b5a196 erased_tail=836 unexpected_changes=0
drift_stable=true drift_samples=2 drift_seconds=10.9
drift bootloader: CHANGED first_diff=+4
drift application: CHANGED first_diff=+0
drift fota_metadata: CHANGED first_diff=+0
drift board_reserved: unchanged
drift rtc_backup: unchanged
program_ok
```

The drift table now names the bootloader-authored rewrite: the application's first
differing byte is `+0`, because where the baseline had the marker pattern the boot
decision had already put back candidate byte 0. `bootloader: CHANGED first_diff=+4` is
this run's own flash of the flow image (bytes 0..3 are the initial stack pointer, equal
in both images).

`run3-03-post-boot-analysis.txt` (reads retained before the restore):

- `application` is byte-identical to the pre-pattern baseline again (`d35e2e9a…`), and its
  4 KiB prefix is **not** the marker pattern — the marker image was erased and replaced.
- Journal slot 1: `state=INSTALLED generation=2 attempts=0`, slot 0:
  `state=INSTALLED generation=3 attempts=1`, both `record_crc_ok=True`, `magic=0x58424A52`,
  `commit=COMMIT`, `image_version=3`, `image_size=39424`. That is exactly
  `INSTALLING(1) → INSTALLED(2) → attempt(3)` for one boot.

UART for run 3 (`run3-uart-install.txt`, 271 B, `B1_REVIEW_CANDIDATE`, SHA-256
`1cf50996…`) contains two boot starts before the install:

```text
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 85006528f29ab3266a7a8d02651784fe31d81fa3
PANDORA FOTA BOOTLOADER READY
BOOTLOADER_COMMIT 85006528f29ab3266a7a8d02651784fe31d81fa3
PANDORA_BOOT_CANDIDATE_INSTALLED
PANDORA_BOOT_ATTEMPT_COMMITTED
PANDORA_BOOT_JUMP_APP
```

The first start is the one the flash write itself caused; the gate's explicit reset
restarted the decision before it had committed anything, and only the second run reached
`INSTALLED`. Nothing was torn (no journal write existed to tear), but it shows the write
and the operator's reset are two separate boots, not one.

## Board state

Every cycle ended with `restore` of the cycle's pre-run snapshot and a `verify`:
`run2-04-verify-final.txt` and `run3-05-verify-final.txt` both report
`verify_ok regions=5`, with `bootloader 762c4a75…` (the original resident image, not the
flow image), `application d35e2e9a…`, `fota_metadata f47a8ec3…` (journal blank again),
`board_reserved 118f9c6f…`, `rtc_backup 38723a2e…`.

## What this establishes and what it does not

Establishes, on this board:

1. A bootloader built from `85006528` really does erase and reprogram the application
   region from the resident W25Q128 candidate: the 4 KiB marker image is gone, the region
   is byte-identical to the pre-marker image, and the journal holds a consistent
   `INSTALLING → INSTALLED → attempt` chain with valid record CRCs.
2. The post-reset drift scan catches a bootloader-authored rewrite when there is a byte
   to see — `application: CHANGED first_diff=+0` in run 3 — and correctly reports
   `application: unchanged` when the install happens to restore the same bytes (run 1).
3. `st-flash write` resets the MCU, so a boot decision can run before the caller's own
   reset; a fixed settle is not a quiescence proof (run 2 caught the pre-install
   `INSTALLING` record, and run 3's capture shows the write-triggered boot being
   restarted by the explicit reset).

Does not establish:

- Any power-loss, brownout, interrupted-write or flash-endurance behaviour.
- The confirm branch (`CONFIRM_REQUEST_MAGIC`); this run only ever takes the attempt path.
- That a byte-identical rewrite can be *detected* by the drift scan: it cannot. Byte
  identity after the boot cannot distinguish "nothing wrote" from "wrote the same bytes",
  which is why the marker image is part of the protocol rather than an optional extra.
- Runtime/UART evidence for the application the bootloader jumps into (the markers stop at
  `JUMP_APP`).
- Any claim about the resident `762c4a75…` bootloader's own behaviour — it ignores the
  W25Q128 candidate, which is why runs 1 and 3 needed the flow image on the board first.

## Artifacts

Evidence text: `00-snapshot-s0.txt`, `01-plan-flash-bootloader.txt`,
`02-write-bootloader.txt`, `03-snapshot-s1.txt`, `04-program-with-reset.txt`,
`06-restore-s0.txt`, `07-verify-final.txt`, `uart-install.txt`/`.json`, `run2-*`
(including `run2-05-write-resets-target.txt`), `run3-*`.

Region payloads and manifests stay outside the repo in
`~/xinyi-hw-evidence/2026-10-28-bootloader-install-drift/`. The journal captures behind
`run2-05-write-resets-target.txt` are
`journal-before-write-reset-test.bin` / `journal-after-write-reset-test.bin` there.
