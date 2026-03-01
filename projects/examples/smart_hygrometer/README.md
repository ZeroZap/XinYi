# 智能温湿度计示例

**版本**: 1.0  
**最后更新**: 2026-03-01

---

## 📋 概述

本示例展示如何使用 XinYi 框架构建一个智能温湿度计，集成以下功能：

- ✅ SHT30 温湿度传感器
- ✅ BMP280 气压传感器
- ✅ OLED 显示屏
- ✅ 多任务实时运行
- ✅ 低功耗设计

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────┐
│         应用层 (Application)         │
│     传感器任务 | 显示任务 | 按键任务   │
├─────────────────────────────────────┤
│         设备驱动层 (Device)          │
│   SHT30  |  BMP280  |  OLED 驱动     │
├─────────────────────────────────────┤
│         HAL 层 (Hardware)            │
│      I2C  |  GPIO  |  SPI           │
├─────────────────────────────────────┤
│         OSAL 层 (OS Abstract)        │
│    FreeRTOS | RT-Thread | 裸机      │
└─────────────────────────────────────┘
```

---

## 📁 项目结构

```
smart_hygrometer/
├── main.c              # 主程序
├── CMakeLists.txt      # CMake 配置
├── README.md           # 本文件
└── config.h            # 配置文件
```

---

## 🔧 硬件要求

| 组件 | 型号 | 接口 | 数量 |
|------|------|------|------|
| MCU | STM32U5/Nucleo | - | 1 |
| 温湿度传感器 | SHT30 | I2C | 1 |
| 气压传感器 | BMP280 | I2C | 1 |
| OLED 显示屏 | 0.96" SSD1306 | I2C | 1 |

### 引脚连接

| 设备 | SCL | SDA | VCC | GND |
|------|-----|-----|-----|-----|
| SHT30 | PB6 | PB7 | 3.3V | GND |
| BMP280 | PB6 | PB7 | 3.3V | GND |
| OLED | PB6 | PB7 | 3.3V | GND |

---

## 🚀 快速开始

### 1. 克隆项目

```bash
git clone https://github.com/ZeroZap/XinYi.git
cd XinYi/projects/examples/smart_hygrometer
```

### 2. 配置硬件

根据实际硬件修改 `main.c` 中的 I2C 配置：

```c
/* 初始化 I2C */
xy_hal_i2c_init(&hi2c1, &i2c_config);
```

### 3. 构建项目

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 4. 烧录程序

```bash
# 使用 OpenOCD
openocd -f interface/stlink.cfg -f target/stm32u5.cfg \
        -c "program smart_hygrometer.elf verify reset exit"
```

---

## 📊 运行效果

### 串口输出

```
[INFO] === XinYi Smart Hygrometer ===
[INFO] System initializing...
[INFO] SHT30 initialized
[INFO] BMP280 initialized
[INFO] System initialized successfully
[INFO] Sensor task started
[INFO] Display task started
[DEBUG] SHT30: T=25.30°C, H=60.50%RH
[DEBUG] BMP280: P=101325Pa
```

### OLED 显示

```
┌────────────────┐
│ T: 25.30 C     │
│ H: 60.50 %     │
│ P: 101325 Pa   │
└────────────────┘
```

---

## 🎯 功能扩展

### 添加 WiFi 上传

```c
#include "xy_mqtt.h"

static void wifi_task(void *arg)
{
    /* 连接 WiFi */
    /* 发布数据到 MQTT 服务器 */
}
```

### 添加低功耗模式

```c
/* 进入停止模式 */
xy_hal_pwr_enter_stop();

/* 唤醒后继续运行 */
xy_hal_pwr_exit_stop();
```

---

## 📝 注意事项

1. **I2C 地址冲突**: SHT30(0x44) 和 BMP280(0x76) 地址不同，可以共用 I2C 总线
2. **上拉电阻**: I2C 总线需要 4.7kΩ 上拉电阻
3. **电源去耦**: 每个传感器需要 100nF 去耦电容

---

## 🔗 相关文档

| 文档 | 说明 |
|------|------|
| [SHT30 驱动](../../../components/sensor/xy_sht30.c) | SHT30 驱动实现 |
| [BMP280 驱动](../../../components/sensor/xy_bmp280.c) | BMP280 驱动实现 |
| [OLED 驱动](../../../components/sensor/xy_oled.c) | OLED 驱动实现 |
| [设备驱动模板](../../../docs/components/device/DRIVER_TEMPLATE.md) | 驱动开发指南 |

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
