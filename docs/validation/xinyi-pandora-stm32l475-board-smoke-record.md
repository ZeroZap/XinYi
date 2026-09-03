# Pandora STM32L475VE Board Smoke Record

**Date**: 2026-09-03
**Source commit**: `9b50ec38c4e32f9e37c93a0f3f70379453c9622f`
**Board**: Pandora STM32L475VE
**Status**: `B1_BOARD_SMOKE_VERIFIED`; `B2_PENDING`
**Evidence classification**: `PROGRAMMING_VERIFIED`; `BOARD_RUNTIME_B1`

This record separates verified programming, observed normal-path board runtime, and still-pending
negative/recovery evidence. B1 does not imply AHT10 NACK/recovery verification.

## Environment

- ST-Link USB identity: `0483:374b` (`ST-LINK/V2.1`)
- ST-Link firmware: `V2J24S11`
- Probe serial: `066AFF313933554D43244015`
- Target detected by `st-info --probe`: STM32L47x/L48x, chip ID `0x415`, 512 KiB Flash,
  96 KiB SRAM
- Toolchain: Arm GNU Toolchain 15.2.Rel1, GCC `15.2.1 20251203`
- Programmer: `st-flash 1.8.0`

## Build and programming evidence

The following focused source contract passed:

```text
ctest --test-dir build/tests/unit -R '^pandora_stm32l475_board$' --output-on-failure
1/1 passed
```

The board target cleanly configured, linked, and produced:

```text
text=7236 data=12 bss=2700 dec=9948
FLASH used=7248 bytes (1.38%)
RAM used=2712 bytes (2.76%)
```

The exact image was programmed at `0x08000000`. `st-flash` reported:

```text
Attempting to write 7248 bytes
Flash written and verified
```

## Historical ST-Link VCP blocker

Before programming, Linux exposed `/dev/ttyACM0` and the probe was visible. Opening the VCP for a
six-second 115200-8-N-1 capture did not return data and did not terminate normally; the capture was
stopped by the 20-second command timeout (`CAPTURE_BYTES=0`). Immediately afterward:

- `/dev/ttyACM0` was absent;
- `lsusb` no longer listed `0483:374b`;
- `st-info --probe` reported `Found 0 stlink programmers`.

This matches an unstable ST-Link V2.1 VCP/USB path, so no UART banner was captured. LED, KEY0, AHT10
ACK/measurement, and NACK recovery were not independently observed and remain
`BOARD_RUNTIME_PENDING` at that time. The independent-UART result below supersedes that B1 blocker;
the board-local ST-Link VCP limitation remains.

## 2026-09-03 independent-UART B1 result

The current committed image was rebuilt with embedded identity
`9b50ec38c4e32f9e37c93a0f3f70379453c9622f`, programmed once with `st-flash --reset write`, and
reported `Flash written and verified`. PE7 was then visually observed toggling at the firmware's
500 ms interval.

An independent WCH-Link UART connected to PA9/PA10/GND enumerated as
`/dev/serial/by-id/usb-wch.cn_WCH-Link_B49C8F0639CE-if01`. A bounded six-second 115200-8-N-1
capture retained 1560 bytes and was classified `B1_REVIEW_CANDIDATE`; review confirmed the real
adapter path, exact flashed commit, and ordered runtime content. The log contains ten complete
normal-path cycles, each with the Pandora banner, matching firmware identity, AHT10 ACK, and a
plausible measurement. Observed ranges were 77153–77180 milli-percent RH and 28583–28601
milli-degrees C.

Retained evidence:

- [raw UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-b1.txt), SHA-256
  `93d4dc22669b26b8b666f4bc4d25968f9b2aa02959968f476fbfe4191730a658`
- [capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-b1.json), SHA-256
  `54f357183833ace492f2382e591fda3e4f75dc90002d625c7ae4d0f676652532`

This closes the normal-path board smoke at B1 for this exact board/image/wiring.

A second eight-second capture retained 2052 bytes while KEY0 was held for about two seconds. It
contains 13 matching firmware cycles and four `KEY0` events, verifying the PD10 active-low key path:

