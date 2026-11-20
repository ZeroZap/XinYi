# openCH OSAL concurrency: three contended-resource scenarios on the board

Board: openCH CH32V307, WCH-Link CH549F. New image
`boards/opench_ch32v307/osal_freertos_concurrency_main.c` closes the OSAL gap the earlier 12 smokes
left open: every existing image exercised one primitive from a **single task** in sequence. This one
runs three contended scenarios, each with multiple FreeRTOS tasks racing through `xy_os_*`.

Verdict is read from RAM (the VCP ring is lossy): `xy_image_steps` / `xy_image_failed` /
`xy_image_last`, plus two `xTickCount` samples and the `pc` the core parks in.

## Scenarios

1. **Counting semaphore, max count 2** — three workers each loop 50 times through
   `xy_os_semaphore_acquire` (wait-forever) → bump `g_sem_active` → check ≤ 2 → `xy_os_delay(1)`
   → decrement → `xy_os_semaphore_release`. A violation counter records any observed active count
   above 2, and the final `xy_os_semaphore_get_count` must be back to 2.
2. **Mutex atomicity** — two workers each `xy_os_mutex_acquire` → `++g_counter` → release → yield,
   1000 iterations each, forcing interleaving. The final `g_counter` must be exactly 2000; any lost
   increment (the failure mode a missing mutex produces) fails the scenario.
3. **Multi-producer queue** — two producers push ten `uint32` each (1..10 and 11..20), one consumer
   drains all twenty. Count must be 20 and the sum must be 210 (FIFO + no loss / no duplication).

## Results

Single run: `steps=6 failed=0 ticks=1467->4704 pc=prvIdleTask last=…PASS_20261103_A`.

Soak: 20 rounds, every round `steps=[6,…]` and `failed=0`, last marker
`OPENCH_OSAL_CONCURRENCY_PASS_20261103_A` — the sequence is deterministic across resets.

## Boundary

Single-core interleaving proof (FreeRTOS on one RISC-V core: tasks race via preemption and yields,
not true parallelism). It proves the OSAL `xy_os_*` wrappers preserve semaphore max-count, mutex
mutual exclusion and queue FIFO/no-loss under contention. It is not a stress benchmark: the worker
counts are small (3/2/3) and bounded (50/1000/20 iterations), and it does not exercise priority
inversion, ISR-side release under contention, or hour-scale continuous soak.
