# 2026-10-29 Pandora OSAL/FreeRTOS runtime chain — provenance

Board: Pandora STM32L475VE, ST-Link V2.1 `V2J24S11`, evidence UART CP2102 `/dev/ttyUSB0`
(115200 8N-1, opened and flushed **before** `st-flash reset`).

## Images

The runtime image (`pandora_stm32l475_rtos`) is 42+ KiB, larger than the 32 KiB
bootloader region, so every flash cycle is two snapshot-gated spans:
`rtos-part1.bin` at `0x08000000` and `rtos-part2.bin` at `0x08008000`
(`tools/pandora_hil_program.py run`, each `span_identical=true unexpected_changes=0`).

| image | commit | size | evidence |
|---|---|---|---|
| `rtos-image-29fd5ba9.bin` | `29fd5ba9` | 42564 B | resource-lifecycle markers on the board; PM block reached |
| `rtos-image-abffec77.bin` | `abffec77` | 42564 B | `pre-eventfix-abffec77-run-*.log` |
| `pandora_stm32l475_rtos.bin` | `45cde0ef` | 42604 B | `final-run-*.log`, `pm-boundary-record.json` |

## Runs of the abffec77 image (identical bytes)

- `run-a`: full chain, 32 HAL sleep cycles, boundary + repeat, zero error markers.
- `run-b` / `run-c`: window closed mid-chain, no error marker.
- `run-d`: `OSAL_EVENT_MISMATCH` — the event flag was used as if it were a queue slot.
  Fixed in `45cde0ef` (timeout now prints `OSAL_EVENT_COALESCED`; the queue stays strict).

An earlier run of `29fd5ba9` printed `OSAL_MUTEX_MISMATCH` and the next run of the same
bytes completed the chain; `abffec77` replaced that exact-equality cross-check with the
bound a depth-2 queue can actually guarantee.

## Runs of the 45cde0ef image

- `final-run-3`: **complete** — 7444 B, 2 boots, 32 `OSAL_PM_HAL_SLEEP_CYCLE`,
  `OSAL_PM_CHARGER_BOUNDARY_OK`, `OSAL_PM_HAL_SLEEP_REPEAT_OK`, zero error markers.
  Validator: `B1_PM_CHARGER_FAILCLOSED_HAL_SLEEP_PASS`.
- `final-run-1` / `final-run-2`: no error marker, but only 2–4 fast/slow cycles inside a
  70 s window. The same image therefore does **not** produce a reproducible timeline;
  the suspected cause (`uart_text()` holds interrupts off for the whole blocking send,
  ~1.7 ms per line at 115200) is read from source and has **not** been measured yet.

## Snapshots

`pandora-state-pre` is the board state before the first write of this session. The gate
snapshots for the later images were pruned to their `program.json` records
(`gate-pre2/…`, `gate-mid2/…`, `gate-pre3/…`, `gate-mid3/…`) because each was valid only
for the single write it authorised.
The board was left restored to `2026-10-29-fota-confirm/pandora-state-s0`
(bootloader `762c4a75…`, application `d35e2e9a…`), verified byte-identical.

## Not evidence for

PM system sleep/wake (no charger IC: unreachable by design), power current, wake latency,
STOP/STANDBY/SHUTDOWN, endurance, FOTA confirm branch, STM32U5 runtime, charger hardware.
