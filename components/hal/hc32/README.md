# XinYi HAL - HC32 平台支持

**版本**: 1.0.0  
**日期**: 2026-03-16  
**维护者**: XinYi Team  
**状态**: 🟡 开发中

---

## 📖 概述

XinYi HAL 的 HC32(小华半导体) 平台支持，基于 xhsc 官方驱动库实现统一 HAL 接口。

### 支持的 MCU 系列

| 系列 | 内核 | 主频 | Flash | SRAM | 状态 |
|------|------|------|-------|------|------|
| **HC32L021** | Cortex-M0+ | 32MHz | 16KB | 2KB | 🟡 开发中 |
| **HC32L19x** | Cortex-M0+ | 32MHz | 256KB | 32KB | 🟡 开发中 |
| **HC32F460** | Cortex-M4 | 200MHz | 512KB | 64KB | 📋 计划中 |

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────┐
│        XinYi 应用层                      │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│      XinYi HAL 统一接口层                │
│  (xy_hal_gpio.h / xy_hal_uart.h / ...)  │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│      HC32 平台适配层                     │
│  (hc32/xy_hal_gpio_device.c)            │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│      xhsc 官方驱动库                     │
│  (HC32L021/driver/inc + src)            │
└─────────────────────────────────────────┘
```

---

## 📁 目录结构

```
hal/
├── inc/                      # 统一 HAL 头文件
│   ├── xy_hal.h
│   ├── xy_hal_gpio.h
│   ├── xy_hal_uart.h
│   ├── xy_hal_spi.h
│   └── ...
│
├── hc32/                     # HC32 平台适配
│   ├── README.md            # 本文档
│   ├── CMakeLists.txt       # 构建配置
│   ├── hc32l021/            # HC32L021 系列
│   │   ├── xy_hal_gpio_device.c
│   │   ├── xy_hal_uart_device.c
│   │   ├── xy_hal_spi_device.c
│   │   └── ...
│   └── hc32l19x/            # HC32L19x 系列
│       └── ...
│
└── stm32/                    # STM32 平台适配 (参考)
└── wch/                      # WCH 平台适配 (参考)
```

---

## 🔧 开发进度

### HC32L021

| 模块 | 状态 | 文件 | 说明 |
|------|------|------|------|
| **GPIO** | 🟡 开发中 | xy_hal_gpio_device.c | 基础 GPIO 操作 |
| **UART** | 📋 计划 | xy_hal_uart_device.c | 串口通信 |
| **SPI** | 📋 计划 | xy_hal_spi_device.c | SPI 通信 |
| **I2C** | 📋 计划 | xy_hal_i2c_device.c | I2C 通信 |
| **Timer** | 📋 计划 | xy_hal_timer_device.c | 定时器 |
| **ADC** | 📋 计划 | xy_hal_adc_device.c | 模数转换 |

### HC32L19x

| 模块 | 状态 | 说明 |
|------|------|------|
| **所有模块** | 📋 计划 | 参考 HC32L021 实现 |

---

## 🚀 快速开始

### 1. 克隆依赖

```bash
# 确保 xhsc 库已更新
cd /home/eugene/zerozap
git clone https://github.com/ZeroZap/xhsc.git
```

### 2. 配置 CMake

```cmake
# CMakeLists.txt
set(HAL_PLATFORM "HC32" CACHE STRING "HAL Platform")
set(HC32_SERIES "HC32L021" CACHE STRING "HC32 Series")

# 添加 HC32 HAL
add_subdirectory(components/hal/hc32)
```

### 3. 包含头文件

```c
#include "xy_hal.h"
#include "xy_hal_gpio.h"

int main(void)
{
    /* 初始化 GPIO */
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .speed = XY_HAL_GPIO_SPEED_HIGH
    };
    
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.0");
    xy_hal_gpio_configure(gpio, XY_HAL_GPIO_PIN_0, &cfg);
    
    /* 切换 GPIO 电平 */
    xy_hal_gpio_write(gpio, XY_HAL_GPIO_PIN_0, 1);
    xy_hal_gpio_toggle(gpio, XY_HAL_GPIO_PIN_0);
    
    return 0;
}
```

---

## 📋 API 映射表

### GPIO

| XinYi HAL API | xhsc 官方 API | 状态 |
|--------------|--------------|------|
| `xy_hal_gpio_bind()` | - | 🟡 |
| `xy_hal_gpio_configure()` | `GPIO_Init()` | 🟡 |
| `xy_hal_gpio_write()` | `GPIO_WriteBit()` | 🟡 |
| `xy_hal_gpio_read()` | `GPIO_ReadInputDataBit()` | 🟡 |
| `xy_hal_gpio_toggle()` | `GPIO_ToggleBits()` | 🟡 |

### UART

| XinYi HAL API | xhsc 官方 API | 状态 |
|--------------|--------------|------|
| `xy_hal_uart_bind()` | - | 📋 |
| `xy_hal_uart_configure()` | `UART_Init()` | 📋 |
| `xy_hal_uart_write()` | `UART_SendData()` | 📋 |
| `xy_hal_uart_read()` | `UART_ReceiveData()` | 📋 |

---

## 🔗 相关文档

- [XinYi HAL 统一层](../README.md)
- [STM32 平台实现](../stm32/README.md)
- [WCH 平台实现](../wch/README.md)
- [xhsc 官方库](https://github.com/ZeroZap/xhsc)

---

## 🤝 贡献指南

### 开发新模块

1. 参考 STM32/WCH平台实现
2. 在 `hc32/hc32l021/` 创建对应文件
3. 实现统一 HAL API
4. 添加测试用例
5. 更新本文档

### 代码规范

- 遵循 XinYi 编码规范
- 使用统一错误码 (`xy_hal_error_t`)
- 添加完整 Doxygen 注释
- 提供使用示例

---

## 📝 开发日志

### 2026-03-16
- ✅ 创建 HC32 平台目录结构
- ✅ 编写 README 文档
- 🟡 开始 GPIO 驱动实现

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0  
**最后更新**: 2026-03-16
