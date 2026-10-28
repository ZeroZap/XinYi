# XinYi HAL 平台实现与证据矩阵

**建立日期**：2026-08-26

**范围**：Sprint 2 前置项 S2-1

**事实源**：`components/hal/CMakeLists.txt`、各平台 `xy_hal_*.c`、当前可追溯验证记录

**证据边界**：源文件存在只表示 implementation source present；compile、QEMU、Host 和 Board 必须分别取证，不能互相升级。

## 1. 状态定义

- `SOURCE_PRESENT`：平台目录存在对应 wrapper；不代表已编译或运行。
- `PARTIAL_SOURCE`：存在 wrapper，但明确包含 unsupported 路径或只覆盖部分 API。
- `UNSUPPORTED`：缺少该平台 wrapper，或实现明确 fail-closed 返回 not-supported。
- `HOST_CONTRACT`：PC fake/simulation 契约已由 canonical Host CTest 覆盖；不代表目标平台行为。
- `COMPILE_ONLY`：有可追溯 clean target compile；不代表板级运行。
- `QEMU_PARTIAL`：仅有部分 STM32F4 QEMU 场景；不代表完整外设或实板。
- `BOARD_PENDING`：没有满足项目证据要求的板级日志与 capture。

## 2. 平台实现矩阵

下表审计 Sprint 2 最小证据链关注的 GPIO、UART、I2C、SPI、Timer、I2S、DMA。STM32L4 除 DMA 已有 dedicated wrapper 外仍复用 STM32F4 wrapper，因此单独列出。

> 2026-10-28 校准：STM32F4/STM32L4 两个 branch 此前都**未定义** `STM32_HAL_ENABLED`，而
> `stm32f4/*.c` 全部包在该 guard 内 —— 因此 root `xy_hal` archive 里这些 TU 全是 0-byte 空对象
> （实测 `arm-none-eabi-size` text=0，archive 导出 0 个 `xy_hal_*` 符号），平台 gate 却一直"通过"。
> 现两 branch 都定义该 guard，并由 `tests/unit/hal_component/check_hal_platform_build_contract.py`
> 锁定（新增 CTest `hal_platform_build_contract`）。下表 "SOURCE_PRESENT" 仅指源文件被真实编译进
> archive，仍不构成 runtime/实板证据。

