# Pandora stale confirm request: the board, not the flow, decides whether this branch is real

S6-191 changed `flow_install()` so that an install of a new candidate clears the confirm
slot before the flow reads it. The argument was sound and the Host contract pinned it, but
the argument's premise - "a request that is in the slot when a candidate gets installed was
written by the image that install just replaced" - had never been produced on hardware. It
could not be: the only writer of that slot is a running application, and an application only
runs after its own install. So the branch had Software-only evidence and the record said so.

This run makes the state reachable and then observes both outcomes on the board from the
same seeded state: the current bootloader refuses the request, a build with the clear
removed honours it.

## The missing fixture

`boards/pandora_stm32l475/fota_slot_seed_main.c` writes the confirm slot, reads it back,
prints what it read and spins. It never resets, because the value has to survive into the
next boot and the capture has to show the write landed. Two targets are built from it:

| image | compile definition | job |
|---|---|---|
| `pandora_stm32l475_fota_slot_seed` | `PANDORA_SLOT_CONFIRM_VALUE=0x46525132` (`REQUEST`) | leave a request behind |
| `pandora_stm32l475_fota_slot_clear` | `PANDORA_SLOT_CONFIRM_VALUE=0` | leave both FOTA slots empty |

Both also clear the restage slot, so a review-gated bootloader cannot be held at that gate
by an earlier run's value.

## Setup, on the board

Snapshot `s0` before any write is the state the run must return to: bootloader
`762c4a75…`, application `d35e2e9a…`, journal `f47a8ec3…`, board_reserved `118f9c6f…`,
rtc_backup `38723a2e…` (all sixteen backup registers zero). Every write below went through
`tools/pandora_hil_program.py`, i.e. re-read every recoverable region, compared it to a
snapshot taken after the previous write, refused on any mismatch, and verified the written
span byte-for-byte (`span_identical=true`, `unexpected_changes=0`, per-write records in
`s0.json/program.json` … `s7.json/program.json`).

The candidate in W25Q128 is the one the 2026-10-29 confirm-chain run left at `0xF00000`
(source commit `56ee6030…`); this run did not re-program it. The application region was
restored to the pre-run image before each boot, so every boot below really installed.

Seeding (`uart-seed-3.txt`, 195 B):

```
PANDORA FOTA SLOT SEED READY
SEED_COMMIT 963f2f183b515d469a5743d9ba5e65450c57a322
SEED_INTENT=0x46525132
SEED_SLOT1_BEFORE=0x00000000
SEED_SLOT1=0x46525132
SEED_SLOT2=0x00000000
SEED_DONE
```

i.e. the slot goes from "no request" to "request" on the board, read back from the register.

## Run A - the current bootloader, same seeded request (`uart-boot-fixed-2.txt`, 2892 B, sha256 `88e7436b…`)

```
READY / BOOTLOADER_COMMIT 963f2f18… / CANDIDATE_INSTALLED / ATTEMPT_COMMITTED / JUMP_APP
  app: READY / FIRMWARE_COMMIT 56ee6030… / OSAL chain… / CONFIRM_REQUESTED → reset
READY / COMMIT / CANDIDATE_ALREADY_INSTALLED / ATTEMPT_COMMITTED / JUMP_APP
  app: READY / COMMIT / CONFIRM_REQUESTED → reset
READY / COMMIT / CANDIDATE_ALREADY_INSTALLED / CANDIDATE_CONFIRMED / JUMP_APP
  app: READY / COMMIT / CONFIRM_ACKNOWLEDGED / CONFIRM_FIXTURE_DONE → stop
```

`BOOTLOADER_CHAIN_REVIEW_CANDIDATE chain=CONFIRM terminal=jump boots=3 attempts=2`.

Frame 1 is the point: the seeded request was discarded with the install, one attempt was
charged, and the image was **not** confirmed before running. The confirm that does arrive in
frame 3 is for a request the application wrote after it had run in frame 2 - the legitimate
path, visible in the same capture.

## Run B - the clear removed (`uart-boot-mutant.txt`, 357 B, sha256 `ca8d1485…`)

The only change is `flow_install()`'s slot write, deleted for this build. Nothing else was
touched; the mutant image is `a2fcaaf4…` (15708 B) versus `67a419d6…` (15768 B) for the
committed bootloader. Same board state, same candidate, same seed procedure:

