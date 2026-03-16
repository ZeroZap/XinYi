# XinYi 支持的开发板

**版本**: 1.1.0  
**最后更新**: 2026-03-16  
**维护者**: XinYi Team

---

## 📋 概述

XinYi 框架支持多种开发板和 MCU 平台，覆盖 STM32、WCH、HC32 以及 PC 仿真环境。

---

## 🎯 推荐开发板

### STM32 系列 (完整支持)

| 开发板 | MCU | Flash | SRAM | 主频 | 状态 | 购买链接 |
|--------|-----|-------|------|------|------|---------|
| **NUCLEO-U575ZI** | STM32U575ZI | 2MB | 786KB | 160MHz | ✅ 完整支持 | [ST 官网](https://www.st.com/) |
| **NUCLEO-F429ZI** | STM32F429ZI | 2MB | 256KB | 180MHz | ✅ 完整支持 | [ST 官网](https://www.st.com/) |
| **NUCLEO-F446RE** | STM32F446RE | 512KB | 128KB | 180MHz | ✅ 完整支持 | [ST 官网](https://www.st.com/) |
| **NUCLEO-F103RB** | STM32F103RB | 128KB | 20KB | 72MHz | ✅ 支持 | [ST 官网](https://www.st.com/) |
| **NUCLEO-L476RG** | STM32L476RG | 1MB | 96KB | 80MHz | ✅ 支持 | [ST 官网](https://www.st.com/) |
| **NUCLEO-H743ZI** | STM32H743ZI | 2MB | 512KB | 400MHz | ✅ 支持 | [ST 官网](https://www.st.com/) |

### WCH 沁恒系列 (完整支持)

| 开发板 | MCU | Flash | SRAM | 主频 | 状态 | 说明 |
|--------|-----|-------|------|------|------|------|
| **CH32U5-EVT** | CH32U596 | 512KB | 128KB | 144MHz | ✅ 完整支持 | USB 高速 MCU |
| **CH32V307VCT6** | CH32V307 | 256KB | 48KB | 144MHz | ✅ 完整支持 | 经典 RISC-V |
| **CH32F203VCT6** | CH32F203 | 256KB | 48KB | 144MHz | ✅ 支持 | ARM Cortex-M3 |

### HC32 小华系列 (部分支持)

| 开发板 | MCU | Flash | SRAM | 主频 | 状态 | 说明 |
|--------|-----|-------|------|------|------|------|
| **HC32F460** | HC32F460KCTA | 512KB | 64KB | 200MHz | ⏳ 部分支持 | 高性能 MCU |
| **HC32L136** | HC32L136C8PA | 64KB | 8KB | 32MHz | ⏳ 计划中 | 低功耗 MCU |

### PC 仿真 (完整支持)

| 平台 | 编译器 | 状态 | 说明 |
|------|--------|------|------|
| **Linux x64** | GCC 9.0+ | ✅ 完整支持 | 推荐开发环境 |
| **Windows x64** | MinGW | ✅ 支持 | MSYS2 环境 |
| **macOS** | Clang | ✅ 支持 | Homebrew 安装 |

---

## 🔧 开发板详细配置

### NUCLEO-U575ZI (旗舰推荐)

**资源**:
- Flash: 2MB
- SRAM: 786KB
- 主频：160MHz
- 外设：UART×5, SPI×3, I2C×4, ADC×3, DAC×2

**板载资源**:
```c
// LED
#define LED_GREEN    PA5
#define LED_BLUE     PB7
#define LED_RED      PB14

// 按键
#define USER_BUTTON  PC13

// UART (ST-Link 虚拟 COM 口)
#define VCOM_UART    USART1

// I2C (MEMS 传感器)
#define I2C_MEMS     I2C1
```

**引脚定义表**:
| 引脚 | 功能 | 说明 |
|------|------|------|
| PA0 | ADC1_IN0 | 模拟输入 |
| PA1 | ADC1_IN1 | 模拟输入 |
| PA2 | USART2_TX | 串口发送 |
| PA3 | USART2_RX | 串口接收 |
| PA5 | SPI1_SCK | SPI 时钟 |
| PA6 | SPI1_MISO | SPI 主入 |
| PA7 | SPI1_MOSI | SPI 主出 |
| PB6 | I2C1_SCL | I2C 时钟 |
| PB7 | I2C1_SDA | I2C 数据 |
| PC0 | ADC1_IN10 | 模拟输入 |
| PC1 | ADC1_IN11 | 模拟输入 |

**使用示例**:
```bash
# 编译
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
         -DHAL_PLATFORM=STM32 \
         -DBOARD=NUCLEO_U575ZI
make -j$(nproc)

# 烧录
make flash
```

---

### CH32U5-EVT (国产推荐)

**资源**:
- Flash: 512KB
- SRAM: 128KB
- 主频：144MHz
- 外设：UART×6, SPI×3, I2C×3, USB FS/HS

**板载资源**:
```c
// LED
#define LED_GREEN    PC0
#define LED_RED      PC1

// 按键
#define USER_BUTTON  PA0

// UART
#define DEBUG_UART   UART1

// USB
#define USB_FS       USBD
```

**使用示例**:
```bash
# 编译
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/wch-gcc.cmake \
         -DHAL_PLATFORM=WCH \
         -DBOARD=CH32U5_EVT
make -j$(nproc)

# 烧录 (WCH-Link)
make flash
```

---

## 📦 外设支持矩阵

| 外设 | STM32U5 | STM32F4 | WCH CH32 | HC32 | PC |
|------|---------|---------|----------|------|----|
| **GPIO** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **UART** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **SPI** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **I2C** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **ADC** | ✅ | ✅ | ✅ | ⏳ | ✅ |
| **DAC** | ✅ | ✅ | ❌ | ⏳ | ✅ |
| **PWM** | ✅ | ✅ | ✅ | ⏳ | ✅ |
| **USB** | ✅ | ✅ | ✅ | ❌ | ✅ |
| **CAN** | ✅ | ✅ | ❌ | ⏳ | ✅ |
| **RTC** | ✅ | ✅ | ✅ | ⏳ | ✅ |

**图例**:
- ✅ 完整支持
- ⏳ 部分支持/开发中
- ❌ 不支持

---

## 🔌 常用外设模块

### 传感器模块

| 传感器 | 接口 | 支持开发板 | 驱动状态 |
|--------|------|-----------|---------|
| **DHT11** | GPIO | 所有 | ✅ 完成 |
| **SHT40** | I2C | 所有 | ✅ 完成 |
| **BMI088** | SPI | 所有 | ✅ 完成 |
| **VL53L1X** | I2C | 所有 | ✅ 完成 |
| **LPS22HB** | I2C/SPI | 所有 | ✅ 完成 |
| **SGP40** | I2C | 所有 | ✅ 完成 |

### 显示模块

| 模块 | 接口 | 支持开发板 | 驱动状态 |
|------|------|-----------|---------|
| **OLED (SSD1306)** | I2C/SPI | 所有 | ✅ 完成 |
| **LCD (ST7735)** | SPI | 所有 | ✅ 完成 |
| **TFT (ILI9341)** | SPI | 所有 | 📋 计划 |

### 通信模块

| 模块 | 接口 | 支持开发板 | 驱动状态 |
|------|------|-----------|---------|
| **ESP8266** | UART | 所有 | ✅ 完成 |
| **ESP32** | UART/SPI | 所有 | 📋 计划 |
| **RC522 RFID** | SPI | 所有 | ✅ 完成 |
| **SIM800L** | UART | 所有 | 📋 计划 |

---

## 🛠️ 开发工具

### 调试器/烧录器

| 工具 | 支持平台 | 价格 | 说明 |
|------|---------|------|------|
| **ST-Link V2** | STM32 | ¥30 | 官方推荐 |
| **ST-Link V3** | STM32 | ¥150 | 高速调试 |
| **WCH-Link** | WCH | ¥30 | 沁恒官方 |
| **J-Link OB** | 多平台 | ¥50 | 兼容性好 |
| **DAP-Link** | 多平台 | ¥40 | 开源方案 |

### 开发环境

| IDE | 平台 | 价格 | 推荐度 |
|-----|------|------|-------|
| **VSCode + PlatformIO** | 跨平台 | 免费 | ⭐⭐⭐⭐⭐ |
| **CLion** | 跨平台 | 付费 | ⭐⭐⭐⭐ |
| **STM32CubeIDE** | 跨平台 | 免费 | ⭐⭐⭐⭐ |
| **Keil MDK** | Windows | 付费 | ⭐⭐⭐ |
| **IAR EWARM** | Windows | 付费 | ⭐⭐⭐ |

---

## 📚 相关文档

| 文档 | 说明 |
|------|------|
| [工具链配置](toolchain.md) | 编译器/构建系统配置 |
| [快速入门](QUICK_START.md) | 5 分钟上手指南 |
| [开发者指南](DEVELOPER_GUIDE.md) | 完整开发流程 |
| [移植指南](porting.md) | 移植到新平台 |

---

## 🔗 外部资源

- [STM32 官方文档](https://www.st.com/)
- [WCH 沁恒官网](https://www.wch.cn/)
- [HC32 小华官网](https://www.hcsemi.com/)
- [PlatformIO 开发板数据库](https://platformio.org/boards)

---

## 🤝 贡献新开发板支持

如果你有新开发板需要支持，请参考以下步骤：

### 1. 创建板级支持包 (BSP)

```
bsp/<board_name>/
├── board.c          # 板级初始化
├── board.h          # 板级配置
├── pinmap.c         # 引脚映射
├── peripherals.c    # 外设配置
└── README.md        # 使用说明
```

### 2. 添加 CMake 配置

```cmake
# bsp/<board_name>/CMakeLists.txt
set(BOARD_SOURCES
    board.c
    pinmap.c
    peripherals.c
)
```

### 3. 提交 PR

- 提供开发板规格书
- 提供测试报告
- 更新本文档

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0  
**最后更新**: 2026-03-16