| Platform | GPIO | UART | I2C | SPI | Timer | I2S | DMA | Host | Compile/QEMU | Board |
|---|---|---|---|---|---|---|---|---|---|---|
| STM32U5 | `PARTIAL_SOURCE`（EXTI 依赖 SDK IRQ 定义） | `SOURCE_PRESENT` | `SOURCE_PRESENT` | `SOURCE_PRESENT` | `SOURCE_PRESENT` | `UNSUPPORTED`（显式 stub） | `SOURCE_PRESENT` | `HOST_CONTRACT`（通用 API/PC，不是 U5 runtime） | `COMPILE_ONLY` 历史记录；当前 SDK/toolchain 不可用，需重跑 | `BOARD_PENDING` |
| STM32F4 | `SOURCE_PRESENT`（真实编译进 archive；12/15 wrapper，见下） | `SOURCE_PRESENT`（真实编译进 archive；`xy_hal_uart_*` 全 11 个公开入口 + 3 个 vendor 完成回调已导出） | `SOURCE_PRESENT`（真实编译进 archive；`xy_hal_i2c_*` 全 12 个公开入口 + 3 个 vendor 完成回调已导出） | `UNSUPPORTED`（同上：deferred） | `UNSUPPORTED`（同上：deferred） | `UNSUPPORTED`（无 wrapper） | `SOURCE_PRESENT`（真实编译进 archive；`xy_hal_dma_*` 符号已导出并引用 `HAL_DMA_*`） | `HOST_CONTRACT`（通用 API/PC） | `QEMU_PARTIAL`；不覆盖完整外设矩阵 | `BOARD_PENDING` |
| STM32L4 | `PARTIAL_SOURCE`（框架 HAL 复用 F4 wrapper；Pandora board smoke 直接使用 CubeL4 HAL） | `SOURCE_PRESENT`（dedicated L4 wrapper 已真实编译；runtime 仍仅 Pandora board-local） | `PARTIAL_SOURCE`（框架 wrapper 复用 F4；Pandora AHT10 probe 为 board-local software I2C） | `SOURCE_PRESENT`（dedicated L4 wrapper 已真实编译；SPI/DMA B1 为 board-local image 证据） | `SOURCE_PRESENT`（dedicated L4 wrapper 已真实编译） | `UNSUPPORTED` | `SOURCE_PRESENT`（dedicated L4 wrapper） | `HOST_CONTRACT`（通用 API/PC） | PC/L4/U5 compile；Pandora FreeRTOS image link | `B1_BOARD_SMOKE`；framework DMA1 Channel1、8-word SRAM→SRAM polling B1；SPI/peripheral-DMA/IRQ/callback/recovery pending |
| WCH CH32V30x | `PARTIAL_SOURCE`（IRQ configure unsupported） | `PARTIAL_SOURCE`（advanced config unsupported） | `PARTIAL_SOURCE`（advanced config unsupported） | `PARTIAL_SOURCE`（advanced config unsupported） | `UNSUPPORTED` | `UNSUPPORTED` | `UNSUPPORTED` | `HOST_CONTRACT`（通用 API/PC） | `COMPILE_ONLY` pending | `BOARD_PENDING` |
| HC32L021 | `PARTIAL_SOURCE`（IRQ paths unsupported） | `UNSUPPORTED` | `UNSUPPORTED` | `UNSUPPORTED` | `UNSUPPORTED` | `UNSUPPORTED` | `UNSUPPORTED` | `HOST_CONTRACT`（通用 API/PC） | `COMPILE_ONLY` pending | `BOARD_PENDING` |
| PC simulation | `HOST_CONTRACT` | `HOST_CONTRACT` | `HOST_CONTRACT` | `HOST_CONTRACT` | `UNSUPPORTED`（PC source set 未选 Timer wrapper） | `UNSUPPORTED` | `UNSUPPORTED`（PC source set 未选 DMA wrapper） | `HOST_CONTRACT` | n/a | n/a |

> PC 的 SYS 实现位于聚合文件 `xy_hal_pc.c`；矩阵仍以 `components/hal/CMakeLists.txt` 实际
> source selection 为准，不能把未选入 `xy_hal` 的 helper 文件或通用声明记作 Host contract。

## 3. 平台结论

### STM32U5

S2-2 所需 GPIO/UART/I2C/SPI/Timer/DMA wrapper 已存在，I2S 明确 fail-closed。当前只允许声明 source-present、历史 compile-only 和通用 Host contract；板级结果仍由 [STM32U5 HAL/HIL 记录](xinyi-stm32u5-hal-hardware-validation-record.md)保持 `BLOCKED_NO_HARDWARE`。

### STM32F4 / STM32L4

STM32F4 的 UART、SPI、Timer 大量路径为显式 not-supported；STM32L4 对这些路径仍复用该组 wrapper。DMA 已改用 dedicated STM32L4 实现，但不能据此写成完整 HAL 支持。