- [KEY0 UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-key0.txt), SHA-256
  `57e69784ce8a436f969fd9562826cf667aca34c3561523a99544e8e33b720f1b`

No retained capture contains an AHT10 NACK, so negative/recovery behavior remains pending and B2 is
not granted.

## 2026-09-03 OSAL/FreeRTOS current-HEAD runtime revalidation

The clean source commit `ac3f20f48c4a7c2044f6befa2758b4e2a335b628` was rebuilt as the
Pandora RTOS image. The ELF contained that exact identity; the BIN SHA-256 was
`88855cc10c94108a55023a75c7fdd4a759a40e873fc4237dd43022ad7fd953f9`. `st-flash --reset write`
reported `Flash written and verified` for 11688 bytes.

A reset-synchronized eight-second capture from the independent WCH-Link retained 489 bytes. It
contains the exact banner and source identity, 16 `OSAL_TASK_FAST` events at 506–508 ms intervals
(mean 506.53 ms), and eight `OSAL_TASK_SLOW` events at 1013–1014 ms intervals (mean 1013.14 ms).
This verifies OSAL-created FreeRTOS task scheduling, SysTick progress, and `xy_os_delay()` timing on
the current board/image at B1. PE7 is toggled by the fast task in the verified source, but its physical
state was not visually reconfirmed during this run; ISR-to-task signaling and queue/semaphore stress
remain pending.

Retained evidence:

- [raw RTOS UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-freertos-ac3f20f4.txt),
  SHA-256 `fe0e414949acc2ea694c4f5c4c3aa8de5b1cc14856f0b3a6f52002cd7c33a2e5`
- [RTOS capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-freertos-ac3f20f4.json),
  SHA-256 `d36ed473012d57ef61b70025a3bcb64641029df94b746b44968f3e7efe081e44`

## 2026-09-03 OSAL semaphore runtime result

The clean firmware commit `c9b2d3d18fd6ffdc2f9ade88ecb176f5a8f1d046` added a binary OSAL
semaphore between the existing tasks. The fast task releases it every 500 ticks; the slow task blocks
with a 1200-tick timeout and emits `OSAL_SEM_TAKE` only after a successful acquire. The application
continues to use only `xy_os_*` synchronization APIs.

`st-flash --reset write` programmed and verified 13084 bytes. A reset-synchronized six-second capture
retained 670 bytes and showed 12 strictly ordered `OSAL_TASK_FAST → OSAL_SEM_TAKE → OSAL_TASK_SLOW`
cycles, no timeout marker, and release-to-acquire latency of 0–2 ms. Fast-task intervals were
506–507 ms (mean 506.64 ms). This grants task-context OSAL semaphore B1 for this board/image; queues,
ISR-to-task synchronization, and long-duration stress remain pending.

Retained evidence:

- [raw semaphore UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-semaphore-c9b2d3d1.txt),
  SHA-256 `70063960c910ae32fddbbab0ca2180a7bd0297d34638b518358a7e0dfb517b12`
- [semaphore capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-semaphore-c9b2d3d1.json),
  SHA-256 `ab1fc1d39d4ecc1fcb7eb95ded8b4a5f88396bc3c7d5b753676802f577c67ee5`

## 2026-09-03 OSAL message queue runtime result

The clean firmware commit `4ebf46dacd8c906455fb541ca70c25b692cc51d8` added a depth-2 OSAL
message queue carrying a monotonic `uint32_t` sequence from the fast task to the slow task. The
consumer checks every payload against its expected sequence and stops with `OSAL_QUEUE_MISMATCH`
before printing a receive marker on any gap, duplicate, or reorder.

`st-flash --reset write` programmed and verified 13336 bytes. A reset-synchronized six-second
WCH-Link capture retained 1078 bytes and showed 12 repeated ordered cycles of
`OSAL_TASK_FAST → OSAL_QUEUE_SEND → OSAL_SEM_TAKE → OSAL_QUEUE_RECV → OSAL_TASK_SLOW`, with no
queue mismatch or semaphore timeout marker. This grants task-context OSAL message-queue B1 for this
board/image. The sequence values are checked inside the firmware but are not separately printed in
the retained UART log; ISR-to-task behavior, resource exhaustion, and long-duration stress remain
pending.

