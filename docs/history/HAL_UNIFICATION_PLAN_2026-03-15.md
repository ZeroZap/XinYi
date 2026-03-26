# HAL 统一开发计划

**日期**: 2026-03-15  
**维护者**: ese  
**状态**: 🟡 进行中

---

## 📋 任务概述

根据 `DEVELOPMENT_PRIORITY.md` 中的 P0 核心架构任务 #2，完成 HAL 统一工作：

**目标**: 统一多平台 HAL API，参考 Zephyr 设备模型

**TODO**:
- [x] 审计现有 HAL API (GPIO/UART/SPI/I2C)
- [ ] 统一 GPIO API (STM32/WCH/HC32)
- [ ] 统一 UART API
- [ ] 统一 SPI/I2C API
- [ ] 添加 HAL 测试套件

**工时**: 12 小时

---

## 🔍 现有 HAL 架构审计

### 头文件结构 (components/hal/inc/)

| 文件 | 行数 | 状态 | 说明 |
|------|------|------|------|
| `xy_hal_gpio.h` | 350+ | ✅ 良好 | 完整 GPIO API |
| `xy_hal_uart.h` | 180+ | ✅ 良好 | 完整 UART API |
| `xy_hal_spi.h` | 180+ | ✅ 良好 | 完整 SPI API |
| `xy_hal_i2c.h` | 180+ | ✅ 良好 | 完整 I2C API |
| `xy_hal_adc.h` | 300+ | ✅ 良好 | 完整 ADC API |
| `xy_hal_pwm.h` | 350+ | ✅ 良好 | 完整 PWM API |
| `xy_hal_timer.h` | 150+ | ✅ 良好 | 定时器 API |
| `xy_hal_wdg.h` | 120+ | ✅ 良好 | 看门狗 API |
| `xy_hal_rtc.h` | 100+ | ✅ 良好 | RTC API |
| `xy_hal_flash.h` | 120+ | ✅ 良好 | Flash API |
| `xy_hal_dma.h` | 120+ | ✅ 良好 | DMA API |
| `xy_hal_sys.h` | 500+ | ✅ 良好 | 系统 HAL |

### 平台实现

| 平台 | 目录 | 实现文件 | 状态 |
|------|------|---------|------|
| **STM32U5** | `hal/stm32/stm32u5/` | 15 个驱动文件 | ✅ 完整 |
| **STM32F4** | `hal/stm32/stm32f4/` | 待实现 | ⏳ 空 |
| **WCH CH32** | `hal/wch/src/` | 待检查 | ⏳ 部分 |
| **HC32** | `hal/hc32/` | 未创建 | ❌ 缺失 |

---

## 🎯 统一 API 设计原则

### 1. 与 Zephyr 设备模型对齐

参考 Zephyr 的 device 模型设计：

```c
/* Zephyr 风格设备模型 */
struct device {
    const char *name;
    void *config;
    void *data;
    const struct device_api *api;
};

/* 设备 API 结构 */
struct gpio_driver_api {
    int (*pin_configure)(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags);
    int (*port_get_raw)(const struct device *dev, gpio_port_value_t *value);
    int (*port_set_masked_raw)(const struct device *dev, gpio_port_pins_t mask, gpio_port_value_t value);
};
```

### 2. XinYi HAL 统一结构

```c
/* 统一设备基类 */
typedef struct xy_hal_device {
    const char *name;              /* 设备名称 */
    xy_hal_dev_type_t type;        /* 设备类型 */
    const void *config;            /* 配置数据 */
    void *data;                    /* 运行时数据 */
    const void *api;               /* API 虚表 */
    uint8_t flags;                 /* 设备标志 */
} xy_hal_device_t;

/* 统一 GPIO API */
typedef struct xy_hal_gpio_api {
    xy_hal_error_t (*configure)(xy_hal_device_t *dev, uint32_t pin, 
                                const xy_hal_gpio_config_t *cfg);
    xy_hal_error_t (*write)(xy_hal_device_t *dev, uint32_t pin, uint8_t value);
    int32_t (*read)(xy_hal_device_t *dev, uint32_t pin);
    xy_hal_error_t (*toggle)(xy_hal_device_t *dev, uint32_t pin);
} xy_hal_gpio_api_t;

/* 使用示例 */
xy_hal_device_t *gpio_dev = xy_hal_device_get("GPIOA");
xy_hal_gpio_api_t *api = (xy_hal_gpio_api_t *)gpio_dev->api;

api->configure(gpio_dev, 5, &cfg);
api->write(gpio_dev, 5, 1);
```

