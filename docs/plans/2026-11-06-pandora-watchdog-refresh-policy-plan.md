# Pandora watchdog refresh policy — board slice plan

Date: 2026-11-06
Owner: Zero
Board: Pandora STM32L475VE (reference board), CP2102 `/dev/ttyUSB0` 115200 8N1, ST-Link/V2.1

## Why this slice

`docs/validation/component-evidence-matrix.md` row `SYS` carries the Board column
`n/a` and the limitations column ends with:

> power-loss/brownout 区分与 watchdog refresh policy 仍 pending；U5 仅补充 compile

That row is the priority-#1 area of the sprint ("Pandora reference-board SYS strong
backend / 复位原因闭环"). The reset-reason half of it is closed: S2-04 and S5-107
already produced write/verify/read-back + UART B1/B2 for software reset, IWDG timeout
and ST-Link pin reset. **The refresh half has never been run.**

What exists today is the opposite of a refresh policy. `boards/pandora_stm32l475/main.c`
starts the IWDG in the `PANDORA_RESET_KIND_SOFTWARE` branch and then spins in an empty
`for (;;)` — the board is *deliberately* left to die of watchdog expiry so the
`WATCHDOG` boot path gets exercised. That proves detection of a watchdog reset. It
proves nothing about the other half of the contract: that a watchdog which **is
refreshed** on schedule keeps the board alive, and that the refresh is what keeps it
alive rather than the watchdog being inert.

## What "watchdog refresh policy" has to mean to be worth a claim

A claim of "the refresh policy works" needs an anchor that could fail. Three things
have to be true at once, and each one is separately falsifiable:

1. **The watchdog is armed and counting.** Not asserted — measured. The image reports
   the window it armed through the canonical HAL, and the board stays alive across a
   span that is many multiples of that window, which an unarmed watchdog could not
   survive.
2. **A refresh actually extends the deadline.** The board must still be alive well
   past the point where an unrefreshed watchdog would have expired it.
3. **Removing the refresh kills the board.** The negative control. If the refresh is
   removed, the watchdog must actually fire and reset the board — otherwise point 2
   is satisfied by a watchdog that was never really counting, and the whole claim is
   decoration.

Point 3 is the one that makes the evidence mean something. A capture that only shows
"the board survived" is indistinguishable from a capture of an inert watchdog.

### What the STM32L4 backend can and cannot observe

The plan originally asked for a *countdown*: a remaining time that decreases across
the run and jumps back up after each feed. That is not observable on this backend, and
the image does not pretend otherwise.

`xy_hal_iwdg_get_remaining_time()` on STM32L4 is **computed**, not read: the HAL
returns `(reload + 1) * prescaler * 1000 / 32000` from the configuration it was given
(`components/hal/stm32/stm32l4/xy_hal_wdg.c`). The IWDG has no readable down-counter
on this part, so there is no register to poll and no way to observe the remaining time
fall. A capture that claimed to show one would be reporting the same computed constant
twice.

The claim is therefore anchored on the two things the part really does expose:

- **elapsed wall time** from the canonical `xy_hal_sys_get_tick_count()`, and
- **the reset cause** on the next boot, which the board's own SYS owner decodes from
  the hardware flags — a watchdog expiry is distinguishable from a power-on, and the
  image additionally requires that the retained marker says the feed phase finished.

The negative control carries the weight that the countdown would have carried, and it
carries it better: a countdown is a number the image prints, while a reset the board
did not ask for is an event that happened to it.

## The slice

A new board image `boards/pandora_stm32l475/watchdog_main.c` that runs the watchdog
against the **canonical** `xy_hal_iwdg_*` HAL (never a vendor `HAL_IWDG_*` call), and
prints a machine-checkable marker stream:

- arm the IWDG with a known reload/prescaler and report the window that arithmetic
  implies,
- feed on a schedule, reporting the elapsed wall time at each feed so the capture
  shows the board advancing across a span that is many multiples of the window,
- run for a bounded window that is several multiples of the watchdog timeout, which
  is only survivable *because* of the refreshes,
- then **stop refreshing** and let the watchdog fire, so the capture ends with a
  real watchdog reset and a `WATCHDOG` attribution on the next boot.

The image then needs a fail-closed validator,
`boards/pandora_stm32l475/validate_watchdog_capture.py`, that is bound to the exact
firmware commit in the capture (the same identity binding the PID validator uses) and
that rejects: a missing marker, an out-of-order marker, an error marker, a feed
interval that is not shorter than the armed window, a survival shorter than one
window, and a capture whose terminal state is not the expected watchdog reset.

The validator's own cases become a CTest (`tests/unit/hal_component/`), including
mutation checks that each rejection rule can actually fire.

The board guard `tests/unit/hal_component/check_pandora_stm32l475_board.py` gains the
new entry so it cannot silently drift: canonical SYS init, board-owned startup,
canonical SysTick dispatch, canonical `xy_hal_iwdg_*` calls, no vendor IWDG calls,
and the same three post-mortem words the other images publish.

## Verification gates

1. Focused: build the new target, run the validator's own CTest.
2. Full Host: `make test-unit`.
3. Compile gates: PC, STM32L4, STM32U5 (U5 stays enhancement-only).
4. Board: clean build from a committed HEAD, `nm` the ELF for the symbols and for the
   firmware identity string, snapshot-gated write via `tools/pandora_hil_program.py`,
   read-back comparison, UART capture through `capture_window.py`, machine-parse the
   capture with the validator.
5. `git diff --check`, path-limited commit, push, read back `HEAD == origin/main`
   with ahead/behind 0/0.

## Boundaries this slice will not claim

- Not power-loss or brownout: those need a physical rail event and are explicitly
  out of scope for an unattended run.
- Not endurance, not a timing/precision claim about the watchdog window: the LSI
  frequency is a range, not the nominal 32 kHz the HAL arithmetic assumes, so the
  measured remaining time is reported as observed, not as a datasheet value.
- Not a claim that any *application* has a correct refresh policy. This proves the
  HAL refresh contract on the target, not a product's scheduling.
- Not a scheduler/RTOS claim: this image is bare-metal, so the feed interval is a
  busy-wait interval, not a task deadline.