Retained evidence:

- [raw message-queue UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-queue-4ebf46da.txt),
  SHA-256 `72496d9db75e6a615fb12884754e4dc457a89a6a4b1df9e7801a085e2c9bc193`
- [message-queue capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-queue-4ebf46da.json)

## 2026-09-03 OSAL event-flags runtime result

The clean firmware commit `48ca0509640fd9f4fbb745957ec588e25131e160` added an OSAL event-flags
object to the existing synchronized task pipeline. The fast task sets a data-ready bit after queueing
each payload; the slow task waits for that bit with wait-all/auto-clear semantics before receiving the
queue item. All application synchronization remains behind `xy_os_*` APIs.

`st-flash --reset write` programmed and verified 14144 bytes. A six-second capture from the
independent WCH-Link UART retained 1284 normalized bytes and showed 12 strictly ordered
`OSAL_TASK_FAST → OSAL_QUEUE_SEND → OSAL_EVENT_SET → OSAL_SEM_TAKE → OSAL_EVENT_WAIT →
OSAL_QUEUE_RECV → OSAL_TASK_SLOW` cycles. There were no event/queue mismatch or semaphore-timeout
markers. This grants bounded task-context event-flags B1 for this exact board/image; ISR-to-task,
timeout/resource exhaustion, shutdown/re-init, and long-duration stress remain pending.

Retained evidence:

- [raw event-flags UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-event-flags-48ca0509.txt),
  SHA-256 `b802709edaf1215d176abf64d7336555a50e98599f162eb6fc70826e5428bad2`
- [event-flags capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-event-flags-48ca0509.json)

### OSAL/FreeRTOS mutex runtime result

Firmware `33c3a665032f62efde0e410cf21a4fd74d04975d` protects a shared producer/consumer
sequence with an OSAL mutex while retaining the semaphore, message-queue, and event-flags chain.
`st-flash --reset write` programmed and verified 14448 bytes. A six-second WCH-Link capture retained
1881 bytes and contained 12 complete ordered cycles:
`OSAL_MUTEX_FAST → OSAL_TASK_FAST → OSAL_QUEUE_SEND → OSAL_EVENT_SET → OSAL_SEM_TAKE →
OSAL_EVENT_WAIT → OSAL_QUEUE_RECV → OSAL_MUTEX_SLOW → OSAL_TASK_SLOW`.
The mutex-protected value matched in all cycles; mutex timeout/mismatch and all prior primitive error
markers were absent. Producer-to-consumer mutex roundtrip was 9–10 ms. This grants task-context OSAL
mutex B1 for this exact board/image; ISR-to-task, resource exhaustion, and long-duration stress remain
pending.

Retained evidence:

- [raw mutex UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-mutex-33c3a665.txt),
  SHA-256 `327fa9f1a060204a1555c0fd6319c067749527d16db41ef61772b66896ede63a`
- [mutex capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-mutex-33c3a665.json),
  SHA-256 `42acc0c119ad50f8d613b977df020f4ffdf00547197d866ba8f61a75c0764486`

### OSAL/FreeRTOS ISR-to-task semaphore result

Firmware `8443f907a8ec912317a774f2129d03b7746ac7b0` adds the explicit
`xy_os_semaphore_release_from_isr()` boundary. The FreeRTOS backend uses
`xSemaphoreGiveFromISR()` and `portYIELD_FROM_ISR()`; the Pandora SysTick handler releases a dedicated
binary semaphore once per second, and a task blocks on the public OSAL acquire API.

The clean image linked at 14912 bytes and embedded the exact firmware commit. Its binary SHA-256 is
`b27254d848216e920c65ace6aa4a8bf1cf753a37275c187b05be780d7392a3ba`. `st-flash 1.8.0`
programmed and verified all 14912 bytes through the ST-LINK/V2.1. A seven-second capture from the
independent WCH-Link UART retained 2025 bytes and contained 6 `OSAL_ISR_TAKE` markers, 13 producer
cycles, and no `OSAL_ISR_TIMEOUT` marker. This grants bounded SysTick ISR-to-task semaphore B1 for
this exact board/image. It does not establish arbitrary peripheral IRQ, other ISR-safe primitives,
resource exhaustion, shutdown/re-init, long-duration stress, or STM32U5 runtime evidence.

