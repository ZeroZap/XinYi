# XinYi Embedded Framework

[English](#english) | [中文](#chinese)

---

<a name="english"></a>

## English

### Overview

XinYi is a modular embedded C framework designed for resource-constrained microcontroller systems. It provides hardware abstraction layers (HAL), communication protocols, cryptographic libraries, and RTOS abstraction to enable cross-platform portable embedded applications.

### Key Features

- **Hardware Abstraction Layer (HAL)**: Portable interfaces for UART, SPI, I2C, PWM, RTC, timers, DMA, and GPIO
- **Multi-RTOS Support**: Unified OSAL layer supporting FreeRTOS, RT-Thread, and bare-metal
- **Communication Protocols**: MQTT, Modbus, AT commands, ISO7816 (SIM card)
- **Cryptography**: AES, HMAC, RNG, CRC, Base64, MD5
- **Data Management**: EEPROM, NOR Flash, TLV encoding, NVM storage
- **Utilities**: Custom C library (xy_clib), state machines, logging system

### Project Structure

```
XinYi/
├── bsp/xy_hal/          # Hardware Abstraction Layer
│   ├── inc/             # HAL interface headers
│   ├── stm32/           # STM32 platform implementations
│   └── PC/              # PC simulation layer
├── components/          # Core components
│   ├── Bank/            # Battery management system
│   ├── crypto/          # Cryptographic libraries
│   ├── dm/              # Data management (EEPROM, Flash, TLV)
│   ├── net/             # Network protocols (MQTT, Modbus, AT, ISO7816)
│   ├── osal/            # OS Abstraction Layer
│   ├── trace/           # Logging system (xy_log)
│   ├── xy_clib/         # Custom C library
│   └── ...
├── utils/script/        # Build and formatting scripts
├── .clang-format        # Code formatting configuration
├── xy_code_style.md     # Comprehensive coding standards
└── RULEBOOK.md          # Development guidelines
```

### Getting Started

#### Prerequisites

- C99-compatible compiler (GCC, Clang, or ARM toolchain)
- CMake 3.10+ or Make
- (Optional) FreeRTOS or RT-Thread SDK
- clang-format for code formatting

#### Building

**Using Make (recommended):**
```bash
# Build all components
make

# Build specific component
make crypto

# Clean build artifacts
make clean

# Run tests
make test
```

**Using CMake:**
```bash
# Create build directory
mkdir build && cd build

# Configure project
cmake ..

# Build all components
make

# Build specific component
make xy_crypto

# Run tests
make test
```

**Using build scripts (cross-platform):**
```bash
# Build with Make
./build.sh make all

# Build with CMake
./build.sh cmake all

# Run tests
./build.sh make test
```

### Unified Build System

The framework now includes a unified build system with:

- **Component-level builds**: Each component has its own Makefile and CMakeLists.txt
- **Top-level builds**: Build all components from the project root
- **Kconfig support**: Configuration system for enabling/disabling components
- **Cross-platform scripts**: Works on Windows (batch) and Linux/macOS (bash)

Each component now includes:
- `Makefile`: GNU Make build configuration
- `CMakeLists.txt`: CMake build configuration
- `Kconfig`: Component configuration options

### Coding Standards

All code **MUST** follow the guidelines in:
- **[xy_code_style.md](xy_code_style.md)**: Detailed C coding conventions
- **[RULEBOOK.md](RULEBOOK.md)**: AI code generation and development rules
- **[.clang-format](.clang-format)**: Automated formatting configuration

**Quick Reference:**
- Use C99 standard
- 4 spaces indentation (no tabs)
- Lowercase function/variable names with underscores
- Use `xy_log_d()`, `xy_log_e()` for debug/error logging (not `printf()`)
- All public functions must have Doxygen comments

### Logging System

Use the `xy_log` API instead of raw `printf()`:

```c
#include "xy_log.h"

/* Set log level */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* Usage */
xy_log_d("Debug: value = %d\n", value);      /* Debug */
xy_log_i("Info: initialized\n");             /* Info */
xy_log_w("Warning: low memory\n");          /* Warning */
xy_log_e("Error: failed with code %d\n", err); /* Error */
```

Log levels: `NEVER`, `ERROR`, `WARN`, `INFO`, `DEBUG`, `VERBOSE`

### Module Documentation

Each major component includes:
- **README.md**: API reference, examples, configuration
- **Configuration header** (`*_cfg.h`): Compile-time options
- **Examples**: Usage demonstrations

Key modules:
- [bsp/xy_hal](bsp/xy_hal/README.md): Hardware abstraction
- [components/osal](components/osal/README.md): RTOS abstraction
- [components/crypto](components/crypto/ReadMe.md): Cryptography
- [components/net/xy_iso7816](components/net/xy_iso7816/README.md): SIM card protocol

### Contributing

Before submitting code:

1. **Read** [RULEBOOK.md](RULEBOOK.md) and [xy_code_style.md](xy_code_style.md)
2. **Format** code with clang-format:
   ```bash
   clang-format -i your_file.c
   # Or use the script:
   ./utils/script/format_staged.sh
   ```
3. **Test** your changes on target platform
4. **Document** new APIs with Doxygen comments
5. **Use** `xy_log_*()` for logging, not `printf()`

### License

[Specify your license here]

### Contact

[Your contact information]

---

<a name="chinese"></a>

## 中文

### 概述

XinYi 是一个模块化的嵌入式 C 框架，专为资源受限的微控制器系统设计。它提供硬件抽象层（HAL）、通信协议、加密库和 RTOS 抽象，以实现跨平台可移植的嵌入式应用程序。

### 主要特性

- **硬件抽象层（HAL）**：UART、SPI、I2C、PWM、RTC、定时器、DMA 和 GPIO 的可移植接口
- **多 RTOS 支持**：统一的 OSAL 层，支持 FreeRTOS、RT-Thread 和裸机
- **通信协议**：MQTT、Modbus、AT 命令、ISO7816（SIM 卡）
- **加密**：AES、HMAC、RNG、CRC、Base64、MD5
- **数据管理**：EEPROM、NOR Flash、TLV 编码、NVM 存储
- **工具**：自定义 C 库（xy_clib）、状态机、日志系统

### 项目结构

```
XinYi/
├── bsp/xy_hal/          # 硬件抽象层
│   ├── inc/             # HAL 接口头文件
│   ├── stm32/           # STM32 平台实现
│   └── PC/              # PC 仿真层
├── components/          # 核心组件
│   ├── Bank/            # 电池管理系统
│   ├── crypto/          # 加密库
│   ├── dm/              # 数据管理（EEPROM、Flash、TLV）
│   ├── net/             # 网络协议（MQTT、Modbus、AT、ISO7816）
│   ├── osal/            # 操作系统抽象层
│   ├── trace/           # 日志系统（xy_log）
│   ├── xy_clib/         # 自定义 C 库
│   └── ...
├── utils/script/        # 构建和格式化脚本
├── .clang-format        # 代码格式化配置
├── xy_code_style.md     # 综合编码标准
└── RULEBOOK.md          # 开发指南
```

### 快速开始

#### 前置要求

- C99 兼容编译器（GCC、Clang 或 ARM 工具链）
- CMake 3.10+ 或 Make
- （可选）FreeRTOS 或 RT-Thread SDK
- clang-format 用于代码格式化

#### 构建

**使用 Make：**
```bash
cd components/crypto
make
```

**使用 CMake：**
```bash
mkdir build && cd build
cmake ..
make
```

### 编码标准

所有代码**必须**遵循以下指南：
- **[xy_code_style.md](xy_code_style.md)**：详细的 C 编码约定
- **[RULEBOOK.md](RULEBOOK.md)**：AI 代码生成和开发规则
- **[.clang-format](.clang-format)**：自动格式化配置

**快速参考：**
- 使用 C99 标准
- 4 空格缩进（无制表符）
- 函数/变量名小写，带下划线
- 使用 `xy_log_d()`、`xy_log_e()` 进行调试/错误日志记录（而非 `printf()`）
- 所有公共函数必须有 Doxygen 注释

### 日志系统

使用 `xy_log` API 而不是原始的 `printf()`：

```c
#include "xy_log.h"

/* 设置日志级别 */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 使用 */
xy_log_d("调试: 值 = %d\n", value);          /* 调试 */
xy_log_i("信息: 已初始化\n");                /* 信息 */
xy_log_w("警告: 内存不足\n");                /* 警告 */
xy_log_e("错误: 失败，代码 %d\n", err);       /* 错误 */
```

日志级别：`NEVER`、`ERROR`、`WARN`、`INFO`、`DEBUG`、`VERBOSE`

### 模块文档

每个主要组件包括：
- **README.md**：API 参考、示例、配置
- **配置头文件**（`*_cfg.h`）：编译时选项
- **示例**：使用演示

关键模块：
- [bsp/xy_hal](bsp/xy_hal/README.md)：硬件抽象
- [components/osal](components/osal/README.md)：RTOS 抽象
- [components/crypto](components/crypto/ReadMe.md)：加密
- [components/net/xy_iso7816](components/net/xy_iso7816/README.md)：SIM 卡协议

### 贡献

提交代码前：

1. **阅读** [RULEBOOK.md](RULEBOOK.md) 和 [xy_code_style.md](xy_code_style.md)
2. **格式化** 代码使用 clang-format：
   ```bash
   clang-format -i your_file.c
   # 或使用脚本：
   ./utils/script/format_staged.sh
   ```
3. **测试** 您的更改在目标平台上
4. **文档化** 新 API 使用 Doxygen 注释
5. **使用** `xy_log_*()` 进行日志记录，而非 `printf()`

### 许可证

[在此指定您的许可证]

### 联系方式

[您的联系信息]
