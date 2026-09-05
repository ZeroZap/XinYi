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

| Platform | GPIO | UART | I2C | SPI | Timer | I2S | DMA | Host | Compile/QEMU | Board |
|---|---|---|---|---|---|---|---|---|---|---|
| STM32U5 | `PARTIAL_SOURCE`（EXTI 依赖 SDK IRQ 定义） | `SOURCE_PRESENT` | `SOURCE_PRESENT` | `SOURCE_PRESENT` | `SOURCE_PRESENT` | `UNSUPPORTED`（显式 stub） | `SOURCE_PRESENT` | `HOST_CONTRACT`（通用 API/PC，不是 U5 runtime） | `COMPILE_ONLY` 历史记录；当前 SDK/toolchain 不可用，需重跑 | `BOARD_PENDING` |
| STM32F4 | `SOURCE_PRESENT` | `UNSUPPORTED`（wrapper 返回 not-supported） | `SOURCE_PRESENT` | `UNSUPPORTED`（wrapper 返回 not-supported） | `UNSUPPORTED`（wrapper 返回 not-supported） | `UNSUPPORTED`（无 wrapper） | `SOURCE_PRESENT` | `HOST_CONTRACT`（通用 API/PC） | `QEMU_PARTIAL`；不覆盖完整外设矩阵 | `BOARD_PENDING` |
| STM32L4 | `PARTIAL_SOURCE`（框架 HAL 复用 F4 wrapper；Pandora board smoke 直接使用 CubeL4 HAL） | `UNSUPPORTED`（框架 wrapper 复用 F4 stub；Pandora smoke 直接使用 CubeL4 UART） | `PARTIAL_SOURCE`（框架 wrapper 复用 F4；Pandora AHT10 probe 为 board-local software I2C） | `UNSUPPORTED`（复用 F4 stub） | `UNSUPPORTED`（复用 F4 stub） | `UNSUPPORTED` | `SOURCE_PRESENT`（dedicated L4 wrapper） | `HOST_CONTRACT`（通用 API/PC） | PC/L4/U5 compile；Pandora FreeRTOS image link | `B1_BOARD_SMOKE`；framework DMA1 Channel1、8-word SRAM→SRAM polling B1；SPI/peripheral-DMA/IRQ/callback/recovery pending |
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