```
READY / BOOTLOADER_COMMIT 963f2f18… / CANDIDATE_INSTALLED / CANDIDATE_CONFIRMED / JUMP_APP
  app: READY / FIRMWARE_COMMIT 56ee6030… / CONFIRM_ACKNOWLEDGED / CONFIRM_FIXTURE_DONE → stop
```

The candidate was **confirmed on its first boot, before it ever executed**, zero attempts
were charged, and the application then read the acknowledgement for an image that had never
run. That is precisely the S6-191 defect, on silicon, from a one-hunk build.

The mutant source was restored from a byte copy: `pandora_fota_boot_flow.c` sha256
`993faa9c…` before and after, `git diff` empty, and the rebuilt bootloader came out
byte-identical (`67a419d6…`). The mutant is a manual temporary change; nothing in the tree
can weaken the clear without a diff.

## What the validator could and could not see

Both captures are accepted as `chain=CONFIRM` by the validator as it stood: a confirmation
is a confirmation. The difference - `attempts=0` and a confirm decision inside the frame that
performed the install - was printed but not judged, so the run that reports the defect would
have been filed as a valid confirmation.

`validate_fota_bootloader_capture.py` now rejects a frame whose decision is
`CANDIDATE_CONFIRMED` while its install marker is `CANDIDATE_INSTALLED`: the request can only
have come from the image the install replaced. Against the archived bytes:

| capture | validator at `963f2f18` | validator now |
|---|---|---|
| `uart-boot-mutant.txt` | `CHAIN_REVIEW_CANDIDATE chain=CONFIRM boots=1 attempts=0` | `VALIDATION_FAILED` – `boot frame 0: candidate confirmed in the same boot that installed it` |
| `uart-boot-fixed-2.txt` | `CHAIN_REVIEW_CANDIDATE chain=CONFIRM boots=3 attempts=2` | unchanged |

The Host contract grew four cases (`StaleRequestConfirmationTests`, validator suite 53 → 57),
two of which replay these two board captures from `docs/validation/evidence/…`, so a
regression in either the rule or the flow surfaces as a red test rather than as a reread of
this record.

## Cleanup

`uart-slot-clear.txt` leaves both FOTA slots at `0`. Then
`tools/pandora_hil_state.py restore --snapshot s0.json --yes` and `verify`:

```
bootloader 762c4a75…  application d35e2e9a…  fota_metadata f47a8ec3…
board_reserved 118f9c6f…  rtc_backup 38723a2e…   verify_ok regions=5
```

All five regions are byte-identical to the pre-run snapshot, RTC backup included.

The mutant run was not the last one: the clean capture of the current bootloader
(`uart-boot-fixed-2.txt`) was taken afterwards, and it leaves that bootloader at
`0x08000000` with the candidate installed and the journal `CONFIRMED`. A second
`restore --snapshot s0.json` was therefore performed at the end and re-verified:
`verify_ok regions=5`, same five hashes. Both restores are the same operation from
the same snapshot; the second one is the state the board was left in.

## Also kept: a mis-timed capture

`uart-seed-1.txt` (10105 B) is what a capture window looks like when it closes before the
programming reset lands: it contains nothing but the application output that was already
streaming (`ICM20608_*`), and the seed's own banner appears in `uart-seed-1b.txt` instead.
`uart-boot-fixed-1.txt` is the same artefact from the other side - its first frame was cut
off by an extra reset mid-install, and the validator correctly calls it
`frame 0 ended without a decision marker`. Both are kept because they are the evidence for
the capture rule used afterwards: arm the UART, flush it, then reset inside the window.

## Boundary

- One board (Pandora STM32L475VE, ST-Link V2.1 `V2J24S11`, CP2102 `/dev/ttyUSB0`), one
  candidate, one seeded request per run, bounded 6-25 s windows.
- RTC backup retention is shown across debug resets and software resets only. **No power was
  removed**, so the slot's brownout/power-loss behaviour is untouched by this record.
- Not covered: flash endurance, interrupted/partial install, secure (signed) FOTA, bootloader
  vector handoff correctness, or any runtime conclusion about the OSAL chain the application
  prints before it requests confirmation.
- The mutant proves the clear is load-bearing; it is not a claim that the mutant could occur
  in a shipped image.
