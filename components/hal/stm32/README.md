# STM32 HAL 后端

**日期**: 2026-11-02
**状态**: 逐平台实现/编译/实板状态以
[平台实现与证据矩阵](../../../docs/validation/hal-platform-evidence-matrix.md) 为准

本目录是 `components/hal/inc/` 公共接口的 STM32 后端。命名与包含规则只有一条：
**后端实现的函数名必须与公共头里的声明逐字一致**，不要加平台后缀。

---

## 目录结构（真实布局）

```text
components/hal/stm32/
├── stm32f4/                  # STM32F4 wrapper + stm32f4xx_hal_conf.h
├── stm32l4/                  # STM32L4 wrapper + stm32l4xx_hal_conf.h（Pandora 参考板）
├── stm32u5/                  # STM32U5 wrapper + stm32u5xx_hal_conf.h
├── stm32_hal.h               # 族宏 -> 厂商头的公共入口
├── example_usage.c           # 板侧阅读样例（不被任何构建目标编译，见下）
├── CMakeLists.txt            # 独立模板工程，未被根构建引用
└── Makefile.template         # 独立模板，未被根构建引用
```

根构建**不使用**本目录的 `CMakeLists.txt` / `Makefile.template`：平台由根 CMake 的
`HAL_PLATFORM` 选择，源文件由 `components/hal/CMakeLists.txt` 按平台 glob。两个模板文件是
2025-10 的独立工程遗留物，里面的头文件清单仍写着 `../inc/xy_hal_pin.h`（该头已不存在），
可以 configure、不能 build —— 它们既不是入口也不是证据。

---

## 每个系列实际有哪些 wrapper

| 系列 | 已有 `xy_hal_*.c` |
|------|-------------------|
| `stm32f4/` | adc, dac, dma, exti, flash, gpio, i2c, lp_timer, pwm, rng, rtc, spi, timer, uart, wdg |
| `stm32l4/` | delay, dma, flash, gpio, i2c, qspi, rtc, spi, sys, time, timer, uart, wdg |
| `stm32u5/` | adc, can, dac, dma, exti, flash, gpio, i2c, i2s, lp_timer, pwm, rng, rtc, spi, sys, time, timer, uart, wdg |

`stm32f4/` 少 `sys`/`time`/`delay` 的 dedicated 文件不等于功能缺失——那几类由共享
`stm32_hal.h` 与其他路径提供，具体到外设的结论仍以证据矩阵为准。L4 的多数外设复用 F4 wrapper
（`components/hal/CMakeLists.txt` 的 L4 分支把 `stm32/stm32f4` 也 glob 进来），只有上表列出的
文件是 L4 专属。

---

## 函数命名：与公共头逐字一致

```text
错：加了平台后缀，公共头里没有这个名字
    xy_hal_gpio_init_stm32(...)

对：与 components/hal/inc/xy_hal_gpio.h 的声明完全一致
    xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                    const xy_hal_gpio_config_t *config);
```

注意接口本身在 2.0 里已经从 `xy_hal_pin_*` 改名为 `xy_hal_gpio_*`：
`xy_hal_pin_init()` / `xy_hal_pin_config_t` **不再存在**，现在分别是 `xy_hal_gpio_init()` 与
`xy_hal_gpio_config_t`（含 `mode` / `pull` / `otype` / `speed` / `alternate`）。仓库里仍有几份
2025-10 的 HAL 设计文档引用旧名，属于待清理的文档债，不是可用接口。

---

## 编译与平台选择

```bash
make HAL_PLATFORM=STM32F4              # 可用 BUILD_TESTS=ON
make HAL_PLATFORM=STM32L4              # Pandora 参考板
make HAL_PLATFORM=STM32U5
```

直接 CMake：

```bash
cmake -B build/l4 -S . -DHAL_PLATFORM=STM32L4 -DCMAKE_BUILD_TYPE=Release \
      -DSTM32L4_CHIP=STM32L431xx
cmake --build build/l4 -j"$(nproc)"
```

要点：

- `HAL_PLATFORM` 是 CMake 缓存变量（`PC`/`STM32F4`/`STM32U5`/`STM32L4`/`WCH`/`HC32`），不是
  Kconfig 符号；`STM32*_CHIP` 可覆盖具体型号。
- **STM32F4 与 STM32L4 两个分支都必须定义 `STM32_HAL_ENABLED`**：`stm32f4/*.c` 全部包在这个
  guard 里（L4 分支复用它们），缺了它每个 TU 会编成空对象、archive 导出 0 个 `xy_hal_*` 符号而
  平台 gate 依旧"通过"。这条由 CTest `hal_platform_build_contract` 锁定。
- STM32U5 的 `xy_hal_i2s.c` 是显式 `NOT_SUPPORTED` stub，不要当实现用。
- 厂商 SDK 以 submodule 形式位于 `MCU/ST/`；缺头文件通常是 submodule 未初始化，不是应该改
  include 路径。

---

## 系列头与族宏

`stm32_hal.h` 按族宏引入对应厂商头（每个系列目录里还有自己的 `*_hal_conf.h`）：

```c
#if defined(STM32F4)
    #include "stm32f4xx_hal.h"
#elif defined(STM32L4)
    #include "stm32l4xx_hal.h"
#elif defined(STM32U5)
    #include "stm32u5xx_hal.h"
#endif
```

`components/hal/CMakeLists.txt` 负责把 `HAL_PLATFORM_STM32`、族宏（`STM32F4`/`STM32L4`/`STM32U5`）
与 `STM32_HAL_ENABLED` 一起定义好；手写 `-D` 通常是在绕开一个已经存在的机制。

---

## 样例

- **可执行的真样例**：`components/hal/examples/hal_quickstart.c`，由 CTest `hal_quickstart` 编译
  运行（PC 后端）。这是唯一被构建和断言的 HAL 用法样例。
- **板侧阅读样例**：本目录 `example_usage.c` 演示板上的 GPIO/UART/SPI/I2C/PWM/Timer/RTC 调用，
  但它需要厂商 `*_HandleTypeDef` extern 与 `STM32_HAL_ENABLED`，**任何构建目标都不编译它**，因此
  它不构成证据。引用其中代码前请对照公共头，或在 Host 上改为 `hal_quickstart.c` 的形式。

---

## 相关文档

- [HAL 组件说明](../README.md)
- [HAL 统一 API 文档](../../../docs/hal/HAL_UNIFIED_API.md)
- [平台实现与证据矩阵](../../../docs/validation/hal-platform-evidence-matrix.md)
- [HAL 移植指南](../../../docs/hardware/porting.md)

---

**最后更新**: 2026-11-02
