# Pandora STM32L475VE reset-cause attribution: the register, the mask, and the name

The smoke image reported a reset kind, and the evidence was graded by the same
code that produced it. Two defects came out of making the three numbers
independent:

1. `SYS_RESET_CSR 0x...` printed `xy_hal_sys_get_reset_reason()` — the decoder's
   own output — under the name of the register it was decoded from, and
   `validate_sys_reset_request_capture.py` never looked at the value at all. The
   pin / software / watchdog attribution could drift, or a brownout boot could be
   mislabelled, and the same capture would still pass.
2. The kind was chosen by an inline `if` chain in `main.c`:
   `WATCHDOG` → `SOFTWARE` → `EXTERNAL_PIN && !BROWNOUT` → else `POWER_ON`. A
   brownout boot (or a low-power exit) therefore reported
   `SYS_RESET_KIND POWER_ON`. Nothing in the tree could notice: that chain was
   compiled into no test.

## What the image prints now

```
SYS_RESET_CSR <RCC->CSR, latched before the flags are cleared>
SYS_RESET_FLAGS <decoded xy_hal_sys_reset_reason_t mask>
SYS_RESET_REQUEST <retained reason, one-shot>
SYS_RESET_KIND <UNKNOWN|POWER_ON|EXTERNAL_PIN|BROWNOUT|SOFTWARE|WATCHDOG|LOW_POWER>
```

`pandora_reset_kind_from_reason()` owns the attribution with a documented
priority — WATCHDOG > SOFTWARE > BROWNOUT > LOW_POWER > EXTERNAL_PIN — because
an internal reset also asserts NRST on this part. Only an empty mask is
`POWER_ON`; a mask with a bit the decoder does not know is `UNKNOWN` and stops
the boot instead of being folded into `POWER_ON`. A brownout or low-power boot
prints `SYS_POWER_EVENT_BOOT_CONTINUES` rather than masking the event with
another reset.

`xy_hal_sys_get_reset_raw_flags()` publishes `RCC->CSR` on the L4 backend; the
WCH and U5 backends return `XY_HAL_ERROR_NOT_SUPPORTED` rather than inventing a
value, and the image then prints `SYS_RESET_CSR UNAVAILABLE`.

The validator now recomputes the decode from the register and the name from the
mask, and rejects a record that does not publish the register. The archived
2026-10-27 captures fail it, which is the point: they cannot be re-checked.

## Result — two independent runs on one board

Image `pandora_stm32l475_smoke.bin`, 14736 B, SHA-256
`66dde1e343425a449f5c7dd67f98de604d3d6dc3f1d6fc12306c24ad5a6e3a8f`, embedded
`FIRMWARE_COMMIT 66edfb280f0e344b27c2103d8dc22ba3b1151a8a`.

| boot | `SYS_RESET_CSR` | `SYS_RESET_FLAGS` | `SYS_RESET_KIND` | `SYS_RESET_REQUEST` |
|------|-----------------|-------------------|------------------|---------------------|
| 1 (`st-flash reset`, NRST) | `0x04000600` | `0x00000001` | `EXTERNAL_PIN` | `0` |
| 2 (SYS-requested) | `0x14000600` | `0x00000005` | `SOFTWARE` | `1` |
| 3 (IWDG expiry) | `0x24000600` | `0x00000009` | `WATCHDOG` | `0` |

Both captures (`uart-nrst-reset.txt` 11072 B, `uart-nrst-reset-2.txt` 8264 B)
show the same three records, and the validator recomputes all three numbers:

```
SYS reset-request contract: PASS
  POWER_ON/EXTERNAL_PIN -> SYS_RESET_REQUEST 0 (no record before the first SYS-requested reset)
  SOFTWARE -> SYS_RESET_REQUEST 1 (reason recorded before the previous software reset)
  WATCHDOG -> SYS_RESET_REQUEST 0 (record consumed one-shot, not carried into the watchdog boot)
```

