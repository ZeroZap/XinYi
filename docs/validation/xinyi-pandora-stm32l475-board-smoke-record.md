# Pandora STM32L475VE Board Smoke Record

## 2026-09-05 W25Q128 MCU software-reset persistence result

Clean committed firmware `3b6fa2176ad007027941ba23053d071ceb9b29a7` erased the dedicated
W25Q128 test sector at `0x00FFF000`, programmed and verified the 256-byte pattern, recorded a
one-shot recovery marker in an RTC backup register, and called `NVIC_SystemReset()`. On the second
boot it reinitialized QSPI and read the same pattern before any erase/program operation, then emitted
`PANDORA_W25Q128_MCU_RESET_RECOVERED`.

The 28904-byte BIN SHA-256 was `e021f3953135f5242ac3789d78510103c2d30b89a1464f7c335a8465344ae022`.
ST-Link reported write verification, and an independent 28904-byte read-back was byte-identical. A
50-second WCH-Link capture retained 26170 bytes with two matching firmware identities and the strict
`MCU_RESET_STAGED → second boot → MCU_RESET_RECOVERED` chain. Machine validation reported 94
complete cross-component cycles, 46 SysTick ISR wakes, and zero known runtime error markers; capture
SHA-256 was `fc37f6a3194af12747602cc73ac2b35e239e577e4452921b6ce7016de105c64a`.

This grants B2 only for W25Q128 persistence across one MCU software reset. It is not power-loss,
external NRST, four-line QSPI, endurance, performance, or FOTA candidate-storage evidence.

## 2026-09-04 TIM6 peripheral IRQ timeout/restart recovery result

The clean committed firmware `f78f441712322447f81582e95185bbc6a2efd5a8` was built with Arm GNU
15.2.1 and retained SVC, PendSV, SysTick, and TIM6 handler symbols. Its 19652-byte BIN SHA-256 was
`287b48dbda98bb7e364c32f895198e941826d05f8b48b0eefa6a873a86aa5d1b`. ST-Link programmed and
verified the image; a same-length read-back was byte-identical. The runtime deliberately stopped
TIM6 interrupt generation, observed the bounded 900-tick OSAL semaphore timeout, restarted TIM6,
and then observed a fresh IRQ-to-task wake.

The independent WCH-Link 10-second capture retained 5415 bytes and matched the exact firmware
identity. Machine validation found one ordered `OSAL_TIM6_IRQ_TIMEOUT_EXPECTED →
OSAL_TIM6_IRQ_RECOVERED` transition, 20 complete task-pipeline cycles, nine SysTick ISR wakes,
11 TIM6 IRQ wakes, and zero runtime error markers. This grants B2 only for controlled TIM6 update
interrupt disable/timeout/re-enable recovery. It is not physical fault injection, arbitrary IRQ
recovery, IRQ timing performance, SPI/DMA, or complete HAL/RTOS qualification.

Retained evidence:

- [raw UART](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-tim6-recovery-f78f4417.txt)
- [machine analysis](evidence/pandora-stm32l475/2026-09-04/rtos-tim6-recovery-f78f4417.json)

## 2026-09-04 TIM6 peripheral IRQ-to-task result

The clean committed firmware `c9509e81a4338229b48ddd8ad98e16cf9fa596ee` configured TIM6 for a
700 ms update interrupt at NVIC priority 5. `TIM6_DAC_IRQHandler` dispatches through the STM32 HAL
callback, which uses `xy_os_semaphore_release_from_isr()` to wake an OSAL-created task. The 19372-byte
image was programmed and verified by ST-Link. An independent WCH-Link 8-second capture retained 4337
bytes and the exact firmware identity: 16 complete task pipeline cycles, 6 SysTick ISR wakes, 10 TIM6
IRQ wakes, and zero runtime error markers. Capture SHA-256 was
`105c37b0189813dda090852ef1798da8fc4ae8320d37ff6501c5c8b32847bf02`.

This grants B1 only for the TIM6 update IRQ → ISR-safe OSAL semaphore → task path. It does not prove
arbitrary peripheral IRQ support, negative/recovery behavior, IRQ timing performance, SPI/DMA, or
complete HAL/RTOS qualification.

