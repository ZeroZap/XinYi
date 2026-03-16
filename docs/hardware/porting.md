# 硬件移植指南

**版本**: 1.1.0  
**最后更新**: 2026-03-16  
**维护者**: XinYi Team

---

## 📋 概述

本指南介绍如何将 XinYi 框架移植到新的硬件平台。

---

## 🎯 移植步骤

### 1. 创建平台目录

在 `components/hal/` 下创建新平台目录：

```
components/hal/
└── <platform>/          # 新平台目录
    ├── <series>/        # 系列目录
    │   ├── xy_hal_pin.c
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

**必须实现的接口**:
- `xy_hal_pin.c` - GPIO/引脚
- `xy_hal_uart.c` - UART
- `xy_hal_spi.c` - SPI
- `xy_hal_i2c.c` - I2C
- `xy_hal_timer.c` - 定时器
- `xy_hal_pwm.c` - PWM
- `xy_hal_rtc.c` - RTC
- `xy_hal_dma.c` - DMA

### 3. 函数命名规范

**重要**: 函数名必须与 `inc/` 中的声明完全一致！

```c
// ✅ 正确
xy_hal_error_t xy_hal_gpio_init(void *port, uint16_t pin, const xy_hal_gpio_config_t *config)

// ❌ 错误 - 不要添加平台后缀
xy_hal_error_t xy_hal_gpio_init_stm32(void *port, uint16_t pin, const xy_hal_gpio_config_t *config)
```

### 4. 适配平台 HAL

使用平台原生的 HAL 库：

```c
// STM32 示例
#include "stm32f4xx_hal.h"

xy_hal_error_t xy_hal_gpio_init(void *port, uint16_t pin, const xy_hal_gpio_config_t *config)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 启用时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 配置 GPIO
    GPIO_InitStruct.Pin = (1 << pin);
    GPIO_InitStruct.Mode = config->mode;
    GPIO_InitStruct.Pull = config->pull;
    GPIO_InitStruct.Speed = config->speed;
    
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    
    return XY_HAL_OK;
}
```

### 5. 更新构建系统

#### CMakeLists.txt

```cmake
# 添加平台支持
if(PLATFORM STREQUAL "stm32")
    add_subdirectory(components/hal/stm32)
elseif(PLATFORM STREQUAL "hc32")
    add_subdirectory(components/hal/hc32)
endif()
```

#### Makefile

```makefile
# 平台选择
PLATFORM = stm32
SERIES = stm32f4

C_SOURCES += $(wildcard components/hal/$(PLATFORM)/$(SERIES)/*.c)
C_DEFS += -D$(PLATFORM)_HAL_ENABLED
```

---

## 📝 平台特定配置

### STM32 平台

```c
// xy_hal_cfg.h
#define XY_HAL_PLATFORM STM32
#define STM32_SERIES stm32f4
```

### HC32 平台

```c
// xy_hal_cfg.h
#define XY_HAL_PLATFORM HC32
#define HC32_SERIES hc32f4a0
```

---

## 🧪 测试验证

### 1. 编译测试

```bash
# 选择平台编译
make PLATFORM=stm32 SERIES=stm32f4
```

### 2. 基本功能测试

```c
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"

int main(void) {
    // 测试 GPIO
    xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
    };
    xy_hal_gpio_init(GPIOA, 5, &config);
    
    // 测试 UART
    xy_hal_uart_config_t uart_config = {
        .baudrate = 115200,
    };
    xy_hal_uart_init(UART1, &uart_config);
    
    // 发送测试消息
    xy_hal_uart_send(UART1, (uint8_t*)"Hello", 5, 1000);
    
    return 0;
}
```

---

## 📚 参考资源

- [HAL 设计概览](../design/HAL_Component_Design_Overview.md)
- [STM32 实现参考](https://github.com/ZeroZap/XinYi/tree/main/components/hal/stm32)
- [错误码定义](../components/hal/ERROR_CODES.md)

---

## 📞 获取帮助

- 📚 [硬件支持](index.md)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