### 3. 保持向后兼容

现有 API 继续使用，新 API 作为可选增强：

```c
/* 旧 API (保留) */
xy_hal_gpio_init(GPIOA, 5, &cfg);
xy_hal_gpio_write(GPIOA, 5, 1);

/* 新 API (统一设备模型) */
xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
xy_hal_gpio_configure(gpio, 5, &cfg);
xy_hal_gpio_write(gpio, 5, 1);
```

---

## 📝 实现计划

### 阶段 1: GPIO 统一 (3h)

**文件**:
- `components/hal/inc/xy_hal_gpio_types.h` - 统一类型定义
- `components/hal/inc/xy_hal_gpio_dev.h` - 设备模型 API
- `components/hal/stm32/stm32u5/xy_hal_gpio_device.c` - STM32U5 实现
- `components/hal/wch/src/xy_hal_gpio_device.c` - WCH 实现

**内容**:
- 统一 GPIO 引脚定义
- 统一 GPIO 配置结构
- 设备模型 API 封装
- 平台特定实现

---

### 阶段 2: UART 统一 (3h)

**文件**:
- `components/hal/inc/xy_hal_uart_types.h`
- `components/hal/inc/xy_hal_uart_dev.h`
- `components/hal/stm32/stm32u5/xy_hal_uart_device.c`
- `components/hal/wch/src/xy_hal_uart_device.c`

**内容**:
- 统一 UART 配置 (波特率/数据位/停止位/校验)
- 统一收发 API (阻塞/非阻塞/中断/DMA)
- 回调函数标准化

---

### 阶段 3: SPI/I2C 统一 (3h)

**文件**:
- `components/hal/inc/xy_hal_spi_types.h`
- `components/hal/inc/xy_hal_i2c_types.h`
- `components/hal/inc/xy_hal_i2c_dev.h`
- 平台实现文件

**内容**:
- 统一总线配置
- 统一传输 API
- 设备地址管理

---

### 阶段 4: HAL 测试套件 (3h)

**文件**:
- `components/hal/tests/test_gpio.c`
- `components/hal/tests/test_uart.c`
- `components/hal/tests/test_spi.c`
- `components/hal/tests/test_i2c.c`

**内容**:
- GPIO 基本功能测试
- UART 回环测试
- SPI 主从通信测试
- I2C 设备扫描测试

---

## 🔧 统一 API 示例

### GPIO 统一 API

```c
#include "xy_hal_gpio_dev.h"

/* 1. 获取 GPIO 设备 */
xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
if (!gpio) {
    /* 错误处理 */
    return;
}

/* 2. 配置 GPIO */
xy_hal_gpio_config_t cfg = {
    .mode = XY_HAL_GPIO_MODE_OUTPUT,
    .pull = XY_HAL_GPIO_PULL_NONE,
    .otype = XY_HAL_GPIO_OTYPE_PP,
    .speed = XY_HAL_GPIO_SPEED_HIGH,
};

xy_hal_gpio_configure(gpio, 5, &cfg);  /* PA5 推挽输出 */

/* 3. 控制 GPIO */
xy_hal_gpio_write(gpio, 5, 1);  /* 高电平 */
xy_hal_gpio_write(gpio, 5, 0);  /* 低电平 */

/* 4. 读取 GPIO */
int32_t value = xy_hal_gpio_read(gpio, 3);  /* 读取 PA3 */

/* 5. 翻转 GPIO */
xy_hal_gpio_toggle(gpio, 5);

/* 6. 释放设备 */
xy_hal_gpio_unbind(gpio);
```