**Date**: 2026-09-03
**Source commit**: `9b50ec38c4e32f9e37c93a0f3f70379453c9622f`
**Board**: Pandora STM32L475VE
**Status**: `B1_BOARD_SMOKE_VERIFIED`; `B2_PENDING`
**Evidence classification**: `PROGRAMMING_VERIFIED`; `BOARD_RUNTIME_B1`

This record separates verified programming, observed normal-path board runtime, and still-pending
negative/recovery evidence. B1 does not imply AHT10 NACK/recovery verification.

## 2026-09-04 SYS strong-backend reset and identity result

The clean firmware commit `28f4b21dc06e2189145dadc67e78340f9a22be90` linked the Pandora-owned
`xy_sys_init()`, reset-reason, chip-ID, and software-reset implementations. The 7824-byte BIN had
SHA-256 `fff3594813642400df343c765a8e1d84b2fc491d40af7519efb342cb19721b38` and was programmed with
ST-Link write verification. A separate 7824-byte Flash read-back was byte-identical.

An eight-second WCH-Link capture retained the transition from the previously running RTOS image to
the SYS image. The first SYS boot recorded RCC CSR `0x0C000600`, stable 96-bit UID
`001B002E3647501320313556`, and `SYS_SOFTWARE_RESET_REQUEST`. The board then reset through
`NVIC_SystemReset()` without human input. The next boot recorded RCC CSR `0x14000600`, the same UID,
`SYS_RESET_KIND SOFTWARE`, and `SYS_SOFTWARE_RESET_OK`, followed by two matching firmware/AHT10
normal-path cycles. This grants B1 for Pandora chip identity and B2 for the automated software-reset
reason/recovery path only. Power-on versus NRST is not disambiguated by this run; watchdog and
physical-reset causes remain pending.

Retained evidence:

- [raw SYS UART log](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-sys-reset-28f4b21d.txt),
  SHA-256 `15bf255077d82b0aeec1496a9d0e38e11ff240ceb7131f8173857cc3b9b6ae9a`
- [SYS capture metadata](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-sys-reset-28f4b21d.json),
  SHA-256 `a0a1edf0e0f529c6121748f1b752be4029c4addf695e88e4fa86200b791f2099`

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

### 2026-09-04 OSAL resource recovery and lifecycle runtime result

Commit `9c2d4d4e811a8b835f6432290bd12b6fd5000dcd` adds a one-shot OSAL task that exhausts
a two-block memory pool and a depth-one message queue without waiting, frees/receives one item,
verifies both objects accept work again, then deletes and recreates both objects. The focused source
guard, full 206-test Host suite, PC/STM32U5/STM32L4 root builds and Pandora FreeRTOS link pass. The
linked ELF has text/data/bss `16056/16/19768`; the BIN SHA-256 is
`731fe90a3a7cfe32e53b5f0d94517cea85999fd244dae02f8ba70a5d3387cb21`.

That first verification found zero ST-Link programmers, so it did not produce board evidence. On
2026-09-04 the same board/probe returned. Clean HEAD
`e6cd0906f0937c36d566eb88439b510b545d8250` rebuilt to text/data/bss `16056/16/19768`; its ELF and
BIN SHA-256 values are `f30104ef1d9d92fc7d97fb4f6816301e767748216062623700f5a12cc34186ae`
and `62eb007d21d12676af98abdcb38db9613be3e21023cc8004cf09afcf4bc20b1f`.

`st-flash 1.8.0` programmed and verified 16080 bytes, then a separate 16080-byte read-back matched
the BIN byte-for-byte. An eight-second reset-synchronized independent WCH-Link capture retained 2651
bytes with the exact firmware identity followed by one ordered
`OSAL_RESOURCE_EXHAUSTED → OSAL_RESOURCE_RECOVERED → OSAL_LIFECYCLE_REINIT` chain. It also retained
16 complete pre-existing task pipeline cycles and seven ISR wake markers, with zero resource,
semaphore, queue, event, mutex, or ISR error marker. This grants bounded B1 for no-wait memory-pool
and queue exhaustion/recovery plus delete/recreate on this exact board/image. It does not establish
blocking timeout, long-duration stress, performance, arbitrary peripheral IRQ, STM32U5 runtime, or
complete RTOS qualification.

