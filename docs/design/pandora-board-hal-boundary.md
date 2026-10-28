# Pandora board HAL 架构边界

Pandora STM32L475VE 的应用与组件调用链固定为：

```text
application → component → xy_hal_* → platform backend → STM32 vendor HAL
```

应用、组件、board entry 和 board 辅助模块不得直接调用 `HAL_*`。外设初始化、传输、
中断控制、复位、RTC backup、watchdog 和 Flash 操作均由 `xy_hal_*` 公共接口及
`components/hal/stm32/stm32l4/` backend 持有。

## Board-local startup 例外

`boards/pandora_stm32l475/pandora_platform_startup.c` 是唯一的 board-local vendor clock owner。
它负责 Pandora 的 HSE/PLL、总线分频、电压档位和 Flash latency，并只允许以下三个直接
vendor HAL 调用：

- `HAL_PWREx_ControlVoltageScaling`
- `HAL_RCC_OscConfig`
- `HAL_RCC_ClockConfig`

该白名单是精确集合，不允许按 `HAL_RCC_*`、`HAL_PWR*` 或文件目录做宽泛豁免。
`__HAL_RCC_PWR_CLK_ENABLE()` 是该 owner 内为电压档位配置启用 PWR 时钟的 vendor macro，
不扩展可调用的 `HAL_*` 函数集合。新增时钟策略必须先修改本架构决策和 fail-closed policy，
不得复制到各个 board entry。

启动/时钟 owner 例外不改变上层分层，也不要求再包装一层只对 Pandora 有意义的通用 HAL。
startup/IRQ 汇编、vector dispatch 和 canonical STM32L4 backend 仍是允许接触 vendor/CMSIS
实现细节的底层边界，但不属于 board application/component owner。

## 自动守护与证据边界

`pandora_hal_boundary_policy` 会递归扫描 `boards/pandora_stm32l475/**/*.c`：

1. 除 `pandora_platform_startup.c` 外，任何直接 `HAL_*()` 调用均失败；
2. startup owner 出现白名单外调用时失败；
3. startup owner 缺少当前精确调用集合时也失败，防止文档与实现漂移；
4. 该 owner 的 Host 契约必须留在 fake vendor seam 上：`tests/unit/hal_component/fake_vendor/stm32l4xx_hal.h`
   必须以 `BEFORE` 先于 board 头路径进入 `test_pandora_platform_startup`，且仍保留三个白名单
   vendor 函数与 `__HAL_RCC_PWR_CLK_ENABLE` 宏，seam 或顺序被删/被换即失败。

Clock/power Host 契约 `pandora_platform_startup`（25 个用例）把上述三个 vendor 调用与 PWR
时钟宏当作可计数、可注入失败的钩子，验证升序调用链（PWR 时钟 → VOS1 电压档 → 振荡器 →
时钟切换）、逐阶段失败短路、只启用 HSE、PLL 由 HSE 驱动、四条总线全部 DIV1、Flash latency
为 4 个等待周期，并用 board 自身 `HSE_VALUE=8000000` 推导 datasheet 不变量（VCO 输入/输出
范围、SYSCLK 恰为 STM32L475 上限 80 MHz）。fake 常量取真实 vendor 数值，因此改动任一 PLL
因子或分频都会先在这里失败，而不是产出一个静默超频的 board 镜像。

已知潜伏约束（不属当前缺陷）：`PLLQ = DIV2` 使 CLK48 域为 80 MHz，而 SDMMC1 / RNG / USB
要求恰为 48 MHz。当前没有任何 Pandora 镜像选择 CLK48 消费者，`CLK48SEL` 保持默认 PLLQ；
启用其中任一外设时必须先为它配独立时钟源，不得直接复用本 PLL。

该 policy 与 clean target build 仅证明 source/compile ownership，**不构成新的 B1/B2**，
也不升级现有外设、FOTA、复位、显示或 RTOS 实板证据。Host 契约同样只是 Host/compile 级：
不证明 HSE 起振、PLL 锁定、真实 80 MHz SYSCLK、总线时序或电源切换时序。
