# XinYi STM32U5 HAL/HIL 硬件验证记录

**建立日期**：2026-08-25
**当前状态**：`BLOCKED_NO_HARDWARE`
**范围**：Sprint 2 前置项 S0-08 / S2-1～S2-2，STM32U5 GPIO、UART、I2C、SPI、IRQ 与 DMA 最小证据链
**证据边界**：Host, QEMU, and compile-only results cannot be promoted to board, timing, or production evidence.

## 1. 当前阻塞

本记录建立时，开发主机未检测到可识别的开发板、调试器或串口设备；STM32U5 vendor submodule 工作树也未初始化，当前 shell 的 `PATH` 中没有 `arm-none-eabi-gcc`。因此没有烧录、板级运行、逻辑分析或示波器证据。

解除阻塞至少需要：

- 明确的 STM32U5 reference board、板卡修订版和 MCU；
- SWD 调试器/烧录器；
- GPIO/UART/I2C/SPI 回环或外设夹具；
- 可制造 I2C NACK、UART/SPI timeout 和总线恢复场景的接线；
- 逻辑分析仪或示波器；
- 已初始化的 STM32U5 SDK submodules 和可用 ARM GNU toolchain。

Current result: `BLOCKED_NO_HARDWARE`

## 2. 运行身份与工具

| Field | Value |
|---|---|
| Operator / date | pending |
| Git SHA | pending |
| Board / revision / MCU | pending |
| Debugger / programmer | pending |
| Logic analyzer / oscilloscope | pending |
| Toolchain / build command | pending |
| Clock tree / optimization | pending |
| Firmware project / image | pending |
| Raw log / capture directory | pending |

Git SHA 必须是已经推送到 `origin/main` 的完整 SHA。日志目录必须保存原始串口日志、烧录输出、逻辑分析 capture 和必要的照片，不得只保存最终布尔值。

## 3. 夹具与接线清单

| Field | Value |
|---|---|
| GPIO pins | pending |
| UART instance / pins | pending |
| I2C instance / pins / pull-ups | pending |
| SPI instance / pins / chip select | pending |
| IRQ source / priority | pending |
| DMA controller / channel / request | pending |
| Loopback / peripheral modules | pending |
| Supply voltage / current limit | pending |
| Ground and analyzer channels | pending |

接线图必须记录端口、pin、AF、有效电平、总线频率、上拉值以及分析仪通道映射。不得把示例引脚当作已验证板级映射。

## 4. 软件前置门禁

板级运行前记录以下命令和原始输出：

```bash
make test-unit
make HAL_PLATFORM=STM32U5 -j"$(nproc)"
git diff --check
```

| Gate | Result / artifact |
|---|---|
| HAL focused Host contracts | pending |
| Full Host CTest | pending |
| Clean STM32U5 compile | pending |
| Firmware link map / image size | pending |
| Local/remote SHA sync | pending |

这些门禁通过也只建立 Host/C1 证据，不能替代真实 STM32U5 运行。

## 5. 必测场景

每项必须记录 API 序列、返回码、callback 事件、串口日志、逻辑分析 capture 和实际结果。

| ID | Scenario | Minimum acceptance | Actual result / evidence |
|---|---|---|---|
| HAL-01 | GPIO output/input/IRQ | 输出翻转与输入采样一致；IRQ callback 次数、边沿和优先级可追溯 | pending |
| HAL-02 | UART blocking + timeout | TX/RX 字节正确；断开 RX 时 timeout/error mapping 明确，无假成功 | pending |
| HAL-03 | UART IRQ or DMA | 完成/错误 callback 正确，缓冲区和长度可追溯 | pending |
| HAL-04 | I2C normal transaction | 目标地址、START/address/data/STOP 与返回码一致 | pending |
| HAL-05 | I2C negative recovery | NACK 和 timeout 映射正确；bus reset / re-init 后事务恢复 | pending |
| HAL-06 | SPI normal transaction | mode、frequency、CS 和 TX/RX 字节与配置一致 | pending |
| HAL-07 | SPI timeout/error recovery | stalled/absent peripheral 不报告成功；deinit→init 后恢复 | pending |
| HAL-08 | IRQ callback isolation | 多实例/多源 callback 不串路由，错误事件到达正确上下文 | pending |
| HAL-09 | DMA transfer | 至少一条真实 DMA 路径完成；完成/错误事件、长度和数据一致 | pending |

## 6. 原始证据要求

- 串口日志包含 firmware SHA、板卡身份、时钟、各场景开始/结束和原始错误码。
- I2C capture 必须可见地址、ACK/NACK、数据和 STOP；负向场景记录注入方式。
- SPI capture 必须可见 CS、clock polarity/phase、频率和数据。
- UART capture 必须记录波特率、帧格式、TX/RX 与 timeout 注入。
- GPIO/IRQ capture 必须记录刺激源、边沿和 callback timestamp/counter。
- DMA 必须记录所用 request/channel、buffer/length、完成或错误 callback。
- 所有 timing 数据必须注明仪器、采样率、样本数和测量点；估算值无效。

## 7. 结果分类

只能选择一个：

- `BLOCKED_NO_HARDWARE`：缺板卡、调试器、夹具、SDK/toolchain 或仪器；当前分类。
- `COMPILE_ONLY`：clean STM32U5 build 通过，但没有实板执行。
- `BOARD_FAILED`：已运行实板且失败，原始日志/capture 与 blocker 已保留。
- `BOARD_PASSED_BASIC`：HAL-01、HAL-02、HAL-04、HAL-06 与对应返回码均有实板证据。
- `BOARD_PASSED_NEGATIVE`：`BOARD_PASSED_BASIC` 且 HAL-05、HAL-07、HAL-08 通过。
- `BOARD_PASSED_DMA`：`BOARD_PASSED_NEGATIVE` 且 HAL-03、HAL-09 至少证明一条 IRQ/DMA 路径。

Only a real STM32U5 board run with raw logs and captures may select a `BOARD_` result.

## 8. 签署与声明限制

| Field | Value |
|---|---|
| Executor | pending |
| Reviewer | pending |
| Conclusion date | pending |
| Related issue / commit | pending |
| Open exceptions | no hardware/toolchain/initialized STM32U5 SDK in current environment |

在达到相应 `BOARD_` 级别前，只能引用已实际通过的 Host、QEMU 或 compile evidence。不得由本模板推导 hardware-validated、performance-validated、production-ready 或安全结论。