Retained evidence:

- [raw ISR semaphore UART log](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-isr-8443f907.txt),
  SHA-256 `90555a79311f360e1c72ebd920d4d533caec940cd4b8f5b1fb8aa8c5ac470fdc`
- [ISR semaphore capture metadata](evidence/pandora-stm32l475/2026-09-03/uart-wchlink-osal-isr-8443f907.json)

### OSAL resource recovery candidate — not board evidence

Commit `9c2d4d4e811a8b835f6432290bd12b6fd5000dcd` adds a one-shot OSAL task that exhausts
a two-block memory pool and a depth-one message queue without waiting, frees/receives one item,
verifies both objects accept work again, then deletes and recreates both objects. The focused source
guard, full 206-test Host suite, PC/STM32U5/STM32L4 root builds and Pandora FreeRTOS link pass. The
linked ELF has text/data/bss `16056/16/19768`; the BIN SHA-256 is
`731fe90a3a7cfe32e53b5f0d94517cea85999fd244dae02f8ba70a5d3387cb21`.

During this verification WCH-Link UART remained visible as `/dev/ttyACM1`, but `st-info --probe`
found zero ST-Link programmers. Therefore the candidate was not programmed and no UART runtime log
was captured. `OSAL_RESOURCE_EXHAUSTED`, `OSAL_RESOURCE_RECOVERED`, and
`OSAL_LIFECYCLE_REINIT` remain required future markers, not B1 observations.

## Remaining B2 work

1. Force an AHT10 NACK followed by reconnection, ACK, and a plausible measurement in one retained
   capture.
2. Grant B2 only after reviewing the real device path and ordered negative/recovery bytes.

Use the bounded capture helper after the board or independent USB-TTL adapter appears:

```text
python3 boards/pandora_stm32l475/capture_uart.py \
  --device /dev/ttyACM0 \
  --timeout 6 \
  --firmware-commit <exact-flashed-commit> \
  --output build/pandora-runtime/uart.log \
  --metadata build/pandora-runtime/capture.json
```

The helper exits nonzero and records `NO_DATA_TIMEOUT`, `DEVICE_OPEN_FAILED`, `CAPTURE_IO_FAILED`, or
`CAPTURE_CONTENT_MISMATCH` rather than creating false runtime evidence. Captured bytes only receive
`CAPTURED` and `B1_REVIEW_CANDIDATE` when they contain the exact firmware banner
`PANDORA STM32L475VE XINYI SMOKE OK`, followed by the exact matching `FIRMWARE_COMMIT <sha>`
identity marker, the exact sensor acknowledgement `AHT10 0x38 ACK`, and an
AHT10 measurement within the sensor's plausible output range (0–100000 milli-percent RH and
-50000–150000 milli-degrees C); banner-only, missing-ACK, misplaced/mismatched identity, out-of-range,
and unrelated bootloader/noise bytes are retained but rejected. These markers must occur in firmware
order: banner, matching commit, ACK, then plausible measurement; reordered retained bytes are also
rejected. The candidate classification still
requires human review of the real device path, flashed commit, raw log and metadata before this record
may grant B1. A serial
EOF/disconnect is classified immediately as an I/O failure instead of being mislabeled as a no-data
timeout. Its PTY Host contract enforces banner-plus-ACK-plus-measurement eligibility, content mismatch,
bounded no-data timeout, disconnect, missing-device refusal, firmware commit binding, and metadata
generation. A successful PTY test is not board evidence; only retained bytes from the real
board/adapter may support B1/B2.

If the same retained capture also contains the firmware banner followed in order by `AHT10 0x38 NACK`,
a later ACK and a plausible measurement, the helper records `B2_REVIEW_CANDIDATE`. A NACK before the
firmware banner or after an earlier successful measurement remains only a B1 candidate. This only
identifies a recovery-log candidate;
human review must still confirm the real device path, ordering, flashed commit and raw bytes before the
record can grant B2.
