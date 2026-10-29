# 2026-10-29 Pandora OSAL/FreeRTOS runtime timeline — measurement, not inference

Board: Pandora STM32L475VE, ST-Link V2.1 `V2J24S11`, evidence UART CP2102 `/dev/ttyUSB0`
(115200 8N-1). Every capture opened and flushed the port **before** `st-flash reset`.

This directory answers the one question S6-148 left open: *why* did the same runtime image
complete the chain in one capture and advance only 2–4 fast/slow cycles inside the same 70 s
window in another? S6-148 had a source-level hypothesis (`uart_text()` holds interrupts off for
the whole blocking send, ~1.7 ms per line, so the kernel tick is masked while logging). That
hypothesis is **refuted by measurement**, and the real cause was a silent halt.

## What was measured

`boards/pandora_stm32l475/validate_runtime_timeline.py` (`capture`/`analyze`, CTest
`pandora_runtime_timeline`, 11 cases incl. a real PTY window) keeps the timebase outside the
board: the host records the arrival offset of every UART line, and the analysis reports
per-cycle wall time, the largest marker-to-marker gaps, and the excess wall time of windows
whose only firmware work is a known `xy_os_delay()`.

| run | image (embedded commit) | window | bytes / lines | cycles | slowest cycle | worst delay excess | outcome |
|---|---|---|---|---|---|---|---|
| `run1` | `6ed9c8e9` 42604 B | 70 s | 3559 / 151 | 4 | 1109 ms | 60.3 ms | output stops at 3.28 s |
| `run2` | `5cbcfd24` 43356 B | 70 s | 3781 / 158 | 4 | 1123 ms | 75.0 ms | same stop, now attributed |
| `run3` | `01456fb4` 43372 B | 80 s | 7412 / 339 | 13 | 1123 ms | 75.0 ms | PM chain complete, stops at 7.9 s |
| `run4` | `7e59e90c` 43420 B | 90 s | 49814 / 2783 | 171 | 1128 ms | 40.0 ms | continuous, zero error markers |

The logger hypothesis is dead on the numbers: the delay windows that ask for 500 kernel ticks
measurably take 500–575 ms of wall time with all logging in place, and after the fixes the
largest gaps in the whole capture are the intended `OSAL_TASK_SLOW → OSAL_MUTEX_FAST` 499 ms
delay, not multi-second stalls. What looked like a slow timeline was the image **halting**.

## The two halts, in the order they were uncovered

1. `xy_image_failed = 2` (a `fail()` check), last marker `OSAL_MULTI_CONSUMER_1_TAKE` /
   `PANDORA_SPI_DMA_ABORT_RECOVERY_OK` depending on the run. The image had ~100 `fail()`
   call sites and most print nothing first, so the stop could not be attributed from the
   post-mortem words it already published. `5cbcfd24` makes `fail()` record `__LINE__` in
   `xy_image_failed_line` before halting.
2. With that in place, `run2`'s RAM read reported **line 588**: right after `OSAL_TASK_FAST`
   the fast task released `sync_sem` and `xy_os_semaphore_release()` returned
   `XY_OS_ERROR_RESOURCE`. That status means the binary semaphore is already signalled, which
   this fixture produces whenever the producer runs ahead of the consumer — the queue, not the
   hint, carries the item. `01456fb4` accepts exactly that status (counting it) and keeps
   every other status fatal; `run3`'s RAM read reports `os_semaphore_redundant_releases = 1`
   at the end, i.e. the tolerated case really did occur.
3. `run3` then completed the PM block (32 `OSAL_PM_HAL_SLEEP_CYCLE`, boundary, repeat) and
   `OSAL_MULTI_PRODUCER_OK`, and stopped again at 7.9 s: this time in
   `vApplicationStackOverflowHook`, task name `osal-isr`. Both ISR-side tasks were created from
   the same attribute literal, so that name could not say which one overflowed.
   `7e59e90c` gives the IPC ISR stress producer its own attribute (1536 B, `osal-ipc-isr`) and
   raises the wake consumer to 1024 B.

## Run4 (accepted) detail

- Image 43420 B, embedded commit `7e59e90c…` (= `HEAD` at build time); two snapshot-gated spans
  (`span_identical=true unexpected_changes=0`, `drift_stable=true`).
- Capture 49814 B / 2783 lines over 90 s: 171 fast/slow cycles, `OSAL_PM_INIT_REFUSED
  4294967295` → `OSAL_PM_CHARGER_BOUNDARY_OK` → 32 × `OSAL_PM_HAL_SLEEP_CYCLE` →
  `OSAL_PM_HAL_SLEEP_REPEAT_OK`, `OSAL_MULTI_PRODUCER_OK`, 32 `OSAL_IPC_ISR_STREAM_DELIVER`,
  `OSAL_TIM6_IRQ_RECOVERED`, two `OSAL_RESOURCE_DELETED`/`OSAL_LIFECYCLE_REINIT`, **zero**
  error/mismatch markers.
- Post-mortem RAM read after the window: `xy_image_failed = 0` — the image was still running
  when the capture closed (the 12 × 44 s IPC ISR stress phase is longer than the window).
- `validate_pm_sleep_capture.py` on the same capture → **`B1_PM_CHARGER_FAILCLOSED_HAL_SLEEP_PASS`**
  (`pm-boundary-record-run4.json`, 49814 B, errors=0), so S6-148's PM boundary verdict now stands
  on a deterministically paced image rather than on a chain that silently stopped.
- After the last write: `restore` from `pandora-state-pre` + `verify_ok regions=5` — the board is
  back byte-exactly at the pre-run state (`762c4a75…` bootloader, `d35e2e9a…` application).

## Snapshots

`pandora-state-pre` is the board state before the first write (the committed `s0` baseline).
`pandora-state-mid…mid7` are the per-write gate snapshots; their region dumps were pruned to the
`program.json` records each write authorised, because each was valid for exactly one write.
Raw RAM dumps (`postmortem-ram*.bin`) are kept — they are the attribution evidence.

## Not evidence for

Kernel-tick-loss attribution (that needs firmware-side counters, not host arrival times);
interrupt latency, scheduler jitter or throughput; PM system sleep/wake (no charger IC on this
board); STOP/STANDBY/SHUTDOWN; power measurement; FOTA confirm branch, flash endurance,
power-loss or brownout; STM32U5 runtime. The tick-loss question is now answered negatively for
this logger at this baud rate, which is not the same as proving the logger costs nothing.
