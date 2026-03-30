# XinYi (芯一) Framework

> **嵌入式系统框架** - 为 STM32/国产 MCU 打造的统一开发平台

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-STM32%20%7C%20CH32%20%7C%20HC32-orange.svg)](docs/hardware/)
[![Build](https://img.shields.io/badge/build-cmake-green.svg)](docs/BUILD_GUIDE.md)

---

## 📋 目录

- [简介](#简介)
- [特性](#特性)
- [快速开始](#快速开始)
- [组件列表](#组件列表)
- [项目结构](#项目结构)
- [硬件支持](#硬件支持)
- [文档](#文档)
- [许可](#许可)

---

## 简介

**XinYi (芯一)** 是 ZeroZap 组织开发的嵌入式系统框架，为 STM32、CH32、HC32 等 MCU 提供统一的开发平台。

### 设计目标

- **统一抽象**: 屏蔽不同 MCU 系列的差异，提供一致的 API
- **模块化设计**: 按需选择组件，灵活配置
- **AI 友好**: 支持 AI Agent 自动化开发和代码生成
- **生产就绪**: 经过实际项目验证的稳定框架

### 核心组件

```
XinYi
├── components/           # 可复用组件
│   ├── sensor/          # 传感器框架 (60+ 驱动)
│   ├── actuator/        # 执行器框架 (舵机/继电器)
│   ├── smbus/           # SMBus/PMBus 协议栈
│   ├── hal/             # 硬件抽象层
│   ├── osal/            # 操作系统抽象层
│   ├── trace/           # 日志和命令框架
│   ├── net/             # 网络协议 (MQTT/Modbus)
│   ├── crypto/          # 密码学
│   ├── dm/              # 数据管理
│   └── ...
└── bsp/                 # 板级支持包
```

---

## 特性

### ✨ 核心特性

- **多 MCU 支持**: STM32F1/F4/U5/G0, CH32V30X/X03X, HC32
- **多 OS 后端**: RT-Thread, FreeRTOS, Zephyr, Bare-metal
- **统一 API**: 一套代码，多平台运行
- **组件化架构**: 按需选择，灵活配置
- **60+ 传感器驱动**: 开箱即用的传感器支持

### 🛠️ 内置组件

| 组件 | 描述 | 状态 |
|------|------|------|
| **Sensor** | 传感器框架 (60+ 驱动) | 🟢 稳定 |
| **Actuator** | 执行器控制 (舵机/继电器/PWM) | 🟢 稳定 |
| **SMBus/PMBus** | 电源管理总线协议 | 🟢 稳定 |
| **HAL** | 硬件抽象层 | 🟢 稳定 |
| **OSAL** | OS 抽象层 | 🟢 稳定 |
| **Trace/Log** | 日志系统 | 🟢 稳定 |
| **Net** | 网络协议 (MQTT/Modbus) | 🟢 稳定 |
| **Crypto** | 密码学 (AES/CRC/SHA) | 🟢 稳定 |
| **GUI** | 图形界面 | 🟡 开发中 |
| **FOTA** | 固件无线升级 | 🟡 开发中 |

---

## 快速开始

### 环境要求

- **工具链**: ARM GCC (arm-none-eabi-gcc 9+)
- **构建工具**: CMake 3.10+
- **操作系统**: Linux, macOS, Windows (WSL)

### 构建 (PC 平台)

```bash
cd XinYi
rm -rf build && mkdir build && cd build
cmake .. -DPLATFORM_PC=ON -DXY_CONFIG_SENSOR_ENABLED=ON \
         -DXY_CONFIG_ACTUATOR_ENABLED=ON -DXY_CONFIG_SMBUS_ENABLED=ON
make -j4

# 完整功能测试
make help  # 查看所有目标
```

详细指南见 [BUILD_GUIDE.md](docs/BUILD_GUIDE.md)。

### 最小示例

```c
#include "xy.h"
#include "xy_log.h"
#include "xy_smbus.h"

int main(void) {
    // 初始化日志
    xy_log_init();

    // 初始化传感器
    gps_device_t gps;
    gps_register(&gps);

    // 初始化执行器
    actuator_device_t relay;
    relay_init(&relay);
    relay_on(&relay);

    LOG_I("XinYi 启动成功!");
    return 0;
}
```

---

## 组件列表

### 传感器框架 (`components/sensor/`)

| 类型 | 驱动数量 | 说明 |
|------|----------|------|
| 温湿度 | 8 | SHT30/40, AHT20, HDC1080, DHT22 等 |
| IMU/加速度 | 6 | MPU6050, BMI088, ICM20948, LSM6DSO 等 |
| 气压/高度 | 4 | BMP280, BMP390, DPS368 等 |
| 光照/颜色 | 6 | BH1750, TCS34725, VCNL4040 等 |
| 气体传感 | 8 | SGP30, SGP40, MQ 系列等 |
| GPS | 1 | AT6558/LC86L/UBLOX (NMEA) |
| 电流/功率 | 5 | INA219, INA226, ACS712 等 |
| 角度编码 | 2 | AS5600, MA730 |
| 其他 | 20+ | 距离、颜色、紫外线等 |

### 执行器框架 (`components/actuator/`)

| 类型 | 说明 |
|------|------|
| 继电器 | 开/关/翻转/脉冲 |
| 舵机 | 角度控制/速度/扫描 |
| PWM | 通用 PWM 输出 |
| 直流电机 | 速度控制 |
| 步进电机 | 位置控制 |

### SMBus/PMBus (`components/smbus/`)

| 功能 | 说明 |
|------|------|
| SMBus 协议 | 字节/字/块读写, PEC 校验 |
| PMBus 协议 | 电源管理设备支持 |
| 格式转换 | Linear/VID/Direct |

---

## 项目结构

```
XinYi/
├── components/           # 可复用组件
│   ├── sensor/          # 传感器框架
│   │   ├── sensor_core.h/c    # 核心框架
│   │   ├── sensors/           # 60+ 驱动
│   │   └── examples/          # 示例代码
│   ├── actuator/        # 执行器框架
│   ├── smbus/           # SMBus/PMBus
│   ├── hal/             # 硬件抽象层
│   ├── osal/            # OS 抽象层
│   ├── trace/           # 日志/命令
│   ├── net/             # 网络协议
│   ├── crypto/          # 密码学
│   ├── dm/              # 数据管理
│   ├── pid/             # PID 控制器
│   ├── ipc/             # 进程间通信
│   ├── pm/              # 电源管理
│   ├── gui/             # 图形界面
│   └── fota/            # 固件升级
├── bsp/                 # 板级支持包
├── docs/                # 文档
│   └── BUILD_GUIDE.md   # 构建指南
├── examples/            # 示例代码
├── tests/               # 单元测试
└── CMakeLists.txt       # 构建系统
```

---

## 硬件支持

### 支持的 MCU 系列

| 系列 | MCU 型号 | 状态 | 驱动 |
|------|---------|------|------|
| **STM32U5** | STM32U585 | 🟢 稳定 | ✅ |
| **STM32F4** | STM32F407/429 | 🟢 稳定 | ✅ |
| **STM32F1** | STM32F103 | 🟢 稳定 | ✅ |
| **STM32G0** | STM32G071 | 🟡 开发中 | 🔄 |
| **CH32V30x** | CH32V307 | 🟢 稳定 | ✅ |
| **CH32X03x** | CH32X035 | 🟢 稳定 | ✅ |
| **HC32L13x** | HC32L136 | 🟡 开发中 | 🔄 |

### 支持的通信接口

| 接口 | 状态 | 说明 |
|------|------|------|
| GPIO | ✅ | 通用输入输出 |
| UART | ✅ | 串口通信 |
| I2C | ✅ | I2C/SMBus/PMBus |
| SPI | ✅ | SPI 通信 |
| ADC | ✅ | 模数转换 |
| PWM | ✅ | PWM 输出 |
| CAN | ✅ | CAN 总线 |
| USB | 🟡 | USB 设备/主机 |

---

## 文档

| 文档 | 说明 |
|------|------|
| [BUILD_GUIDE.md](docs/BUILD_GUIDE.md) | 构建指南 |
| [docs/](docs/) | 完整文档目录 |
| [COMPONENTS_STATUS.md](COMPONENTS_STATUS.md) | 组件状态 |

---

## 许可

XinYi 采用 **Apache License 2.0** 许可。

详见 [LICENSE](LICENSE) 文件。

---

**维护者**: ZeroZap Team  
**GitHub**: https://github.com/ZeroZap/XinYi  
**最后更新**: 2026-03-30