2026-10-28 收口了一处会制造**假绿**的平台构建缺陷：`components/hal/CMakeLists.txt` 的 STM32F4 与
STM32L4 两个 branch 都没有定义 `STM32_HAL_ENABLED`，而 `stm32f4/*.c` 的全部实现都包在
`#ifdef STM32_HAL_ENABLED` 内，于是 root `xy_hal` archive 中这些 TU 全部编译成 0-byte 空对象：
`arm-none-eabi-size` 实测 text/data/bss 全为 0（含此前的 `build/stm32l4`、`build/pandora-rtos`），
archive 导出 0 个 `xy_hal_*` 符号，而 `make HAL_PLATFORM=STM32L4/F4` 一直返回成功。现两个 branch
都定义该 guard；STM32L4 另补入此前从未编译的 dedicated `stm32l4/xy_hal_spi.c` 与
`stm32l4/xy_hal_timer.c`（此前 root gate 里 SPI/Timer 既无 F4 实现也无 L4 实现），并显式排除
会与它们重名的 F4 副本。`xy_hal_pwm.c` 与 `xy_hal_lp_timer.c` 已按当前公开契约重写
（`xy_hal_error_t` 返回值、init 状态跟踪、L4 LPTIM1 counter/IT 与真实 callback 转发；F4 无 LPTIM →
显式 `XY_HAL_ERROR_NOT_SUPPORTED`）。修复后同一 L4/`xy_hal` target：`xy_hal_pwm.c.obj` 756 B text、
`xy_hal_lp_timer.c.obj` 444 B、`xy_hal_dac.c.obj` 696 B、`xy_hal_exti.c.obj` 504 B、
`xy_hal_rng.c.obj` 704 B，archive 导出 `xy_hal_pwm_init`/`xy_hal_lptimer_init`/`xy_hal_spi_init`/
`xy_hal_timer_init`/`xy_hal_dac_init`/`xy_hal_exti_init`/`xy_hal_rng_init`；F4 branch archive 由 0 个
导出符号变为 126 个。

`xy_hal_qspi.c` 曾被记为 "51 处契约漂移"，用同一 L4 gate 的交叉编译 flags 复现后证明该诊断错误：
全部报错都来自未声明 `QSPI_HandleTypeDef`/`QSPI_*`/`HAL_QSPI_Init`，根因是组件
`stm32l4/stm32l4xx_hal_conf.h` 从未定义 `HAL_QSPI_MODULE_ENABLED`（只有 Pandora board 目录自带的
conf 打开了它，所以 board 镜像一直能编）。现组件 conf 启用该 module 并 include 家族头，L4 branch
显式 append 该 wrapper：`xy_hal_qspi.c.obj` text 376 B，archive 导出
`xy_hal_qspi_init/deinit/command/tick_ms`。回归门同时新增两条 fail-closed 规则（L4 不得有未 append
的孤儿 wrapper；每个 append 的 wrapper 必须在 `L4_MODULE_REQUIREMENTS` 登记所需 HAL module，且该
module 在组件 conf 中既 enable 又 include），删除 enable / 删除 include / 删除 CMake append 三个
负向变异全部被判 FAIL。

仍**未**迁移（显式记录为 deferred，重加即编译失败）：STM32F4 的 `xy_hal_rtc/spi/timer.c`，
以及 F4 专属 `xy_hal_adc.c`（`ADC->CCR`/3-cycle sample time 仅 F4 可用）。

`xy_hal_i2c.c` 已完成迁移（deferred 集合由 4 项缩到 3 项）。旧实现**根本无法编译**：它引用了公开
HAL 从未声明的 `xy_hal_i2c_handle_t`，并返回 `XY_HAL_I2C_ADDRESSINGMODE_*` 而非 vendor 的
`I2C_ADDRESSINGMODE_*`（F4 gate 实测 undeclared + conflicting types）。除编译中断外还有语义缺陷：
全部入口 `int` 而非 `xy_hal_error_t`、`dev_addr << 1` 前不做范围校验（调用方传 8-bit 地址会静默
寻址到另一颗器件）、`reg_addr` 无条件按 8-bit 截断、`register_callback` 不记录、`xy_hal_i2c_error`/
`set_error_cb` 缺失。现按 L4 dedicated wrapper 契约重写，并补 F4 专属部分：`ClockSpeed`/`DutyCycle`
（F4 仍用显式时钟+占空比而非预计算 `Timing`）、7-bit 地址上限校验、`i2c_mem_address_size()` 按
`reg_addr` 选择 8/16-bit 存储器地址宽度、callback 上下文真实记录并由三个 vendor 回调分发。
analog/digital filter 只在提供 I2C FLTR 寄存器的 F4 型号上存在（`STM32F407` 没有），因此
`HAL_I2CEx_*` 调用用 `I2C_FLTR_ANOFF`/`I2C_FLTR_DNF` 守卫。回归门：CTest
`stm32f4_i2c_wrapper_contract`（编译中断符号、F4 时钟/占空比、地址校验、memadd 选择、callback
上下文、错误域；对迁移前源为 25+ 项 RED）。

