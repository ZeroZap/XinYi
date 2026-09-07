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
3. startup owner 缺少当前精确调用集合时也失败，防止文档与实现漂移。

该 policy 与 clean target build 仅证明 source/compile ownership，**不构成新的 B1/B2**，
也不升级现有外设、FOTA、复位、显示或 RTOS 实板证据。
