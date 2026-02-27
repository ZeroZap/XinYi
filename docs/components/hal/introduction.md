# HAL 组件 - 硬件抽象层

**状态**: ✅ 完善 | **测试**: 11 用例 | **版本**: 2.0

---

## 📖 简介

XinYi 硬件抽象层（HAL）为常见嵌入式外设提供统一的、平台无关的接口。

### 核心特性

- ✅ **统一 API** - 跨平台可移植
- ✅ **完整外设** - GPIO/UART/SPI/I2C 等 17+ 外设
- ✅ **多平台支持** - STM32/HC32/WCH/PC 仿真
- ✅ **零开销** - 编译时选择平台
- ✅ **DMA 支持** - 高效数据传输

### 支持的外设

| 类别 | 外设 | 说明 |
|------|------|------|
| **数字 I/O** | GPIO | 输入/输出/中断 |
| **串行通信** | UART | 异步串口 |
| **串行通信** | SPI | 同步串口 |
| **串行通信** | I2C | 两线接口 |
| **串行通信** | I2S | 音频接口 |
| **串行通信** | CAN | 总线接口 |
| **模拟 I/O** | ADC | 模数转换 |
| **模拟 I/O** | DAC | 数模转换 |
| **定时器** | Timer | 通用定时器 |
| **定时器** | PWM | 脉宽调制 |
| **定时器** | RTC | 实时时钟 |
| **定时器** | WDG | 看门狗 |
| **数据传输** | DMA | 直接内存访问 |
| **数据传输** | EXTI | 外部中断 |
| **低功耗** | LP Timer | 低功耗定时器 |
| **其他** | RNG | 随机数生成 |
| **其他** | IR | 红外接口 |
| **其他** | TGPIO | 触摸 GPIO |

---

## 🚀 快速开始

### 1. 配置平台

```c
// xy_hal_cfg.h

// 选择平台
#define XY_HAL_PLATFORM STM32
// #define XY_HAL_PLATFORM HC32
// #define XY_HAL_PLATFORM WCH
// #define XY_HAL_PLATFORM PC_SIM
```

### 2. GPIO 使用示例

```c
#include "xy_hal_gpio.h"

int main(void) {
    // 配置 GPIO
    xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_LOW,
    };
    
    // 初始化 PA5
    xy_hal_gpio_init(GPIOA, 5, &config);
    
    while (1) {
        // 切换 LED
        xy_hal_gpio_toggle(GPIOA, 5);
        xy_hal_delay_ms(1000);
    }
    
    return 0;
}
```

### 3. UART 使用示例

```c
#include "xy_hal_uart.h"

int main(void) {
    // 配置 UART
    xy_hal_uart_config_t config = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
    };
    
    // 初始化 UART1
    xy_hal_uart_init(UART1, &config);
    
    // 发送数据
    const char *msg = "Hello, HAL!\r\n";
    xy_hal_uart_send(UART1, (const uint8_t *)msg, strlen(msg), 1000);
    
    return 0;
}
```

### 4. I2C 传感器读取示例

```c
#include "xy_hal_i2c.h"

int main(void) {
    // 配置 I2C
    xy_hal_i2c_config_t config = {
        .speed = XY_HAL_I2C_SPEED_STANDARD,  // 100kHz
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
    };
    
    // 初始化 I2C1
    xy_hal_i2c_init(I2C1, &config);
    
    // 读取传感器 (地址 0x68, 寄存器 0x00)
    uint8_t data[2];
    xy_hal_i2c_mem_read(I2C1, 0x68, 0x00, data, 2, 1000);
    
    return 0;
}
```

---

## 📋 API 参考

### GPIO

| 函数 | 说明 |
|------|------|
| `xy_hal_gpio_init()` | 初始化 GPIO |
| `xy_hal_gpio_set()` | 设置高电平 |
| `xy_hal_gpio_clear()` | 设置低电平 |
| `xy_hal_gpio_toggle()` | 切换电平 |
| `xy_hal_gpio_read()` | 读取电平 |

### UART

| 函数 | 说明 |
|------|------|
| `xy_hal_uart_init()` | 初始化 UART |
| `xy_hal_uart_send()` | 发送数据 |
| `xy_hal_uart_receive()` | 接收数据 |
| `xy_hal_uart_dma_send()` | DMA 发送 |

### SPI

| 函数 | 说明 |
|------|------|
| `xy_hal_spi_init()` | 初始化 SPI |
| `xy_hal_spi_transfer()` | 数据传输 |
| `xy_hal_spi_send()` | 发送数据 |
| `xy_hal_spi_receive()` | 接收数据 |

### I2C

| 函数 | 说明 |
|------|------|
| `xy_hal_i2c_init()` | 初始化 I2C |
| `xy_hal_i2c_write()` | 写入数据 |
| `xy_hal_i2c_read()` | 读取数据 |
| `xy_hal_i2c_mem_write()` | 内存写入 |
| `xy_hal_i2c_mem_read()` | 内存读取 |

---

## 🔧 平台支持

### STM32

| 系列 | 状态 | 备注 |
|------|------|------|
| STM32U5 | ✅ 完整 | 参考实现 |
| STM32F4 | ✅ 完整 | - |
| STM32F1 | ✅ 完整 | - |
| STM32L4 | ✅ 完整 | - |
| STM32H7 | 🔄 进行中 | - |

### 其他平台

| 平台 | 状态 | 备注 |
|------|------|------|
| HC32 | 📋 占位符 | 可实现 |
| WCH | 📋 占位符 | 可实现 |
| PC 仿真 | ✅ 完整 | 开发测试 |

---

## 🧪 测试用例

HAL 组件包含 11 个测试用例：

| 测试类别 | 用例数 | 说明 |
|----------|--------|------|
| 版本宏 | 1 | HAL 版本 |
| 错误码 | 2 | 值/顺序 |
| Handle | 3 | 结构/初始化/操作 |
| 状态类型 | 1 | 传统类型 |
| 子模块 | 1 | 头文件包含 |
| 配置 | 1 | 配置验证 |
| PC 仿真 | 1 | 仿真层 |

运行测试：

```bash
ctest -R test_hal --output-on-failure
```

---

## 📝 移植指南

### 移植到新平台

1. **创建平台目录**
   ```
   components/hal/<platform>/
   ```

2. **实现 HAL 接口**
   - 复制 `stm32/` 目录作为参考
   - 实现所有 `xy_hal_*.c` 文件
   - 保持函数签名一致

3. **配置平台**
   ```c
   // xy_hal_cfg.h
   #define XY_HAL_PLATFORM <your_platform>
   ```

4. **测试验证**
   - 运行单元测试
   - 硬件在环测试

---

## 📚 相关文档

- [支持平台](platforms.md)
- [API 参考](api-reference.md)
- [移植指南](porting.md)
- [错误码](error-codes.md)

---

## 📞 获取帮助

- 📚 [API 文档](api-reference.md)
- ❓ [常见问题](../about/faq.md)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