`xy_hal_uart.c` 已完成迁移（deferred 集合由 5 项缩到 4 项，F4 分支现真实构建它）。旧实现的缺陷是
结构性的：全部入口声明 `int` 而非 `xy_hal_error_t`（重加即编译失败）、`send`/`recv` 把传输长度当
返回值发布（公开契约此处是错误码）、`flush` 无超时地 `while (gState != READY);` 死等、三个 vendor
完成回调是 `void` 函数却试图返回值、`register_callback` 不记录任何上下文、`available` 恒返回 0，
且 `xy_hal_uart_error`/`xy_hal_uart_set_error_cb` 完全缺失。现按 L4 dedicated wrapper 的契约重写：
枚举映射到 vendor `UART_*` 常量、`uart_status()` 把 `HAL_TIMEOUT/HAL_BUSY` 映射为
`XY_HAL_ERROR_TIMEOUT/BUSY`、`uart_config_check()` 在第一次 `Init` 写入前拒绝越界与不可表示配置
（F4 无 7-bit 字长与 1.5 停止位 → `XY_HAL_ERROR_NOT_SUPPORTED`，不再静默加宽）、`flush` 改为报告
busy 而非死等、callback 上下文真实记录并由三个 vendor 回调按 `TX_DONE/RX_DONE/ERROR` 分发、补齐
`error`/`set_error_cb`。

`xy_hal_dma.c` 已完成迁移（deferred 集合由 6 项缩到 5 项，F4 分支现真实构建它）。重写原因不只是签名
漂移：旧实现把 `XY_HAL_DMA_*` 枚举值直接当 STM32 HAL 常量返回，宽度三档返回未声明的
`XY_HAL_DMA_PDATAALIGN_*` 并用同一个 periph/mem 映射配置两侧，memory increment 误用 `DMA_PINC_*`，
`register_callback` 不做任何登记就返回成功，且全部入口声明为 `int` 而非 `xy_hal_error_t`（重加进
构建即在 `-fsyntax-only` 阶段报 undeclared + conflicting types）。现按 L4 dedicated wrapper 的契约
重写：独立 `to_periph_alignment/to_mem_alignment`、`to_periph_incr/to_mem_incr`，方向/模式/优先级/
宽度映射到 `DMA_*` 常量，`dma_config_valid()` 在第一次 `hdma->Init` 写入前拒绝越界枚举，
`register_callback` 真正调用 `HAL_DMA_RegisterCallback` 并保存 callback/arg 上下文，
`xy_hal_dma_irq_handler` 补实现。回归门：CTest `stm32f4_dma_wrapper_contract`（映射形状、双侧
helper、域校验顺序、callback 登记、错误域、IRQ hook；对迁移前源为 35 项 RED），以及
`stm32f4_uart_wrapper_contract`（枚举映射、配置域 fail-closed、错误域、`flush` 不得无界自旋、
callback 记录与 vendor 事件分发、void 回调不得返回值；对迁移前源为 30+ 项 RED）。

`hal_platform_build_contract` 新增反向断言：F4 分支 filter 掉的 wrapper 必须出现在
`XY_HAL_F4_DEFERRED_SOURCES` 中，因此已迁移的 wrapper 不能再被静默排除出平台门；branch 缺 guard、
deferred 集合变更、列了 deferred 却未真正排除、L4 同时 glob F4 副本与新 wrapper（只会在定义 guard
后暴露的重复符号）依旧被拒绝。
以上全部为 source/compile 级证据，不升级任何 STM32F4/L4 framework 外设的 runtime 或实板结论；
Pandora 参考板镜像（`pandora_stm32l475_smoke` 14296 B 与既有记录一致、`pandora_stm32l475_rtos`
clean link）本身不受影响，因为 board target 自带 `STM32_HAL_ENABLED` 与显式 source list。

