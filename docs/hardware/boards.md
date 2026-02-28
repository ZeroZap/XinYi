# 支持的开发板

**最后更新**: 2026-02-28

---

## 📋 概述

XinYi 框架支持多种 STM32 开发板，以及其他平台的开发板。

---

## 🎯 推荐开发板

### STM32 系列

| 开发板 | MCU | 特点 | 状态 |
|--------|-----|------|------|
| **NUCLEO-U575ZI** | STM32U575ZI | 超低功耗，资源丰富 | ✅ 完整支持 |
| **NUCLEO-F429ZI** | STM32F429ZI | 高性能，经典选择 | ✅ 完整支持 |
| **NUCLEO-F103RB** | STM32F103RB | 入门级，成本低 | ✅ 支持 |
| **NUCLEO-L476RG** | STM32L476RG | 低功耗，高性能 | ✅ 支持 |

### 其他平台

| 开发板 | MCU | 状态 |
|--------|-----|------|
| 待添加 | HC32 | 📋 计划中 |
| 待添加 | WCH | 📋 计划中 |

---

## 🔧 开发板配置

### NUCLEO-U575ZI

**资源**:
- Flash: 2MB
- SRAM: 786KB
- 主频: 160MHz
- 外设: UARTx5, SPIx3, I2Cx4, ADCx3

**引脚定义**:
```c
// 板载 LED
#define LED_GREEN    PA5
#define LED_BLUE     PB7
#define LED_RED      PB14

// 板载按键
#define USER_BUTTON  PC13
```

**使用示例**:
```c
#include "xy_hal_gpio.h"

int main(void) {
    // 配置 LED
    xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
    };
    
    xy_hal_gpio_init(GPIOA, 5, &config);
    
    while (1) {
        xy_hal_gpio_toggle(GPIOA, 5);
        xy_hal_delay_ms(500);
    }
    
    return 0;
}
```

---

## 📦 购买建议

### 入门级

- **NUCLEO-F103RB** - 约 ¥30-50
  - 适合初学者
  - 资源足够学习
  - 社区资源丰富

### 进阶级

- **NUCLEO-F429ZI** - 约 ¥80-120
  - 高性能
  - 适合复杂项目
  - 完整的 XinYi 支持

### 专业级

- **NUCLEO-U575ZI** - 约 ¥100-150
  - 超低功耗
  - 最新 STM32U5 系列
  - XinYi 参考平台

---

## 🔗 相关文档

- [快速入门](../getting-started/quickstart.md)
- [HAL 使用指南](../components/hal/introduction.md)
- [移植指南](porting.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
