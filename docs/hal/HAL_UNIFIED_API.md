# HAL 统一 API 文档

**版本**: 2.0  
**日期**: 2026-03-18  
**状态**: ✅ 完成

---

## 📋 概述

XinYi HAL (Hardware Abstraction Layer) 提供统一的硬件抽象层，支持多平台 (STM32/WCH/HC32) 的外设访问。

---

## 🏗️ 架构设计

### 分层架构

```
┌─────────────────────────────────┐
│    应用层 (Application)          │
└───────────────┬─────────────────┘
                │ 调用
┌───────────────▼─────────────────┐
│   Driver 层 (外围芯片驱动)        │
│  xy_bq25620_init()              │
│  xy_dht11_read()                │
└───────────────┬─────────────────┘
                │ 使用
┌───────────────▼─────────────────┐
│    HAL 层 (统一 API) ⭐           │
│  xy_hal_gpio_bind()             │
│  xy_hal_i2c_transfer()          │
│  xy_hal_uart_write()            │
└───────────────┬─────────────────┘
                │ 实现
┌───────────────▼─────────────────┐
│  平台层 (STM32/WCH/HC32/PC)      │
│  xy_hal_gpio_stm32.c            │
│  xy_hal_gpio_wch.c              │
│  xy_hal_gpio_hc32.c             │
└───────────────┬─────────────────┘
                │ 访问
┌───────────────▼─────────────────┐
│   MCU SDK (官方库)              │
│  STM32Cube / HC32 / MRS         │
└─────────────────────────────────┘
```

---

## 🔧 支持的模块

| 模块 | STM32 | WCH | HC32 | PC (QEMU) | 状态 |
|------|-------|-----|------|-----------|------|
| GPIO | ✅ | ✅ | ✅ | ✅ | 100% |
| UART | ✅ | ✅ | ✅ | ✅ | 100% |
| SPI | ✅ | ✅ | ✅ | ✅ | 100% |
| I2C | ✅ | ✅ | ✅ | ✅ | 100% |
| Timer | ⏳ | ⏳ | ⏳ | ⏳ | 0% |
| ADC | ⏳ | ⏳ | ⏳ | ⏳ | 0% |

---

## 📖 API 使用示例

### GPIO

```c
#include "xy_hal_gpio.h"

/* 绑定 GPIO 引脚 */
xy_hal_gpio_t led = xy_hal_gpio_bind("GPIOC.13");

/* 配置为输出 */
xy_hal_gpio_config_t config = {
    .mode = XY_HAL_GPIO_MODE_OUTPUT,
    .pull = XY_HAL_GPIO_PULL_NONE,
    .speed = XY_HAL_GPIO_SPEED_LOW
};
xy_hal_gpio_configure(led, 13, &config);

/* 控制 LED */
xy_hal_gpio_write(led, 13, 1);  // 点亮
xy_hal_gpio_write(led, 13, 0);  // 熄灭
xy_hal_gpio_toggle(led, 13);    // 翻转
```

### UART

```c
#include "xy_hal_uart.h"

/* 绑定 UART 外设 */
xy_hal_uart_t uart = xy_hal_uart_bind("USART2");

/* 配置波特率 */
xy_hal_uart_config_t config = {
    .baudrate = 115200,
    .word_length = 8,
    .stop_bits = 1,
    .parity = XY_HAL_UART_PARITY_NONE
};
xy_hal_uart_configure(uart, &config);

/* 发送数据 */
const char *msg = "Hello, XinYi!\n";
xy_hal_uart_write(uart, (uint8_t*)msg, strlen(msg));
```

### I2C

```c
#include "xy_hal_i2c.h"

/* 绑定 I2C 外设 */
xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");

/* 配置 I2C */
xy_hal_i2c_config_t config = {
    .clock_speed = 100000,  // 100kHz
    .addr_mode = XY_HAL_I2C_ADDR_7BIT,
    .own_address = 0
};
xy_hal_i2c_init(i2c, &config);

/* 写入传感器寄存器 */
uint8_t reg = 0x01;
uint8_t value = 0x10;
xy_hal_i2c_master_transmit(i2c, 0x68, &reg, 1, 100);
xy_hal_i2c_master_transmit(i2c, 0x68, &value, 1, 100);

/* 读取传感器数据 */
uint8_t data[6];
xy_hal_i2c_master_receive(i2c, 0x68, data, 6, 100);
```

### SPI

```c
#include "xy_hal_spi.h"

/* 绑定 SPI 外设 */
xy_hal_spi_t spi = xy_hal_spi_bind("SPI1");

/* 配置 SPI */
xy_hal_spi_config_t config = {
    .baudrate = 1000000,  // 1MHz
    .mode = XY_HAL_SPI_MODE_0,
    .bit_order = XY_HAL_SPI_MSB_FIRST
};
xy_hal_spi_init(spi, &config);

/* 读写数据 */
uint8_t tx_data = 0x55;
uint8_t rx_data;
xy_hal_spi_transfer(spi, &tx_data, &rx_data, 1, 100);
```