Retained evidence:

- [raw resource/lifecycle UART log](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-osal-resource-e6cd0906.txt),
  SHA-256 `3808c1623b409665ac6d6c89171e4294c66a7c2b52cbb8ac80ec95d66b327c37`
- [resource/lifecycle metadata](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-osal-resource-e6cd0906.json)

### 2026-09-04 OSAL blocking-timeout runtime result

The first blocking-timeout image exposed a real adapter bug: FreeRTOS queue/semaphore/mutex wait
failures all used a generic mapping and returned `XY_OS_ERROR` after the requested wait expired.
Commit `34bb39f4f6f944c608bdf0381aa2fc6a878fe1a7` adds a dedicated wait-result mapping to
`XY_OS_ERROR_TIMEOUT` and preserves generic error mapping for non-wait operations.

The clean image embedded that exact commit, linked at text/data/bss `16192/16/19768`, and produced a
16216-byte BIN with SHA-256 `2c80d81840570461ef54096a1c49b857b18fb2fd5d4b3697100038398836aae3`.
`st-flash 1.8.0` programmed and verified all bytes; a separate 16216-byte read-back was byte-identical.
An eight-second WCH-Link capture retained 2677 bytes and one ordered
`OSAL_RESOURCE_EXHAUSTED → OSAL_BLOCKING_TIMEOUT_OK → OSAL_RESOURCE_RECOVERED →
OSAL_LIFECYCLE_REINIT` chain. The timeout probe keeps the queue full, requests 100 ticks, verifies
the OSAL status, and accepts only an elapsed 100–120 tick window before printing success. The same
capture retained 16 complete task pipeline cycles and seven ISR wake markers with no timeout,
resource, queue, event, mutex, semaphore, or ISR error marker.

This grants B1 only for the bounded queue blocking-timeout/error-mapping path. The UART marker proves
the firmware's tick-window assertion passed; it is not an external timing/performance measurement and
does not establish long-duration stress, arbitrary peripheral IRQ, cross-component concurrency, or
STM32U5 runtime.

Retained evidence:

- [raw blocking-timeout UART log](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-osal-timeout-34bb39f4.txt),
  SHA-256 `01a5f25a77d393bea4fe633275977ac170485bd551318f1180a20c26e0845979`
- [blocking-timeout metadata](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-osal-timeout-34bb39f4.json)

### 2026-09-04 OSAL bounded stress result

Firmware `50fad12a4da72d8ec6ca83f2ac36cbc44666c967` adds an explicit `OSAL_STRESS_READY`
marker and a fail-closed Host-tested validator for identity, ordered task cycles, ISR wake counts,
one-shot resource/timeout/lifecycle markers, and all known runtime error markers. The image linked at
text/data/bss `16228/16/19768`, produced a 16252-byte BIN with SHA-256
`6474ba7a8c2efa36367fc29cda0f76635fd1ce6d0879c40773ac36716fd2616d`, and was programmed,
verified, and read back byte-for-byte through ST-Link.

A reset-synchronized 120-second WCH-Link capture retained 36677 bytes. Against declared thresholds
of at least 200 complete ordered task-pipeline cycles, 100 SysTick ISR-to-task wakeups, and zero
runtime error markers, the validator reported `STRESS_REVIEW_CANDIDATE` with 234 cycles, 116 wakeups,
and no errors. The resource exhaustion, bounded queue timeout, recovery, and lifecycle re-init markers
each appeared once in order. Review grants bounded stress B1 for this exact board/image/interval.
It does not establish multi-hour endurance, performance, arbitrary peripheral IRQ, cross-component
IPC/Trace/Device/PM concurrency, STM32U5 runtime, or complete RTOS qualification.

Retained evidence:

- [raw bounded-stress UART log](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-osal-stress-50fad12a.txt),
  SHA-256 `8eb267be19e9948fec672bcd3068502a2d0ce4c1e4a8aca004ad95f2348413cd`
- [bounded-stress metadata](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-osal-stress-50fad12a.json)