Pandora STM32L475VE 现有 board-local smoke target 可 clean cross-compile/link，并包含 LED、USART1、
KEY0 和 AHT10 `0x38` software-I2C 初始化、测量、数值换算及 ACK/NACK 恢复 probe。该目标直接
链接 STM32CubeL4 HAL，不会升级 XinYi STM32L4 framework wrapper 的能力状态；本次结果仅为
`COMPILE_ONLY`。2026-09-03 已通过 ST-Link 完成当前 image write/verify，并通过接入 PA9/PA10/GND
的独立 WCH-Link UART 留存匹配固件 SHA 的 banner、10 次 AHT10 ACK 与合理测量；PE7 500 ms
翻转及 KEY0 输入也已观察并留存日志。详见
[Pandora board smoke record](xinyi-pandora-stm32l475-board-smoke-record.md)。
这些证据将 board-local smoke 正常路径升级为 B1，但不会升级对应 framework GPIO/UART/I2C wrapper，
也不构成 AHT10 NACK/recovery 的 B2。2026-09-05 clean `6b2ff630` image 另以 framework
`xy_hal_dma_*` 完成 DMA1 Channel1 的 8-word SRAM→SRAM polling copy；22152-byte image write/verify
及 read-back byte-identical，20 秒 reset-synchronized UART capture 为 10975 bytes、SHA-256=`ed4ed44d...`，
出现一次 `PANDORA_DMA_MEM2MEM_OK`、39 个完整跨组件周期、19 次 SysTick ISR、26 次 TIM6
wake、2P/2C consumer 为 9/7，错误 marker 为 0。
该结果升级 STM32L4 framework DMA mem2mem polling 与 completion IRQ/callback 为 B1。随后 clean
`f95dfed6` image 以 active transfer software abort→deinit→re-init→polling copy 完成受控恢复 B2；
23684-byte image write/verify/read-back byte-identical，20 秒 UART capture 11034 bytes、
SHA-256=`fcfe3674...`，`PANDORA_DMA_STOP_RECOVERY_OK` 一次且错误 marker 为 0。SPI、peripheral
request、error IRQ、物理故障、DMA timeout 与性能仍 pending。

SPI1 TX request→DMA1 Channel3→completion IRQ→framework callback→OSAL task 已取得 B1；clean
`3c3e5f60` image 进一步完成 SPI/DMA deinit→re-init→重新注册 callback→第二次 TX DMA 的受控
recovery B2。25828-byte image write/verify/read-back byte-identical，20 秒 UART capture 11086 bytes、
SHA-256=`9e05bbc3...`，`PANDORA_SPI_DMA_TX_OK` 与 `PANDORA_SPI_DMA_RECOVERY_OK` 各一次，
错误 marker 为 0。该链未配置 SPI GPIO 或外设，不能作为电气信号、外设响应、RX/full-duplex、
物理故障或性能证据。

clean committed `480c3f1c` image 又在第三轮 active TX DMA 上执行 `HAL_SPI_DMAStop`，随后
deinit/re-init SPI/DMA、重注册 callback 并完成 fresh TX DMA IRQ→OSAL task。26160-byte image
write/verify/read-back byte-identical，BIN SHA-256=`5562ace4...`；20 秒 UART capture 11141 bytes、
SHA-256=`d9d9ae29...`，normal/re-init/abort-recovery marker 各一次，validator error marker 为 0。
这只升级受控 software abort recovery B2，不是物理故障、电气响应、RX/full-duplex 或性能证据。