### UART 统一 API

```c
#include "xy_hal_uart_dev.h"

/* 1. 获取 UART 设备 */
xy_hal_uart_t uart = xy_hal_uart_bind("USART1");

/* 2. 配置 UART */
xy_hal_uart_config_t cfg = {
    .baudrate = 115200,
    .wordlen = XY_HAL_UART_WORDLEN_8B,
    .stopbits = XY_HAL_UART_STOPBITS_1,
    .parity = XY_HAL_UART_PARITY_NONE,
    .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
    .mode = XY_HAL_UART_MODE_TX_RX,
};

xy_hal_uart_configure(uart, &cfg);

/* 3. 发送数据 (阻塞) */
const char *msg = "Hello UART!";
xy_hal_uart_write(uart, msg, strlen(msg), 100);

/* 4. 接收数据 (阻塞) */
char buf[64];
size_t len = xy_hal_uart_read(uart, buf, sizeof(buf), 100);

/* 5. 异步发送 (中断/DMA) */
xy_hal_uart_write_async(uart, msg, strlen(msg), tx_complete_cb, NULL);

/* 6. 释放设备 */
xy_hal_uart_unbind(uart);
```

### I2C 统一 API

```c
#include "xy_hal_i2c_dev.h"

/* 1. 获取 I2C 设备 */
xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");

/* 2. 配置 I2C */
xy_hal_i2c_config_t cfg = {
    .clock_speed = 400000,  /* 400kHz */
    .addr_mode = XY_HAL_I2C_ADDR_7BIT,
    .duty_cycle = XY_HAL_I2C_DUTY_2,
};

xy_hal_i2c_configure(i2c, &cfg);

/* 3. 写入寄存器 */
uint8_t reg = 0x00;
uint8_t value = 0x55;
xy_hal_i2c_reg_write(i2c, 0x68, &reg, 1, &value, 1, 100);

/* 4. 读取寄存器 */
uint8_t rx_data;
xy_hal_i2c_reg_read(i2c, 0x68, &reg, 1, &rx_data, 1, 100);

/* 5. 扫描 I2C 总线 */
for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    if (xy_hal_i2c_probe(i2c, addr, 10) == XY_HAL_OK) {
        printf("Found device at 0x%02X\n", addr);
    }
}

/* 6. 释放设备 */
xy_hal_i2c_unbind(i2c);
```

---

## 📊 平台支持矩阵

| API | STM32U5 | STM32F4 | WCH CH32 | HC32 | 备注 |
|-----|---------|---------|----------|------|------|
| **GPIO** | ✅ | ⏳ | ⏳ | ❌ | 统一 API |
| **UART** | ✅ | ⏳ | ⏳ | ❌ | 统一 API |
| **SPI** | ✅ | ⏳ | ⏳ | ❌ | 统一 API |
| **I2C** | ✅ | ⏳ | ⏳ | ❌ | 统一 API |
| **ADC** | ✅ | ⏳ | ❌ | ❌ | - |
| **PWM** | ✅ | ⏳ | ❌ | ❌ | - |

---

## ✅ 验收标准

### 代码质量
- [ ] 所有统一 API 有完整 Doxygen 文档
- [ ] 每个平台实现通过编译
- [ ] 无编译器警告 (-Wall -Wextra)

### 功能测试
- [ ] GPIO 测试：输入/输出/中断模式
- [ ] UART 测试：阻塞/非阻塞收发
- [ ] SPI 测试：主模式收发
- [ ] I2C 测试：主模式读写/设备扫描

### 文档完善
- [ ] API 使用示例
- [ ] 平台移植指南
- [ ] 迁移指南 (旧 API -> 新 API)

---

## 📚 参考文档

- Zephyr HAL: https://docs.zephyrproject.org/latest/hardware/index.html
- STM32 HAL: https://www.st.com/en/embedded-software/stm32cube-mcu-packages.html
- WCH CH32: https://www.wch.cn/downloads/
- HC32: https://www.hcsemi.com/

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
