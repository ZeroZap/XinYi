# 硬件移植指南

**版本**: 1.2.0
**最后更新**: 2026-11-02
**维护者**: XinYi Team

---

## 📋 概述

本指南介绍如何将 XinYi 框架移植到新的硬件平台。

移植分两步：**在 `components/hal/` 下落地一份外设实现**，**在 `cmake/platform/` 下落地一个平台片段**。
两者都以 `HAL_PLATFORM` 这个 CMake 缓存变量为入口，没有单独的配置头文件。

---

## 🎯 移植步骤

### 1. 创建实现目录

在 `components/hal/` 下创建新平台目录：

```
components/hal/
└── <platform>/          # 新平台目录
    ├── <series>/        # 系列目录
    │   ├── xy_hal_gpio.c
    │   ├── xy_hal_uart.c
    │   └── ...
    └── README.md
```

### 2. 实现 HAL 接口

复制参考实现并修改：

```bash
# 复制 STM32F4 实现作为参考
cp -r components/hal/stm32/stm32f4 components/hal/<platform>/<series>
```

**必须实现的接口**（文件名与公共头同名，见 `components/hal/inc/`）：

- `xy_hal_gpio.c` - GPIO/引脚
- `xy_hal_uart.c` - UART
- `xy_hal_spi.c` - SPI
- `xy_hal_i2c.c` - I2C
- `xy_hal_timer.c` - 定时器
- `xy_hal_pwm.c` - PWM
- `xy_hal_rtc.c` - RTC
- `xy_hal_dma.c` - DMA

`sys` / `time` / `delay` 这类基础能力在部分系列上由共享的 `stm32_hal.h` 提供，
没有专属文件不等于功能缺失——具体结论以证据矩阵为准，不以文件是否存在反推。

### 3. 函数命名规范

**重要**: 函数名必须与 `components/hal/inc/` 中的声明逐字一致（返回类型、参数类型、参数个数）：

```c
// ✅ 正确 - 与 components/hal/inc/xy_hal_gpio.h 的声明一致
xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config);

// ❌ 错误 - 不要添加平台后缀
xy_hal_error_t xy_hal_gpio_init_stm32(xy_hal_gpio_port_t port, uint8_t pin,
                                      const xy_hal_gpio_config_t *config);
```

### 4. 适配平台 HAL

使用平台原生的 HAL 库。下面的骨架取自 `components/hal/stm32/stm32f4/xy_hal_gpio.c`，
是这一层真正要做的事：**参数校验 → 枚举映射 → 单次 HAL 调用**。

```c
// STM32 示例
#include "stm32f4xx_hal.h"
#include "xy_hal_gpio.h"

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    // 先拒绝非法入参，别把判断交给 HAL 去做
    if (!port || !config || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = (uint32_t)(1U << pin);

    // XY 枚举 → 厂商位域：必须逐项映射，不能把 config->mode 直接赋进去
    switch (config->mode) {
    case XY_HAL_GPIO_MODE_OUTPUT: gpio_init.Mode = GPIO_MODE_OUTPUT_PP; break;
    case XY_HAL_GPIO_MODE_INPUT:  gpio_init.Mode = GPIO_MODE_INPUT;     break;
    case XY_HAL_GPIO_MODE_AF:     gpio_init.Mode = GPIO_MODE_AF_PP;     break;
    default:                      gpio_init.Mode = GPIO_MODE_ANALOG;    break;
    }

    HAL_GPIO_Init((GPIO_TypeDef *)port, &gpio_init);

    return XY_HAL_OK;
}
```

两点容易踩空：

- `xy_hal_gpio_port_t` 在嵌入式平台上是 `void *`，板上传进来的就是 `GPIOA`
  （`GPIO_TypeDef *`），实现里显式转回具体类型即可；
- 端口时钟不在 `xy_hal_gpio_init()` 里开——那是板级/启动代码的职责，
  这一层只负责把一次配置交给 HAL。

`pull` / `speed` / `otype` 同理，各有自己的映射表。真实实现
（`components/hal/stm32/stm32f4/xy_hal_gpio.c`）把三张表抽成了文件内的 `static` 映射函数，
这里为了只依赖公共头而写成 `switch`：**映射本身才是这一层的工作量**，直接赋值编译得过，
跑起来是错的。

### 5. 登记平台片段

平台在 CMake 配置期选定，编译器参数、宏、SDK 头路径、链接参数都由
`cmake/platform/<HAL_PLATFORM>.cmake` 提供。根 `CMakeLists.txt` 只做两件事：
找不到该文件就 `FATAL_ERROR`，找到就 `include()`。

```cmake
set(_platform_cmake "${CMAKE_SOURCE_DIR}/cmake/platform/${HAL_PLATFORM}.cmake")
if(NOT EXISTS "${_platform_cmake}")
    message(FATAL_ERROR
        "Unknown HAL_PLATFORM='${HAL_PLATFORM}'.\n"
        "Available: PC  STM32F4  STM32U5  STM32L4  WCH  HC32\n"
        "Pass -DHAL_PLATFORM=<value> to cmake.")
endif()
include("${_platform_cmake}")
```

