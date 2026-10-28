# XinYi 组件证据台账

### 2026-10-29 WCH HAL 闭环三件：时基可替换、延时不再掐死调度器、Timer 接管 NVIC

- **证据等级**：实板运行时（openCH CH32V307，WCH-Link）。三条都有 RED→GREEN 对照，
  不是"跑通一次"。
- **时基**：`xy_hal_tick_ms_internal()` 先查 `xy_hal_tick_registered_source`，没有才走
  SysTick 推导；FreeRTOS 后端在 `xy_os_kernel_start()` 装 `xTaskGetTickCount() *
  portTICK_PERIOD_MS`。判据用物理量锚定：256 字节 @115200 8N1 = 22.2 ms 导线时间，
  实板 `xy_probe_uart_burst_ms = 0x16 = 22 ms`；移除绑定后同一镜像读 **0 ms**、
  `stage` 停在 11 之前。证明的是"与导线一致"，**不是**示波器级绝对校准。
- **延时**：默认 `xy_hal_delay_us/ms` 会改写 `SysTick->CMP`/`CTLR` 并在结束时 `CTLR &= ~1U`
  关闭计数器，RTOS 下等于停掉调度 tick；守卫此前只禁止调用、未记录原因。内核改装的慢源
  在调度器运行时 `vTaskDelay`，未启动时有界自旋（`configCPU_CLOCK_HZ` 标定，不碰 SysTick）。
  实板：绑定前 `HAL_DELAY_ERROR_A`、**tick 停止推进**、镜像卡在第 5 步（后续
  `xy_os_msgqueue_get` 永不超时）；绑定后 6 步、`failed=0`、走到 `TIMEOUT_OK`。
- **Timer/NVIC**：`xy_hal_timer_enable_irq()` 此前只开外设中断，**从不碰 NVIC**，这正是
  `sdk`/`isr_stress` 必须写厂商 `TIM2+NVIC` 的真实原因（此前的例外理由不完整）。现按
  `xy_hal_exti.c` 约定由 wrapper 接管 NVIC（优先级 1/0）。迁移后守卫 `TIMER_BOUNDARY`
  清空，12 个镜像零厂商调用；实板 3 轮行为与迁移前逐项一致。
- **未覆盖 / 边界**：STM32 侧时基只到链接级（`HAL_GetTick` 由 `xy_hal_time.c.obj` 提供、
  ST 弱定义被丢弃），**无运行时证据**；U5 无 `xy_hal_sys_get_tick_count()`；I2S/DMA
  仍缺失；SPI/I2S/DMA 无实板取证；自旋标定 `/8` 为经验值未验证；NVIC 优先级在 HAL 内固定
  为 1/0，无 API 可调。


### 2026-10-28 Pandora AHT10 canonical owner + board runtime (B1)

- U7 AHT10 was the last onboard sensor without a canonical Device owner. `main.c` drove it inline: an
  `xy_i2c_device_t` built by hand, the `0xE1 0x08 0x00` calibration command, `0xAC 0x33 0x00` per
  measurement and a private re-implementation of the 20-bit humidity/temperature decode — code
  outside `xy_aht10` and outside every host test. New board module
  `boards/pandora_stm32l475/pandora_aht10_owner.{h,c}` makes `xy_aht10` the owned instance with the
  same commit-on-success contract as the AP3216C/ICM20608 owners (`init` commits a candidate only on
  success; `sample` commits only after a complete converted frame; access before init is refused
  with `XY_DEVICE_INVALID_PARAM` and zero bus traffic).
- New validation image `sensor_aht10_main.c` (`pandora_stm32l475_aht10`) owns U7 through that module
  and carries no AHT10 protocol code: soft I2C4 (`SCL=PD6 SDA=PC1`) address scan, driver init, then
  bounded `AHT10 SAMPLE hum_centi=… temp_centi=…` lines with an AHT10 public-range check
  (0–10000 centi-%RH, −4000–8500 centi-C). The board policy guard now pins the image to
  `xy_aht10_*` and rejects `0xE1U`/`0xACU`/`xy_i2c_device_*`/the raw conversion constants in it.
- Host: `pandora_sensor_owners` grows an AHT10 command model on the same `xy_hal_i2c_*` seam,
  15 → 23 cases (single calibration write plus the 10 ms settle, `0xAC 0x33 0x00` then one bare
  6-byte frame decoding to 5000 centi-%RH / 2500 centi-C, busy-status and bus-fault output
  preservation, owner kept on a rejected re-init, fail-closed `deinit`).
- New capture judge `boards/pandora_stm32l475/validate_aht10_capture.py` (`capture`/`analyze`) and
  CTest `pandora_aht10_capture_validator` (9 cases, incl. a real pty window and a zero-byte
  `NO_DATA_TIMEOUT` reject), CTest suite 298 → 299. `capture` opens and flushes the UART *before*
  running the reset command so the boot header cannot be lost to a race — `capture_uart.py` is
  welded to the smoke image's banner and could not judge this image.
- Board runtime (accepted run): image 9120 B SHA-256 `bf6f70cc…f9b84f`, embedded commit
  `8deb9173…`, snapshot-gated write to `0x08000000` (`span_identical=true erased_tail=1120
  unexpected_changes=0`), UART opened before `st-flash reset`. Capture 1629 B → verdict
  **`AHT10_SOFT_I2C4_B1`**: `READY`→`FIRMWARE_COMMIT`→`AHT10_BUS=SOFT_I2C4`→`PANDORA_I2C4_INIT_DONE`
  →`I2C4_ACK=0x38`→`SCAN_DONE count=1`→`AHT10_DRIVER_READY`, then 31 samples (21 unique, no
  malformed lines, no error markers): humidity 54.58–54.65 %RH, temperature 30.53–30.68 °C. Post-run
  `verify` showed **only** the bootloader region changed (application/journal/board_reserved/RTC
  identical); `restore` + `verify_ok regions=5` returned the board byte-exactly to the pre-run state
  (`762c4a75…` bootloader, `d35e2e9a…` application).
- Real defect found by the failed first attempt: writing this image to `0x08008000` is not durable on
  a board whose resident bootloader holds a valid W25Q128 candidate — the reset that `st-flash write`
  performs lets the bootloader install the candidate over it (`application` → `d27a6a26…`, the
  capture contained only `ICM20608_*` lines from commit `2a596d90…`). These validation images link at
  `0x08000000` (`STM32L475VETX_FLASH.ld`); `0x08008000` belongs to `STM32L475VETX_FOTA_APP.ld`. The
  validator failed closed in both attempts; the reproduction is retained as
  `attempt2-app-slot.log`.
- Evidence path: `boards/pandora_stm32l475/pandora_aht10_owner.{h,c}`,
  `sensor_aht10_main.c`, `validate_aht10_capture.py`, board `CMakeLists.txt`,
  `tests/unit/hal_component/test_pandora_sensor_owners.c`,
  `tests/unit/sensor/test_pandora_aht10_capture.py`, `tests/unit/CMakeLists.txt`,
  `tests/unit/hal_component/check_pandora_stm32l475_board.py`, `docs/design/unit-test-inventory.md`,
  `docs/validation/evidence/pandora-stm32l475/2026-10-28-aht10-owner/`; Host `make test-unit`
  299/299; PC, STM32L4 (incl. the new board target) and STM32U5 builds; `git diff --check`.
- Boundaries: one board, one 6 s window, one image. No controlled humidity/temperature stimulus, no
  response time, accuracy, condensation, long-run or thermal claim; the ranges above are what the
  part reported in a still room. Soft-I2C timing margins, external pull-ups and bus recovery are not
  covered. `main.c` still carries its inline AHT10 path for the smoke image (separate decision).

### 2026-10-28 Pandora board-owned canonical sensor owners (host)

- The two Pandora sensor validation images stopped owning their devices through the legacy
  `sensor_*` compatibility wrappers. `sensor_ap3216c_main.c` no longer calls
  `ap3216c_create_light/proximity/ir` (which `SENSOR_MALLOC`a three `sensor_device_t` objects and
  re-maps results to `sensor_err_t`), and `sensor_icm20608_main.c` no longer calls
  `icm20608_create_accel` nor reaches back into `((icm20608_priv_t *)accel->priv_data)->device` for
  its sample. Both images now own one canonical Device driver instance through the new board module
  `boards/pandora_stm32l475/pandora_ap3216c_owner.{h,c}` /
  `pandora_icm20608_owner.{h,c}`, and the board `CMakeLists.txt` no longer compiles
  `components/sensor/sensors/sensor_{ap3216c,icm20608}.c` into either image.
