# openCH OSAL board soak: 13 images x 20 restart rounds

Board: openCH CH32V307, WCH-Link CH549F. This is the hardware-layer soak that closed the OSAL
gap left after the software-layer hardening (S6-126~149, all 23 OSAL checkers green).

## Method

`tools/sweep_opench_osal_images.py --rounds 20` per image: each round programs the image through
WCH OpenOCD (`program ... verify reset`, adapter 1000), lets it run, then reads the verdict from
**RAM** — `xy_image_steps` / `xy_image_failed` / `xy_image_last` — plus two `xTickCount` samples and
the `pc` the core parks in. The VCP ring is lossy, so the RAM globals are the authoritative
channel; `steps` advancing proves the image reached its terminal marker.

Each image also re-runs the same sequence on every round, so a round is a full
re-program + reset + run cycle, not just a repeated observation of one boot.

## Result

13 images, 20 rounds each, **260 rounds, zero failures**. Every image's step count was identical
across all 20 rounds (e.g. queue_lifecycle 9/9/9/…/9), and each ended on its expected terminal
marker:

| image | steps/round | terminal marker |
|---|---|---|
| osal_freertos_sdk_smoke | 4 | `OPENCH_OSAL_FREERTOS_FAST_ISR_OK_A` |
| osal_freertos_isr_stress_smoke | 2 | `OPENCH_OSAL_ISR_STRESS_WORKER_OK_A` |
| osal_freertos_timer_flags_smoke | 6 | `OPENCH_OSAL_TIMER_FLAGS_WORKER_OK_A` |
| osal_freertos_oneshot_timer_smoke | 7 | `OPENCH_OSAL_ONESHOT_TIMER_WORKER_OK_A` |
| osal_freertos_thread_lifecycle_smoke | 7 | `OPENCH_OSAL_THREAD_LIFECYCLE_PASS_20261018_A` |
| osal_freertos_semaphore_lifecycle_smoke | 9 | `OPENCH_OSAL_SEMAPHORE_LIFECYCLE_PASS_20261018_A` |
| osal_freertos_mutex_lifecycle_smoke | 9 | `OPENCH_OSAL_MUTEX_LIFECYCLE_PASS_20261018_A` |
| osal_freertos_event_flags_lifecycle_smoke | 9 | `OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_PASS_20261018_A` |
| osal_freertos_queue_lifecycle_smoke | 9 | `OPENCH_OSAL_QUEUE_LIFECYCLE_PASS_20261018_A` |
| osal_freertos_mempool_lifecycle_smoke | 9 | `OPENCH_OSAL_MEMPOOL_LIFECYCLE_UNALIGNED_SIZE_OK_A` |
| osal_freertos_primitives_smoke | 6 | `OPENCH_OSAL_PRIMITIVES_TIMEOUT_OK_A` |
| osal_freertos_stall_probe_smoke | 8 | `OPENCH_OSAL_PROBE_RX_IRQ_READY_A` |
| osal_freertos_concurrency_smoke | 6 | `OPENCH_OSAL_CONCURRENCY_PASS_20261103_A` |

The step counts also match the S6-136 baseline recorded for these images, so the soak agrees with
the earlier single-run evidence instead of merely repeating it.

## Boundary

20 restart rounds prove **deterministic re-entry**: each image reaches the same terminal state
from a cold reset every time, with no degraded or stuck run. This is a reset-cycle soak, not an
hour-scale continuous soak: it does not cover long-running tick drift, thermal effects, or
multi-hour uptime. Contended-resource behaviour is covered separately by the concurrency image
(see `../2026-11-03-osal-concurrency/`), not by this one.

## Notes

- Two concurrent `sweep` runs must not overlap: they both claim the WCH-Link USB interface and the
  second one fails with `LIBUSB_ERROR_IO` / `claim interface failed`. Run them serially.
- The 12-image batch was split across two invocations purely because of the outer command timeout;
  the per-image results are identical either way.