一个平台片段要提供五组变量，缺任何一组都会在编译或链接期变成难查的问题：

```cmake
# cmake/platform/STM32F4.cmake
set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)

if(NOT DEFINED STM32F4_CHIP)
    set(STM32F4_CHIP STM32F407xx)     # 可用 -DSTM32F4_CHIP=... 覆盖
endif()

set(PLATFORM_DEFINES
    HAL_PLATFORM_STM32
    PLATFORM_STM32F4
    STM32F4
    STM32F4xx
    ${STM32F4_CHIP}
    USE_HAL_DRIVER
    ARM_CORTEX_M4
)

set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32f4
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/inc
    # ... 再加厂商 SDK 的头路径
)

set(PLATFORM_LINKER_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")
```

新增一个平台取值时，除了写出片段文件，还要把它加进根 `CMakeLists.txt` 的取值列表：

```cmake
set(HAL_PLATFORM "PC" CACHE STRING "Target platform: PC | STM32F4 | STM32U5 | STM32L4 | WCH | HC32")
set_property(CACHE HAL_PLATFORM PROPERTY STRINGS PC STM32F4 STM32U5 STM32L4 WCH HC32)
```

非 PC 平台的交叉编译器必须在 `project()` 之前设定，芯片型号类变量（`STM32F4_CHIP`、
`STM32U5_CHIP`、`STM32L4_CHIP`、`WCH_CHIP`、`HC32_CHIP`）都可以在命令行覆盖。

---

## 📝 平台选择

`Makefile` 是 `HAL_PLATFORM` 的薄包装：它把平台名转小写作为构建目录，其余全部转给 CMake。

```makefile
HAL_PLATFORM ?= PC
BUILD_TYPE   ?= Release

_PLATFORM := $(shell echo $(HAL_PLATFORM) | tr '[:upper:]' '[:lower:]')
BUILD_DIR ?= build/$(_PLATFORM)
```

```bash
make HAL_PLATFORM=STM32F4                      # 默认芯片 STM32F407xx
make HAL_PLATFORM=STM32L4 STM32L4_BOARD=pandora_stm32l475
```

Makefile 只把 `HAL_PLATFORM`、`STM32L4_BOARD`、`BUILD_TYPE`、`KCONFIG_OVERRIDES` 四个变量转给 CMake，
其余 `make FOO=bar` 会被 make 静默丢弃。所以要换芯片型号，直接调 CMake：

```bash
cmake -B build/stm32f4 -DHAL_PLATFORM=STM32F4 -DSTM32F4_CHIP=STM32F405xx -DCMAKE_BUILD_TYPE=Release
```

`STM32_HAL_ENABLED` 之类"这个板级目标用了厂商 HAL"的宏写在**板级** `CMakeLists.txt` 的
`target_compile_definitions()` 里，不是全局配置——移植新板时按 `boards/pandora_stm32l475/CMakeLists.txt`
的写法在自己的板级目标里补上。

---

## 🧪 测试验证

### 1. 编译测试

```bash
# 选择平台编译
make HAL_PLATFORM=STM32F4
```

### 2. 基本功能测试

HAL 的句柄就是平台外设本身：板上是 `GPIOA`（`GPIO_TypeDef *`）与 `&huart1`
（`UART_HandleTypeDef *`），PC 仿真传 `xy_hal_pc.h` 里的端口对象。下面这段在两种平台上都是同一份接口：

```c
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"

int gpio_and_uart_demo(xy_hal_gpio_port_t port, void *uart)
{
    xy_hal_gpio_config_t config = {
        .mode  = XY_HAL_GPIO_MODE_OUTPUT,
        .pull  = XY_HAL_GPIO_PULL_NONE,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_LOW,
    };
    if (xy_hal_gpio_init(port, 5, &config) != XY_HAL_OK) {
        return 1;
    }

    xy_hal_uart_config_t uart_config = {
        .baudrate = 115200,
        .wordlen  = XY_HAL_UART_WORDLEN_8B,
    };
    if (xy_hal_uart_init(uart, &uart_config) != XY_HAL_OK) {
        return 1;
    }

    if (xy_hal_uart_send(uart, (const uint8_t *)"Hello", 5, 1000) != XY_HAL_OK) {
        return 1;
    }
    return 0;
}
```

---

## 📚 参考资源

- [HAL 设计概览](../design/HAL_Component_Design_Overview.md)
- [STM32 实现参考](https://github.com/ZeroZap/XinYi/tree/main/components/hal/stm32)
- [错误码定义](../../components/hal/ERROR_CODES.md)

---

## 📞 获取帮助

- 📚 [硬件支持](index.md)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