Pandora V2.4 原理图确认 U9 为 W25Q128，QSPI Bank1 使用 PE10 CLK、PE11 NCS、PE12–PE15
IO0–IO3。clean committed `37fa7b95` image 通过 CubeL4 QSPI 单线 `0x9F` 读取并严格匹配
JEDEC ID `EF 40 18`；27600-byte image write/verify/read-back byte-identical，20 秒 UART capture
11150 bytes、SHA-256=`36258be5...`，`PANDORA_W25Q128_JEDEC_ID_OK` 一次且错误 marker 为 0。
该结果只证明板载 16 MiB Flash 的识别 B1；擦写、四线模式、掉电恢复、耐久和 FOTA candidate
storage 仍 pending。

clean committed `5375f02f` image 随后固定使用 W25Q128 末尾独立 4 KiB 测试扇区
`0x00FFF000`，执行 write-enable/WEL 校验、sector erase、busy polling、256-byte pattern page
program 与 byte-for-byte read-back。首次实板运行暴露 512-byte task stack 不足；增至 1536 bytes
后，28332-byte image write/verify/read-back byte-identical，BIN SHA-256=`ce548238...`；20 秒
reset-synchronized capture 11187 bytes、SHA-256=`79186b49...`，
`PANDORA_W25Q128_ERASE_WRITE_READ_OK` 一次、39 个完整跨组件周期、19 次 SysTick ISR、
26 次 TIM6 IRQ、2P/2C consumer 为 9/7，错误 marker 为 0。该结果只升级受控测试区
erase/program/read B1；四线模式、真实掉电、耐久和 FOTA candidate storage 仍 pending。

clean committed `11002838` image 在上述写入完成后显式 QSPI deinit→re-init，再读取同一测试地址并
逐字节匹配 256-byte pattern。28560-byte image write/verify/read-back byte-identical，BIN
SHA-256=`6e1eb361...`；20 秒 capture 11355 bytes、SHA-256=`0c30e2b2...`，
`PANDORA_W25Q128_PERSISTENCE_STAGED` 与 `PANDORA_W25Q128_PERSISTENCE_RECOVERED` 各一次，
错误 marker 为 0。该结果仅证明 QSPI controller re-init 后的数据保持，不等于 MCU/板级复位、
真实掉电恢复、耐久、四线模式或 FOTA candidate storage。

FOTA 现通过 `xy_fota_w25q128` flash-ops adapter 使用 canonical W25Q128 driver。clean committed
`2b5d04a2` image 在独立 `0x00FFE000` 4 KiB candidate 测试区完成 300-byte erase、跨页
program 与 read-back；30820-byte image write/verify/read-back byte-identical，BIN
SHA-256=`a7a41e0e...`。双 boot UART chain 17714 bytes、SHA-256=`865c20a9...`，
`PANDORA_W25Q128_FOTA_CANDIDATE_STORAGE_OK` 出现且错误 marker 为 0。该结果仅证明 bounded
candidate storage B1，不是完整 image download/CRC/signature、bootloader handoff、掉电或 secure FOTA。

### WCH / HC32

WCH 只有部分基础 wrapper，Timer/I2S/DMA 缺失；HC32L021 当前只有 GPIO wrapper，且 IRQ 不支持。两者都没有 canonical compile 或实板证据，不得描述为 production-ready。

## 4. Sprint 2 非实板前置门禁

- [x] 逐平台拆分 implementation / unsupported / Host / compile / QEMU / Board 状态。
- [x] 明确 STM32L4 复用 STM32F4 wrapper，而非 dedicated implementation。
- [x] 明确 STM32U5 I2S、STM32F4 UART/SPI/Timer、WCH 长尾、HC32 非 GPIO 缺口。
- [ ] 恢复 STM32U5 SDK 与 ARM toolchain 后重跑 clean compile，并保存原始日志。
- [ ] 为选定 I2C→Device→Driver 纵切建立 Host error/re-init contract。
- [ ] 硬件到位后按 HAL-01～HAL-09 填写 Board 证据。

只有最后一项完成并保留原始日志/capture 后，才能升级相应 Board 状态。