## 2026-09-04 Device-helper AHT10 vertical-slice result

Commit `b94fc3c2161042603b7c02dd055f86caf21ed36b` moved the board-owned software-I2C
transport behind `xy_hal_i2c_master_transmit/receive()` and routed the AHT10 application path through
`xy_i2c_device_init/write/read()`. It also corrected the canonical I2C Device helpers to return
`XY_DEVICE_OK` on successful transfers rather than a positive byte count, matching their public
`xy_error_t` contract and existing drivers.

The clean image embedded the exact commit, linked at text/data/bss `8160/12/2708`, and produced an
8172-byte BIN with SHA-256 `4abeccb02f9bec6847ff87f72c01e9c0b2e45c345936781df47a9a28de9a93b7`.
ST-Link programmed and verified all bytes; a separate 8172-byte read-back was byte-identical. An
eight-second reset-synchronized WCH-Link capture retained 2355 bytes and 13 complete ordered
`banner → matching firmware identity → AHT10 ACK → plausible measurement` cycles. Humidity ranged
from 64596 to 64653 milli-percent and temperature from 31187 to 31202 milli-degrees C; no NACK marker
appeared. This grants B1 for the Pandora board-owned software-I2C → HAL API → Device helper → AHT10
vertical slice on this exact image. It does not establish hardware-I2C peripheral, NACK recovery,
timing performance, or another Device-model sensor driver.

Retained evidence:

- [raw Device/AHT10 UART log](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-device-aht10-b94fc3c2.txt),
  SHA-256 `3b00c2be1fe95dab4c4de21902ffc38c06d70fdc8f76599232d6bce9ccb0972c`
- [Device/AHT10 capture metadata](evidence/pandora-stm32l475/2026-09-04/uart-wchlink-device-aht10-b94fc3c2.json),
  SHA-256 `118446a3d8894781b609915ce231c73e9362142fb0f284b07010ba45a0971ebc`

## 2026-09-05 IPC Broker queue saturation/recovery result

Clean committed firmware `1e3acc3452b13d036310c59e144575118fa8d1e0` fills the bounded
Broker server queue to its configured depth of two and requires the third send to return
`XY_BROKER_QUEUE_FULL`. It then clears the queue and leaves the established producer/consumer,
Device lookup, Trace, PM tick, SysTick ISR, and TIM6 paths running.

The 19868-byte BIN (SHA-256 `6fcf312e963560f45299618d585a37770e3416c5bb39205745ba8c9245531512`)
was programmed with ST-Link write verification and read back byte-identically. A reset-synchronized
12-second WCH-Link capture retained 6257 bytes and matched the exact firmware identity. The validator
found one ordered `OSAL_IPC_SATURATED → OSAL_IPC_RECOVERED` chain, 23 complete ordered application
pipeline cycles, 12 SysTick ISR wakes, 13 TIM6 peripheral IRQ wakes, and no runtime error markers.
This grants bounded task-context IPC queue saturation/recovery B2 for this board/image. It does not
establish ISR ingress, multiple producers/consumers, throughput, performance, or long-duration stress.

Retained evidence:

- [raw IPC saturation UART log](evidence/pandora-stm32l475/2026-09-05/uart-wchlink-ipc-saturation-1e3acc34.txt),
  SHA-256 `f55af0509d927f0e6f3ccaa89cad4ea3e58e8198735dbbcd1886c1c94b7aa630`
- [IPC saturation metadata](evidence/pandora-stm32l475/2026-09-05/rtos-ipc-saturation-1e3acc34.json),
  SHA-256 `256e56c8f7168426a1096c2625b3ec096d8c3084dc48feba9fd60c83b62e7ea9`

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

## 2026-09-05 W25Q128 quad-output read result

Clean committed firmware `8f2981b2bca29af62fcda9200f9c235ff16804e0` added a `0x6B`
quad-output fast-read transaction: instruction and 24-bit address use one line, data uses four lines,
and the command supplies eight dummy cycles. The firmware reads the existing 256-byte pattern from
the dedicated final 4 KiB test sector and compares every byte before emitting the success marker.

