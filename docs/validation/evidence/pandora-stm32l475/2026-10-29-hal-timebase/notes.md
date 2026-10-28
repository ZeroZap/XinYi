# 2026-10-29 — Pandora HAL 毫秒时基：绑定在 STM32 上根本没有编译进去，以及 pre-scheduler 归零

## 结论先说

三个镜像、同一块 Pandora、同一套判据（`xy_hal_sys_get_tick_count()` → 板载 `HAL_GetTick()`），
差别只在 `freertos_bind_hal_timebase()` 放在哪里 / 是否编译：

| 变体 | pre-scheduler 读到的 HAL tick | 任务里跨 20 tick 的 HAL tick 差 |
|---|---|---|
| 绑定在 `xy_os_kernel_start()`（HEAD `1bf2c3fd`） | **4** | **20** |
| 绑定改回 `xy_os_kernel_init()`（本地变异，未入库） | **0** | 20 |
| 绑定整段没编译（缺 `components/hal/inc`，`7130db27`） | 4（`uwTick` 回退） | 20 |

即：

1. **pre-scheduler 窗口**里，把内核 tick 当 HAL 时基用会让 `HAL_GetTick()` 恒为 **0**——任何以它计时
   的等待在 `xy_os_kernel_init()` 到 `vTaskStartScheduler()` 之间都**永不到期**。把绑定移到调度器启动前
   一刻后，同一位置读到 4（板载 SysTick 推进的 `uwTick`），窗口恢复成活的时钟。
2. **任务上下文**里两种绑定都是 20——证明 post-scheduler 的 HAL 时钟跟得上内核 tick，也说明
   pre-scheduler 读数才是这条判据的判别项（只看任务内差值会漏掉缺陷）。
3. 最右一行是**发现缺陷的那条**：`__has_include("xy_hal_time.h")` 在 STM32 上为假（`xy_osal` 的
   include 路径没有 `components/hal/inc`），于是 `freertos_bind_hal_timebase()` 与
   `freertos_hal_tick_ms()` 整个不编译；`HAL_GetTick()` 一直走 `uwTick` 回退——它能用**只因为**
   `rtos_handlers.c` 的 `SysTick_Handler` 每个 tick 还调 `xy_hal_sys_tick_irq_handler()`（`HAL_IncTick()`）。
   换句话说 S6-141/143/145/146 在内核侧"注册时基源"这件事，在参考板上从来没生效过。

## 镜像与操作

- 绿镜像：`1bf2c3fd`，BIN 42576 B，SHA-256 `a7fec77b723edf7518bd6970cee5d504071d4d29ca38a4e43a22245709b56148`。
- 红镜像：同一 commit + 本地把 `freertos_bind_hal_timebase();` 放回 `xy_os_kernel_init()`，BIN 42592 B，
  SHA-256 `44ac3357…`+`2dd6f572…`（两段拼接）。
- 两个镜像都 > 32 KiB bootloader 段，按 `0x08000000/32768 B` + `0x08008000/其余` 两段分别门控写入，
  每段 `span_identical=true unexpected_changes=0`；两段都落盘后**显式 `st-flash reset`**。
- 捕获：CP2102 `/dev/ttyUSB0` 115200 8N1，复位前 2 s 打开并 flush，12 s 窗口。
- 收尾：`restore --snapshot 2026-10-29T014913/pandora-state` → `verify_ok regions=5`
  （bootloader `762c4a75…`、application `d35e2e9a…` 与运行前逐字节相同）。

## 判据为什么可信

- `OSAL_PRE_SCHEDULER_HAL_TICK` 打的是 `xy_hal_sys_get_tick_count()`，在 L4 上就等于 `HAL_GetTick()`，
  也就是 HAL 自己在用的那个时钟，不是另起一套。
- 0 vs 4 的差别只在"绑定位置"一处：红镜像只改动了那一次调用位置，其余字节同源；两镜像
  `.text` 42380 → 42396 B，确实重新编译过（此前"绑定没生效"时两者逐字节相同，正是线索）。
- 误差边界：pre-scheduler 读数是"启动已过 4 ms"这类小整数，不做精度声明；20 tick ↔ 20 ms 依赖
  1 kHz tick（`components/kernel/osal/config/freertos/stm32l4/FreeRTOSConfig.h`）。

## 边界

- 只证明 Pandora STM32L475、该镜像、该 tick 频率下的这一条时钟关系；不构成 HAL 等待行为变化、
  tickless、`xy_hal_delay_us` 在 RTOS 下的语义、U5/F4/HC32 平台的运行时证据。
- 红镜像的变异**没有入库**（有意），因此不会自动重跑；入库的只有
  `tests/unit/kernel/check_freertos_hal_timebase_binding.py`（静态顺序/包含守卫）与两个镜像 marker。
- W25Q128 `0xF00000` 上仍有 S6-142 写入的 v2 candidate，快照/恢复不覆盖片外 SPI flash，本次未触碰。

## 文件

- `timebase-bound-green.log` — HEAD 镜像（`1bf2c3fd`）
- `timebase-bound-red.log` — 绑定放回 init 的变异镜像
- `timebase-inert-binding.log` — 绑定被 `__has_include` 编译掉的镜像（`7130db27`）
- `measurement.json` — 三个日志的字节数/SHA-256、commit 身份、两个 marker 读数、镜像与恢复指纹