---

## 🎯 命名规范

### 设备绑定

使用字符串名称绑定设备，格式：`"外设名.引脚/片选"`

```c
/* GPIO */
xy_hal_gpio_bind("GPIOA.5");    // PA5
xy_hal_gpio_bind("GPIOC.13");   // PC13

/* UART */
xy_hal_uart_bind("USART1");     // USART1
xy_hal_uart_bind("UART4");      // UART4

/* I2C */
xy_hal_i2c_bind("I2C1");        // I2C1
xy_hal_i2c_bind("I2C3");        // I2C3

/* SPI */
xy_hal_spi_bind("SPI1");        // SPI1
xy_hal_spi_bind("SPI2");        // SPI2
```

### 错误处理

所有 HAL API 返回 `xy_hal_error_t` 类型：

```c
typedef enum {
    XY_HAL_OK = 0,          // 成功
    XY_HAL_ERROR,           // 通用错误
    XY_HAL_TIMEOUT,         // 超时
    XY_HAL_BUSY,            // 忙
    XY_HAL_EINVAL,          // 无效参数
    XY_HAL_ENODEV,          // 设备不存在
} xy_hal_error_t;
```

使用示例：
```c
xy_hal_error_t ret = xy_hal_gpio_configure(led, 13, &config);
if (ret != XY_HAL_OK) {
    // 处理错误
}
```

---

## 🧪 测试验证

### QEMU 测试平台

所有 HAL 模块已通过 QEMU STM32F4 验证：

```bash
cd tests/qemu_stm32f4/hal_test
./run_test.sh
```

**测试结果**: 11/11 通过 ✅

### 测试覆盖

| 测试项 | 平台 | 状态 |
|--------|------|------|
| GPIO Bind | STM32F4 QEMU | ✅ |
| GPIO Configure | STM32F4 QEMU | ✅ |
| GPIO Write/Read | STM32F4 QEMU | ✅ |
| GPIO Toggle | STM32F4 QEMU | ✅ |
| UART Write | STM32F4 QEMU | ✅ |
| SPI Transfer | STM32F4 QEMU | ✅ |
| I2C Write/Read | STM32F4 QEMU | ✅ |
| LED Blink | STM32F4 QEMU | ✅ |
| 综合工作流 | STM32F4 QEMU | ✅ |

---

## 📦 编译配置

### CMake 选项

```bash
# 选择目标平台
cmake -B build -DXY_MCU=stm32u5
cmake -B build -DXY_MCU=stm32f4
cmake -B build -DXY_MCU=wch
cmake -B build -DXY_MCU=hc32
cmake -B build -DXY_MCU=pc  # QEMU/PC 模拟

# 启用 HAL 模块
cmake -B build -DXY_HAL_GPIO=ON
cmake -B build -DXY_HAL_UART=ON
cmake -B build -DXY_HAL_SPI=ON
cmake -B build -DXY_HAL_I2C=ON
```

### Kconfig 配置

```
# HAL 配置
CONFIG_HAL_GPIO=y
CONFIG_HAL_UART=y
CONFIG_HAL_SPI=y
CONFIG_HAL_I2C=y

# 平台选择
CONFIG_MCU_STM32U5=y
# CONFIG_MCU_STM32F4 is not set
# CONFIG_MCU_WCH is not set
# CONFIG_MCU_HC32 is not set
```

---

## 📁 目录结构

```
components/hal/
├── inc/                    # 公共头文件
│   ├── xy_hal_gpio.h
│   ├── xy_hal_uart.h
│   ├── xy_hal_spi.h
│   ├── xy_hal_i2c.h
│   └── ...
├── stm32/stm32u5/          # STM32U5 实现
│   ├── xy_hal_gpio.c
│   ├── xy_hal_uart.c
│   └── ...
├── stm32/stm32f4/          # STM32F4 实现
│   ├── xy_hal_gpio.c
│   └── ...
├── wch/ch32u5/             # WCH 实现
│   ├── xy_hal_gpio.c
│   └── ...
├── hc32/hc32l021/          # HC32 实现
│   ├── xy_hal_gpio.c
│   └── ...
├── PC/                     # PC/QEMU 模拟
│   ├── xy_hal_gpio_pc.c
│   └── ...
└── tests/                  # 单元测试
    ├── test_gpio.c
    ├── test_uart.c
    └── ...
```

---

## 🔗 相关文档

- [HAL 验证报告](HAL_VALIDATION_REPORT_2026-03-18.md)
- [QEMU 调试指南](../../docs/QEMU_DEBUG_GUIDE.md)
- [开发者指南](../../docs/DEVELOPER_GUIDE.md)

---

**最后更新**: 2026-03-18  
**维护者**: XinYi Team