The 29116-byte BIN had SHA-256
`323b473021eccf7f36465c245ff877a36dacac80d4ed93f81bc5e2f2f2004d84`. ST-Link reported write
verification success, and a same-length read-back was byte-identical. The ELF retained the exact
firmware identity and SVC/PendSV/SysTick/TIM6/DMA handler symbols.

A reset-synchronized 30-second independent WCH-Link capture retained 16312 bytes. It contained the
exact firmware identity, `PANDORA_W25Q128_QUAD_READ_OK` once, 58 complete ordered cross-component
cycles, 29 SysTick ISR task wakes, and no validator error marker. This grants B1 only for this bounded
quad-output data-read transaction. It does not prove quad page programming, throughput, endurance,
power-loss/NRST recovery, or FOTA candidate-image storage.

## 2026-09-06 single-ISR sustained ingress/backpressure recovery

Clean committed firmware `e33f8d284bcff46a327e04cd9e89c094ad07f140` lets the existing TIM6
producer fill a four-slot single-producer ISR ingress ring while the task consumer is deliberately
delayed. The task then drains 16 monotonic accepted payloads through a dedicated Broker server,
requires observed queue-full backpressure, and verifies that the normal task producer completed at
least one independent Broker delivery during the same bounded interval.

The 35052-byte BIN (SHA-256 `a5c864cc17014ca2cd637e6b3b7400b65383c7dce5125b0e9b624d55217271fa`)
was programmed with ST-Link verification. A reset-synchronized 30-second independent WCH-Link
capture retained 14201 bytes (SHA-256 `e2ae3a3b2086e42e5f5e06424c0fec79f2d21680167e2732cc9662f74055a512`).
The last-boot validator found one backpressure marker, exactly 16 ordered ISR stream deliveries, one
normal-task producer progress marker, one recovery marker, one sustained-completion marker, and no
ISR ingress error marker. This grants bounded B2 for single-TIM6-producer backpressure recovery and
non-starvation of the established normal task producer. It is not a performance/throughput result,
multi-ISR-producer evidence, or long-duration stress qualification.

Retained evidence:

- [raw sustained-ingress UART log](evidence/pandora-stm32l475/2026-09-06/uart-wchlink-ipc-isr-sustained-e33f8d28.txt)
- [machine validation metadata](evidence/pandora-stm32l475/2026-09-06/ipc-isr-sustained-e33f8d28.json)

## 2026-09-06 single-ISR repeated recovery stress

Clean committed stress-only firmware `be7cf9509a2d7400804606ce7f19be5a387f0829` disables the
unrelated one-shot FOTA reset while preserving the established TIM6 ISR ingress and cross-component
task pipeline. The 35020-byte BIN had SHA-256
`6aef27b0b1ddb54140fe0e98674b425135ad28acc51ff09c10129c5df949e1ef`; an exact-length ST-Link
read-back was byte-identical. The ELF contained the exact firmware identity and distinct
SVC/PendSV/SysTick wrapper and FreeRTOS port-handler symbols.

The first reset-synchronized 660-second capture completed only 11 of the required 12 recovery cycles,
so the exact-count validator rejected it and no stress evidence was granted. Without changing the
firmware, a second reset-synchronized capture ran for 720 seconds and retained 371021 bytes (SHA-256
`76bb3c83182b6a4b74b2caee310bd347db2c4009286fa047d66ef43147218994`). The dedicated fail-closed
validator matched the exact firmware identity, 12 backpressure markers, 192 strictly monotonic ISR
stream deliveries, 12 normal-task producer-progress markers, 12 recoveries, one sustained-completion
marker, 1387 complete ordered `IPC -> PM -> Device -> Trace -> IPC` cycles, and zero ISR ingress error
markers. This grants bounded B2 for a single TIM6 producer across 12 repeated recovery cycles and a
12-minute interval. It is not a throughput result, multi-ISR-producer evidence, or multi-hour
endurance qualification.

Retained evidence:

- [raw 12-cycle stress UART log](evidence/pandora-stm32l475/2026-09-06/uart-wchlink-ipc-isr-stress-be7cf950.txt)
- [machine validation metadata](evidence/pandora-stm32l475/2026-09-06/ipc-isr-stress-be7cf950.json)