`0x600` in every read is `RCC_CSR.MSISRANGE_4` (and `LSION`/`LSIRDY` share the
register), which is why the raw value cannot be compared to the mask directly
and why the decoder must be recomputed rather than trusted. The boots after the
chain ran the rest of the smoke image (66 banners, 66 `AHT10 0x38 ACK`, one
`FOTA_BOOT_CONTRACT_OK` in the first capture), so the attribution change did not
disturb the existing boot contract.

## The board found a defect in the first version of this change

The first programmed build printed

```
SYS_RESET_CSR 0x00000600
SYS_RESET_FLAGS 0x00000007
SYS_RESET_KIND SOFTWARE
```

`0x600` carries none of the cause bits. `main.c` read the register *after*
`xy_sys_init()` had already called `__HAL_RCC_CLEAR_RESET_FLAGS()` — the exact
ordering the new HAL contract warns about. The validator refused that capture
(216 B, `uart-program.txt`, 11 bytes of the record only; the core was halted by
the programmer's read-back before the next boot):

```
$ ./validate_sys_reset_request_capture.py --capture uart-program.txt --firmware-commit 2aecb046…
SYS reset-request contract: FAIL: record 0: SYS_RESET_FLAGS 0x00000007 does not match CSR 0x00000600 (decoded 0x00000000)
SYS reset-request contract: FAIL: observed 1 SYS reset record(s), expected at least 3
```

That log is kept as-is (`validator-before-ordering-fix.log`). The fix
(`66edfb28`) latches the register inside `xy_sys_init()` before the clear and
publishes it through `pandora_sys_reset_csr()`; the host test now asserts
`raw_seq < clear_seq`, so the ordering cannot regress silently.

## Verification

- Focused: `pandora_sys` 19/19 (16 attribution/register cases + 3 register
  cases), `pandora_sys_reset_capture_validator` 11/11.
- Full host suite: `make test-unit` → **310/310**.
- Compile gates: PC (`make`), `HAL_PLATFORM=STM32L4`, `HAL_PLATFORM=STM32U5`, and
  a clean `STM32L4_BOARD=pandora_stm32l475` configure+build of the smoke image
  (`build/pandora-s6-159`); `git diff --check` clean.
- Board guard: `check_pandora_stm32l475_board.py` PASS, including the new rule
  that `main.c` must not test `XY_HAL_SYS_RESET_REASON_` bits inline.
- Symbols in the flashed ELF: `pandora_reset_kind_from_reason`,
  `pandora_reset_kind_name`, `pandora_sys_reset_kind`,
  `xy_hal_sys_get_reset_raw_flags` (`nm`), i.e. the code the capture exercises
  is in the image.
- Program: `tools/pandora_hil_program.py` snapshot-gated write at `0x08000000`,
  `span_identical=true`, `erased_tail=1648`, `unexpected_changes=0`.
- Board: Pandora STM32L475VE, ST-Link V2.1 `V2J24S11` serial
  `066AFF313933554D43244015`, chipid `0x415`, 512 KiB flash; UART CP2102
  `/dev/ttyUSB0` 115200 8N1, capture armed before `st-flash reset`.
- Restore: `tools/pandora_hil_state.py restore` from `pre-snapshot.json` then
  `verify` → `verify_ok regions=5`, bootloader `762c4a75…`, application
  `d35e2e9a…`. Only the manifests and program records are committed
  (`*.bin` is git-ignored), so re-creating the dumps needs a fresh snapshot of a
  board in that state; the region hashes are the reproducible part.

## Boundary

This proves, on one Pandora STM32L475VE and one committed image, that the three
reset causes this board can be made to produce unattended (NRST, SYS-requested,
IWDG) decode correctly from the register, are attributed as reported, and that
the retained-request chain still holds.

- **Brownout and low-power attribution is host-only.** Inducing a supply fault
  needs manual intervention, which this run did not have; the priority table is
  covered by host tests, not by a board observation.
- A cold power-on may also assert `BORRSTF` on this part. No unattended run could
  produce one, so `POWER_ON` here means "no cause bit set" (a debug-port core
  reset), not "mains power was applied".
- Not covered: power-loss/brownout retention of the RTC backup record, reset
  endurance, WWDG, reset-cause latency, or any claim about other boards.