- Owner contract: `init` builds a candidate instance and commits only on success (a failed or
  re-init leaves the caller's owner byte-identical, not zeroed); `sample` commits only after every
  channel returns OK (a partial fault preserves the caller's previous sample); `read_reg` /
  `read_raw` are refused with `XY_DEVICE_INVALID_PARAM` before a successful init, with no bus
  traffic.
- New CTest `pandora_sensor_owners` (15 cases) seats two host-side I2C register files on the
  canonical `xy_hal_i2c_*` seam so one fixture pins both owners through the unmodified
  `xy_ap3216c` / `xy_icm20608` drivers. AP3216C: reset-then-`ALS_PS` init with both 50 ms settles and
  the 7-bit `0x1E`/100 ms transfer contract, the 6-byte `IR_DATA_L` block, atomic three-channel
  sampling (ALS `*350` millilux, 12-bit proximity mask, 10-bit IR mask) mirroring the driver cache,
  and output-preserving fail-closed paths on the `0x40` out-of-range flag or a NACKing bus.
  ICM20608: identity read strictly before any write (foreign `WHO_AM_I` -> `XY_DEVICE_NOT_FOUND`
  with zero writes and zero delay), reset/100 ms settle plus the seven documented
  write-and-read-back registers landing `0x01/0x00/0x08/0x08/0x09/0x04/0x04`, the 14-byte
  `ACCEL_XOUT_H` burst decoded at the committed 4 g / 500 dps ranges (500 / -500 / 125 mg,
  3906 / -3906 / 0 mdps, 3126 centi-degC), and output preservation when the bus dies mid-frame.
- Reverse proof: four mutations each turn the suite red — non-atomic AP3216C sample commit (2
  failures), ICM20608 owner committing its candidate before the init result (1), `ADDR_DEFAULT` ->
  `ADDR_ALT` in the ICM owner (5), AP3216C raw block read starting at `ALS_DATA_L` (1). Mutation
  sources were restored from byte-identical backups and the suite re-verified green.
- On-image proof (real cross build): `make HAL_PLATFORM=STM32L4 STM32L4_BOARD=pandora_stm32l475`
  builds both images; `arm-none-eabi-nm` shows
  `pandora_ap3216c_owner_{init,read_reg,read_raw,sample}` + `xy_ap3216c_init` in the AP3216C image
  and `pandora_icm20608_owner_{init,read_reg,read_raw,sample}` + `xy_icm20608_init_i2c` in the
  ICM20608 image, with **zero** `sensor_*` symbols and no `ap3216c_create_*` /
  `icm20608_create_*` / `icm20608_priv` symbol in either ELF (`.text` 11316 B and 10380 B).
- Evidence path: `boards/pandora_stm32l475/pandora_{ap3216c,icm20608}_owner.{h,c}`,
  `boards/pandora_stm32l475/sensor_{ap3216c,icm20608}_main.c`,
  `boards/pandora_stm32l475/CMakeLists.txt`,
  `tests/unit/hal_component/test_pandora_sensor_owners.c`, `tests/unit/CMakeLists.txt`,
  `tests/unit/hal_component/check_pandora_stm32l475_board.py`,
  `docs/design/unit-test-inventory.md`; focused CTest `pandora_sensor_owners` 15/15; Host
  `make test-unit` 291/291; PC, STM32L4 (board images), STM32U5 builds; `git diff --check`.
- Current status: source/Host/compile `DONE`; nothing was flashed. Does not upgrade I2C timing,
  clock stretching, bus recovery, real AP3216C/ICM20608 communication, board-runtime behaviour or
  product qualification. The UART marker streams of both images are unchanged, so the existing
  capture validators still apply; the on-board owner change itself is still unverified at runtime.

### 2026-10-28 Pandora hardware I2C3 -> Device -> driver vertical slice (host)

- New CTest `pandora_hw_i2c_stack` (13 cases) closes the same seam as `pandora_device_stack` for the
  other transport style on the reference board: the handle-based hardware I2C3 owner
  (`boards/pandora_stm32l475/pandora_hw_i2c.c`, vendor `Instance`/AF4/RCC path) drives the real
  `xy_i2c_device_*` bus helpers into the canonical `xy_ap3216c` Device driver, against a host-side
  AP3216C register model sitting on the canonical `xy_hal_i2c_*` seam.
- Covered: the handle returned to the driver is the board-owned I2C3 handle with `Instance` assigned
  before controller init (timing `0x10909CEC`, 100 kHz, 7-bit, duty 2) and the driver never
  reconfigures the controller; every transfer carries the canonical 7-bit address `0x1E` and the
  driver's 100 ms timeout; init writes `SYSTEM_CONFIG=0x04` (reset) then the requested mode, each
  followed by a 50 ms settle; ALS decode (`raw * 350` millilux), proximity 12-bit field
  (`(H & 0x3F) << 4 | (L & 0x0F)`) with the low-byte `0x40` out-of-range flag mapped to
  `XY_DEVICE_IO_ERROR` while cache/output stay untouched, IR 10-bit field (`(H & 0x03) << 8 | L`);
  register-select-then-read framing per quantity with disjoint register pairs; address `0` accepted
  as the board default; failed re-init preserves a live owner and its cache; reads and deinit fail
  closed with no extra bus traffic when the device stops ACKing; deinit writes power-down and clears
  the owner, and a failed deinit keeps the owner live.
- Reverse proof: five mutations each turn the suite red — dropping `i2c3.Instance = I2C3` (1 failure),
  ALS scale 350 -> 349 (2), proximity high-byte mask `0x3F` -> `0x7F` (1), removing the reset settle
  delay (1), and pre-shifting `dev_addr` in the bus helper (6). All mutation sources were restored
  with `git checkout --`.
- Evidence path: `tests/unit/hal_component/test_pandora_hw_i2c_stack.c`,
  `tests/unit/CMakeLists.txt`, `docs/design/unit-test-inventory.md`; focused CTest
  `pandora_hw_i2c_stack` 13/13; Host `make test-unit` 290/290; PC, STM32L4, STM32U5 builds;
  `clang-format --dry-run --Werror`; `git diff --check`.
- Current status: source/Host/compile `DONE`; no production code changed, nothing was flashed. Does
  not upgrade I2C timing, clock stretching, bus recovery, real AP3216C communication, board-runtime
  behaviour or product qualification. The Pandora `sensor_ap3216c` image still ran the legacy
  `sensor_*` framework at the time of this slice; that on-board ownership gap was closed by the
  newer 2026-10-28 board-owned canonical sensor owner slice above (source/Host/compile only).

### 2026-10-28 Pandora I2C -> Device -> driver vertical slice (host)

- New CTest `pandora_device_stack` (14 cases) closes the seam between three layers that each had
  only isolated coverage: the board-owned software-I2C transport
  (`boards/pandora_stm32l475/pandora_soft_i2c.c`), the canonical `xy_i2c_device_*` bus helpers and
  the canonical `xy_aht10` / `xy_icm20608` Device drivers. A host-only I2C slave model decodes the
  bit-banged waveform, ACKs/NACKs per 7-bit address, keeps a register file for the
  register-addressed device and a command/frame model for the command-addressed device, and logs
  every committed frame.
- Covered: transport owner/pin/clock wiring, AHT10 init command and 80 ms measure/read frame with
  fixed-point decode plus busy (cache-preserving) and address-NACK fail-closed paths, ICM20608
  WHO_AM_I identity binding (0xAE accepted, foreign identity rejected before any configuration
  write), the documented eight-write configuration sequence followed by the seven-register
  read-back verify order, read-back disagreement -> `XY_DEVICE_FAIL`, absent alternate address ->
  `XY_DEVICE_IO_ERROR` without touching the live device, 14-byte sample block decoding
  (4 g / 500 dps / 3500 centi-degC) with cache preservation on transport fault,
  `xy_icm20608_verify_configuration()` re-reads through the transport, and two devices sharing one
  board bus by address with no crosstalk.
- Reverse proof: four mutations each turn the suite red — `dev_addr << 1` removed from the
  transport write phase (12 failures), NACK swallowed in `master_transmit` (3 failures), WHO_AM_I
  comparison removed from the driver (1 failure), AHT10 humidity shift `>> 20` -> `>> 19`
  (2 failures).
- Evidence path: `tests/unit/hal_component/test_pandora_device_stack.c`,
  `tests/unit/CMakeLists.txt`, `docs/design/unit-test-inventory.md`; focused CTest
  `pandora_device_stack` 14/14; Host `make test-unit` 289/289; PC, STM32L4, STM32U5 builds;
  `clang-format --dry-run --Werror`; `git diff --check`.
- Current status: source/Host/compile `DONE`; no production code changed, nothing was flashed. Does
  not upgrade bit-bang timing/rise time, clock stretching, bus recovery, real AHT10/ICM20608
  communication, board-runtime behaviour or product qualification. The Pandora `sensor_icm20608` /
  `sensor_ap3216c` images still ran the legacy `sensor_*` framework at the time of this slice, so
  the canonical drivers were host-proven but not yet the on-board owners; that gap was closed by
  the newer 2026-10-28 board-owned canonical sensor owner slice above.

### 2026-10-27 Pandora SYS reset-request retention across a reset

- Board-owned `xy_sys_reset(reset_by)` now records the reason in RTC backup slot 3 as
  `PANDORA_RESET_REQUEST_MAGIC | reason` (`0..255`, slots 0..2 stay owned by the W25Q128/FOTA
  flows) before issuing the software reset; `xy_sys_init()` decodes the record, latches it, and
  clears it one-shot; the new `xy_sys_reset_request()` accessor plus `XY_SYS_RESET_REQUEST_*`
  constants expose the contract and the weak default stays `XY_ERROR_NOT_SUPPORTED`.
- Evidence path: `boards/pandora_stm32l475/pandora_sys.c`, `boards/pandora_stm32l475/main.c`,
  `components/sys/xy_sys/xy_sys.{h,c}`, `tests/unit/sys/test_pandora_sys.c`,
  `tests/unit/sys/test_sys_core.c`, `boards/pandora_stm32l475/validate_sys_reset_request_capture.py`;
  focused CTest `pandora_sys` 12/12; Host `make test-unit` 280/280; PC, STM32L4, STM32U5 builds;
  Pandora board contract guard; `git diff --check`.
- Hardware: Pandora STM32L475VE, `st-flash 1.8.0` write/verify (chipid `0x415`, ST-Link V2.1
  `V2J24S11`), 14296-byte BIN SHA-256 `d96fee89…2025`, independent read-back byte-identical;
  CP2102 UART 115200 8N1 captured `SYS_RESET_REQUEST 0 → 1 → 0` for
  `POWER_ON/EXTERNAL_PIN → SOFTWARE → WATCHDOG`, and the validator rejects the pre-change
  `41a11c70` capture and a wrong commit binding. Evidence:
  `docs/validation/evidence/pandora-stm32l475/2026-10-27-sys-reset-request/`.
- Current status: source/Host/compile + bounded board HIL `DONE`; not power-loss/brownout or
  backup-domain-reset retention, not concurrent use of slots 0..2, and not product/release
  qualification.

### 2026-10-27 Pandora SYS deterministic MAC from unique ID

- Board-owned `xy_sys_get_mac_addr()` derives a deterministic unicast, locally-administered MAC
  from the 96-bit unique ID (low 6 bytes of uid[0..1]; MAC bit 0 clear + bit 1 set);
  `xy_sys_get_sw_ver`/`xy_sys_get_hw_ver` gained the weak attribute so boards can override them;
  Pandora smoke emits `SYS_MAC_ADDR`.
- Evidence path: `boards/pandora_stm32l475/pandora_sys.c`,
  `components/sys/xy_sys/xy_sys.c`, `boards/pandora_stm32l475/main.c`,
  `tests/unit/sys/test_pandora_sys.c`; focused CTest `pandora_sys` 5/5; Host `make test-unit`
  279/279; Pandora STM32L4, STM32U5, PC and WCH compile; `git diff --check`.
- Hardware: Pandora STM32L475VE, `st-flash 1.8.0` write/verify (chipid `0x415`, ST-Link V2.1),
  CP2102 UART 115200 8N1; `SYS_MAC_ADDR 2E001B001350` identical across EXTERNAL_PIN/SOFTWARE/
  WATCHDOG reset kinds; `SYS_CHIP_ID 001B002E3647501320313556`. Evidence:
  `docs/validation/evidence/pandora-stm32l475/2026-10-27-sys-mac-addr/`.
- Current status: source/Host/compile + bounded board HIL `DONE`; not networking/Ethernet
  qualification (L475 has no factory Ethernet MAC); reset-cause/watchdog electrical behavior
  beyond the observed CSR values is not claimed.

### 2026-10-20 FreeRTOS OSAL memory-pool-context atomic retirement

- `freertos_clear_mempool_context()` now finds and clears the bounded owner slot within one
  FreeRTOS critical section; concurrent creators cannot reuse a pool owner slot before retirement
  completes.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host unit suite; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade memory-pool runtime, concurrency, resource exhaustion, long-run stability,
  or product qualification.

### 2026-10-20 FreeRTOS OSAL message-queue-context atomic retirement

- `freertos_clear_msgqueue_context()` now finds and clears the bounded owner slot within one
  FreeRTOS critical section; concurrent creators cannot reuse queue metadata before retirement
  completes.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host unit suite; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade queue runtime, concurrency, throughput, resource exhaustion, long-run
  stability, or product qualification.

### 2026-10-20 FreeRTOS OSAL mutex-context atomic retirement

- `freertos_clear_mutex_context()` now finds and clears the bounded owner slot within one FreeRTOS
  critical section; concurrent creators cannot reuse the slot before retirement completes.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host unit suite; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade mutex runtime, competition/priority behavior, resource exhaustion,
  long-run stability, or product qualification.

### 2026-10-20 FreeRTOS OSAL event-flags-context atomic retirement

- `freertos_clear_event_flags_context()` now finds and fully clears the bounded owner slot within
  one FreeRTOS critical section; concurrent creators cannot reuse partially retired event-group/name
  metadata.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host unit suite; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade event-flags runtime, concurrency, ISR-to-task behavior, resource exhaustion,
  long-run stability, or product qualification.

### 2026-10-20 FreeRTOS OSAL semaphore-context atomic retirement

- `freertos_clear_semaphore_context()` now fully clears the bounded owner slot within one FreeRTOS
  critical section; concurrent creators cannot reuse partially retired semaphore/name metadata.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host 279/279; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  focused `opench_ch32v307_osal_freertos_semaphore_lifecycle_smoke` link; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade semaphore runtime, concurrency, ISR-to-task behavior, resource exhaustion,
  long-run stability, or product qualification.

### 2026-10-20 FreeRTOS OSAL timer-context atomic retirement

- `freertos_clear_timer_context()` now fully clears the bounded owner slot within one FreeRTOS
  critical section; concurrent creators cannot reuse partially retired timer/callback metadata.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host unit suite; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade timer runtime, concurrency, callback timing, resource exhaustion, long-run
  stability, or product qualification.

### 2026-10-20 FreeRTOS OSAL thread-context atomic retirement

- `freertos_clear_thread_context()` now finds and fully clears the bounded owner slot within one
  FreeRTOS critical section; concurrent creators cannot reuse a partially retired context.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host unit suite; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade scheduler, concurrency, resource exhaustion, long-run stability, or product
  qualification.

### 2026-10-20 FreeRTOS OSAL memory-pool-context atomic reservation

- `freertos_publish_mempool_context()` now reserves a bounded owner slot inside one FreeRTOS
  critical section; memory-pool creation releases backing memory and its wrapper when adapter
  publication cannot reserve a slot.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host `make test-unit` 279/279; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade memory-pool runtime, concurrency, resource exhaustion, long-run stability,
  or product qualification.

### 2026-10-20 FreeRTOS OSAL message-queue-context atomic reservation

- `freertos_publish_msgqueue_context()` now reserves a bounded owner slot inside one FreeRTOS
  critical section; message-queue creation deletes the native queue and frees its wrapper when
  publication cannot reserve an adapter-owned slot.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host `make test-unit` 279/279; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade queue runtime, concurrency, resource exhaustion, throughput, long-run
  stability, or product qualification.

### 2026-10-20 FreeRTOS OSAL event-flags-context atomic reservation

- `freertos_publish_event_flags_context()` now performs slot lookup, clearing, and publication
  inside one FreeRTOS critical section; concurrent event-flags creators cannot reserve the same
  adapter-owned slot. Native event-group creation still gets deleted when publication fails.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host `make test-unit` 279/279; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade event-flags runtime, concurrency, resource exhaustion, long-run stability,
  or product qualification.

### 2026-10-20 FreeRTOS OSAL timer-context atomic reservation

- `freertos_alloc_timer_context()` now finds, clears, and marks a bounded owner slot active
  within one FreeRTOS critical section; concurrent timer creators cannot reserve the same slot.
  Native timer creation failure still clears the reservation.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host unit suite; PC, STM32L4, STM32U5, and WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed,
  so this does not upgrade timer runtime, concurrency, callback timing, resource exhaustion,
  long-run stability, or product qualification.

### 2026-10-20 FreeRTOS OSAL mutex-context atomic reservation

- `freertos_publish_mutex_context()` now reserves bounded mutex owner slots inside one FreeRTOS
  critical section and reports whether publication succeeded. Mutex creation releases both the
  native mutex and wrapper when the adapter owner table is full.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host `make test-unit` 279/279; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade mutex runtime, competition/priority behavior, resource exhaustion,
  long-run stability, or product qualification.

**建立日期**：2026-08-17
**维护入口**：[Sprint 跟踪看板](../plans/SPRINT_TRACKER.md)
**审计基线**：[全组件状态审计与 Sprint 计划](../plans/2026-08-17-component-audit-sprint-plan.md)

> 本文件记录“我们有什么证据”，不记录愿景。状态只能由真实执行、审查或硬件记录升级。
>
> **当前平台边界（2026-09-04）**：Pandora STM32L475VE 是正式 reference board 与 Sprint 1–4
> 实板验收基线；STM32U5/M33/TrustZone 仅作后续 enhancement compile compatibility，不再阻塞
> 基础验收。SSD1306 deferred，不选择、不推进。无人值守证据仅允许自动构建、测试、ST-Link
> 测试、ST-Link 独立 UART 采集；人工接线、按键、目视确认和物理故障注入不得作为自动闭环步骤。

### 2026-10-20 FreeRTOS OSAL thread-context atomic reservation

- `freertos_alloc_thread_context()` now finds, clears, and marks a bounded owner slot active
  within one FreeRTOS critical section before `xTaskCreate()`; concurrent creators cannot reserve
  the same slot.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host `make test-unit` 278/278; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed,
  so this does not upgrade scheduler, concurrency, resource exhaustion, long-run stability, or
  product qualification.

### 2026-10-20 FreeRTOS OSAL semaphore-context atomic reservation

- `freertos_alloc_semaphore_context()` now performs slot lookup, clearing, and `active`
  reservation inside one FreeRTOS critical section; concurrent semaphore creators cannot reserve
  the same adapter-owned context. Native semaphore creation still happens before publication of
  the semaphore handle, and context exhaustion cleans up the native semaphore.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy/CTest
  `freertos_osal_thread_flags`; Host `make test-unit` 279/279; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed,
  so this does not upgrade semaphore runtime, concurrency, resource exhaustion, long-run
  stability, or product qualification.

### 2026-10-20 Pandora SYS host contract

- Added `pandora_sys` Unity/CTest coverage for the Pandora board-owned SYS override: reset-cause
  snapshot and HAL flag clear, reset parameter validation/HAL failure propagation, and unique-ID
  output plus failure preservation.
- Evidence path: `boards/pandora_stm32l475/pandora_sys.c`,
  `tests/unit/sys/test_pandora_sys.c`, and `tests/unit/CMakeLists.txt`; focused CTest
  `pandora_sys`; Host `make test-unit` 279/279; Pandora STM32L4 clean build; STM32U5 and WCH
  FreeRTOS compile; PC build; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed,
  so this does not upgrade reset-cause hardware evidence, watchdog/power-cycle evidence, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL memory-pool address-boundary guard

- `xy_os_mempool_free()` now performs pool range and stride checks in `uintptr_t` space, with an
  explicit address-addition overflow guard before entering the free-list critical section.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed,
  so this does not upgrade memory safety, concurrency, resource exhaustion, long-run stability,
  or product qualification.

### 2026-10-19 FreeRTOS OSAL stack high-water conversion boundary

- `xy_os_thread_get_stack_space()` now retains the native high-water mark in `UBaseType_t`,
  rejects word counts whose byte conversion cannot fit `uint32_t`, and only then publishes the
  OSAL byte-unit result. Foreign/stale handles and unrepresentable values return zero.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade stack telemetry runtime, scheduler, concurrency, long-run stability, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL mutex owned-delete boundary

- `xy_os_mutex_delete()` now rejects an owned mutex before native deletion; the openCH CH32V307 lifecycle image checks the negative path before release and successful deletion.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c`, `boards/opench_ch32v307/osal_freertos_mutex_lifecycle_main.c`, and `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy, Host, PC/STM32L4/STM32U5/WCH FreeRTOS compile, focused image link, and `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so this does not upgrade mutex runtime, concurrency, recovery, long-run stability, or product qualification.

### 2026-10-19 FreeRTOS OSAL queue receive output preservation

- `xy_os_msgqueue_get()` now writes the FIFO priority value `0` only after
  `xQueueReceive()` succeeds; timeout/error paths preserve the caller's priority output.
- The openCH CH32V307 queue lifecycle image now checks a `0xA5` priority sentinel across an
  empty-queue timeout and emits an explicit output-preservation marker.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c`,
  `boards/opench_ch32v307/osal_freertos_queue_lifecycle_main.c`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; focused openCH queue lifecycle image link; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade queue runtime, throughput, concurrency, long-run stability, or product
  qualification.

### 2026-10-19 FreeRTOS OSAL timer callback native-handle binding

- FreeRTOS timer callback now requires the adapter context recovered from
  `pvTimerGetTimerID()` to still identify the current native timer before invoking the OSAL
  callback, preventing stale/misbound context dispatch.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC, STM32L4, STM32U5, and
  WCH FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade timer runtime, callback timing, concurrency, long-run stability, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL memory-pool deletion/alloc-free race boundary

- Memory-pool delete now enters the critical section, marks an otherwise empty pool as
  adapter-owned `deleting`, and rejects alloc/free while deletion is in progress. This closes the
  alloc/free-vs-delete state transition before backing storage is released.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c`,
  `boards/opench_ch32v307/osal_freertos_mempool_lifecycle_main.c`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC, STM32L4, STM32U5, and WCH
  FreeRTOS compile; focused openCH memory-pool lifecycle image link; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade memory safety, concurrency, resource exhaustion, long-run stability, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL memory-pool block-size/stride separation

- FreeRTOS memory pools now retain the caller-requested block size as public metadata while using
  a separately checked, pointer-aligned internal stride for free-list storage. Non-pointer-aligned
  sizes no longer place free-list pointers at misaligned addresses, and alignment rounding overflow
  is rejected before allocation.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c`,
  `boards/opench_ch32v307/osal_freertos_mempool_lifecycle_main.c`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy and CTest; Host
  `make test-unit` 278/278; PC, STM32L4, STM32U5, and WCH FreeRTOS compile; focused openCH
  memory-pool lifecycle image link; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade memory safety, concurrency, resource exhaustion, long-run stability, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL memory-pool outstanding-allocation deletion boundary

- `xy_os_mempool_delete()` now returns `XY_OS_ERROR_RESOURCE` while any pool block is still
  allocated, before freeing caller-owned or adapter-owned backing storage. The openCH
  CH32V307 lifecycle image exercises this negative path before foreign/interior/double-free,
  reuse, and successful deletion.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c`,
  `boards/opench_ch32v307/osal_freertos_mempool_lifecycle_main.c`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; focused CTest `freertos_osal_thread_flags`; Host
  `make test-unit` 278/278; PC, STM32L4, STM32U5, and WCH FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed,
  so this does not upgrade memory safety, concurrency, resource exhaustion, long-run stability,
  or product qualification.

### 2026-10-19 FreeRTOS OSAL thread-lifecycle smoke inventory alignment

- The openCH CH32V307 thread-lifecycle image now expects the backend's explicit fail-closed
  thread inventory contract (`xy_os_thread_get_count() == 0`); it no longer claims one tracked
  thread while `xy_os_thread_enumerate()` intentionally publishes no kernel-global handles.
- Evidence path: `boards/opench_ch32v307/osal_freertos_thread_lifecycle_main.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC/STM32L4/STM32U5/WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed,
  so this does not upgrade thread enumeration, scheduler, concurrency, long-run stability, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL timer callback ABI contract

- The focused FreeRTOS OSAL policy now guards the existing timer callback boundary: the native
  FreeRTOS callback must recover the adapter-owned context through `pvTimerGetTimerID()`, invoke
  the OSAL callback with its original caller argument, and bind that context via `xTimerCreate()`.
  Direct callback-type casts are rejected by policy.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; `git diff --check`.
- Current status: source/Host policy `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade timer callback timing, scheduler, concurrency, long-run stability, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL configured thread stack-size ownership

- Adapter-owned FreeRTOS thread contexts now retain the configured stack size in OSAL byte units;
  the public getter returns that value only for a tracked live handle and returns zero for a
  stale/foreign handle. The default adapter stack is represented as `256 * sizeof(StackType_t)`;
  explicit attributes retain the validated caller byte size.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC/STM32L4/STM32U5/WCH
  FreeRTOS compile; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade scheduler, stack telemetry runtime, concurrency, long-run stability, or
  product qualification.

### 2026-10-19 FreeRTOS OSAL fast-thread-retirement race guard

- Thread context ownership is published before `xTaskCreate()` and the creator only writes the
  returned native handle when the trampoline has not already retired the context. This prevents a
  task that returns immediately from being resurrected as a live adapter-owned context.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`;
  `make HAL_PLATFORM=WCH KCONFIG_OVERRIDES=OSAL_BACKEND_FREERTOS=ON`;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so
  this does not upgrade scheduler, concurrency, long-run stability, or product qualification.

### 2026-10-19 FreeRTOS OSAL thread/timer name retention

- FreeRTOS thread and timer names are now copied into adapter-owned bounded contexts;
  name getters reject untracked handles and return the retained bounded name rather than
  depending on native object-name lifetime/availability.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`;
  `make HAL_PLATFORM=WCH KCONFIG_OVERRIDES=OSAL_BACKEND_FREERTOS=ON`;
  `git diff --check`.
- Current status: source/Host/compile `DONE`, commit `743e4331` pushed and remote equality
  verified (`HEAD...origin/main=0/0`). Target programming/UART runtime was not executed, so
  this does not upgrade FreeRTOS runtime, scheduler, concurrency, or product qualification.

### 2026-10-19 FreeRTOS OSAL named synchronization-object boundary

- FreeRTOS event flags, mutexes, and semaphores now retain bounded 16-byte names and expose
  them only for adapter-owned live contexts; event-flags publication failure releases the native
  object instead of publishing an untracked handle.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags`; Host 278/278; PC/STM32L4/STM32U5/WCH FreeRTOS compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; target programming/UART runtime was not executed,
  so this does not upgrade FreeRTOS runtime, concurrency, long-run stability, or product
  qualification.

### 2026-10-18 FreeRTOS OSAL smoke identity fail-closed guard

- All 11 openCH CH32V307 FreeRTOS OSAL smoke images now reject a missing
  `XINYI_FIRMWARE_COMMIT` at compile time instead of embedding an `unknown` identity marker.
- Evidence path: the 11 `boards/opench_ch32v307/osal_freertos_*_main.c` sources,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; PC/STM32L4/STM32U5/WCH compile;
  `git diff --check`.
- Current status: source/Host/compile `DONE`; target programming/UART runtime was not executed,
  so this does not upgrade to HIL evidence.
- Still not allowed to claim: any FreeRTOS runtime, scheduler/concurrency, ISR-to-task behavior,
  long-run stability, or product qualification.

### 2026-10-18 FreeRTOS OSAL primitives identity binding

- openCH CH32V307 primitives image now binds pre-scheduler and boot markers to
  `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used by the image.
- Evidence path: `boards/opench_ch32v307/osal_freertos_primitives_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; WCH primitives target compile;
  `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: task/synchronization runtime, concurrency, long-run stability, or
  product qualification.

### 2026-10-18 FreeRTOS OSAL message-queue lifecycle identity policy guard

- The policy test now guards the existing openCH CH32V307 queue lifecycle image binding of
  pre-scheduler and boot markers to `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used
  by the image.
- Evidence path: `boards/opench_ch32v307/osal_freertos_queue_lifecycle_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; WCH queue target compile; PC/STM32L4/STM32U5
  compile; `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: queue runtime, concurrency, throughput, long-run stability, or
  product qualification.

### 2026-10-18 FreeRTOS OSAL timer/thread-flags identity binding

- openCH CH32V307 timer/thread-flags image now binds pre-scheduler and boot markers to
  `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used by the image.
- Evidence path: `boards/opench_ch32v307/osal_freertos_timer_flags_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; PC/STM32L4/STM32U5/WCH compile;
  `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: timer runtime, ISR-origin thread flags, long-run stability, or
  product qualification.

### 2026-10-18 FreeRTOS OSAL ISR-stress identity binding

- openCH CH32V307 ISR-stress image now binds pre-scheduler and boot markers to
  `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used by the image.
- Evidence path: `boards/opench_ch32v307/osal_freertos_isr_stress_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; PC root build; WCH FreeRTOS target compile;
  `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: task/synchronization runtime, concurrency, long-run stability, or product
  qualification.

### 2026-10-18 FreeRTOS OSAL SDK qualification image identity binding

- The openCH CH32V307 SDK/OSAL qualification image now binds its pre-scheduler and task-side boot
  markers to `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used by the image.
- Evidence path: `boards/opench_ch32v307/osal_freertos_sdk_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; PC/STM32L4/STM32U5/WCH compile; and
  `git diff --check`.
- Current status: software/compile evidence only until target programming and UART runtime are
  executed; this does not upgrade scheduler, ISR-to-task, concurrency, or product qualification.

### 2026-10-18 FreeRTOS OSAL semaphore lifecycle identity binding

- openCH CH32V307 semaphore lifecycle image now binds pre-scheduler and boot markers to
  `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used by the image.
- Evidence path: `boards/opench_ch32v307/osal_freertos_semaphore_lifecycle_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; PC root build; WCH FreeRTOS target compile;
  `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: semaphore runtime, ISR-to-task, concurrency, long-run stability,
  or product qualification.

### 2026-10-18 FreeRTOS OSAL one-shot timer identity binding

- openCH CH32V307 one-shot timer lifecycle image now binds pre-scheduler and boot markers to
 `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used by the image.
- Evidence path: `boards/opench_ch32v307/osal_freertos_oneshot_timer_main.c`,
 `boards/opench_ch32v307/CMakeLists.txt`, and
 `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
 `freertos_osal_thread_flags_ok`; Host 278/278; PC root build; WCH FreeRTOS target compile;
 `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
 does not upgrade to HIL evidence.
- Still not allowed to claim: timer runtime, callback timing, concurrency, long-run stability,
  ISR-to-task behavior, or product qualification.

### 2026-10-18 FreeRTOS OSAL memory-pool lifecycle identity binding

- openCH CH32V307 memory-pool lifecycle image now binds pre-scheduler and boot markers to
  `XINYI_FIRMWARE_COMMIT`, matching the exact source commit used by the image. The image covers
  metadata, allocation/exhaustion, foreign and double-free rejection, block reuse, and delete.
- Evidence path: `boards/opench_ch32v307/osal_freertos_mempool_lifecycle_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; PC root build; WCH FreeRTOS target compile;
  `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: memory-pool runtime, resource-exhaustion qualification, concurrency,
  long-run stability, or product qualification.

### 2026-10-18 FreeRTOS OSAL message-queue priority boundary

- FreeRTOS message queues use a native FIFO queue and cannot preserve the public `msg_prio` contract. Nonzero priorities now fail closed before native queue access instead of being silently discarded.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; PC/STM32L4/STM32U5/WCH FreeRTOS builds; `git diff --check`.
- Current status: source/Host/compile `DONE`; no target programming/UART runtime was executed, so this does not upgrade FreeRTOS queue runtime, priority ordering, throughput, concurrency, or product qualification.

### 2026-10-19 FreeRTOS OSAL message-queue caller-storage boundary

- Dynamic FreeRTOS message-queue creation now rejects nonzero `mq_mem`/`mq_size` caller-storage requests before context or native queue allocation; static storage is not implemented by this adapter.
- Evidence path: `components/kernel/osal/backend/freertos/xy_os_freertos.c` and `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy `freertos_osal_thread_flags_ok`; Host `make test-unit` 278/278; STM32L4/STM32U5/WCH FreeRTOS builds; `git diff --check`.
- Current status: Host/compile `DONE`; no target programming/UART runtime was executed, so this does not upgrade static allocation, queue runtime, concurrency, throughput, or product qualification.

### 2026-10-18 FreeRTOS mutex lifecycle contract

- 旧边界 -> 新边界：mutex dynamic create/delete 现以 bounded adapter-owned context table 约束 owner；context exhaustion 在 native mutex 创建前拒绝，创建失败释放 wrapper，acquire/release/owner/delete 仅接受 tracked live context，delete 后清除 owner slot 并释放 wrapper。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；PC/L4/U5/WCH FreeRTOS builds；`git diff --check`。
- 适用范围：FreeRTOS mutex owner/lifecycle/failure-atomic source contract。
- 仍不允许宣称：本轮未新增 target runtime marker；mutex 完整 runtime、竞争/优先级协议、长稳或产品资格仍 pending。

### 2026-10-19 FreeRTOS OSAL timer tick conversion consistency guard

- 旧边界 -> 新边界：timer start 现使用与其他 wait API 相同的 checked `freertos_timeout_to_ticks()`，将有限 tick 安全转换为 `TickType_t`，显式拒绝 zero/不可表示 period，并直接传播 `xTimerChangePeriod()` 结果。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy、Host 278/278、PC/L4/U5/WCH FreeRTOS compile、`git diff --check`。
- 仍不允许宣称：未执行 target program/UART runtime；不升级定时精度、callback timing、并发、长稳或产品资格声明。


- 旧边界 -> 新边界：policy 现要求所有 FreeRTOS `xTaskCreate()` 使用 adapter-owned `freertos_thread_entry`，避免新增路径绕过 tracked context retirement；NULL/foreign/stale lifecycle 仍 fail-closed。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy、Host 278/278、PC/L4/U5/WCH FreeRTOS compile、`git diff --check`。
- 仍不允许宣称：未执行 target program/UART runtime；不升级 scheduler、并发、长稳或产品资格声明。


- 旧边界 -> 新边界：thread context 现保存 entry function/argument，native task 统一经 adapter trampoline 启动；entry 返回后 trampoline 清除 tracked context 再删除当前 task，避免已退出 task 的 adapter metadata 残留并允许 context slot 重用。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy `freertos_osal_thread_flags_ok`；Host 278/278；PC/L4/U5/WCH FreeRTOS compile；`git diff --check`。
- 适用范围：FreeRTOS adapter-owned thread context retirement after normal entry return。
- 仍不允许宣称：本轮未执行 target program/UART runtime；不升级 scheduler、并发、长稳或产品资格声明。


- openCH CH32V307 image now covers event-flags create/set/wait/clear/timeout/delete and stale-handle
 rejection, with source commit identity marker.
- Evidence path: `boards/opench_ch32v307/osal_freertos_event_flags_lifecycle_main.c`,
 `boards/opench_ch32v307/CMakeLists.txt`, and
 `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
 `freertos_osal_thread_flags_ok`; Host 278/278; WCH target compile; PC/L4/U5 root compile;
 `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
 does not upgrade to HIL evidence.
- Still not allowed to claim: FreeRTOS event-flags runtime, ISR-to-task, concurrency, long-run
 stability, or product qualification.

### 2026-10-18 FreeRTOS OSAL event-flags ownership boundary

- 旧边界 -> 新边界：FreeRTOS adapter 现以 bounded owner table 跟踪动态创建的 event group；
  set/clear/get/wait/delete 在 native event-group access 前拒绝未跟踪 handle，delete 后清除
  owner slot。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy
  `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；`make`；
  `make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；
  `make HAL_PLATFORM=WCH KCONFIG_OVERRIDES=OSAL_BACKEND_FREERTOS=ON`；`git diff --check`。
- 适用范围：FreeRTOS dynamic event-flags handle ownership and fail-closed native access boundary。
- 仍不允许宣称：FreeRTOS event-flags runtime、资源耗尽、并发、ISR-to-task、长稳或 product
  qualification。

### 2026-10-18 FreeRTOS OSAL memory-pool ownership boundary

- 旧边界 -> 新边界：FreeRTOS adapter 现以 bounded owner table 跟踪动态创建的 memory pool；
  name/alloc/free/capacity/block-size/count/space/delete 在 pool state access 前拒绝未跟踪
  handle，delete 后清除 owner slot；owner table 耗尽时在 allocation 前 fail-closed。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy
  `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；`make`；
  `make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；
  `make HAL_PLATFORM=WCH KCONFIG_OVERRIDES=OSAL_BACKEND_FREERTOS=ON`；
  `git diff --check`。
- 适用范围：FreeRTOS dynamic memory-pool handle ownership and fail-closed state access boundary。
- 仍不允许宣称：FreeRTOS memory-pool runtime、资源耗尽、并发、长稳或 product qualification。

### 2026-10-18 FreeRTOS OSAL message-queue ownership boundary

- 旧边界 -> 新边界：FreeRTOS adapter 现以 bounded owner table 跟踪动态创建的 message queue；
  put/get/count/space/capacity/reset/delete 在 native queue access 前拒绝未跟踪 wrapper，
  delete 后清除 owner slot。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy
  `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；`make`；
  `make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS dynamic message-queue handle ownership and fail-closed native access boundary。
- 仍不允许宣称：FreeRTOS queue runtime、资源耗尽、并发、吞吐、长稳或 product qualification。

### 2026-10-18 FreeRTOS OSAL thread lifecycle smoke

- openCH CH32V307 thread lifecycle image covers source identity, metadata, thread flags,
  suspend/resume, inventory boundary, and NULL lifecycle guards.
- Evidence path: `boards/opench_ch32v307/osal_freertos_thread_lifecycle_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; Host 278/278; PC/STM32L4/STM32U5/WCH FreeRTOS compile;
  `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: FreeRTOS thread runtime, scheduler, concurrency, long-run stability,
  or product qualification.

### 2026-10-18 FreeRTOS OSAL timer ownership boundary

- 旧边界 -> 新边界：FreeRTOS adapter 现以 bounded owner table 跟踪动态创建的 timer；timer name/start/stop/is-running/delete 在 native timer access 前拒绝未跟踪 handle，删除仅在 native deletion 成功后释放 owner slot。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；`make`；`make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS dynamic timer handle ownership and fail-closed native access boundary。
- 仍不允许宣称：FreeRTOS timer runtime、queue exhaustion、callback timing、长稳或 product qualification。

### 2026-10-18 FreeRTOS OSAL semaphore ownership boundary

- 旧边界 -> 新边界：FreeRTOS adapter 现以 bounded owner table 跟踪动态创建的 semaphore；
  acquire/release/release-from-isr/count/delete 在 native semaphore access 前拒绝未跟踪
  handle；owner table 耗尽时释放 native semaphore，成功 delete 后清除 owner slot。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy
  `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；`make`；
  `make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS dynamic semaphore handle ownership and fail-closed native access boundary。
- 仍不允许宣称：FreeRTOS semaphore runtime、ISR-to-task runtime、并发、资源耗尽、长稳或
  product qualification。

### 2026-10-18 FreeRTOS OSAL mutex ownership boundary

- 旧边界 -> 新边界：FreeRTOS adapter 现以 bounded owner table 跟踪动态创建的 mutex；
  acquire/release/owner/delete 在 native mutex access 前拒绝未跟踪 handle，成功 delete 后
  清除 owner slot。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy
  `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；`make`；
  `make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS dynamic mutex handle ownership and fail-closed native access boundary。
- 仍不允许宣称：FreeRTOS mutex runtime、owner completeness、并发、优先级协议、长稳或
  product qualification。

### 2026-10-18 FreeRTOS OSAL message-queue lifecycle smoke

- openCH CH32V307 image now includes source identity, FIFO/full/reset/delete, and stale-handle
  rejection markers for the tracked FreeRTOS message-queue wrapper.
- Evidence path: `boards/opench_ch32v307/osal_freertos_queue_lifecycle_main.c`,
  `boards/opench_ch32v307/CMakeLists.txt`, and
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused policy
  `freertos_osal_thread_flags_ok`; WCH queue target compile; `git diff --check`.
- Current status: Host/compile `DONE`; target programming/UART runtime was not executed, so this
  does not upgrade to HIL evidence.
- Still not allowed to claim: FreeRTOS queue runtime, resource exhaustion, concurrency, throughput,
  long-run stability, or product qualification.


- 旧边界 -> 新边界：`xy_os_mutex_get_owner()` 现验证 mutex/native owner，并只发布
  FreeRTOS adapter owner table 中已跟踪的 task；未跟踪 kernel-global owner 或无效 mutex 返回
  `NULL`，避免向 OSAL 上层泄露未拥有的 task handle。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused
  `freertos_osal_thread_flags`；Host `make test-unit` 278/278；`make`；
  `make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS OSAL mutex-owner publication and adapter-ownership boundary。
- 仍不允许宣称：FreeRTOS runtime mutex ownership completeness、scheduler、并发、延迟、长稳或
  product qualification。

### 2026-10-18 FreeRTOS OSAL mutex lifecycle smoke

- 新增 openCH CH32V307 FreeRTOS mutex lifecycle image，覆盖 normal/recursive acquire-release、
  delete 后 stale handle fail-closed，以及 source commit identity marker。
- 证据路径：`boards/opench_ch32v307/osal_freertos_mutex_lifecycle_main.c`、
  `boards/opench_ch32v307/CMakeLists.txt`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`。
- 当前状态：Host/compile `DONE`；未执行 target program/UART runtime，不升级为 HIL 证据。
- 仍不允许宣称：FreeRTOS mutex runtime、并发、优先级协议、ISR-to-task、长稳或 product
  qualification。

### 2026-10-17 FreeRTOS OSAL thread inventory ownership boundary

- 旧边界 -> 新边界：FreeRTOS `xy_os_thread_get_count()` 与 `xy_os_thread_enumerate()` 不再
  将 kernel-global task inventory 当作 XinYi OSAL adapter-owned threads 发布；当前明确返回
  空 inventory，避免向上层泄露未跟踪 handle 或 NULL 占位项。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy
  `freertos_osal_thread_flags_ok`；focused CTest `freertos_osal_thread_flags`；Host
  `make test-unit` 278/278；`make`；`make HAL_PLATFORM=STM32L4`；
  `make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS OSAL thread-count/enumeration ownership and fail-closed contract。
- 仍不允许宣称：FreeRTOS runtime inventory completeness、scheduler、并发、延迟、长稳或
  product qualification。

### 2026-10-17 FreeRTOS OSAL memory-pool input and timeout boundary

- 旧边界 -> 新边界：FreeRTOS dynamic memory-pool create 现拒绝非零未知属性、caller-owned
  control-block 以及无 backing storage 却带非零 `mp_size` 的请求；pool allocation 复用
  checked `TickType_t` conversion，无法表示的 finite timeout 在 polling 前 fail-closed，
  `XY_OS_WAIT_FOREVER` 显式映射 `portMAX_DELAY`。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused
  `freertos_osal_thread_flags`；Host `make test-unit` 278/278；PC/L4/U5/WCH FreeRTOS
  builds；`git diff --check`；committed as `9eaf16b4` and synchronized to `origin/main`。
- 适用范围：FreeRTOS memory-pool public input and native-timeout representability contract。
- 仍不允许宣称：runtime allocation behavior, concurrency, resource exhaustion, scheduler
  latency, long-run 或 product qualification。

### 2026-10-17 FreeRTOS semaphore lifecycle contract

- 旧边界 -> 新边界：dynamic semaphore create/delete 现统一要求 adapter-owned tracked context；context exhaustion 时删除已创建 native semaphore，避免 orphan object；acquire/release/release-from-ISR/count/delete 均拒绝 NULL、foreign 或 stale handle。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；PC/L4/U5/WCH FreeRTOS builds；`git diff --check`。
- 适用范围：FreeRTOS semaphore adapter ownership and failure-atomic lifecycle contract。
- 仍不允许宣称：本轮未新增 target program/runtime marker；semaphore 完整 HIL、长稳或产品资格仍 pending。


- 旧边界 -> 新边界：dynamic memory-pool adapter 现拒绝 unsupported attribute/control-block/storage combinations，并对 alloc finite timeout 做 checked `TickType_t` conversion；非法配置或超范围 timeout 在 allocator/polling 前 fail-closed。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；PC/L4/U5/WCH FreeRTOS builds；`git diff --check`。
- 适用范围：FreeRTOS dynamic memory-pool public attributes and allocation-timeout representability contract。
- 仍不允许宣称：runtime allocation latency、并发、长稳或产品资格。


- 旧边界 -> 新边界：mutex acquire、semaphore acquire、message-queue put/get 不再将超过 native `TickType_t` 的 finite timeout 窄化后传入 FreeRTOS；统一 checked conversion，`XY_OS_WAIT_FOREVER` 显式映射 `portMAX_DELAY`。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy `freertos_osal_thread_flags_ok`；Host `make test-unit` 278/278；PC/L4/U5/WCH FreeRTOS builds；`git diff --check`。
- 适用范围：FreeRTOS OSAL synchronization wait timeout representability/fail-closed contract。
- 仍不允许宣称：runtime timeout precision、scheduler latency、stress、long-run 或 product qualification。

### 2026-10-17 FreeRTOS OSAL all-tick-input representability boundary

- 旧边界 -> 新边界：thread/event flags wait、relative/absolute delay 不再将 finite timeout/tick
  输入直接窄化为 native `TickType_t`；现统一 checked conversion，无法表示的输入在 notification,
  event-group 或 delay kernel access 前 fail-closed，`XY_OS_WAIT_FOREVER` 显式映射
  `portMAX_DELAY`。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused
  `freertos_osal_thread_flags`；Host `make test-unit` 278/278；`make`、
  `make HAL_PLATFORM=STM32L4`、`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS OSAL tick-input representability/fail-closed contract。
- 仍不允许宣称：Pandora runtime、timeout precision、scheduler latency、stress、long-run 或
  product qualification。


- 旧边界 -> 新边界：dynamic timer start 现拒绝无法由 native `TickType_t` 表示的 tick 输入，避免窄化后调用 FreeRTOS timer command。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`；Host `make test-unit` 278/278；`make`、`make HAL_PLATFORM=STM32L4`、`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS dynamic timer start tick representability/fail-closed contract。
- 仍不允许宣称：Pandora 本变更 runtime、计时精度、timer queue exhaustion、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL synchronization-object attribute boundary

- 旧边界 -> 新边界：FreeRTOS dynamic mutex 现仅接受 recursive attribute，semaphore 与
  message queue 拒绝未知 attribute bits 以及 caller-owned control-block memory；所有拒绝
  均发生在 native kernel allocation 前。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused
  `freertos_osal_thread_flags`；Host `make test-unit` 278/278；`make`、
  `make HAL_PLATFORM=STM32L4`、`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS dynamic synchronization-object public attribute/fail-closed contract。
- 仍不允许宣称：Pandora 本变更 runtime、caller-owned/static allocation、priority protocol、
  concurrency、latency、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL native-width creation boundary

- 旧边界 -> 新边界：FreeRTOS dynamic semaphore 与 message queue 现拒绝不能由 native
  `UBaseType_t` 表示的 count/size 输入，所有拒绝均发生在 wrapper/kernel allocation 前。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused
  `freertos_osal_thread_flags`；Host `make test-unit` 278/278；`make`、
  `make HAL_PLATFORM=STM32L4`、`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS dynamic semaphore/message-queue representability and fail-closed contract。
- 仍不允许宣称：Pandora 本变更 runtime、资源耗尽、吞吐、并发、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL timer-create attribute boundary

- 旧边界 -> 新边界：dynamic FreeRTOS adapter 不再把未知 timer type 当作 one-shot，也不再静默
  忽略非零 attribute bits 或 caller-owned control block；这些请求在 context/kernel allocation 前
  fail-closed。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused
  `freertos_osal_thread_flags`、`freertos_stm32l4_compile`、`freertos_stm32u5_compile`、
  `pandora_freertos_runtime` 4/4；Host `make test-unit`；PC、STM32L4、STM32U5 root compile；
  `git diff --check`。
- 适用范围：FreeRTOS dynamic-allocation software timer create 的公开 type/attribute contract。
- 仍不允许宣称：Pandora 本变更 runtime、caller-owned/static allocation、timer queue exhaustion、
  callback timing、计时精度、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL thread-create attribute boundary

- 旧边界 -> 新边界：dynamic FreeRTOS adapter 不再静默忽略 joinable、caller-owned control-block/stack、TrustZone/reserved 属性，也不再把 byte stack size 未校验地窄化为 `uint16_t` depth；不支持或不可表示的请求在 `xTaskCreate()` 前 fail-closed。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`、`freertos_stm32l4_compile`、`freertos_stm32u5_compile`、`pandora_freertos_runtime` 4/4；Host `make test-unit`；PC、STM32L4、STM32U5 root compile；`git diff --check`。
- 适用范围：FreeRTOS dynamic-allocation thread create 的公开属性与 byte-to-stack-word representability contract。
- 仍不允许宣称：Pandora 本变更 runtime、caller-owned/static allocation、join/detach、TrustZone、scheduler 行为、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL thread-flags argument boundary

- 旧边界 -> 新边界：`xy_os_thread_flags_set()`、`clear()`、`wait()` 不再把空 mask、保留 error bit `0x80000000` 或未知 wait option 传入 FreeRTOS task-notification API；非法输入统一返回 OSAL flag error sentinel，且不产生 kernel side effect。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`、`freertos_stm32l4_compile`、`freertos_stm32u5_compile`、`pandora_freertos_runtime` 4/4；Host `make test-unit`；PC、STM32L4、STM32U5 root compile；`git diff --check`。
- 适用范围：FreeRTOS thread-notification-backed OSAL thread flags 的 public argument/fail-closed contract。
- 仍不允许宣称：Pandora 本变更 runtime、ISR-origin thread flags、并发线性化、延迟、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL scheduler-start fail-closed boundary

- 旧边界 -> 新边界：`xy_os_kernel_start()` 不再在 `vTaskStartScheduler()` 异常返回后报告 `XY_OS_OK`；由于正常启动不会返回，返回路径现 fail-closed 为 `XY_OS_ERROR`。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`、`freertos_stm32l4_compile`、`freertos_stm32u5_compile`、`pandora_freertos_runtime` 4/4；Host `make test-unit`；PC、STM32L4、STM32U5 root compile；`git diff --check`。
- 适用范围：FreeRTOS scheduler start 的异常返回值边界，不把 heap/port 启动失败伪报为成功。
- 仍不允许宣称：Pandora 本变更 runtime、失败原因诊断、scheduler 可用性、调度延迟、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL kernel-state boundary

- 旧边界 -> 新边界：`xy_os_kernel_get_state()` 不再将除 running 外的全部 FreeRTOS scheduler 状态都报告为 `READY`；现将 not-started/running/suspended 映射为 OSAL `READY`/`RUNNING`/`LOCKED`，未知状态 fail-closed 为 `ERROR`。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`、`freertos_stm32l4_compile`、`freertos_stm32u5_compile`、`pandora_freertos_runtime` 4/4；Host `make test-unit`；PC、STM32L4、STM32U5 root compile；`git diff --check`。
- 适用范围：FreeRTOS scheduler state 到公开 OSAL kernel state 的 source/compile contract。
- 仍不允许宣称：Pandora 本变更 runtime、跨 task 并发线性化、调度延迟、公平性、长稳或产品资格。

### 2026-10-17 FreeRTOS OSAL scheduler-lock state boundary

- 旧边界 -> 新边界：`xy_os_kernel_lock()`、`unlock()` 与 `restore_lock()` 不再固定返回 0；现读取 FreeRTOS scheduler state，返回 previous lock state，幂等处理已处于目标状态的调用，并对 scheduler 尚未启动/未知状态及非法 restore token fail-closed。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`、`freertos_stm32l4_compile`、`freertos_stm32u5_compile`、`pandora_freertos_runtime` 4/4；Host `make test-unit` 278/278；PC、STM32L4、STM32U5 root compile；`git diff --check`。
- 适用范围：single observable FreeRTOS scheduler suspended/running state 与 OSAL 0/1 restore token；重复 lock/unlock 不额外改变 scheduler nesting。
- 仍不允许宣称：Pandora 本变更 runtime、完整嵌套 lock count、跨 task 并发线性化、调度延迟、长稳或产品资格。

### 2026-10-16 FreeRTOS OSAL absolute-delay boundary

- 旧边界 -> 新边界：`xy_os_delay_until()` 不再用 absolute target 作为 `vTaskDelayUntil()` previous-wake value 并传入 zero increment；现读取 current tick，以 signed modular delta 判断已到期目标，并仅对未来目标调用 `vTaskDelay(target - now)`。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`、`freertos_stm32l4_compile`、`freertos_stm32u5_compile`、`pandora_freertos_runtime` 4/4；Host `make test-unit` 278/278；PC、STM32L4、STM32U5 root compile；`git diff --check`。
- 适用范围：32-bit FreeRTOS tick 域内、目标与当前时间差小于半个 tick range 的 absolute-delay contract；past/current targets return immediately。
- 仍不允许宣称：Pandora 本变更 runtime、跨半 range deadline 排序、tick 精度、调度延迟、长稳或产品资格。

### 2026-10-16 FreeRTOS OSAL bounded priority mapping

- 旧边界 -> 新边界：OSAL `LOW..ISR` 不再被直接截断并大量折叠到 FreeRTOS 最高 native priority；完整 OSAL priority domain 现有界、单调地缩放到 `0..configMAX_PRIORITIES-1`。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags` 1/1；Host `make test-unit` 278/278；PC、STM32L4、STM32U5 root compile；`git diff --check`。
- 适用范围：显式 thread create/set 拒绝 `NONE`、`ERROR`、`RESERVED` 与越界 priority；native priority query 映射回有效 OSAL domain。
- 仍不允许宣称：Pandora 本变更 runtime、精确调度公平性、抢占延迟、所有 priority level 唯一对应、长稳或产品资格。

### 2026-10-16 openCH official WCH FreeRTOS + XinYi OSAL integration

- 旧等级 -> 新等级：openCH CH32V307 OSAL/FreeRTOS bounded B1 runtime；Pandora evidence unchanged
- 证据路径/命令：`boards/opench_ch32v307/{CMakeLists.txt,osal_freertos_sdk_main.c}`、`docs/validation/evidence/opench-ch32v307/2026-10-16-osal-freertos-sdk/`；WCH-Link program/verify/reset 与 UART machine parse（pre-scheduler、OSAL boot、task sync、fast ISR 各 1，error 0）；focused `opench_ch32v307_osal_freertos_sdk_smoke`；Host `make test-unit` 278/278；PC/STM32L4/STM32U5/WCH Release builds；WCH FreeRTOS configure/build with `-DKCONFIG_OVERRIDES=OSAL_BACKEND_FREERTOS=ON`；`git diff --check`。
- 适用范围：root Kconfig/CMake 可选择 WCH FreeRTOS；board image 直接消费 pinned WCH SDK kernel/RISC-V PFIC port/startup/linker，XinYi OSAL adapter 通过 `xy_os_*` 创建 worker/IRQ tasks、delay、semaphore acquire/release；TIM2 fast IRQ 使用官方 interrupt-stack switch，并完成 10 次 ISR→task wake while worker continues.
- 仍不允许宣称：Pandora 本变更 runtime、WCH 全部 OSAL primitive、全部 HAL IRQ handler、nested IRQ、长稳/吞吐、公平性或 product qualification；WCH target runtime evidence remains limited to the recorded openCH image and exact SDK commit.

### 2026-10-16 FreeRTOS Event Flags argument and wait-result boundary

- 旧等级 -> 新等级：H1 FreeRTOS OSAL source/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`, `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused `freertos_osal_thread_flags`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`。
- 适用范围：event-flags set/clear/wait reject empty masks, FreeRTOS-reserved control bits, and unknown wait options before backend calls; wait returns the OS error sentinel when the requested any/all condition is not satisfied, preventing timeout/partial results from being reported as success.
- 仍不允许宣称：Pandora/FreeRTOS runtime、ISR-origin event flags、concurrency linearizability、latency、stress、recovery 或 product qualification。

### 2026-10-16 FreeRTOS OSAL thread ownership fail-closed

- 旧等级 -> 新等级：FreeRTOS OSAL source/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused `freertos_osal_thread_flags`；`make test-unit` 278/278；`make`；`make HAL_PLATFORM=STM32L4`；`make HAL_PLATFORM=STM32U5`；`git diff --check`。
- 适用范围：FreeRTOS adapter 对没有可观察 ownership-retirement/join primitive 的 detach/join 入口 fail-closed；NULL handle 在 detach/join/terminate 前拒绝，避免伪造成功或进入 kernel API。
- 仍不允许宣称：Pandora/FreeRTOS runtime、thread ownership completeness、scheduler safety、latency/throughput/stress 或 product qualification。

### 2026-10-16 openCH FreeRTOS OSAL timer/thread-flags runtime

- 旧等级 -> 新等级：openCH CH32V307 OSAL timer/thread-flags bounded B1 runtime；Pandora evidence unchanged
- 证据路径/命令：`boards/opench_ch32v307/{CMakeLists.txt,FreeRTOSConfig.h,osal_freertos_timer_flags_main.c}`、`docs/validation/evidence/opench-ch32v307/2026-10-16-osal-freertos-timer-flags/`；WCH-Link program/verify/reset 与 UART machine parse；focused `opench_ch32v307_osal_freertos_timer_flags_smoke`；Host `make test-unit` 278/278；PC/STM32L4/STM32U5/WCH Release builds；`git diff --check`。
- 适用范围：periodic XinYi OSAL timer callback 通过 OSAL thread flags 向 monitor 发送 20 次通知；timer active/stop/post-stop stability、flags timeout 与独立 worker liveness 均通过，error marker 为 0；官方 SDK standalone port smoke 同步编译 `timers.c`。
- 仍不允许宣称：Pandora 本变更 runtime、ISR-origin thread flags、timer queue exhaustion、callback overrun handling、计时精度、长稳、性能或 product qualification。

### 2026-10-16 WCH SPI lifecycle ownership fail-closed

- 旧等级 -> 新等级：WCH SPI lifecycle H1/C1；hardware evidence unchanged
- 证据路径/命令：`components/hal/wch/ch32x/src/xy_hal_spi.c`、`tests/unit/hal_component/check_wch_spi_lifecycle.py`；focused `wch_spi_lifecycle`；Host `make test-unit` 278/278；PC/STM32L4/STM32U5/WCH Release builds；`clang-format --dry-run --Werror`；`git diff --check`。
- 适用范围：SPI1/SPI2/SPI3 分别跟踪 lifecycle；重复 init、未初始化 blocking/DMA/stop/callback/auxiliary/error 入口 fail-closed；成功 deinit 清除 owner/callback；DMA/stop/software-CS unsupported 路径仍先验证公开输入与 lifecycle。
- 仍不允许宣称：Pandora runtime、openCH 新 runtime、SPI 电气/时序、DMA 功能、外设响应、恢复、性能、耐久或 product qualification。

### 2026-10-16 WCH I2C lifecycle ownership fail-closed

- 旧等级 -> 新等级：WCH I2C lifecycle H1/C1；hardware evidence unchanged
- 证据路径/命令：`components/hal/wch/ch32x/src/xy_hal_i2c.c`、`tests/unit/hal_component/check_wch_i2c_lifecycle.py`；focused `wch_i2c_lifecycle`；Host `make test-unit` 277/277；PC/STM32L4/STM32U5/WCH Release builds；`clang-format --dry-run --Werror`；`git diff --check`。
- 适用范围：I2C1/I2C2 独立 owner state；重复 init、未初始化 blocking/DMA/readiness/callback/error 操作 fail-closed；成功 deinit 清除 owner/callback，DMA unsupported 路径仍先验证公开输入。
- 仍不允许宣称：Pandora runtime、openCH 新 runtime、注入式 NACK/stuck-bus recovery、I2C 电气/时序、传感器精度、性能、耐久或 product qualification。

### 2026-10-16 WCH GPIO alternate-function boundary fail-closed

- 旧等级 -> 新等级：WCH HAL B1 bounded API-boundary runtime evidence; no Pandora claim change
- 证据路径/命令：`components/hal/wch/ch32x/src/xy_hal_gpio.c`、`boards/opench_ch32v307/{CMakeLists.txt,gpio_alternate_boundary_main.c}`、`docs/validation/evidence/opench-ch32v307/2026-10-15-gpio-alternate-boundary/`；WCH-Link program/verify/reset 与 UART machine parse（boot/init-reject/set-reject 各 1，error 0）；focused image 与 reusable `xy_hal`；Host `make test-unit` 276/276；PC/STM32L4/STM32U5 Release builds；`git diff --check`。
- 适用范围：本次 openCH CH32V307/PC4 bounded run 证明 backend 对 nonzero alternate 在寄存器或 tracked state 变更前 fail-closed，zero/default 路径保持可用；read-back/tool verify 已通过。
- 仍不允许宣称：Pandora 本变更 runtime、AFIO pin-remap 功能、GPIO 电气/时序、中断、恢复、耐久或 product qualification。

### 2026-10-16 RT-Thread OSAL ISR semaphore release fail-closed

- 旧等级 -> 新等级：H1 RT-Thread backend ISR contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_sync_lifecycle.py`；focused `rtthread_osal_sync_lifecycle`；Host `make test-unit` 276/276；PC/STM32L4/STM32U5 Release builds；`git diff --check`。
- 适用范围：RT-Thread backend 没有已证明的 ISR-safe semaphore release/yield handoff 时，`xy_os_semaphore_release_from_isr()` 不再错误复用 task-context release，明确返回 OS error sentinel；调用者可观察到该能力未实现。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、ISR-to-task synchronization、scheduler safety、latency/throughput/stress 或 product qualification。

### 2026-10-16 RT-Thread OSAL thread detach fail-closed

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_thread_lifecycle.py`；focused `rtthread_osal_thread_lifecycle`；Host `make test-unit` 276/276；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：RT-Thread 无可观察 owner-retirement hook 时，tracked `xy_os_thread_detach()` 明确 fail-closed，不调用 kernel detach；untracked handle 仍在任何 kernel 操作前拒绝。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、detach/join semantics、scheduler safety、latency/throughput/stress 或 product qualification。

### 2026-10-16 RT-Thread OSAL mutex-owner thread guard

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_sync_lifecycle.py`；focused `rtthread_osal_sync_lifecycle`；Host `make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：`xy_os_mutex_get_owner()` 先验证 mutex 为 bounded project owner，再只发布 bounded project-owned tracked thread；kernel-global/untracked owner 返回 NULL。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、mutex ownership completeness、priority inheritance/inversion safety、latency/throughput/stress 或 product qualification。

### 2026-10-16 RT-Thread OSAL current-thread owner guard

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_thread_lifecycle.py`；focused `rtthread_osal_thread_lifecycle`；Host `make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：`xy_os_thread_get_id()` 只返回 bounded project-owned tracked current-thread handle；untracked kernel-global current threads 返回 NULL，不向上层泄露。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、scheduler safety、thread identity completeness、latency/throughput/stress 或 product qualification。

### 2026-10-16 RT-Thread OSAL timer name owner guard

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_timer.py`；focused `rtthread_osal_timer`；Host `make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：timer name query 只接受 bounded project timer context；外部/untracked kernel handle 返回 NULL，且不会在 owner validation 前访问对象字段。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、timer callback timing、scheduler safety、latency/throughput/stress 或 product qualification。

### 2026-10-16 RT-Thread OSAL thread inventory owner contract

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_thread_lifecycle.py`；focused `rtthread_osal_thread_lifecycle`；Host `make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：thread count/enumeration 只遍历 bounded project contexts；enumeration 发布真实 live handles，并在 caller capacity 截断时返回实际写入数量，不再将 kernel-global count 配上 NULL 占位。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、kernel-global thread visibility、scheduler safety、latency/throughput/stress 或 product qualification。

### 2026-10-16 RT-Thread OSAL thread join fail-closed contract

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_thread_lifecycle.py`；focused `rtthread_osal_thread_lifecycle`；Host `make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：`xy_os_thread_join()` 先要求 bounded project-owned thread context；untracked handle 返回参数错误，tracked handle 因 RT-Thread 无 join primitive 明确返回 unsupported error，不再误报成功。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、thread join semantics、scheduler safety、latency/throughput/stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL dynamic-thread runtime ownership

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_thread_lifecycle.py`；focused `rtthread_osal_thread_lifecycle`/`rtthread_osal_priority`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：dynamic thread name/state/stack/priority/suspend/resume/detach/terminate 仅接受 bounded project context 跟踪的 handles；untracked handles 在字段访问或 kernel control/delete 前 fail-closed；kernel delete failure 保留 owner 供重试。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、scheduler safety、ordering/latency/throughput/stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL event-flags owner lifecycle

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_event_flags.py`；focused `rtthread_osal_event_flags`/`ipc_event_group` 2/2；`make test-unit` 276/276；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：event flags 在 kernel allocation 前保留 bounded project context；create failure 回收 reservation；name/set/clear/get/wait/delete 拒绝 untracked handles；kernel delete failure 保留 owner 供重试。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、task/ISR synchronization safety、ordering/latency/throughput/stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL memory-pool owner lifecycle

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_mempool.py`；focused RT-Thread policy 3/3；`make test-unit` 276/276；PC/L4/U5 Release builds；`git diff --check`；实现提交 `2b170487`。
- 适用范围：memory pool 在 kernel allocation 前保留 bounded project context；create failure 回收 reservation；name/alloc/free/query/delete 拒绝 untracked handles；kernel delete failure 保留 owner 供重试。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、memory safety、concurrency、fragmentation、latency/throughput/stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL mutex/semaphore runtime owner integrity

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_sync_lifecycle.py`；focused RT-Thread policy；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：mutex/semaphore name/acquire/release/owner/count runtime 入口拒绝 untracked handles，避免绕过 bounded project context；create/delete 失败原子与现有 ISR release delegation 保持。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、task/ISR synchronization safety、priority inversion behavior、latency/throughput/stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL message-queue owner lifecycle

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_msgqueue.py`；focused RT-Thread policy；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：message queue 在 kernel allocation 前保留 bounded project context；拒绝 zero geometry、context exhaustion 与 untracked handles；create failure 回收 reservation；delete 仅在 kernel 成功后释放 owner context；fixed-size binary payload/current receive API 保持。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、queue ordering/latency/throughput/stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL dynamic timer owner/start atomicity

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_timer.py`；focused RT-Thread policy；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：dynamic timer 在 kernel allocation 前保留 bounded project context；context exhaustion 与 create failure 不留下未跟踪 kernel object；start/stop/query/delete 拒绝 untracked handles；start 拒绝 zero ticks、传播 timeout-control failure；delete 仅在 kernel delete 成功后释放完整 context。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、timer callback cadence/latency/precision、stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL mutex/semaphore context lifecycle

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_sync_lifecycle.py`；focused RT-Thread policy；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：dynamic mutex/semaphore 在 kernel allocation 前保留 bounded project context；context exhaustion 与 create failure 不留下临时/未跟踪 kernel object；delete 只接受 tracked handle，kernel delete 失败时保留 context 供重试；semaphore 拒绝 zero capacity 与 `initial_count > max_count`。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、mutex/semaphore 调度/优先级/ISR 行为、latency、stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL dynamic thread context lifecycle

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_thread_lifecycle.py`；focused RT-Thread policy 7/7；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：dynamic thread create 在 kernel allocation 前保留 bounded project context、startup 前绑定 handle；context exhaustion 与 create/startup failure 不留下 untracked thread；terminate 仅在 kernel delete 成功后释放 context。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、scheduler correctness、thread join/detach semantics、latency、stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL Timer callback/API/lifecycle contract

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_timer.py`；focused `rtthread_osal_timer`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：timer callback 获得 project-owned context 后转发 caller argument；one-shot/periodic 在 create flags 中确定；active state 使用 tracked RT-Thread control API；kernel delete 失败时保留 context 供重试。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、timer 精度、latency、jitter、callback concurrency、stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL Thread Priority bounded mapping

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_priority.py`；focused `rtthread_osal_priority`/`rtthread_osal_mempool`/`rtthread_osal_msgqueue`/`rtthread_osal_event_flags`/`rtthread_osal_thread_flags`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：XY `IDLE..ISR` thread priority 在 `0..RT_THREAD_PRIORITY_MAX-1` 内按相反方向缩放；create/set 在调用 RT-Thread 前拒绝 NONE/error/reserved/out-of-range priority，默认 thread priority 统一取 XY NORMAL 映射值。
- 仍不允许宣称：RT-Thread target runtime、Pandora 本变更 runtime、精确逐级 priority 保真、调度公平性、priority inversion、latency、stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL Memory Pool free API contract

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_mempool.py`；focused `rtthread_osal_mempool`/`rtthread_osal_msgqueue`/`rtthread_osal_event_flags`/`rtthread_osal_thread_flags`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：Memory Pool free 对齐 tracked RT-Thread 的单参数 `void rt_mp_free(void *block)` API；NULL pool/block 继续 fail-closed，合法 free 完成后返回 `XY_OS_OK`；policy 同时固定 bounded/forever allocation timeout 映射。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、跨线程/ISR safety、double-free/foreign-block recovery、resource exhaustion、latency、stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL Memory Pool ownership/query contract

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_mempool.py`；focused `rtthread_osal_mempool`/`rtthread_osal_msgqueue`/`rtthread_osal_event_flags`/`rtthread_osal_thread_flags`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：Memory Pool create 不再额外分配并泄漏一块未交给 RT-Thread 的 heap buffer；zero dimensions 在调用内核前 fail-closed；capacity/block-size/used/free queries 返回 tracked RT-Thread metadata，不再对 live pool 固定返回 0。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、跨线程/ISR safety、resource-exhaustion recovery、latency、stress 或 product qualification。

### 2026-10-15 RT-Thread OSAL unsupported Thread Flags fail-closed

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_thread_flags.py`；focused `rtthread_osal_thread_flags`/`rtthread_osal_event_flags`/`rtthread_osal_msgqueue`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：RT-Thread backend 尚未实现 per-thread flags；set/clear/get/wait 现统一返回 OSAL error sentinel，clear/get 不再用 0 冒充成功或空 flags。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、thread-flags 功能、ISR safety、并发 linearizability、latency、stress、recovery 或 product qualification。

### 2026-10-15 RT-Thread OSAL Message Queue fixed-binary contract

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_msgqueue.py`；focused `rtthread_osal_msgqueue`/`rtthread_osal_event_flags`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：Message Queue put 始终按创建时的 fixed `msg_size` 发送二进制 payload，不再依赖 `strlen()`；get 使用 tracked RT-Thread 四参数 `rt_mq_recv()`，只接受完整 fixed-size message，并将不支持的 priority 输出确定为 0。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、priority queue semantics、ISR safety、concurrency linearizability、latency、stress、recovery 或 product qualification。

### 2026-10-15 RT-Thread OSAL Event Flags argument validation

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_event_flags.py`；focused `rtthread_osal_event_flags`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：Event Flags set/clear/wait 在进入 RT-Thread IPC 前拒绝 zero mask、bit 31 error-reserved mask 与未定义 wait options，非法输入统一返回 OS error sentinel。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、ISR safety、concurrency linearizability、latency、stress、recovery 或 product qualification。

### 2026-10-15 RT-Thread OSAL Event Flags set return semantics

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_event_flags.py`；focused `rtthread_osal_event_flags`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：Event Flags set 传播 `rt_event_send()` failure；成功后在 event spinlock 下返回包含已有 flags 的完整 post-set snapshot，不再只回显 requested mask。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、跨 send/snapshot 的线性化保证、ISR 并发、latency、stress、recovery 或 product qualification。

### 2026-10-15 RT-Thread OSAL Event Flags get compile contract

- 旧等级 -> 新等级：H1 RT-Thread backend source/compile contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_event_flags.py`；focused `rtthread_osal_event_flags`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：Event Flags get 在 RT-Thread event spinlock 下读取完整当前 set，不再依赖 tracked upstream 未定义的 `RT_EVENT_CTRL_GET` command。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、stale-handle 探测、ISR 并发、latency、stress、recovery 或 product qualification。

### 2026-10-15 RT-Thread OSAL Event Flags clear/wait semantics

- 旧等级 -> 新等级：H1 RT-Thread backend source contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/rtthread/xy_os_rtthread.c`、`tests/unit/kernel/check_rtthread_osal_event_flags.py`；focused `rtthread_osal_event_flags`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：RT-Thread Event Flags clear 在内核 spinlock 下原子返回完整旧值并只清请求位；wait 的 `XY_OS_FLAGS_NO_CLEAR` 不再反向触发 `RT_EVENT_FLAG_CLEAR`。
- 仍不允许宣称：RT-Thread target compile/runtime、Pandora 本变更 runtime、stale-handle 探测、ISR 并发、latency、stress、recovery 或 product qualification。

### 2026-10-15 OSAL/IPC Event Flags clear error propagation

- 旧等级 -> 新等级：H1 OSAL/IPC failure contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、`tests/unit/{kernel/check_freertos_osal_thread_flags.py,ipc/test_ipc_event_group.c}`；focused `freertos_osal_thread_flags`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：FreeRTOS NULL Event Flags handle 的 clear 不再返回合法空值；OSAL error sentinel 由 IPC wrapper 映射为错误并保持 caller output。bare-metal invalid/deleted handle 由 focused IPC 回归覆盖。
- 仍不允许宣称：Pandora 本变更 runtime、FreeRTOS stale non-NULL handle 探测、ISR 并发、latency、stress、recovery 或 product qualification。

### 2026-10-15 OSAL/IPC Event Flags get error propagation

- 旧等级 -> 新等级：H1 OSAL/IPC failure contract strengthened; no Board claim change
- 证据路径/命令：`components/kernel/osal/backend/{baremetal,freertos}/xy_os_*.c`、`components/ipc/src/xy_event_group.c`、`tests/unit/{kernel/test_osal.c,ipc/test_ipc_event_group.c}`；focused `osal_baremetal`/`ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：bare-metal invalid/deleted handle 与 FreeRTOS NULL handle 的 Event Flags get 统一发布 OS error sentinel；IPC wrapper 不再把 backend failure 当作空 flags 成功，也不覆盖 caller output。
- 仍不允许宣称：Pandora 本变更 runtime、其他 backend stale-handle 探测、ISR 并发、latency、stress、recovery 或 product qualification。

### 2026-10-14 IPC Broker ISR corrupted-length integrity bound

- 旧等级 -> 新等级：H1 ISR ingress corruption boundary strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：wake callback 将已发布 slot 的 payload length 损坏为超出固定 payload storage 时，integrity 重算最多读取 `XY_BROKER_MAX_MSG_SIZE`，publish 安全回滚 ring 并返回错误，不再越界读取或崩溃。
- 仍不允许宣称：Pandora 本变更 runtime、恶意内存破坏隔离、多 ISR producer 并发安全、throughput、recovery、endurance 或 product qualification。

### 2026-10-14 IPC Broker ISR publish/drain reentrancy isolation

- 旧等级 -> 新等级：H1 ISR ingress publication boundary strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.{h,c}`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：ISR publish 在 wake callback 返回并完成 owner/ring/message/generation postcondition validation 前保持 publishing guard；callback 重入 publish/drain 均 fail-closed，不会提前把未完成验证的 slot 送入 Broker queue。
- 仍不允许宣称：Pandora 本变更 runtime、任意多生产者/消费者并发安全、lock-free、throughput、recovery、endurance 或 product qualification。

### 2026-10-14 IPC Broker ISR wake-failure postcondition validation

- 旧等级 -> 新等级：H1 ISR ingress failure-path integrity contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：wake callback 返回非 OK 时仍重验 ingress owner/geometry、ring head/tail、broker/server generation 与刚发布消息 integrity；若 callback 同时损坏状态，恢复 publish 前 ring 并返回 `XY_BROKER_ERROR`，纯 wake 失败仍保留原错误码。
- 仍不允许宣称：Pandora 本变更 runtime、任意 ISR 并发安全、多生产者 lock-free、throughput、recovery、endurance 或 product qualification。

### 2026-10-14 IPC Broker ISR source-generation binding

- 旧等级 -> 新等级：H1 ISR ingress owner contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：ISR publish slot 固化 publish 前 source generation（包括未注册 source 的零值）；wake callback 若注册或替换 source owner，publish 回滚，drain 也拒绝 generation 漂移消息。
- 仍不允许宣称：Pandora 本变更 runtime、任意 ISR 并发安全、多生产者 lock-free、throughput、recovery、endurance 或 product qualification。

### 2026-10-14 Trace checked dynamic-level setter contract

- 旧等级 -> 新等级：H1/Host runtime-filter API contract strengthened; no Board claim change
- 证据路径/命令：`components/trace/xy_log/{inc/xy_log.h,src/xy_log.c}`、`tests/unit/trace/test_trace_{core,release}.c`；focused `trace_component`/`trace_release_contract`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：新增 `xy_log_try_set_dynamic_level()`，Debug 接受 ERROR..DEBUG，Release 接受 ERROR..WARN，超出当前 ceiling 或 `uint8_t` 越界值返回 false 且保持原等级；旧 void setter 保持兼容并委托同一 contract。
- 仍不允许宣称：Pandora Trace runtime filtering、并发写入安全、ISR safety、throughput、performance、recovery、endurance 或 product qualification。

### 2026-10-14 Trace active-contract documentation sync

- 旧等级 -> 新等级：H1 documentation aligned with existing Host contracts; no evidence-level or Board claim change
- 证据路径/命令：`components/trace/README.md`；focused `trace_component`/`trace_release_contract`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：公开维护说明现准确记录 compile-time filtering、linked `xy_trace` runtime filtering、Release WARN ceiling、两个 focused targets 与 internal backing-storage ownership。
- 仍不允许宣称：Pandora Trace runtime filtering、并发/ISR safety、丢日志策略、throughput、performance、recovery、endurance 或 product qualification。

### 2026-10-14 Trace Release external-level fail-closed contract

- 旧等级 -> 新等级：H1/Host Release-filter contract strengthened; no Board claim change
- 证据路径/命令：`components/trace/xy_log/src/xy_log.c`、`tests/unit/trace/test_trace_release.c`；focused `trace_component`/`trace_release_contract`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：Release runtime getter 会将外部强符号覆盖、内存扰动或旧调用方留下的越界动态等级收敛到 WARN ceiling，避免非法高值放开 INFO/DEBUG；后续 ERROR/WARN 合法切换保持可用。
- 仍不允许宣称：Pandora Trace runtime、并发写入安全、ISR safety、memory-corruption recovery、throughput、performance、endurance 或 product qualification。

### 2026-10-14 Trace Release dynamic-level ceiling contract

- 旧等级 -> 新等级：H1/Host Release-filter contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/trace/test_trace_release.c`、`tests/unit/CMakeLists.txt`；focused `trace_release_contract`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：独立 Release-mode target 固定默认 runtime level 为 WARN，允许 ERROR/WARN 切换，并验证 INFO/DEBUG 越过 release ceiling 时不改变当前 level。
- 仍不允许宣称：Pandora Trace runtime、concurrent loss behavior、ISR safety、throughput、performance、recovery、endurance 或 product qualification。

### 2026-10-14 IPC Broker ISR wake callback message integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：ISR publish 在 wake callback 返回后重算刚发布消息的 integrity tag；callback 改写 payload 或 metadata 时恢复 publish 前 ring state、返回错误且损坏 slot 不可 drain。
- 仍不允许宣称：密码学完整性、不受信任 callback 隔离、多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Broker ISR wake callback route-owner integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：ISR publish 在 wake callback 返回后重新校验 source/destination server generation；callback 注销并重注册 route owner 时回滚 pre-publish ring state、返回错误且 stale slot 不可 drain。
- 仍不允许宣称：不受信任 callback 隔离、多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Broker ISR wake callback post-state integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：ISR ingress 在 wake callback 返回后重新校验 owner、ring geometry、head/tail 与 broker generation；callback 破坏状态时恢复 publish 前 ring state、返回错误且不允许损坏消息进入 drain。
- 仍不允许宣称：不受信任 callback 隔离、多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Broker ISR ingress payload integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.{h,c}`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：ISR publish 对 stored route/generation/priority/length/payload 建立 32-bit integrity tag；drain 在 broker enqueue 前重算校验，检测到 payload 或 metadata 损坏即丢弃 slot，后续合法 ingress 可恢复。
- 仍不允许宣称：密码学完整性、攻击者防护、多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Broker ISR ingress stored-message integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：drain 在向 broker 转发前校验 stored route/message identifiers 与 payload length；损坏 slot fail-closed 丢弃，且后续合法 publish/drain 可恢复。
- 仍不允许宣称：多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Broker ISR ingress wake owner integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.{h,c}`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：ISR ingress 初始化固化 wake callback/context owner；公开 callback 或 context alias 损坏时，publish/drain 在 ring access 或 wake 前 fail-closed。
- 仍不允许宣称：多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Broker ISR ingress ring geometry integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.{h,c}`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：ISR ingress 初始化固化 storage capacity；公开 capacity 或 head/tail 边界损坏时，publish/drain 在 ring 访问或 wake callback 前 fail-closed。
- 仍不允许宣称：多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Broker ISR ingress owner integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.{h,c}`、`tests/unit/ipc/test_ipc_broker.c`；focused `ipc_broker`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：ISR ingress 初始化固化 self/storage owner；复制 handle 或公开 storage alias 损坏时，publish/drain 在访问 ring 或执行 wake callback 前 fail-closed。
- 仍不允许宣称：多 producer/consumer 并发安全、ISR memory ordering 跨架构充分性、Pandora runtime、吞吐或 endurance。

### 2026-10-14 IPC Pipe lifecycle and ring integrity

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/pipe/xy_pipe.{h,c}`、`tests/unit/ipc/test_ipc_pipe.c`；focused `ipc_pipe`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：caller-owned pipe 首次使用要求 zero-init；live owner 重复 init、复制 handle、公开 buffer/size/count/full/topology 损坏均在 ring 访问前 fail-closed；固化 owner 允许原对象 teardown 恢复。
- 仍不允许宣称：线程安全、ISR ingress、blocking semantics、Pandora runtime、throughput 或 endurance。

### 2026-10-14 IPC Event Group transport-owner recovery

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/inc/xy_event_group.h`、`components/ipc/src/xy_event_group.c`、`tests/unit/ipc/test_ipc_event_group.c`；focused `ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：公开 `os_flags` 被损坏时 set/get/clear/wait fail-closed 且保持 caller output；deinit 使用初始化时固化的实际 OSAL owner 回收资源并清零 lifecycle，避免损坏公开 handle 阻断 teardown。
- 仍不允许宣称：跨线程并发安全、ISR ingress、Pandora runtime、transport recovery、throughput 或 endurance。

### 2026-10-13 IPC Event Group lifecycle ownership

- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/inc/xy_event_group.h`、`components/ipc/src/xy_event_group.c`、`tests/unit/ipc/test_ipc_event_group.c`；focused `ipc_event_group`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live event group 拒绝重复 init，复制出的公开 handle 因 owner identity 不匹配而无法 set/deinit；原 owner 保持可用并可正常 teardown，避免覆盖或双重释放 OSAL event-flags owner。
- 仍不允许宣称：跨线程并发安全、ISR ingress、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC MQ behavior-config integrity

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/inc/xy_mq.h`、`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live queue 的公开 priority/overwrite policy 与初始化时固化策略不一致时，send/recv/query/clear/stats 在改变 queue semantics 或访问 ring 前 fail-closed；deinit 仍可释放 owned buffer。
- 仍不允许宣称：跨线程安全、ISR ingress、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC MQ storage-geometry integrity

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/inc/xy_mq.h`、`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live queue 的公开 config 与初始化时 allocation geometry 不一致时，send/recv/query/clear/stats 在计算 slot stride 或访问 ring 前 fail-closed；deinit 仍可释放 owned buffer。
- 仍不允许宣称：跨线程安全、ISR ingress、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC MQ stored-message integrity

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live queue 的 stored message length/priority 损坏时，send/recv/query/clear/stats 在 ring access 前 fail-closed；slot header 通过 `memcpy` 访问以避免非自然对齐 UB；deinit 仍可释放 owned buffer。
- 仍不允许宣称：跨线程安全、ISR ingress、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC MQ ring-topology integrity

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live queue 的 head/tail/count 虽各自在范围内但不满足 ring topology 时，send/recv/query/clear/stats 在 ring access 前 fail-closed；deinit 仍可释放 owned buffer。
- 仍不允许宣称：跨线程安全、ISR ingress、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC MQ ring-state integrity

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live queue 的 config、head/tail 或 count 损坏时，send/recv/query/clear/stats 在 ring access 前 fail-closed；deinit 仍可释放 owned buffer，避免损坏状态阻断 teardown。
- 仍不允许宣称：跨线程安全、ISR ingress、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer stored-registration integrity

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、`tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live observer slot 的 callback 或 active flag 损坏时，attach/detach/notify/count 在 mutation、callback 或 publication 前 fail-closed；clear 仍可恢复 canonical empty state。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer notification-guard integrity

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、`tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：observer callback 若破坏 subject 的 active notification guard，当前 notify 周期立即 fail-closed，停止后续 callback、返回错误并恢复 idle guard。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer attachment-generation wraparound uniqueness

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、`tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：attachment generation 从 `UINT32_MAX` 回绕时跳过全部仍存活 generation，零值继续保留，从而维持 notification snapshot 对 detach/re-attach 的唯一身份判定。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer attachment-generation integrity

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、`tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live subject 内任一 attachment generation 为零或与另一 active slot 重复时，attach/detach/notify/count 在遍历、callback 或 mutation 前 fail-closed；clear 可恢复 canonical empty state。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer callback-corruption boundary

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、`tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：notification callback 破坏公开 subject count/generation 状态后，当前 notify 周期立即 fail-closed、恢复 `notifying=false`，且不再调度后续 observer。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer corrupted-state boundary

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、`tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`
- 适用范围：公开 subject storage 的 observer count 越界或 attachment generation 为零时，attach/detach/notify/count fail-closed，不遍历或复制越界 storage；clear 可恢复 canonical empty state 与非零 generation。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、throughput、recovery 或 endurance。

---

## 1. 证据等级

| 代码 | 证据 | 最低要求 |
|---|---|---|
| H0 | 无有效验证 | 仅源码、文档、stub 或未执行计划 |
| H1 | Host contract | focused CTest + full Host suite |
| C1 | Target compile | clean cross-compile；记录工具链、芯片、配置 |
| Q1 | QEMU runtime | 实际运行；标明测试内模拟部分 |
| B1 | Board smoke | 实板正常路径；记录板卡、接线、固件与日志 |
| B2 | Board negative/recovery | NACK/timeout/掉电/复位/错误恢复等负向证据 |
| P1 | Performance | 固定硬件、频率、编译参数、样本与统计 |
| S1 | Security review | 来源、许可证、威胁模型、实现审查、允许用途 |
| R1 | Release qualified | CI、目标构建、HIL、文档、制品和已知限制全部满足 |

规则：

- 后一级不能自动由前一级推导。
- Host fake、synthetic timing、compile-only 不得填写为 Board/Performance/Security。
- `pending`、`rejected` 和 `unsupported` 都是有效结论，优于虚假通过。

---

## 2. 当前组件证据矩阵

| 组件 | Host | Compile/QEMU | Board | Security/Performance | 当前允许声明 | 下一证据 |
|---|---|---|---|---|---|---|
| Clib | H1 | PC build | pending | pending | Host 软件可用 | MCU size/heap/性能记录 |
| Kernel/OSAL | H1（bare-metal） | Q1 部分；[FreeRTOS reference](reference-rtos-decision.md) 已有 project-owned config、pinned V10.4.6 Cortex-M33/CM4F ports；root U5/L4 opt-in 可构建 `freertos_kernel` + `xy_osal` | Pandora STM32L475VE 已取得 scheduler/delay、task-context binary semaphore/message queue/event flags/mutex、SysTick ISR 与 TIM6 peripheral IRQ→OSAL semaphore→task、TIM6 interrupt disable→timeout→restart recovery B2、no-wait memory-pool/queue exhaustion→recovery→delete/recreate、depth-one queue 100-tick blocking timeout、2-producer/2-consumer queue 唯一性及 16 条 payload 的 consumer 归属（9/7）、120 秒 bounded stress，以及逐周期严格覆盖 OSAL→IPC Broker→PM tick→Device registry→CLIB Trace sink→IPC delivery 的跨组件 B1 | pending | bare-metal Host 契约；STM32U5 仍为 compile-only；Pandora 仅证明声明区间内的 task primitives、两个明确 ISR-to-task 路径、受控 TIM6 recovery、资源恢复、blocking-timeout、bounded 2P/2C queue、stress 与跨组件纵切，不是物理故障注入、性能、公平性、多小时耐久或完整产品 RTOS 资格 | 补其他外设 IRQ 负向/恢复、PM sleep/wakeup 与长时间耐久 |
| HAL / Storage | H1（PC；canonical W25Q128 command/error/page-boundary、FOTA adapter、RTC backup register 与 SYS IRQ control contract） | Q1 部分；通用 QSPI/RTC backup HAL API、STM32L4 QSPI/I2C/UART/internal-Flash/RTC/SYS init/deinit/IRQ backends（QSPI wrapper 自 2026-10-28 起由 root STM32L4 platform gate 编译并导出符号；此前仅 Pandora board target 编译，组件 `stm32l4xx_hal_conf.h` 缺 `HAL_QSPI_MODULE_ENABLED` 曾被误记为 API 漂移）、canonical W25Q128 Device driver 与 FOTA flash-ops adapter；PC/L4/U5 compile；WCH CH32V307 GPIO/UART/SPI/I2C/DMA/ADC/PWM/watchdog current-contract backends 与 board compile smokes；Pandora board entries 的 system init、UART/GPIO、software-I2C 与 hardware-I2C3 transports（Host contract `pandora_i2c_owners` 把两 owner 的 GPIO 事件流解码回 I2C 帧：引脚映射、START/STOP、`dev_addr << 1`、ACK/NACK 传播、mem/reg 访问与 `is_device_ready`、hw I2C3 AF4 配置与 `Instance` 先于 init 赋值）、FOTA internal-Flash、FOTA boot jump SYS deinit、RTOS/FOTA RTC backup 与 RTOS NVIC owners 已通过 canonical HAL policy/clean-build gate；递归 policy 只允许 `pandora_platform_startup.c` 直接持有三个精确 vendor clock calls，其他 board source 必须走 `xy_hal_*` | Pandora U9 W25Q128 的 JEDEC、erase、single/quad program/read、recovery 已迁移为 canonical path；hardware I2C3 transaction owner 已迁移到 STM32L4 backend；SPI1 TX DMA 已取得 completion、re-init 与 active-abort recovery B1/B2。openCH 当前仅 UART1 TX 有 bounded B1，其余新增 WCH HAL 均为 compile baseline。2026-09-06 无 RX source 实板运行按预期停在 `PANDORA_SPI_DMA_RX_ERROR`，未取得 RX/full-duplex 证据 | pending | 允许声明 Pandora canonical storage/I2C3 与 SPI TX DMA B1/B2；WCH watchdog 等新 backend 只允许声明 compile compatibility；`pandora_platform_startup.c` 是唯一 board-local vendor clock owner，精确白名单为 `HAL_PWREx_ControlVoltageScaling`、`HAL_RCC_OscConfig`、`HAL_RCC_ClockConfig`；不得将未接 RX source 的 timeout 升级为 RX/full-duplex、电气或外设响应证据 | openCH 分外设下载/UART/仪器验证；Pandora 使用已确认的物理 loopback/外设补 SPI RX/full-duplex DMA；掉电/NRST恢复 |
| Device | H1 | PC/L4/U5 build | Pandora software-I2C→HAL→Device→AHT10/AP3216C B1；AP3216C continuous `0x03` 有 244-sample bounded IR response B1。hardware I2C3（PC0/PC1）正常路径 B1 后，clean `b1c2429c` 对无人地址 `0x7F` 得到 HAL I/O failure并恢复访问 `0x1E`，取得配置回读、59 samples/27 unique、error 0 与 Flash byte-identical 回读；registry lifecycle 的 Host 契约另覆盖 open/close 共享引用、PM callback 内注销其他设备时按设备指针重新解析 slot（引用与 PM 状态不错配）、以及 `xy_device_pm_check()` 在 callback 变更注册表后仍遍历全部设备；per-device PM ops 另在空 ops 表与缺失 `set_state`/`set_wakeup` callback 时 fail-closed，不再发布未应用的电源/唤醒状态 | n/a | registry/lifecycle、I2C helper、Pandora hardware-I2C3→Device→AP3216C 正常路径 B1及单次无人地址 NACK→后续设备访问 B2；不等于总线物理故障、stuck-bus recovery、定量 ALS/PS 或完整 I2C HAL 资格 | 如需产品阈值则补受控距离/照度夹具；stuck-low/掉线须人工或安全故障夹具 |
| Display drivers | H1（SSD1306 init/refresh error propagation、GUI adapter；LCD/LED transaction contracts；ST7789 checked fill 使用 256-byte bounded buffer并传播 SPI error；RGB/BGR MADCTL 与 runtime rotation atomic-commit contract；RGB Matrix effect cadence 使用 canonical OSAL tick 与真实 elapsed interval） | PC/L4/U5 build；Pandora onboard ST7789 SPI3 board target 已链接 | 旧 `e125038e` visual B1 因 blue→green→red 顺序而撤回。2026-09-08 clean `7b07057d` 修复镜像经 write/verify/read-back byte-identical，且 Eugene 现场确认 red→green→blue→white→black 与最终象限图案，恢复固定面板/方向/图案 visual B1。2026-09-30 clean `8716d264` 13,508-byte image 经 write/verify/read-back byte-identical；WCH-Link UART 417 bytes、exact identity、90→180→270→0 rotation marker 与 final pattern 严格有序、error 0，证明四次 MADCTL checked write 被 panel path 接受并继续运行；无人值守未取得实际可见方向。SSD1306 deferred | performance pending | Pandora onboard ST7789 SPI3 固定初始方向/颜色顺序/象限图案 visual B1；四个 runtime rotation 仅有 machine-observed command-chain B1，不等于可见方向/坐标映射；RGB Matrix cadence 仅为 Host/compile ownership contract；不等于色准、亮度、刷新率、其他面板或长期可靠性 | 人工确认四方向视觉与坐标映射；SPI/DMA/刷新率及 long-run |
| Storage/24xx | H1（page split、尾页容量/16-bit 边界、I2C error propagation、ACK write-cycle polling、re-init recovery 与 failed re-init live-owner atomicity） | PC/L4/U5 build | pending | pending | fake-I2C Device→Driver 契约；page write 同时受 page 与 configured capacity 限制，写后以 bounded device-ready polling 等待内部 write cycle，timeout 时停止后续 page 且不假成功；重复初始化仅在完整 nested transport 成功后原子替换 owner | 写保护、真实 write-cycle 时序、掉电 B2 |
| Storage/SD SPI | H1（SDHC/SDSC init 与 CSD capacity、OCR power-up/2.7–3.6V compatibility、CCS/CSD identity consistency、SDSC CMD16(512) normalization、block/byte addressing、single-block read/write、verified-write、fail-closed teardown 与 transport-error propagation） | PC/L4/U5 build；Pandora SPI1/PC3 独立 probe image 可链接 | clean committed `1aa7e3e2` 10528-byte image 已 ST-Link write/verify/read-back byte-identical（SHA-256 `dc7f7d7b...dc12`）；reset-synchronized WCH-Link capture 203 bytes，exact identity、SDHC、62521344 blocks/32010928128 bytes、block0 `55AA`、strict marker order、error 0，validator 返回 `B1_PANDORA_TF_READ_PASS` | pending | callback-injected Host driver contract；Pandora 当前插入 TF 卡的 SPI1/PC3 初始化与 block0 read-only B1；`55AA` 仅为观测字节，不证明有效分区表；SDSC/verified-write/deinit 仍仅有 Host/compile evidence | 写入前须定义非破坏测试区与恢复边界；真实 SDSC、write-readback、文件系统、热插拔、性能、耐久与掉电 B2 pending |
| Sensor legacy | H1（SHT30/BH1750/MPU6050/BMP280 已降为 canonical Device owners 的 compatibility wrappers；APDS9960 顶层弱重复 owner 已移除并由 top-level/subdir overlap guard 防回归；SGP30/SGP40/ENS160/IM69D/MAX30102/AEAT-8800/MLX90393 false owners、协议不匹配的 IIS2ICLP/QMA6100、无补偿 BMP390/无 calibration INA219、无来源 DMP6100/CMS/HS-ADS1100/GD30DF owners 与浅层 VL53L1X owner 已移除；AS5048 owner 已限定为 I2C AS5048B 并按 `0xFE/0xFF` datasheet bit layout 解码；含 CCS811 factory/public ops 空总线拒绝、SHT40 序列号初始化失败原子清理、LPS22HB deinit/初始化失败传播与 Coulomb Device-helper 初始化失败传播、无寄存器副作用；LSM6DSL range setter NULL/非法枚举 fail-closed） | PC/L4/U5 build；[active-source manifest](sensor-active-source-manifest.md) 记录 40 个 root sources（39 个 `sensors/` owner + top-level ADT7420） | Pandora ICM20608 已取得在线/配置/静态采样诊断 B1；AP3216C 旧 `0x07` one-shot capture 仅诊断，修为 continuous `0x03` 后取得 static B1，并以 244 samples/139 unique/error 0、IR median 2.5→257.0 的 bounded stimulus-response B1；V2.4 原理图明确 U8=AP3216C | pending | AP3216C 芯片/寄存器/freshness 与 bounded IR response 已建立；ALS 10–11、PS 0–15，不能声明定量 ALS/PS 响应、距离阈值、精度或 calibration；ICM20608 两次动态 capture 未观察到预期响应 | AP3216C/AHT10 NACK recovery；需要产品阈值时补固定距离/照度夹具；ICM20608 动态诊断保持隔离 |
| Sensor new `xy_*` | H1（独立测试） | [manifest](sensor-active-source-manifest.md) 记录 5 个 `experimental-test-only` sources，未进入根 Sensor target；SHT30/ADS1115/MPU6050/BH1750/AHT20/SHT40/HDC1080/TSL2561/MLX90614/LPS22HB/BMI088/BNO055/BMI270 duplicate test-local implementations 已移除或迁出；BQ25620 experimental Sensor owner 及对应未引用 prototype 已删除并由 guard 禁止回归 | pending | pending | 仅 test-local Host 实验实现；不得因 focused test 宣称 product-linked；BQ25620 canonical charger owner 与 MAX17043 product-facing owner 分别保留在 standalone Charger/Fuel Gauge 组件，Pandora 无 charger IC；ADXL362/BME280/ICM20608 继续由 root-linked legacy owner 持有 | 继续冻结新增并迁移高价值 owner |
|| Drivers Sensor | H1（51 个 canonical Device sources；QMA6100P 已以 nested Device I2C owner 接入，固定地址 `0x12/0x13`、chip ID `0x90` 与 14-bit XYZ contract；Pandora active-low capture machine gate 要求 data-ready interrupt 配置寄存器 exact readback `ENABLE1/MAP1/MAP3/PIN_CONFIG/INT_CONFIG=10/10/10/00/0C`。2026-09-27 clean `142e515c` image 经烧录/同长度 byte-identical 回读后，WCH-Link capture 5,449 bytes；software-trigger delta 为 2/2，1 秒真实窗口 PC6/PD15 edge 为 262/263，40/40 sample unique且 sensor status 出现 `0x10`，因此固定 Pandora/QMA6100P 配置下 data-ready GPIO edge→EXTI callback 已取得 B1；动态响应、精度、校准、恢复与 endurance 仍 pending；INA228 I2C 与 INA229 SPI 共享 transport-neutral INA22x conversion core，ENERGY/CHARGE 仅接受 continuous bus+shunt ADC mode，诊断溢出/内存状态异常时拒绝整份 sample，首次 init 失败清零 owner、live re-init 失败保持原 transport/lifecycle，支持六类 raw alert threshold 与 staged diagnostic read，并分别保持独立 Device transport owner；LIS2DW12 已以 nested Device I2C owner提升；CCS811 已以 nested Device I2C owner提升；APDS9960 已以 nested Device I2C owner提升；LSM9DS1 已以双 nested Device I2C owner提升；LSM6DSR 已以 nested Device I2C owner提升；LSM6DSL 已以 nested Device I2C owner提升；LSM6DSO 已以 nested Device I2C owner提升；ADXL362 已以 nested Device SPI owner提升；KX023 已以 nested Device I2C owner提升；BMA400 已以 nested Device I2C owner提升；IST8310 已以 nested Device I2C owner提升；AK09918 已以 nested Device I2C owner提升；QMC5883L 已以 nested Device I2C owner提升；VCNL4040 已以 nested Device I2C owner提升；MAX44009 已以 nested Device I2C owner提升；AS5600 已以 nested Device I2C owner提升；AS5048B 已以 nested Device I2C owner提升；BMI088/BMI270/BNO055 已从 experimental 提升为 root-linked typed owner；LTC2945 已按 Analog Devices register/scaling contract 重建；SGP40 已以 nested Device I2C owner提升；LPS22HB 已迁入 root owner并保留既有 focused contract，Device-helper transport hardening pending；VL53L1X 已以 nested Device I2C owner提升；INA226 已按显式 shunt/current-LSB/calibration/current/power contract 重建并提升；HDC1080/TSL2561/MLX90614 已从 experimental 提升为 root-linked typed owners；MLX90614 保留 PEC 温度/发射率读取及 unsupported EEPROM-write 边界，并新增 fixed-address/nested-lifecycle/deinit contract；AHT10/AP3216C/ICM20608 已迁移为单一 typed owner + legacy compatibility wrapper；INA219 已按显式 shunt/current-LSB/calibration contract 重建；BMP390 使用 pinned Bosch BMP3 SensorAPI 完成 NVM calibration 与温度优先压力补偿；INA219/BMP390/HDC1080/TSL2561/MLX90614/LPS22HB/BMI088/BNO055/BMI270/AK09918/LSM9DS1/LIS2DW12 保持 hardware-pending；AHT20 已迁移为单一 typed owner + legacy compatibility wrappers，SHT40 typed owner 已迁出实验树；BME680 使用 pinned Bosch BME68x API） | PC/L4/U5 build；[manifest](sensor-active-source-manifest.md) 记录 51 个 `device-active-root` sources，均进入 `xy_drivers`；Host suite；Pandora QMA6100P `0x12`/ID `0x90`、40 个静态 sample 与 PC6/PD15 data-ready EXTI edge 已取得 bounded B1；动态响应、精度、校准、recovery 与 endurance pending |
| Actuator | H1（framework + GPIO-backed buzzer/RGB/H-bridge motor fail-safe contracts） | PC/L4/U5 build | Pandora PB2 buzzer 与 PE7/PE8/PE9 RGB 已有 control-path evidence；V2.4 原理图确认 PA1=`MOTOR_A`→IA、PA0=`MOTOR_B`→IB。motor `7c779ce4` 7420-byte image 已 write/verify/read-back byte-identical，SHA-256=`c53d349ca30ebc5bb8f5a346185ab87d546f323a77aa6f48741c2a97d97bddfe`；UART 201 bytes 通过 exact identity、forward short-short-long、最终 PA1/PA0=L/L 与 error 0 machine gate；Eugene 现场确认对应振动且最终完全停止 | safety pending | TC214B 型号及 truth table 来自用户提供资料，未独立抓取 datasheet；motor API 固定 L/L standby、方向切换先回 L/L、单步最长 5 秒；仅授予 fixed forward short-short-long 与 standby stop B1，不包含 reverse、H/H brake、PWM、current、性能或 endurance | PWM/current/重复启停耐久；reverse/brake 仅在另行定义安全夹具后验证 |
| Fuel Gauge | H1（未实现安全模式 fail-closed） | PC build | pending | AES/SHA provider pending；plaintext passthrough 已移除 | Host 驱动契约；`NONE` 明文兼容，未接入 provider 的安全模式返回 unsupported 且保持输出 | 受审查 authentication/encryption provider；SMBus B1/B2 |
| Charger | H1（standalone BQ25620 fake-I2C transaction/status contract；公开 full config 与 direct setters 在首个 I2C 写前拒绝越界或无法按寄存器步长精确表示的 input/charge/precharge/termination current、charge voltage 与 recharge threshold；16-bit little-endian setpoint paths 使用 read-modify-write 保留非目标位并传播 read failure；status/fault 依据 ZHCSMZ9C Rev. C `0x1D/0x1E/0x1F` staged decode，safety-timer expiry 映射公开 charge-timeout fault，top-off timer 编码保持 active charging 而不误报 charge done，歧义 `TS_STAT=001` fail-closed 为 unknown，ICHG/VREG/IINDPM decoded setpoints 同时拒绝低于 datasheet 最小值或高于最大值的编码；public register read 仅允许 datasheet `0x02..0x38` address range） | PC/L4/U5 build | pending | safety pending | `components/drivers/power/charger/xy_bq25620.c` 为 canonical owner；公开 API 与 focused contract 迁移后旧 `components/charger/` compatibility component 及 ownerless Device dispatch 已移除；固定 7-bit 地址 `0x6B`，ICHG/VREG/IINDPM/IPRECHG/ITERM/VRECHG 地址、位域、步长与范围已按 TI ZHCSMZ9C Rev. C 重建，fake transport 对每次 TX/RX 校验目标地址；lifecycle 仅由 canonical `xy_device_t.initialized` 与 owner cookie 持有，live-transport fail-closed、完整 status staged commit、所有 init 失败保持 caller storage、live owner re-init 失败保持原 transport/lifecycle；teardown 的 stop-charge read/write 失败传播且保留完整 live owner供重试，成功后才清理 context | 充电/热故障 B1/B2；Pandora 无 charger IC |
| Analog Devices | H1 | PC build | pending | calibration pending | active 3-source Host 契约 | MCP3008/HX711 实测与标定 |
| MUX | H1 | PC build | pending | protocol security pending | Host typed ops 可用 | Device adapter/真实跨接口验证 |
| PID | H1 | PC build | pending | performance pending | Host 算法可用 | plant simulation/HIL、抖动和饱和恢复 |
| Trace | H1 | PC build | Pandora CLIB printf sink 已在 IPC consumer task 中完成 17 次有序输出 B1 | throughput pending | Host weak sink/format contract；Pandora 仅证明 bounded UART sink 跨组件正常路径 | RTT/ITM、并发丢日志与吞吐策略 |
|| IPC | H1（MQ bounded waits 在 elapsed tick 达到 timeout 时精确返回，不额外等待一个 tick；Observer notification 使用固定 membership snapshot，callback 内 attach 延后、detach 抑制本轮后续 callback，并拒绝同一 subject 的 nested notify 以避免递归失控；Broker direct queue 按最高 priority 优先、同 priority 保持 FIFO；handler 内注销/重注册当前 server 时，本轮 processing 绑定原 owner generation 并停止，replacement owner 的新消息留待后续调用；request response 必须匹配 sequence 与 source/destination generation；OSAL delay timeout 保留 Broker timeout 语义，其他 delay failure fail-closed；server 注销或显式 clear queue 会丢弃 queued messages、按 pending count 计入 dropped，复用同一 ID 不会接收 stale payload；direct/request/ISR route IDs、pub/sub publisher 与 subscriber ID 均拒绝零值；pub/sub 要求 publisher 与 subscriber 都是当前注册 owner，未注册 publisher 在 callback/stats 变化前 fail-closed；publish 以调用开始时的 subscription 快照为成员边界，callback 内替换的 subscription 不会收到触发替换的同一条消息；callback 内 deinit/init Broker 时当前 publish 立即 fail-closed，不继续调用旧 snapshot 或把旧统计写入新 lifecycle；ISR ingress payload 绑定 Broker lifecycle 及 source/destination owner generations，replacement owner 不接收旧 ring payload） | Q1 间接/部分 | Pandora Broker producer→consumer dispatch、单调 payload、Device lookup 与 Trace sink 已完成 bounded B1；depth-2 server queue 已取得 fill→`QUEUE_FULL`→clear→后续 pipeline 恢复 B2；handler rejection 现会向调用者传播并计入 dropped，随后消息可恢复处理；TIM6 single-producer ISR ingress 已取得 12 分钟内 12 次 queue-full→192 条严格单调 delivery→recovery、task producer 非饥饿及 1387 个 IPC→PM→Device→Trace→IPC 完整周期 B2 | concurrency pending | Host 契约 + Pandora bounded task-context、queue saturation、handler rejection、generation-stamped response correlation 与 single-TIM6 ISR ingress 12-cycle stress/recovery；priority dispatch、request/response 与 owner-reuse hardening 仅为 Host/compile contract；不等于实时性、性能吞吐、多 ISR producer、任意 ISR source 或多小时资格 | 多 ISR producer/多小时 endurance stress |
| SYS | H1（timer/SM；默认系统 API fail-closed；RTC backup bounds/read-write） | PC/L4/U5 build；dedicated STM32L4 `xy_hal_sys`/`xy_hal_rtc`/`xy_hal_wdg` backends 与 Pandora board policy 已守护 reset-reason/UID/software-reset/backup-register/IWDG owner 边界 | Pandora strong backend 已取得稳定 96-bit UID B1；自动 software reset、IWDG timeout 与 ST-Link reset command 均取得 reason/recovery B2，且有 Flash write/verify/read-back 与 UART 证据 | n/a | timer/state-machine Host 契约；无 board backend 时 fail-closed；Pandora application/FOTA reset flow 使用 canonical `xy_hal_sys_*`、`xy_hal_rtc_backup_*` 与 `xy_hal_iwdg_*`，vendor RCC/UID/CMSIS reset/RTC backup/IWDG 调用仅位于 STM32L4 backend；既有 chip identity、软件复位、看门狗复位与 ST-Link external-pin reset reason 已实证 | power-loss/brownout 区分与 watchdog refresh policy 仍 pending；U5 仅补充 compile |
| DM | H1（8 目标；FS lifecycle/path/I/O/error contract，coreJSON parser/search，active `xy_json` parse/mutation/malformed-input contract；NVM newest-complete/restart/torn-append、header/payload partial-write、format partial-erase、metadata/checksum corruption recovery 与 legacy→current layout migration） | PC build | [DM 掉电记录](xinyi-dm-power-loss-validation-record.md)当前为 `HOST_INTERRUPTION_GUARDED`，真实 Flash/board pending | durability pending | Host 数据格式、FS abstraction、active JSON 与 NVM restart/corruption 契约；header/payload exhaustive byte-boundary partial-write 与 256-byte format partial-erase sweep 后可重启并重试；legacy additive checksum/magic 可读，current record 使用 CRC-8 并对 metadata/payload corruption 回退上一完整值；caller-owned storage ops 允许非映射逻辑地址 backend | 目标 Flash program-granule/erase 注入、真实 legacy image dump 与真实 Flash B2 |
| PM | H1（umbrella 公开并由 Host 编译消费完整 lifecycle/update、mode dispatch、charger state/control、Fuel Gauge state/SOH、ADC sample/helper API；PM 初始化逐项传播 ADC/charger/Fuel Gauge 错误并仅在全部成功后提交状态；周期更新传播 Fuel Gauge/charger state/自动 start-stop 错误并仅在完整成功后提交 snapshot/timestamp；PM-local Fuel Gauge reset 同步恢复 full-capacity 积分/tick 基线，以 64-bit `mA·ms` 保留亚秒更新积分，并在窄化到公开 `uint8_t` 前 clamp coulomb SOC；charger 与 PM system/Fuel Gauge 统一消费 platform-owned `xy_pm_tick_get()`，不再绕过 OSAL/HAL adapter；charger 无 board backend及 hardware init/enable/disable 失败时不提交软件成功状态，live teardown disable 失败时保留 charger/PM 状态供重试，重复 init 幂等保持 live state且不重复触发 hardware init；PM charging intent 仅在 hardware action 成功后原子提交；sleep lifecycle/state/error contract；deep-sleep/shutdown 无 backend时 fail-closed；dedicated UART evidence validator 要求 exact firmware identity、32 轮 sleep→IRQ wake→state recovery 顺序及零 error marker） | PC/L4/U5 build；STM32L4 shallow SLEEP/WFI 与 PC/U5 compatibility backend 已接入 | Pandora runtime 已证明 PM platform tick adapter 经 `XY_OSAL_AVAILABLE` 使用 canonical OSAL tick；2026-09-08 clean `278cabe8` 38828-byte image 保留 handler/PM symbols，ST-Link write/verify/read-back byte-identical（BIN/read-back SHA-256 `1bb2afff...6293`）；20 秒 WCH-Link capture 8845 bytes，最新完整 boot 严格完成 32 轮 `SLEEP_ENTER→WAKE_IRQ→SLEEP_WAKE_OK→CYCLE n`、repeat marker 一次、零 PM error，validator 返回 `B1_PM_SLEEP_REPEAT_PASS`。**该 B1 已于 2026-10-29 作废（RETIRED）**：它是在 `xy_charger_hw_init()` 仍为 `TODO` 直接 `return XY_PM_OK` 的年代取得的，`xy_pm_init()` 因此能在无 charger IC 的板上成功——测的是桩而非板子；`2dde48b6` 把 charger hardware 改为 fail-closed 后，本板 PM system sleep 按设计不可达。现行证据为 2026-10-29 `45cde0ef` 42604 B 镜像：`OSAL_PM_INIT_REFUSED -1` + `get_state`/`enter_sleep`/`wakeup` 全 `XY_PM_NOT_INITIALIZED`（fail-closed 边界成立），随后 HAL 层 shallow SLEEP/WFI 32 轮且 tick 前进，validator 返回 `B1_PM_CHARGER_FAILCLOSED_HAL_SLEEP_PASS`（7444 B、errors=0） | power pending | Host framework/fallback；public ACTIVE/SLEEP dispatch 委托受检状态转换；charger API 在无 backend 或硬件动作失败时 fail-closed，PM enabled intent 与 teardown live state 保留上一稳定状态，state/status alias 仍返回 canonical snapshot；public ADC sample API 提供 NULL guard 与毫伏输出；DEEP_SLEEP/SHUTDOWN 无 backend时明确 unsupported；Pandora 仅证明 charger fail-closed 边界成立且 HAL 层 shallow SLEEP/WFI 可重复 32 轮（`B1_PM_CHARGER_FAILCLOSED_HAL_SLEEP_PASS`，2026-10-29）；PM system sleep/wake 在本板按设计不可达，旧 32-cycle PM B1 已作废；不等于 STOP/STANDBY/SHUTDOWN、功耗、wake latency、多小时耐久、charger/ADC 实板或 U5 runtime | 补电流、低功耗深度、wake latency 与多小时 endurance；charger/ADC 仅在产品硬件接入后验证 |
| Crypto | H1（契约；SHA-256/HMAC zero-length 与工作状态清理；LWC 仅 focused-test） | PC/L4/U5 Crypto compile | Pandora STM32L475VE 软件路径已完成 SHA-256 `abc`、HMAC-SHA256 quick-brown-fox、AES-128 FIPS-197 单块 encrypt/decrypt KAT 与 1000 次重复运行；clean `35beb5bb` image 10724 bytes，ST-Link write/verify/read-back byte-identical（SHA-256 `3b54b73e...ec7d`），reset-synchronized WCH-Link capture 232 bytes、零 error marker，validator 返回 `B1_CRYPTO_SOFTWARE_KAT_PASS` | product-classification/owner/origin/license/side-channel/allowed-use 清单已机器守护；provenance 均 review-pending；SM2/ECDSA rejected；Ascon/TinyJAMBU/Photon-Beetle 因 provenance/KAT 未闭环而从 root runtime 隔离 | contract-guarded；SHA-256/HMAC/AES 软件实现具有限定 L4 B1 runtime KAT，不等于安全、constant-time、provenance、性能、硬件加速或合规批准；LWC 仍仅允许 focused-test experimentation | SHA-256/HMAC/AES 外部 provenance/license、独立审计、fuzz/side-channel 与性能；LWC authoritative KAT/provenance review；reviewed signature provider |
| GUI | H1（backend 错误传播；SDL fake + real-library headless contract；字体 source-table review；licensed required-UI subset active） | PC build；SDL2 opt-in 缺依赖 fail-closed；canonical CI 用真实 SDL2 headers/library 编译链接 backend，并以 dummy video driver 实跑 window/renderer/texture/fill/flush/event/deinit；fake seam 覆盖错误路径 | pending | legacy 字体视觉 `rejected-needs-regeneration` 且 provenance pending；OFL-1.1 Noto Sans CJK SC 的 15 个 required UI glyph 已按 pinned Host snapshot 接入 active 16x16 table，但尚未视觉/实板批准；performance pending | Host GUI/font/backend contract；SDL headless runtime 仅为 PC runtime evidence，不是人工视觉、性能或屏幕硬件证据；required UI subset 为 distinct/nonblank active table；其余 legacy 16x24/中文 placeholder 不得作为最终产品字体 | 人工视觉审查；替换其余 legacy table；屏幕 B1/P1 |
| Net | H1（core/Modbus/MQTT/AT/CAN/LTE contracts；产品协议 default-off） | PC root；全协议显式 opt-in `xy_net` target | pending | long-run pending | root Kconfig 直接控制 active source/umbrella export；active AT owner 为 lightweight client/server，active MQTT owner 为 `src/xy_mqtt_client.c`；vendor AT trees、legacy MQTT、CAN/LTE 不会因文件存在自动进入产品库 | 明确 modem/CAN 产品选择后补 UART/flow-control/power、attach/PDP/URC/controller 与 B1/B2 |
| Wireless / NRF24L01 | H1（SPI register read/write/restore、floating-bus 与 mismatch fail-closed；PTX ACK/MAX_RT 与 MAX_RT cleanup transport error propagation；identity-bound UART validator 分离 acknowledged PTX、PRX timeout、valid RX payload 与 error/incomplete outcomes） | PC/L4/U5 compile；Pandora SPI2 probe clean link/符号/固件身份 | clean committed `35438285` 11524-byte image 已 ST-Link write/verify/read-back byte-identical；35 秒 WCH-Link capture 337 bytes，exact firmware identity、完整 register snapshot 与 `NRF24_TX_ACK_OK retries=01`；随后 30 秒 PRX 窗口明确 `NRF24_RX_TIMEOUT`，machine validator 返回 `B1_NRF24_ACKNOWLEDGED_TX_PASS`、`rx_outcome=TIMEOUT_NO_PAYLOAD` | RF/performance pending | 允许声明 Pandora onboard NRF24 SPI2 register presence 与一次固定 32-byte payload 的 acknowledged PTX B1；本轮没有接收 payload，ACK 证明兼容接收端响应但 peer identity/payload receipt 未被独立日志绑定，不等于 RX、IRQ transition、range、throughput、recovery 或 endurance | 增加独立已知接收端的 payload/identity 日志，再做 RX、IRQ 与 bounded recovery |
| FOTA | H1（状态机、candidate envelope/tool、source-commit-bound reviewed restage、signature-provider/boot-confirm fail-closed；双记录 journal 与 durable attempt/confirm/rollback；Pandora board-owned internal-Flash adapter contract） | PC/FOTA target；L4/U5 compile compatibility；Pandora bootloader `0x08000000`、application `0x08008000` 与 opt-in candidate programmer 独立链接 | Pandora 一次性 programmer 已将 confirmation-capable `e73254da` candidate（33916 bytes；SHA-256 `507d1610...`）写入 W25Q128 并逐块回读；resident bootloader 随后完成 `INSTALLED→ATTEMPT_COMMITTED→CONFIRM_REQUESTED→CANDIDATE_CONFIRMED→CONFIRM_ACKNOWLEDGED`，且软件复位后 journal 命中同 candidate、跳过重复擦写。14416-byte bootloader ST-Link write/verify/read-back byte-identical；25 秒独立 UART capture SHA-256 `5f3dab7c...`，多轮有序链路无 FOTA error marker | Secure FOTA blocked；无 approved provider | Pandora candidate 安装/执行 B1、软件复位幂等与 durable confirmation B2；Host 证明 malformed candidate/source commit、无效授权和 Flash failure fail-closed，`pandora_fota_flash`/`pandora_fota_install_flash` 的地址范围、8-byte program granule、metadata 双槽 erase 与 init→unlock→write/erase→lock 顺序在 fake `xy_hal_flash_*` 下由真实双记录 journal core 驱动验证；bootloader 决策序列自 2026-10-28 起由 board-owned `pandora_fota_boot_flow.c` 承载并有 17 例 Host 契约（顺序、失败短路、marker 次序），同镜像两次实板引导观察到 `INSTALLED`/`ALREADY_INSTALLED`→`ATTEMPT_COMMITTED`→`JUMP_APP`，但 confirm 分支仍只有 Host 证据。该证据不等于真实掉电/半写、签名安全或量产 updater；2026-10-28 起 `tools/pandora_hil_state.py` 把 bootloader/app/journal/board_reserved 四段 flash 与 RTC backup 变成可快照/比对/恢复的对象（34 例 Host 覆盖 + 链路脚本 `make test-tools`；实板 528 KiB 三遍独立读哈希一致、注入单字节被定位到 `+1024` 并 rc=1、restore 四段回读全等、plan 零写入），并由 `tools/pandora_hil_program.py` 把"写入前必须有已核验的当前快照"变成写入前置条件（36 例 Host、10 种变异全拦；实板写前校验拒绝陈旧快照 exit 2 且零写入、读回 span 全等、`erased_tail=2024` 实证 2 KiB 页擦除、`--reset` 后逐段报告漂移、restore 后 verify 5/5 identical），2026-10-28 起 `PANDORA_BOOT_*` 引导链由 `boards/pandora_stm32l475/validate_fota_bootloader_capture.py`（43 例 Host、CTest `pandora_fota_bootloader_capture_validator`）按 boot frame 判序并对 journal 两槽做 CRC 解码；同一实板把 attempt 阈值走满：`ATTEMPT_COMMITTED`（journal `INSTALLED gen3/attempts=1` + `gen4/attempts=2`）→ 第 3 次 attempt 触发 `ROLLBACK_REQUIRED`（**无** `JUMP_APP`，journal `ROLLED_BACK gen5 attempts=3` 持久）→ 下一次引导只吐 `INSTALL_ERROR` 且 journal 与回滚时逐字节相同（`XY_FOTA_VERSION_ERROR` 拒绝、零写入零擦除），末尾 restore + `verify_ok regions=5`。2026-10-29 起 W25Q128 candidate programmer 的 marker 流由 `boards/pandora_stm32l475/validate_fota_candidate_programmer_capture.py`（19 例 Host、7 种源码变异全拦、CTest `pandora_fota_candidate_programmer_capture_validator`）按 `READY → PROGRAMMER_COMMIT → CANDIDATE_SOURCE_COMMIT → PROGRAMMED_VERIFIED` 判序并分别绑定 programmer 与 envelope 两个 commit 身份，截断帧/错误 marker/未知 `PANDORA_FOTA_CANDIDATE_*`/帧内外来输出一律 fail-closed；同一实板该镜像（53044 B，链接在 `0x08000000`，按区域切成 32 KiB + 20276 B 两段分别门控写入、各自 `span_identical=true`）打印 211 B 有序四行、每 256 B 回读校验通过（`PROGRAMMER_CAPTURE_REVIEW_CANDIDATE`），随后 v2 candidate 由 resident bootloader 装到 `0x08008000` 并 `ATTEMPT_COMMITTED`→`JUMP_APP`。**confirm 分支仍未上板**：该轮捕获窗口与并发会话对同一块板的烧录重叠（`149ccfa7` 于窗口内提交），被 jump 的 application 输出中途停止、RTC slot 0/1 全程为 0，观测无法归属，按证据纪律不主张应用 runtime 结论；窗口后的一次普通复位仍复现 owner 化 flow 的 `ALREADY_INSTALLED`→`ROLLBACK_REQUIRED`（无 `JUMP_APP`，`chain=ROLLBACK`）与 journal `ROLLED_BACK gen5 attempts=3`。只解决烧录前后状态可逆性与写入授权，不改变本节其余证据边界 | 真实掉电/半写；reviewed provider/key provisioning |
| CI/Release | canonical Host gate + PC root build 可用（2026-09-01：200/200）；首个 release input `device_driver_template` 已从 `git archive HEAD` clean export 通过独立 configure/build/run；canonical Host committed clean-export gate 从 `git archive HEAD` 独立 configure/build，并在排除 5 个依赖 Git 仓库状态或递归 archive/build 的 policy tests 后通过 195/195；canonical CI 已将常规 Host、committed clean-export 与 PC artifact reproducibility 分步设为必跑；同一 committed source archive 的两次独立 PC Release `xy_device` 构建产出相同 `libxy_device.a` SHA-256/size；[PC release build environment](pc-release-build-environment.json) 固定 Ubuntu 24.04 runner 与 PC/x86_64/Release/config/tool identity contract，[PC release artifact manifest](pc-release-artifact-manifest.json) 将当前选定 artifact set 限定为 `xy_device` / `libxy_device.a` 的 reproducibility gate-only 项；gate 输出每次实际 CMake/CC/AR/Python identity，生成/上传含 source commit/archive hash、artifact-manifest schema/status、双构建 artifact hash/size 与工具身份的机器 JSON evidence，将验证后的 library、SHA-256、CycloneDX JSON 1.6 SBOM、ephemeral Ed25519 signature/public key 及 bounded license/NOTICE review records 作为 14 天 bounded CI artifact 归档，并以独立调用重读校验 checksum/SBOM/signature 与 legal-pending/source-scope 边界；SBOM 绑定 exact artifact/source archive/source commit 与 10 个直接编译源的 SHA-256 和 Apache-2.0 evidence，状态保持 `REVIEW_PENDING`；[bounded technical license review](pc-release-license-review.json) 已记录这 10 个 first-party source、根许可证 hash 与无冲突文件级声明的扫描结果，但 legal/NOTICE/完整 release scope 仍 pending；临时私钥不归档且每次运行丢弃；[release signing policy](release-signing-policy.json) 固定 Ed25519 设计边界，但 release identity 仍为 `UNASSIGNED`、key custody 为 `NOT_ESTABLISHED`、publication 为 `BLOCKED`，ephemeral CI key 不得升级为 release key；[PC release SBOM policy](pc-release-sbom-policy.json) 固定 bounded artifact 的生成/独立验证边界，但不构成法律/license approval、完整 PC/MCU SBOM 或 R1 | PC build；[Kconfig/CMake 配置矩阵](kconfig-cmake-configuration-matrix.md)已建立，all-off 配置不再泄漏关闭组件 target，Device/Crypto/DM/Sensor/Actuator-only 组合的生成值、focused target 与归档对象已验证；Sensor/Actuator 的两个 framework 开关分别映射到各自相同 root target，独立 Device-driver 路径未混入；非法 Display 子功能组合 fail-closed，OLED/SSD1306、LCD SPI/I8080/ST7789 与 LED/serial RGB 合法组合已验证 source selection；Net core 与 MQTT/AT/CAN/LTE 选择均 default-off，显式全协议组合的 `xy_net` target 已验证；无实现源的 standalone RGB 配置已移除；STM32U5 默认组合曾用 `arm-none-eabi-gcc` clean compile，并验证 STM32U5 与 FS/FlashDB 条件默认值 | n/a | [tracked source dependency inventory](source-dependency-inventory.json) 为机器守护的 `REVIEW_PENDING` 前置；bounded artifact SBOM generation 已 guarded，法律/license approval 仍 pending | development CI only；`unit-tests.yml` 为单一可信 ...

---

### Sprint 6 pre-RC artifact evidence (2026-09-30)

- Pandora STM32L475VE is the sole reference board; STM32U5 remains enhancement compile-only.
- The frozen pre-RC MCU build-gate input is `pandora_stm32l475_rtos.bin` at `0x08000000`; no examples/projects are selected for support.
- Two independent clean-export STM32L4 Release builds from current `git archive HEAD` (`212df50192baa787aea9d6e0e0eadd1a8e4e6f57`) plus pinned STM32CubeL4 commit `9203b3843e219d2025a7f868d7656b5a5d208885` produced identical 39260-byte BINs with SHA-256 `052b81842e506457ad2531589cd73fa6910eaacd70c5e13abe635ae6a8b09d9b`.
- The BIN and `.sha256` pair were independently re-read and verified. This is compile-reproducibility and checksum evidence only; it does not establish MCU SBOM, signature, CI publication, flash/install, runtime, RC, or R1 qualification.
- The explicit opt-in `tests/unit/check_pandora_release_sbom.py` generator and verifier produced
  `build/pandora-release-sbom.json` with 39 linked source components from the RTOS link map and
  linked FreeRTOS/CLIB archive hashes. Status remains `REVIEW_PENDING`; this is bounded input
  inventory evidence only and does not establish complete MCU dependency coverage, legal approval,
  security approval, runtime qualification, RC, or R1.

---

## 3. 证据记录索引

### 2026-10-13 IPC Observer detach/reattach generation boundary

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host notification-membership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.{h,c}`、
  `tests/unit/ipc/test_ipc_observer.c`、`components/ipc/README.md`；focused `ipc_observer`；
  `make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：notification callback 内 detach 后重新 attach 的同一 callback/user-data registration
  取得新的 attachment generation，因此不会在原 notification snapshot 中被错误调用；下一轮可正常调用。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer reentrant-clear boundary

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host reentrancy contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、
  `tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；
  PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：subject notification callback 内的 `xy_subject_clear()`/`xy_subject_deinit()`
  返回 `XY_OBSERVER_ERROR`，不会清空 active snapshot、提前结束本轮 callbacks 或破坏
  `notifying` 状态；notification 结束后仍可正常清理。
- 仍不允许宣称：跨线程并发安全、不同 subject 的 notification ordering、ISR safety、Pandora
  runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC Observer nested-notification boundary

- 组件：IPC / Observer
- 旧等级 -> 新等级：H1/Host reentrancy contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/observer/xy_observer.c`、
  `tests/unit/ipc/test_ipc_observer.c`；focused `ipc_observer`；`make test-unit`；
  PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：同一 subject 正在通知时，callback 发起的 nested `xy_subject_notify()` 返回
  `XY_OBSERVER_ERROR`，不会递归重入 callback 或破坏外层 snapshot；外层结束后可再次通知。
- 仍不允许宣称：跨线程并发安全、不同 subject 的 notification ordering、ISR safety、Pandora
  runtime、throughput、recovery 或 endurance。

### 2026-10-13 IPC MQ allocation-failure atomicity

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host initialization-failure contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`、
  `tests/unit/CMakeLists.txt`；focused `ipc_mq`；`make test-unit`；PC/L4/U5 Release builds；
  `git diff --check`。
- 适用范围：首次初始化先完成内部 storage allocation，再发布 config/buffer/lifecycle owner；
  allocation failure 返回 `XY_MQ_NO_MEM` 并保持完整 caller-owned queue storage 不变。
- 仍不允许宣称：跨线程并发安全、heap exhaustion recovery policy、Pandora runtime、ISR safety、
  throughput、fairness 或 endurance。

### 2026-10-13 IPC MQ negative-priority validation

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host input-validation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；
  focused `ipc_mq`；`make test-unit`；PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：send entry points reject enum values whose unsigned representation lies outside the
  public `LOW..URGENT` domain before waiting, queue mutation, statistics updates, or payload writes.
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、priority fairness、throughput
  或 endurance。

### 2026-10-13 IPC MQ urgent eviction ordering

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host priority overflow contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；
  focused `ipc_mq`; `make test-unit`; PC/L4/U5 Release builds；`git diff --check`。
- 适用范围：启用 priority 的满队列接收 urgent message 时，淘汰当前队列中最早入队的最低
  priority message，而不是无条件淘汰 FIFO oldest；剩余同 priority 顺序、urgent 插入与
  send/drop statistics 保持一致。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、priority fairness、throughput
  或 endurance。

### 2026-10-13 IPC MQ priority receive ordering

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host priority contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/{README.md,src/xy_mq.c}`、
  `tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`; `make test-unit`; PC/L4/U5 Release builds；
  `git diff --check`。
- 适用范围：`priority_enabled=true` 时 receive 从当前队列选择最高 priority，且同 priority
  保持 FIFO；从环形队列中间移除后压紧剩余 slots，并覆盖 wrap-around 后继续 send/receive。
  `priority_enabled=false` 仍保持原 strict FIFO。
- 仍不允许宣称：跨线程并发安全、ISR safety、Pandora runtime、priority fairness、throughput
  或 endurance。

### 2026-10-13 IPC MQ copied-owner isolation

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/{inc/xy_mq.h,src/xy_mq.c}`、
  `tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`; `make test-unit`; PC/L4/U5 Release builds；
  `git diff --check`。
- 适用范围：live queue 绑定其原始 `xy_mq_t` owner 地址；结构体浅拷贝不能复用同一 heap
  buffer 执行 send/receive/query/clear/stats/deinit，拒绝路径保持 alias、原 owner、payload 与
  caller output 不变，避免 double-free 和共享索引破坏。
- 仍不允许宣称：跨线程并发安全、owner 移动、ISR safety、Pandora runtime、throughput、fairness
  或 endurance。

### 2026-10-12 IPC MQ priority-domain validation

- 组件：IPC / message queue
- 旧等级 -> 新等级：H1/Host input-validation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`; `make test-unit`; PC/L4/U5 Release builds；`git diff --check`
- 适用范围：send entry points reject priority values outside the public `LOW..URGENT` enum before waiting, queue mutation, statistics updates, or payload writes; valid priorities retain existing FIFO/urgent behavior.
- 仍不允许宣称：priority scheduling、lock-free/thread-safe queue semantics、ISR safety、Pandora runtime、throughput、fairness 或 endurance。

### 2026-10-12 IPC MQ active-owner validation

- 组件：IPC / message queue
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`; `make test-unit`; PC/L4/U5 Release builds；`git diff --check`
- 适用范围：all send/receive/query/clear/stat entry points require the exact lifecycle cookie, initialized state, and owned storage before reading queue configuration or touching storage; forged `initialized=true` state fails closed and preserves caller-owned queue/output/stat storage.
- 仍不允许宣称：lock-free/thread-safe queue semantics、ISR safety、Pandora runtime、throughput、fairness 或 endurance。

### 2026-10-12 IPC MQ lifecycle ownership

- 组件：IPC / message queue
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/{inc/xy_mq.h,src/xy_mq.c}`、`tests/unit/ipc/test_ipc_mq.c`；focused `ipc_mq`; `make test-unit`; PC/L4/U5 Release builds；`git diff --check`
- 适用范围：live queue rejects repeated initialization without leaking/replacing storage; inactive/double deinit fails closed and preserves owner state; a lifecycle cookie prevents arbitrary uninitialized bytes from being treated as a live owner while preserving existing first-init behavior.
- 仍不允许宣称：lock-free/thread-safe queue semantics、ISR safety、Pandora runtime、throughput、fairness 或 endurance。

### 2026-10-02 VL53L0X Device re-init atomicity

- 组件：Drivers Sensor / VL53L0X
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_vl53l0x_device.c`; focused `sensor_vl53l0x_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, transport, and identity failures; first initialization failures remain fail-closed.
- 仍不允许宣称：VL53L0X board identity, range accuracy, timing, recovery, or endurance.

### 2026-10-02 VL53L1X Device re-init atomicity

- 组件：Drivers Sensor / VL53L1X
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_vl53l1x.c`; focused `sensor_vl53l1x`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization validates live nested transport and preserves a live owner across identity/configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：VL53L1X board identity, range accuracy, timing, recovery, or endurance.

### 2026-10-02 VCNL4040 Device re-init atomicity

- 组件：Drivers Sensor / VCNL4040
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_vcnl4040_device.c`; focused `sensor_vcnl4040_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper and transport failures; first initialization failures remain fail-closed.
- 仍不允许宣称：VCNL4040 board identity, optical response, distance calibration, recovery, or endurance.

### 2026-10-02 HMC5883L Device re-init atomicity

- 组件：Drivers Sensor / HMC5883L
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_hmc5883l.c`; focused `sensor_hmc5883l`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：HMC5883L board identity, magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 AHT20 Device re-init atomicity

- 组件：Drivers Sensor / AHT20
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_aht20.c`; focused `sensor_aht20`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, transport, busy-status, and final-status failures; first initialization failures remain fail-closed.
- 仍不允许宣称：AHT20 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-02 MLX90614 Device re-init atomicity

- 组件：Drivers Sensor / MLX90614
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_mlx90614.c`; focused `sensor_mlx90614`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper and identity failures; first initialization failures remain fail-closed.
- 仍不允许宣称：MLX90614 board identity, temperature accuracy, emissivity calibration, recovery, or endurance.

### 2026-10-02 AS5600/AS5048B Device re-init atomicity

- 组件：Drivers Sensor / AS5600 + AS5048B
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_as5600_device.c`, `tests/unit/sensor/test_as5048b_device.c`; focused `sensor_as5600_device`, `sensor_as5048b_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper failures; first initialization failures remain fail-closed.
- 仍不允许宣称：angle encoder board identity, angle accuracy, magnetic installation, recovery, or endurance.

### 2026-10-02 SHT30 Device re-init atomicity

- 组件：Drivers Sensor / SHT30
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_sht30_device.c`; focused `sensor_sht30_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, transport, and reset-command failures; first initialization failures remain fail-closed.
- 仍不允许宣称：SHT30 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-03 LSM9DS1 Device re-init atomicity

- 组件：Drivers Sensor / LSM9DS1
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lsm9ds1_device.c`; focused `sensor_lsm9ds1_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged dual-transport initialization preserves a live owner across IMU or magnetometer helper failures; first initialization failures remain fail-closed; magnetometer identity uses its canonical WHO_AM_I register.
- 仍不允许宣称：LSM9DS1 board identity, motion/magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 KX023 Device re-init atomicity

- 组件：Drivers Sensor / KX023
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_kx023_device.c`; focused `sensor_kx023_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across transport, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：KX023 board identity, motion accuracy, dynamic response, recovery, or endurance.

### 2026-10-03 QMC5883L Device re-init atomicity

- 组件：Drivers Sensor / QMC5883L
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_qmc5883l_device.c`; focused `sensor_qmc5883l_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：QMC5883L board identity, magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 IST8310 Device re-init atomicity

- 组件：Drivers Sensor / IST8310
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_ist8310_device.c`; focused `sensor_ist8310_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across transport, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：IST8310 board identity, magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 BME680 Device re-init atomicity

- 组件：Drivers Sensor / BME680
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_i2c2_sensor_devices.c`; focused `sensor_i2c2_devices`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner when the nested I2C helper fails during re-init; first initialization failures remain fail-closed.
- 仍不允许宣称：BME680 board identity, environmental accuracy, gas-response quality, recovery, or endurance.

### 2026-10-03 CCS811 Device re-init atomicity

- 组件：Drivers Sensor / CCS811
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_ccs811_device.c`; focused `sensor_ccs811_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner when the nested I2C helper fails during re-init; first initialization failures remain fail-closed and transport readiness rejects non-binary initialized state.
- 仍不允许宣称：CCS811 board identity, eCO2/TVOC accuracy, warm-up quality, recovery, or endurance.

### 2026-10-03 AHT10 Device re-init atomicity

- 组件：Drivers Sensor / AHT10
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_env_i2c_sensors.c`; focused `sensor_env_i2c_sensors`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across nested I2C helper and init-command failures; first initialization failures remain fail-closed.
- 仍不允许宣称：AHT10 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-03 APDS9960 Device re-init atomicity

- 组件：Drivers Sensor / APDS9960
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_apds9960_device.c`; focused `sensor_apds9960_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across nested transport, identity, and enable-write failures; first initialization failures remain fail-closed and require an exact live nested transport.
- 仍不允许宣称：APDS9960 board identity, optical response, gesture classification, timing, recovery, or endurance.

### 2026-10-03 LSM6DSO Device re-init atomicity

- 组件：Drivers Sensor / LSM6DSO
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lsm6dso_device.c`; focused `sensor_lsm6dso_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged initialization preserves a live owner when nested transport or identity/configuration fails; first initialization failures remain fail-closed.
- 仍不允许宣称：LSM6DSO board identity, motion accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 SC7A22H Device re-init atomicity

- 组件：Drivers Sensor / SC7A22H
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_sc7a22h.c`, `tests/unit/sensor/test_i2c2_sensor_devices.c`; focused `sensor_sc7a22h`, `sensor_i2c2_devices`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner when the nested I2C helper fails; first initialization failures remain fail-closed and preserve the existing public contract.
- 仍不允许宣称：SC7A22H board identity, motion accuracy, FIFO, interrupt, recovery, or endurance.

### 2026-10-03 HDC1080 Device re-init atomicity

- 组件：Drivers Sensor / HDC1080
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_hdc1080.c`; focused `sensor_hdc1080`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across nested helper and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：HDC1080 board identity, temperature/humidity accuracy, recovery, or endurance.

### 2026-10-04 SHT40 Device re-init atomicity

- 组件：Drivers Sensor / SHT40
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_sht40.c`; focused `sensor_sht40`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged live-owner preservation covers helper, incomplete transport, serial-command/read, and CRC failures; first initialization failures remain fail-closed.
- 仍不允许宣称：SHT40 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-04 BMI270 Device re-init atomicity

- 组件：Drivers Sensor / BMI270
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_bmi270.c`; focused `sensor_bmi270`; full Host/PC/STM32L4/STM32U5 gate pending
- 适用范围：staged candidate initialization preserves a live owner when reset, identity, or configuration fails; first initialization failures remain fail-closed.
- 仍不允许宣称：BMI270 board identity, motion accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 LSM6DSL/LSM6DSR Device re-init atomicity

- 组件：Drivers Sensor / LSM6DSL + LSM6DSR
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lsm6dsl_device.c`, `tests/unit/sensor/test_lsm6dsr_device.c`; focused `sensor_lsm6dsl_device`, `sensor_lsm6dsr_device`; full Host/PC/STM32L4/STM32U5 gate pending
- 适用范围：staged candidate initialization preserves a live owner across nested transport, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：LSM6DSL/LSM6DSR board identity, motion accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-04 BMA400/KX023/LSM6 Device re-init atomicity

- 组件：Drivers Sensor / BMA400 + KX023 + LSM6 family
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_bma400_device.c`, `test_kx023_device.c`,
  `test_lsm6dso_device.c`, `test_lsm6dsl_device.c`, `test_lsm6dsr_device.c`;
  focused CTests `sensor_bma400_device`, `sensor_kx023_device`,
  `sensor_lsm6dso_device`, `sensor_lsm6dsl_device`, `sensor_lsm6dsr_device`;
  `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged candidate initialization preserves a live owner across nested transport,
  identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：board identity, motion accuracy, calibration, dynamic response, recovery,
  or endurance for these sensors.

### 2026-10-04 ADXL362 Device re-init atomicity

- TDD RED：live owner 重复 `xy_adxl362_init()` 在 SPI transport、identity 或 configuration 失败时会先清空原 transport、lifecycle 与 sample cache。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 identity/filter/power 配置成功后才原子替换。
- 验证：focused `sensor_adxl362_device` 1/1；Host `make test-unit` 264/264；PC、STM32L4、STM32U5 build 与 `git diff --check` 通过。
- 边界：仅为 Host/compile lifecycle contract，不升级 ADXL362 实板身份、运动精度、动态响应、恢复或 endurance 证据。

### 2026-10-04 ADS1115 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ads1115_init()` 在 nested I2C helper 或 configuration read failure 时会先清空原 transport、lifecycle 与 cached conversion state。
- 收口：初始化改用 staged candidate；首次失败清零未初始化 owner，live owner 失败保持原 owner，完整 configuration read 成功后才原子替换。
- 验证：focused `sensor_ads1115`; Host 265/265; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 ADS1115 实板身份、转换精度、校准、恢复或 endurance 证据。

### 2026-10-04 AK09918 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ak09918_init()` 在 transport、身份或配置失败时会先清空原 transport、lifecycle 与 sample cache。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 identity/reset/configuration 成功后才原子替换。
- 验证：focused `sensor_ak09918_device` 1/1；Host `make test-unit` 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check` 通过。
- 边界：仅为 Host/compile lifecycle contract，不升级 AK09918 实板身份、磁场精度、校准、动态响应、恢复或 endurance 证据。

### 2026-10-04 INA219 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ina219_init()` 在 helper/configuration 写失败时会先清空原 transport、lifecycle 与 sample。
- 收口：初始化改用 staged candidate；首次失败清零未初始化 owner，live owner 失败保持原 owner，完整配置成功后才原子替换。
- 验证：focused `sensor_power_monitors`；Host 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 INA219 实板身份、计量精度、校准、恢复或 endurance 证据。

### 2026-10-04 LTC2945 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ltc2945_init()` 在 helper、STATUS 读取或配置写失败时会先清空原 transport、lifecycle、配置与 sample cache。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 STATUS/configuration 成功后才原子替换。
- 验证：focused `sensor_adc_power_monitors`；Host 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 LTC2945 实板身份、计量精度、校准、告警、电气、恢复或 endurance 证据。

### 2026-10-04 BNO055 Device re-init atomicity

- TDD RED：live owner 重复 `xy_bno055_init()` 在 reset、身份或配置失败时会先清空原 transport、lifecycle 与模式/单位状态。
- 收口：初始化改用 staged candidate；首次失败清零未初始化 owner，live owner 失败保持原 owner，完整 identity、firmware、mode 与 unit 配置成功后才原子替换。
- 验证：focused `sensor_bno055`；Host/PC/STM32L4/STM32U5 gate 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 BNO055 实板身份、融合精度、动态响应、恢复或 endurance 证据。

### 2026-10-06 Device owner re-init atomicity continuation

- 组件：Drivers Sensor / LPS22HB、MPU6050、BMI088、INA219、QMA6100P
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lps22hb.c`, `test_mpu6050.c`,
  `test_bmi088.c`, `test_adc_power_monitors.c`, `test_qma6100p.c`; focused CTests
  `sensor_lps22hb`, `sensor_mpu6050`, `sensor_bmi088`, `sensor_adc_power_monitors`,
  `sensor_qma6100p`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`.
- 适用范围：live owners preserve transport/lifecycle/cache across nested-helper,
  identity, status/configuration, and re-init failures; successful candidates commit
  atomically and first initialization failures remain fail-closed.
- 验证结果：Host 265/265、PC/STM32L4/STM32U5 builds and `git diff --check` passed.
- 仍不允许宣称：board identity, measurement/motion accuracy, calibration,
  interrupt/electrical behavior, recovery, endurance, or security approval.

### 2026-10-04 INA228/INA229 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ina228_init()` / `xy_ina229_init()` 在身份失败时会先清空原 transport、lifecycle、配置与 sample 状态。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 identity/configuration 成功后才原子替换。
- 验证：focused `sensor_ina228_device`、`sensor_ina229_device`；Host 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 INA228/INA229 实板身份、计量精度、校准、告警、电气、恢复或 endurance 证据。

### 2026-10-07 24xx EEPROM Device re-init atomicity

- 组件：Storage/24xx EEPROM
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/storage/eeprom/24xx/xy_eeprom_24xx.c`,
  `tests/unit/storage/test_storage_eeprom_24xx.c`; focused `storage_eeprom_24xx`;
  `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged nested-I2C initialization preserves the existing live owner when a
  replacement helper initialization fails; first initialization failures remain fail-closed,
  incomplete helper success is rejected, and successful re-init commits the complete candidate.
- 仍不允许宣称：24xx board presence, write-protect behavior, real write-cycle timing,
  power-loss recovery, endurance, or EEPROM hardware qualification.

### 2026-10-07 W25Q128 Device re-init atomicity

- 组件：HAL / Storage / W25Q128
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/storage/flash/w25q128/xy_w25q128.c`,
  `tests/unit/storage/test_w25q128.c`; focused `storage_w25q128`; `make test-unit`;
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged JEDEC probing preserves a live QSPI/Device owner on failed re-init;
  successful re-init atomically replaces transport and name without attempting duplicate
  registration of the same embedded Device object; initial registration failure restores caller state.
- 仍不允许宣称：W25Q128 board identity, erase/program recovery, power-loss safety,
  performance, endurance, or Flash hardware qualification.

### 2026-10-07 nRF24L01 failed re-probe owner atomicity

- 组件：Wireless / NRF24L01
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/wireless/nrf24l01/xy_nrf24l01.c`,
  `tests/unit/net/test_nrf24l01.c`; focused `wireless_nrf24l01`; `make test-unit`;
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：a failed replacement probe preserves the complete live radio owner, including
  transport callbacks, probed register snapshot, PRX payload width, and lifecycle state; the
  staged candidate is published only after the complete probe succeeds.
- 仍不允许宣称：real peer RX, IRQ edge behavior, RF recovery, range, throughput, endurance,
  or radio hardware qualification.

### 2026-10-07 RGB Matrix effect tick ownership

- 组件：Display / RGB Matrix
- 旧等级 -> 新等级：H1/Host timing contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/display/led_drivers/serial_rgb/xy_rgb_matrix.c`,
  `tests/unit/display/test_display_rgb_matrix.c`; focused `display_rgb_matrix`; `make test-unit`;
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：effect cadence consumes canonical `xy_os_tick_get()` and updates only after the
  configured elapsed interval, including deterministic pre-boundary and boundary checks.
- 仍不允许宣称：WS2812 electrical timing, animation frame-rate accuracy, visual quality,
  performance, endurance, or display hardware qualification.

### 2026-10-07 Device PM idle-time ownership

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host timing contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `components/device/inc/xy_device_pm.h`, `tests/unit/device/test_device_pm.c`; focused
  `device_pm`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`.
- 适用范围：idle timeout 与 activity timestamps 消费 canonical `xy_device_get_tick()`；
  timeout 边界前保持 ACTIVE，达到边界时经 driver PM callback 进入 SLEEP。
- 仍不允许宣称：Pandora sleep/wakeup, power consumption, wake latency, deep-power modes,
  recovery, endurance, or Device PM hardware qualification.

### 2026-10-17 Device PM idle-time tick wrap coverage

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host elapsed-time arithmetic coverage strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit` (278/278);
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：the focused contract drives the canonical Device tick from
  `UINT32_MAX - 10` through wrap to `9`, proving the unsigned elapsed-time calculation
  does not sleep before 20 ticks and requests SLEEP at the exact wrapped boundary.
- 仍不允许宣称：Pandora sleep/wakeup, power consumption, wake latency, deep-power modes,
  recovery, endurance, or Device PM hardware qualification.

### 2026-10-17 FreeRTOS OSAL thread-handle ownership boundary

- 旧边界 -> 新边界：FreeRTOS dynamic task 现登记到 bounded adapter owner table；name/id/state/
  stack-space/priority/suspend/resume 与 thread-flags set 在 native task access 前验证 tracked
  owner，未跟踪 kernel-global handle fail-closed；OSAL-created task exit 清理 owner。
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`、
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`；focused policy
  `freertos_osal_thread_flags_ok`、CTest `freertos_osal_thread_flags`；Host `make test-unit`
  278/278；`make HAL_PLATFORM=PC`、`make HAL_PLATFORM=STM32L4`、`make HAL_PLATFORM=STM32U5`；
  `git diff --check`。
- 适用范围：FreeRTOS OSAL task-handle ownership and fail-closed native access contract。
- 仍不允许宣称：FreeRTOS runtime thread retirement、scheduler、并发、延迟、长稳或产品资格。


- 组件：FreeRTOS OSAL / thread ownership
- 旧边界 -> 新边界：`xy_os_thread_terminate()` no longer reports success for
  `vTaskDelete()`, whose native API provides no observable retirement result; non-NULL
  handles now return `XY_OS_ERROR` and NULL remains a parameter error.
- 证据路径/命令：`components/kernel/osal/backend/freertos/xy_os_freertos.c`,
  `tests/unit/kernel/check_freertos_osal_thread_flags.py`; focused
  `freertos_osal_thread_flags`; Host `make test-unit` (278/278); `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：source/Host/compile unsupported-operation contract for FreeRTOS thread
  termination.
- 仍不允许宣称：FreeRTOS thread ownership, scheduler behavior, runtime termination,
  latency, stress, long-run, or product qualification.

### 2026-10-07 IPC MQ exact timeout boundary

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host timeout contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`, `tests/unit/ipc/test_ipc_mq.c`;
  focused `ipc_mq`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：bounded send/receive waits now return `XY_MQ_TIMEOUT` when elapsed canonical
  OSAL ticks reach the requested timeout, without the previous extra one-tick delay; unsigned
  subtraction keeps tick-wrap behavior bounded.
- 仍不允许宣称：Pandora IPC runtime timing, wake latency, throughput, multi-producer safety,
  ISR safety, recovery, endurance, or product qualification.

### 2026-10-07 IPC Broker request response correlation

- 组件：IPC / Broker
- 旧等级 -> 新等级：H1/Host request-response contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_request()` 仅提取 source queue 中匹配目标 server/message ID 的
  response；不相关消息保持原 FIFO 顺序等待正常 dispatch，不再被误当 response 消耗。
- 仍不允许宣称：Pandora request/response runtime、并发 correlation token、throughput、
  multi-request safety、recovery、endurance 或 product qualification。

### 2026-10-07 Trace runtime dynamic-level filtering

- 组件：Trace
- 旧等级 -> 新等级：H1/Host runtime-filter contract strengthened; no Board claim change
- 证据路径/命令：`components/trace/xy_log/inc/xy_log.h`,
  `components/trace/xy_log/src/xy_log.c`, `tests/unit/trace/test_trace_core.c`;
  focused `trace_component`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：linked `xy_trace` consumers enable runtime filtering so public ERROR/WARN/INFO/DEBUG
  macros emit only when the current dynamic threshold allows them; invalid threshold updates preserve
  the previous level. Source-only focused targets that include `xy_log.h` without linking `xy_trace`
  retain compile-time-only behavior and do not gain an unresolved runtime symbol.
- 仍不允许宣称：Pandora Trace runtime filtering、throughput、concurrent loss behavior、ISR safety、
  performance、recovery、endurance 或 product qualification。

### 2026-10-08 openCH CH32V307 SYS HAL compile baseline

- 组件：HAL / SYS / WCH compile compatibility
- 旧等级 -> 新等级：source-only -> C1 compile baseline; no Board claim
- 证据路径/命令：`components/hal/wch/ch32x/src/xy_hal_sys.c`,
  `boards/opench_ch32v307/sys_identity_main.c`; focused targets
  `opench_ch32v307_sys_identity_smoke` and `xy_hal`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 制品：ELF SHA-256 `af7cdada7959ce64dfaf1bd19484636e0f263c958726c4b97f3477c91c576625`；
  BIN SHA-256 `d2d2b5bb52e354e88aeee493f1a76abdceb650ebab49f9822f2a4619e108776f`。
- 适用范围：CH32V307 system identity、clock/memory query、reset/IRQ/critical-section 与
  bounded unsupported API compile/link reachability；Pandora 仍是唯一 reference-board runtime 基线。
- 仍不允许宣称：openCH flash/read-back、UART runtime、真实 clock/memory identity、reset reason、
  low-power behavior、IRQ runtime、performance、recovery、endurance 或 hardware qualification。

### 2026-10-08 openCH CH32V307 internal Flash HAL compile baseline

- 组件：HAL / Storage / WCH compile compatibility
- 旧等级 -> 新等级：source-only -> C1 compile baseline; no Board claim
- 证据路径/命令：`components/hal/wch/ch32x/src/xy_hal_flash.c`,
  `boards/opench_ch32v307/flash_info_main.c`; focused targets
  `opench_ch32v307_flash_info_smoke` and `xy_hal`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 制品：ELF SHA-256 `a875228b0d89771f1016a297567d5790c356a1a961c28de859152d2001e5af6f`；
  BIN SHA-256 `36808560f087daa744f738e1e6f8942d7a2f84de997d4e0c601a7b796f495cdb`。
- 适用范围：288 KiB address range、72 个 4 KiB page、4-byte program alignment、
  lifecycle/lock/range validation 与 SDK erase/program link reachability；board smoke 只读取
  immutable geometry/boundary metadata，不执行 Flash erase/program。
- 仍不允许宣称：openCH Flash erase/program/read-back runtime、掉电恢复、保护位、bootloader/FOTA、
  performance、endurance 或 hardware qualification。

### 2026-10-08 Device PM owner isolation

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：PM side-table 以 Device identity 独立索引，不再覆盖 canonical
  `xy_device_t.data` driver-private storage；两个 Device 的 PM state 保持隔离。
- 仍不允许宣称：Pandora Device PM runtime、功耗、wake latency、deep-power modes、
  concurrency safety、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM slot lifecycle

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/device/inc/xy_device_pm.h`,
  `components/device/src/xy_device_pm.c`, `tests/unit/device/test_device_pm.c`; focused
  `device_pm`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：公开 deinit 释放固定 PM side-table slot；释放后 owner 返回 not-initialized，
  已满 16-slot table 可复用释放槽位，且不触碰 Device driver-private `data`。
- 仍不允许宣称：Pandora Device PM runtime、并发访问安全、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM public setter fail-closed contract

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host input and callback-failure contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：state/policy setters reject out-of-range enums before callbacks or state mutation;
  wakeup intent is committed only after the optional driver callback succeeds, so failure is retryable.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM consumption output atomicity

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host callback-failure contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：driver power-consumption callbacks write into staged storage; public caller output is
  committed only after callback success and remains unchanged on callback failure.
- 仍不允许宣称：Pandora power measurement、Device PM runtime、功耗精度、wake latency、
  recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM state callback atomicity

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host callback contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：public state reads dispatch the optional driver `get_state` callback; callback failure
  and invalid reported enums preserve caller output and cached state, while a valid result commits
  atomically to both.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM live re-init and driver-state history

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host lifecycle and state-history contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：same-ops re-init preserves a live owner's current/previous state and activity history;
  replacement ops are rejected, and valid driver-reported transitions update wake history atomically.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM idle-timeout fresh window

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host timeout contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：runtime 设置或更新 idle timeout 时以 canonical `xy_device_get_tick()` 重启
  activity window，避免沿用 init 时的旧 timestamp 造成即时休眠；达到新 timeout 边界才进入 SLEEP。
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM policy transition contract

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host policy-transition contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：切换 `ALWAYS_ON` 时先经 driver state callback 将 sleeping owner 原子恢复为
  ACTIVE，callback 失败保持原 policy/state；切换 `AUTO` 时以 canonical Device tick 重启 idle window。
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM ALWAYS_ON state invariant

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host policy-state invariant strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`ALWAYS_ON` owners reject SLEEP、DEEP_SLEEP 与 OFF requests before the driver
  callback, preserving ACTIVE state and avoiding backend side effects.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM driver-report policy invariant

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host policy-state invariant strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`ALWAYS_ON` owners reject valid non-ACTIVE states reported by an optional driver
  callback, preserve caller output and cached ACTIVE state, and accept a later ACTIVE report.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM nested low-power wake invariant

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host wake-state contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：public wakeup returns owners from SLEEP、DEEP_SLEEP or OFF to ACTIVE through the
  optional driver callback; nested low-power transitions cannot restore a stale low-power state.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM activity wake failure propagation

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host activity-wake contract strengthened; no Board claim change
- 证据路径/命令：`components/device/inc/xy_device_pm.h`,
  `components/device/src/xy_device_pm.c`, `tests/unit/device/test_device_pm.c`; focused
  `device_pm`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：public activity recording reports invalid/uninitialized owners and propagates an
  automatic wake callback failure without committing a fresh activity window; a successful retry
  returns the owner to ACTIVE before committing the new activity timestamp.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 IPC Broker pub/sub callback failure propagation

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host callback-failure contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：publish continues notifying active subscribers but returns the first callback error;
  delivered/dropped counters reflect actual callback outcomes and the publish attempt is counted once.
- 仍不允许宣称：Pandora pub/sub runtime、concurrency safety、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-08 IPC Broker empty-topic ownership

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host bounded-resource contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：created topics retain distinct ownership before their first subscriber and after their
  last unsubscribe; duplicate IDs are rejected and the fixed topic table fails closed when full.
- 仍不允许宣称：Pandora pub/sub runtime、concurrency safety、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-08 IPC Broker ISR ingress wake-failure rollback

- 组件：IPC / Broker ISR ingress
- 旧等级 -> 新等级：H1/Host ISR handoff failure contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：ISR ingress publishes its ring tail before requesting the wake, then rolls the tail
  back to the exact previous slot when the ISR-safe wake callback fails; rejected payloads cannot
  be drained later, the same slot remains reusable by a subsequent successful publish, and the
  callback's exact failure code is preserved for caller recovery policy.
- 仍不允许宣称：Pandora multi-ISR runtime、concurrency safety、throughput、arbitrary interrupt
  sources、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request bounded-wait contract

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host bounded-wait contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：zero-timeout requests perform one nonblocking response poll; bounded waits stop and
  return an error when the canonical OSAL delay backend fails instead of spinning forever on a
  non-advancing tick; successful waits poll again after each completed delay.
- 仍不允许宣称：Pandora request/response runtime、concurrent correlation、多请求安全、throughput、
  recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request source-owner guard

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host request ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_request()` 在发送前要求 source server 已注册；无 response queue owner
  时返回 `XY_BROKER_NOT_FOUND`，不再向 destination 留下必然超时且无法关联的 orphan request。
- 仍不允许宣称：Pandora request/response runtime、concurrent correlation、多请求安全、throughput、
  recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request sequence correlation

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response-correlation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：responses preserve the originating request sequence number, and
  `xy_broker_request()` requires source server, destination server, message ID, and sequence to
  match; stale same-route/same-message replies remain queued instead of satisfying a new request.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker response owner validation

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_respond()` 要求原 request 指定的 responder 仍为 registered server；
  responder 注销后拒绝入队，避免伪造或 stale request metadata 向 requester 注入 response。
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request wait owner lifecycle

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host request lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：bounded request wait 同时绑定 requester 与 destination server 的注册代际，并在每次
  OSAL delay 后重新验证；任一 owner 注销或注销后同 ID 重新注册时立即返回
  `XY_BROKER_NOT_FOUND`，不继续使用 stale server slot，也不修改 caller response。
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request destination lifecycle

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host request lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：request wait 绑定 destination server 的注册代际；destination 注销或被同 ID 新 owner
  替换后立即 fail-closed，防止等待中的 request 接受 replacement responder 的同 route/sequence reply。
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker stale request response rejection

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.h`,
  `components/ipc/xy_broker/xy_broker.c`, `tests/unit/ipc/test_ipc_broker.c`; focused
  `ipc_broker`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：Broker-generated requests carry requester/responder registration generations;
  `xy_broker_respond()` rejects a captured request after either owner is unregistered and replaced,
  preventing stale request metadata from injecting a response into a replacement requester's queue.
  Manually constructed unstamped request records retain the existing compatibility behavior.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker sequence-wrap disambiguation

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response correlation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.h`,
  `components/ipc/xy_broker/xy_broker.c`, `tests/unit/ipc/test_ipc_broker.c`; focused
  `ipc_broker`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：Broker request/response sequence identity is widened from 16 to 32 bits; a queued stale
  response cannot alias a fresh request after 65,536 intervening messages with the same route/message
  identity. Existing owner-generation validation remains in force.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  32-bit sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker stamped response boundary

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response provenance contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_respond()` now rejects manually fabricated request records without both
  requester and responder generation stamps. Successful immediate/nonblocking response fixtures use
  Broker-generated request identity, and response dequeue also requires live generation matches.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  32-bit sequence-wrap disambiguation、trusted callers、throughput、recovery、endurance 或 IPC product
  qualification。

### 2026-10-11 IPC Broker subscription registration boundary

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host subscriber-ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_subscribe()` requires an active registered server before auto-creating a
  topic or storing callback/user-data; rejected unregistered subscribers leave topic/statistics state
  unchanged. Existing valid pub/sub fixtures now register their subscriber owners explicitly.
- 仍不允许宣称：Pandora pub/sub runtime、多线程并发安全、trusted subscribers、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 已存在模板/记录

### 2026-10-09 IPC Broker generation across deinit/reinit

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response provenance contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`;
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：server generations now outlive broker deinit/init, so a retained request from a
  previous broker lifecycle cannot be accepted after the same server IDs are registered again.
- 仍不允许宣称：Pandora request/response runtime、多线程并发安全、32-bit sequence-wrap
  disambiguation、trusted callers、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker processing owner-generation boundary

- 组件：IPC / Broker direct queue
- 旧等级 -> 新等级：H1/Host owner-lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_process_msgs()` 绑定开始处理时的 server owner generation；handler
  注销并以同 ID 注册 replacement owner 时，当前调用在计入已交付消息后停止，不会继续用 stale
  server pointer 调用 replacement handler，新 owner 队列由后续 processing 调用处理。
- 仍不允许宣称：Pandora owner replacement runtime、多线程并发安全、throughput、ISR safety、
  recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker direct-message source-generation boundary

- 组件：IPC / Broker direct queue
- 旧等级 -> 新等级：H1/Host source-owner contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：registered source 发出的 direct message 绑定 source generation；消息排队后 source
  被注销或同 ID replacement owner 接管时，processing 丢弃 stale message、计入 dropped 且不调用
  destination handler；replacement owner 的新消息仍可正常交付。未注册 source 的兼容消息保持既有行为。
- 仍不允许宣称：Pandora source replacement runtime、多线程并发安全、trusted source、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker subscription owner lifecycle

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host subscriber-owner lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：server 注销会移除该 owner 的全部 topic subscriptions，并在 topic 失去最后一个
  subscriber 时同步维护 active-topic 统计；同 ID replacement server 不会继承旧 callback/user-data，
  必须显式重新 subscribe 后才能收到 publish。
- 仍不允许宣称：Pandora pub/sub owner replacement runtime、多线程并发安全、throughput、ISR safety、
  recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker publish subscription snapshot

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host publication-membership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：publish 在调用开始时快照 active subscriptions；callback 内注销并在同一 slot
  注册的 replacement subscription 不会收到触发替换的当前消息，但会从下一次 publish 开始接收。
- 仍不允许宣称：Pandora pub/sub replacement runtime、多线程并发安全、callback mutation 的完整
  线性化、throughput、ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker publisher-generation stamping

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host publisher-provenance contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：publish 生成的 broadcast message 绑定当前 registered publisher generation；同一
  server ID 注销并重新注册后，新 publish 携带新的 owner identity，subscriber 可区分 replacement。
- 仍不允许宣称：Pandora pub/sub runtime、多线程并发安全、trusted publishers、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker callback lifecycle reset boundary

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host lifecycle-mutation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：Broker lifecycle 使用跨 deinit/init 单调 generation；subscriber callback 若重置
  Broker，当前 publish 立即返回错误，不继续调用旧 subscription snapshot，也不把旧 publish 的
  sent/delivered/dropped 统计写入新的 Broker lifecycle。
- 仍不允许宣称：Pandora callback mutation runtime、多线程并发安全、完整线性化、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker publish publisher-owner boundary

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host publisher-owner lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：publish 绑定调用开始时的 registered publisher generation；subscriber callback 若注销
  或替换 publisher owner，当前 publish 在计入已完成 callback 的 sent/delivered/dropped 结果后立即
  返回 `XY_BROKER_NOT_FOUND`，不继续调用剩余 snapshot；replacement publisher 的下一次 publish
  使用新 generation 正常交付。
- 仍不允许宣称：Pandora publisher replacement runtime、多线程并发安全、完整线性化、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker direct-processing lifecycle reset boundary

- 组件：IPC / Broker direct queue
- 旧等级 -> 新等级：H1/Host lifecycle-mutation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：direct-message handler 若在 processing callback 内 deinit/init Broker，当前 processing
  通过 lifecycle generation 检测 reset 并立即返回错误；不会把旧 handler 结果或 delivered/dropped
  统计写入新的 Broker lifecycle，也不会继续使用已失效的 server pointer。
- 仍不允许宣称：Pandora callback mutation runtime、多线程并发安全、完整线性化、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker ISR ingress lifecycle binding

- 组件：IPC / Broker ISR ingress
- 旧等级 -> 新等级：H1/Host lifecycle-bound handoff contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.{h,c}`,
  `components/ipc/xy_broker/xy_broker_isr_ingress.{h,c}`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：ISR ingress 初始化绑定当前 active Broker lifecycle generation；Broker 未初始化时拒绝
  建立 ingress，deinit/init 后旧 ring 的 publish/drain 均 fail-closed，drain 丢弃旧 lifecycle payload，
  不会把 reset 前 ISR 数据注入 replacement Broker owner。
- 仍不允许宣称：Pandora Broker reset 与 ISR race runtime、多 ISR producer、多线程并发安全、
  throughput、任意 interrupt source、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker ISR ingress server-owner binding

- 组件：IPC / Broker ISR ingress
- 旧等级 -> 新等级：H1/Host owner-bound handoff contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.{h,c}`,
  `components/ipc/xy_broker/xy_broker_isr_ingress.{h,c}`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：ISR publish 在 ring slot 中快照已注册 source/destination owner generations；drain
  前任一 server 注销或被同 ID replacement owner 接管时，旧 payload 从 ring 丢弃并返回
  `XY_BROKER_NOT_FOUND`，不会进入 replacement destination queue。未注册 source 保留既有
  compatibility behavior，destination 必须仍匹配 publish 时的有效 owner。
- 仍不允许宣称：Pandora server replacement 与 ISR race runtime、多 ISR producer、多线程并发安全、
  throughput、任意 interrupt source、recovery、endurance 或 IPC product qualification。

### 2026-10-11 IPC Broker generation server-slot migration isolation

- 组件：IPC / Broker owner provenance
- 旧等级 -> 新等级：H1/Host owner-generation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.{h,c}`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：server registration generation 不再由数组 slot 分别计数，而是跨 slot 与 Broker
  lifecycle 单调分配并存入 owner。旧 requester 注销、其他 ID 占用旧 slot、同 ID 在新 slot
  注册后，retained request 不会因 generation 数值碰撞而被 replacement owner 接受。
- 仍不允许宣称：Pandora owner migration runtime、多线程并发安全、32-bit generation wrap、
  throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker broadcast destination-owner stamping

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host subscriber-provenance contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：每个 broadcast callback 收到的 message 现在携带该 subscriber server 的 ID 与当前
  owner generation；不再以零 destination identity 交付。回调前若 subscriber owner 已失效则该次
  delivery fail-closed 并计入 dropped，避免 callback 将 broadcast 错归属到 replacement owner。
- 仍不允许宣称：Pandora pub/sub runtime、多线程并发安全、trusted subscribers、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker zero topic-ID boundary

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host input-validation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：topic ID `0` 是无效 sentinel；create/subscribe/unsubscribe/publish 在分配 topic、
  查询 server 或调用 callback 前统一返回 `XY_BROKER_INVALID_PARAM`，不会产生 topic/statistics 副作用。
- 仍不允许宣称：Pandora pub/sub runtime、多线程并发安全、trusted topic namespace、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker zero message-ID boundary

- 组件：IPC / Broker direct、pub/sub 与 request-response
- 旧等级 -> 新等级：H1/Host input-validation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：message ID `0` 是无效 sentinel；send/publish/request/respond 在 enqueue、callback、
  response output 或 statistics 变化前统一返回 `XY_BROKER_INVALID_PARAM`。
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、trusted message namespace、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker ISR zero route-ID boundary

- 组件：IPC / Broker ISR ingress
- 旧等级 -> 新等级：H1/Host input-validation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：ISR ingress 在写 ring slot 或触发 wake callback 前拒绝 destination server ID `0`
  和 message ID `0`；未注册 destination 同样在写 slot 前返回 `NOT_FOUND`；失败均不推进 tail，
  后续合法 payload 仍可使用同一 bounded queue。
- 仍不允许宣称：Pandora multi-ISR runtime、多线程并发安全、trusted route namespace、throughput、
  任意 interrupt source、recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker zero source-ID boundary

- 组件：IPC / Broker direct、pub/sub、request-response 与 ISR ingress
- 旧等级 -> 新等级：H1/Host input-validation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `components/ipc/xy_broker/xy_broker_isr_ingress.c`, `tests/unit/ipc/test_ipc_broker.c`;
  focused `ipc_broker`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：source server ID `0` 是无效 sentinel；direct send、publish、request 与 ISR ingress
  在 enqueue、ring 写入、wake callback、subscriber callback、response output 或 statistics 变化前返回
  `XY_BROKER_INVALID_PARAM`。destination ID `0` 在 direct/request/ISR 路径同样 fail-closed。
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、trusted route namespace、throughput、
  任意 interrupt source、recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker zero server-ID utility boundary

- 组件：IPC / Broker server lifecycle and queue utilities
- 旧等级 -> 新等级：H1/Host input-validation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：server ID `0` 是无效 sentinel；unregister、process、pending-count 与 clear-queue
  utility 在 owner lookup、queue mutation 或 statistics 变化前返回 `XY_BROKER_INVALID_PARAM`。
  Registration query 对零 ID 保持 false，合法 server owner 不受拒绝调用影响。
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、trusted server namespace、throughput、
  ISR safety、recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker inactive lifecycle utility boundary

- 组件：IPC / Broker queue utilities
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：pending-count 与 clear-queue utility 在 Broker 未初始化或已 deinit 时返回
  `XY_BROKER_ERROR`，不再把 lifecycle failure 误报为 owner not-found；registration query 与
  generation query 保持只读的 false/zero 结果。
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-12 IPC MQ allocation overflow boundary

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host allocation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`, `tests/unit/ipc/test_ipc_mq.c`;
  focused `ipc_mq`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_mq_init()` 在 owner mutation 与 allocation 前以 widened arithmetic 校验
  `slot_size × max_msgs` 可由内部 32-bit slot addressing 表示；不可表示的配置返回
  `XY_MQ_INVALID_PARAM` 并保持 caller-owned queue storage 不变。
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-12 IPC Broker bounded stale-message scan

- 组件：IPC / Broker direct queue
- 旧等级 -> 新等级：H1/Host bounded-work contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_process_msgs(server, max_msgs)` 现在以实际 dequeue/检查的消息数限制
  单次工作量；source owner 已替换的 stale 消息仍被丢弃并计数，但不再绕过 `max_msgs` 在一次调用中
  无界扫描整个 queue。返回值仍只统计成功交付给 handler 的消息。
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、实时延迟、throughput、ISR safety、
  recovery、endurance 或 IPC product qualification。

### 2026-10-12 IPC MQ wait-backend failure handling

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host bounded-wait failure contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`, `tests/unit/ipc/test_ipc_mq.c`;
  focused `ipc_mq`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：send/receive bounded waits stop immediately when canonical `xy_os_delay()` fails,
  return `XY_MQ_ERROR`, and preserve queue contents, counters, and caller-owned receive output.
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-12 IPC MQ inactive lifecycle utility boundary

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`, `tests/unit/ipc/test_ipc_mq.c`;
  focused `ipc_mq`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：count/free queries 对未初始化或已 deinit owner 返回零；clear/stats 返回
  `XY_MQ_ERROR`，不会读取 stale config/counters、修改 queue indices 或覆盖 caller-owned stats。
- 仍不允许宣称：Pandora IPC runtime、多线程并发安全、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-04 Device owner re-init atomicity continuation

- 组件：Drivers Sensor / INA219、LTC2945、BNO055、INA228、INA229
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_power_monitors.c`, `test_adc_power_monitors.c`,
  `test_bno055.c`, `test_ina228_device.c`, `test_ina229_device.c`; focused CTests
  `sensor_power_monitors`, `sensor_adc_power_monitors`, `sensor_bno055`,
  `sensor_ina228_device`, `sensor_ina229_device`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged candidate initialization preserves a live owner across helper,
  identity, status-read, and configuration failures; first initialization failures remain
  fail-closed and successful re-initialization commits the complete candidate atomically.
- 仍不允许宣称：board identity, measurement/fusion accuracy, calibration, alert/electrical
  behavior, recovery, endurance, or security approval for these sensors.

- LTE：`docs/validation/xinyi-net-lte-hardware-validation-record-template-2026-08-06.md`
- Fuel Gauge：`docs/validation/xinyi-fuel-gauge-smbus-hardware-validation-record-template-2026-08-06.md`
- GUI 字体硬件：`docs/validation/xinyi-gui-font-rendering-hardware-validation-record-template-2026-08-11.md`
- GUI snapshot review：`docs/validation/xinyi-gui-font-host-snapshot-review-record-template-2026-08-11.md`
- Crypto security/provenance：`docs/validation/xinyi-crypto-security-provenance-review-record-template-2026-08-12.md`
- Crypto benchmark：`docs/validation/xinyi-crypto-benchmark-record-template-2026-08-14.md`

### Sprint 0 待补模板

- [x] `docs/validation/xinyi-stm32u5-hal-hardware-validation-record.md`（2026-08-25 已建立；当前 `BLOCKED_NO_HARDWARE`，有 focused policy guard，未产生实板通过证据）
- [x] `docs/validation/xinyi-display-hardware-validation-record.md`（2026-08-25 已建立；当前 `BLOCKED_NO_HARDWARE`，未产生实板通过证据）
- [x] `docs/validation/xinyi-dm-power-loss-validation-record.md`（2026-08-28 已建立；当前仅 `HOST_INTERRUPTION_GUARDED`，不构成真实 Flash/板级掉电证据）
- [x] `docs/release/known-limitations.md`
- [x] `docs/release/release-checklist.md`（2026-08-31 已建立；当前 `BLOCKED` / `NO-GO`，有 focused policy guard，不构成 R1）

---

## 4. 更新规则

组件发生以下变化时必须同步本台账：

1. 新增/删除 root target 或 active source；
2. 新增 focused CTest、compile probe、QEMU 或 HIL；
3. Kconfig 默认值或 public export 改变；
4. security/provenance 状态改变；
5. README 出现 production、secure、hardware、performance 等能力声明；
6. validation record 从 `pending` 变为 passed/failed/rejected。

每次更新写明：

```text
日期：
组件：
旧等级 -> 新等级：
证据路径/命令：
适用范围：
仍不允许宣称：
提交：
```
