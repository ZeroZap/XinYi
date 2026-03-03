# MUX 通用接口库

**版本**: 1.0.0  
**日期**: 2026-03-02

---

## 📋 概述

MUX (Multiplexer) 通用接口库实现了**软件定义硬件**的核心理念，通过单一物理接口 (USB/UART/SPI) 复用多种外设。

### 核心思想

1. **统一协议封装**: TLV (Type-Length-Value) 格式
2. **插件式管理**: 动态注册/注销外设
3. **跨平台支持**: MCU + PC SDK
4. **软件定义硬件**: 同一硬件通过软件配置实现不同功能

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────┐
│          PC 端 SDK (Python/C++)          │
├─────────────────────────────────────────┤
│          USB/UART 传输层                 │
├─────────────────────────────────────────┤
│          MUX 协议层 (TLV 封装)            │
├─────────────────────────────────────────┤
│    GPIO │ UART │ I2C │ SPI │ PWM │ ...  │
├─────────────────────────────────────────┤
│          HAL (硬件抽象层)                │
└─────────────────────────────────────────┘
```

---

## 📦 TLV 协议格式

```
┌─────────┬───────────┬──────────────┐
│  Type   │  Channel  │    Length    │
│ (1 byte)│ (1 byte)  │  (2 bytes)   │
├─────────┴───────────┴──────────────┤
│            Data (N bytes)           │
└─────────────────────────────────────┘
```

| 字段 | 长度 | 说明 |
|------|------|------|
| Type | 1 字节 | 外设类型 (GPIO/UART/I2C 等) |
| Channel | 1 字节 | 通道号 (0-255) |
| Length | 2 字节 | 数据长度 |
| Data | N 字节 | 实际数据 |

---

## 🔧 使用示例

### 1. 初始化 MUX 管理器

```c
#include "xy_mux.h"

static uint8_t tx_buffer[512];
static uint8_t rx_buffer[512];
static xy_mux_manager_t mux_mgr;

int main(void) {
    /* 初始化 MUX 管理器 */
    xy_mux_init(&mux_mgr, tx_buffer, rx_buffer, sizeof(tx_buffer));
    
    /* 注册外设 */
    xy_mux_gpio_register(&mux_mgr, 0, &gpio_ops, NULL);
    xy_mux_uart_register(&mux_mgr, 0, &uart_ops, NULL);
    
    while (1) {
        /* 处理接收到的数据包 */
        // xy_mux_process_packet(&mux_mgr, rx_data, rx_len);
    }
}
```

### 2. 实现 GPIO 操作接口

```c
#include "xy_mux_gpio.h"

static int32_t gpio_init(uint8_t channel, const void *config) {
    /* 初始化 GPIO 引脚 */
    hal_gpio_init(channel);
    return XY_MUX_OK;
}

static int32_t gpio_read(uint8_t channel, void *data, size_t len) {
    /* 读取 GPIO 电平 */
    uint8_t level = hal_gpio_read(channel);
    *(uint8_t*)data = level;
    return 1;
}

static int32_t gpio_write(uint8_t channel, const void *data, size_t len) {
    /* 写入 GPIO 电平 */
    uint8_t level = *(const uint8_t*)data;
    hal_gpio_write(channel, level);
    return 1;
}

static const xy_mux_ops_t gpio_ops = {
    .init = gpio_init,
    .deinit = NULL,
    .read = gpio_read,
    .write = gpio_write,
    .ioctl = NULL,
};
```

### 3. PC 端 SDK (Python)

```python
import xy_mux

# 连接设备
mux = xy_mux.MuxDevice("/dev/ttyUSB0", baudrate=115200)

# 配置 GPIO
mux.gpio_config(channel=0, direction="output")

# 写入 GPIO
mux.gpio_write(channel=0, level=1)

# 读取 GPIO
level = mux.gpio_read(channel=0)
print(f"GPIO level: {level}")

# 配置 UART
mux.uart_config(channel=0, baudrate=9600)

# UART 读写
mux.uart_write(channel=0, data=b"Hello")
data = mux.uart_read(channel=0, length=10)

# 断开连接
mux.close()
```

---

## 📊 支持的外设类型

| 类型 | ID | 说明 | 状态 |
|------|-----|------|------|
| **GPIO** | 0x01 | 通用输入输出 | ✅ |
| **UART** | 0x02 | 串行通信 | ✅ |
| **I2C** | 0x03 | I2C 总线 | ⏳ |
| **SPI** | 0x04 | SPI 总线 | ⏳ |
| **PWM** | 0x05 | PWM 输出 | ⏳ |
| **ADC** | 0x06 | ADC 采集 | ⏳ |
| **DAC** | 0x07 | DAC 输出 | ⏳ |
| **Sensor** | 0x08 | 传感器数据 | ⏳ |
| **LOG** | 0xFE | 日志输出 | ✅ |
| **CONFIG** | 0xFF | 配置管理 | ✅ |

---

## 🔌 外设操作接口

```c
typedef struct {
    int32_t (*init)(uint8_t channel, const void *config);
    int32_t (*deinit)(uint8_t channel);
    int32_t (*read)(uint8_t channel, void *data, size_t len);
    int32_t (*write)(uint8_t channel, const void *data, size_t len);
    int32_t (*ioctl)(uint8_t channel, int cmd, void *arg);
} xy_mux_ops_t;
```

---

## 📚 API 参考

### 核心 API

| 函数 | 说明 |
|------|------|
| `xy_mux_init` | 初始化 MUX 管理器 |
| `xy_mux_deinit` | 反初始化 |
| `xy_mux_register` | 注册外设 |
| `xy_mux_unregister` | 注销外设 |
| `xy_mux_find` | 查找外设 |
| `xy_mux_process_packet` | 处理接收包 |
| `xy_mux_build_packet` | 构建发送包 |
| `xy_mux_read` | 读取外设 |
| `xy_mux_write` | 写入外设 |
| `xy_mux_ioctl` | 控制外设 |

### GPIO API

| 函数 | 说明 |
|------|------|
| `xy_mux_gpio_register` | 注册 GPIO |
| `xy_mux_gpio_config` | 配置 GPIO |
| `xy_mux_gpio_read` | 读取 GPIO |
| `xy_mux_gpio_write` | 写入 GPIO |
| `xy_mux_gpio_toggle` | 切换 GPIO |

### UART API

| 函数 | 说明 |
|------|------|
| `xy_mux_uart_register` | 注册 UART |
| `xy_mux_uart_config` | 配置 UART |
| `xy_mux_uart_read` | UART 读取 |
| `xy_mux_uart_write` | UART 写入 |

---

## 🔗 相关文档

- [USB2P 原始设计](../../projects/USB2P/)
- [MuxLink 项目](../../projects/MuxLink/)
- [TLV 协议规范](../dm/xy_tlv/README.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
