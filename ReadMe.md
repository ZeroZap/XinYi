# XinYi Embedded Framework - Comprehensive Project Overview

**Version:** 1.0.0
**Last Updated:** 2026-02-25
**Language:** English | 中文

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Project Vision & Goals](#project-vision--goals)
3. [Architecture Overview](#architecture-overview)
4. [Core Components](#core-components)
5. [Component Ecosystem](#component-ecosystem)
6. [Real-World Applications](#real-world-applications)
7. [Development Standards](#development-standards)
8. [Getting Started](#getting-started)
9. [Build System](#build-system)
10. [Contributing Guidelines](#contributing-guidelines)

---

## Executive Summary

**XinYi** is a modular, production-ready embedded C framework designed for resource-constrained microcontroller systems. It provides a unified abstraction layer for hardware, communication protocols, cryptography, and RTOS management, enabling developers to build portable, maintainable embedded applications across multiple platforms.

### Key Characteristics

- **Modular Architecture**: Independent, reusable components with minimal coupling
- **Cross-Platform**: Supports STM32, RT-Thread, FreeRTOS, and bare-metal environments
- **Production-Grade**: Comprehensive error handling, logging, and documentation
- **Developer-Friendly**: Unified APIs, extensive examples, and clear coding standards
- **Scalable**: From simple sensor drivers to complex IoT applications
- **Hardware Abstraction Layer (HAL)**: Portable interfaces for UART, SPI, I2C, PWM, RTC, timers, DMA, and GPIO
- **Multi-RTOS Support**: Unified OSAL layer supporting FreeRTOS, RT-Thread, and bare-metal
- **Communication Protocols**: MQTT, Modbus, AT commands, ISO7816 (SIM card)
- **Cryptography**: AES, HMAC, RNG, CRC, Base64, MD5
- **Data Management**: EEPROM, NOR Flash, TLV encoding, NVM storage
- **Utilities**: Custom C library (xy_clib), state machines, logging system

### 🚀 最新优化 (版本 2.0)

#### ✅ 已完成优化

- **OSAL 多后端支持**: 支持 4 种后端 (Bare-metal/FreeRTOS/RT-Thread/CMSIS-RTX)
- **HAL STM32U5**: 20+ 外设完整实现
- **统一测试系统**: Unity 框架集成，17+ 测试用例
- **智能代理系统**: 项目经理/架构师/开发/测试 四大智能代理
- **构建系统**: CMake/Kconfig/Makefile 统一配置
- **文档系统**: 完整 API 参考和使用指南
- **目录结构**: third_party 分离，组件结构清晰
- **代码质量**: 统一规范，标准化错误处理

#### 🤖 智能代理系统

```bash
# 使用智能代理系统
./.qwen/smart_agent.sh pm status      # 查看项目状态
./.qwen/smart_agent.sh arch review hal # 审查组件
./.qwen/smart_agent.sh dev create new_component  # 创建组件
./.qwen/smart_agent.sh test gen hal   # 生成测试
```

**代理类型**:
- **pm**: 项目经理 (status, tasks, files, search, stats)
- **arch**: 架构师 (review, deps, check, compat) 
- **dev**: 开发工程师 (create, docs, fix, template)
- **test**: 测试工程师 (run, gen, coverage)

#### 📚 文档系统

- **API 参考**: 完整函数文档
- **使用指南**: 详细组件使用说明
- **架构文档**: 系统设计说明
- **配置选项**: Kconfig 选项详解
- **移植指南**: 新平台适配说明

---

## Project Vision & Goals

### Vision

Create a comprehensive, standardized embedded framework that reduces development time while maintaining code quality, portability, and maintainability across diverse microcontroller platforms.

### Goals

1. **Abstraction**: Provide unified interfaces for hardware and OS-specific operations
2. **Reusability**: Enable code sharing across different projects and platforms
3. **Quality**: Enforce consistent coding standards and comprehensive documentation
4. **Efficiency**: Minimize resource overhead while maximizing functionality
5. **Accessibility**: Lower barriers to entry for embedded systems development

---

## Architecture Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  (Projects: Power Bank, Soldering Iron, USB Bridge, etc.)   │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Component Layer                            │
│  ┌──────────────┬──────────────┬──────────────┐             │
│  │   Crypto     │   Network    │   Sensors    │             │
│  │   Data Mgmt  │   Logging    │   Power Mgmt │             │
│  └──────────────┴──────────────┴──────────────┘             │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│              OS Abstraction Layer (OSAL)                     │
│  ┌──────────────┬──────────────┬──────────────┐             │
│  │  FreeRTOS    │  RT-Thread   │  Bare-Metal  │             │
│  └──────────────┴──────────────┴──────────────┘             │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│         Hardware Abstraction Layer (HAL)                     │
│  ┌──────────────┬──────────────┬──────────────┐             │
│  │   UART/SPI   │   I2C/PWM    │   Timer/DMA  │             │
│  │   GPIO/RTC   │   ADC/DAC    │   Interrupt  │             │
│  └──────────────┴──────────────┴──────────────┘             │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│              SDK HAL Layer                                  │
│  ┌──────────────┬──────────────┬──────────────┐             │
│  │   STM32      │   Other MCUs │   PC Sim     │             │
│  └──────────────┴──────────────┴──────────────┘             │
└─────────────────────────────────────────────────────────────┘
```

### Layered Design Benefits

| Layer | Purpose | Benefit |
|-------|---------|---------|
| **Application** | Project-specific logic | Rapid development, code reuse |
| **Components** | Domain-specific functionality | Modularity, testability |
| **OSAL** | OS abstraction | Multi-RTOS support |
| **HAL** | Hardware abstraction | Platform portability |
| **Platform** | MCU-specific implementations | Hardware optimization |

---

## Core Components

### 1. Hardware Abstraction Layer (HAL)

**Location:** `components/hal/`

**Purpose:** Unified interface for hardware peripherals

**Supported Interfaces:**
- **Serial Communication**: UART, SPI, I2C
- **Analog I/O**: ADC, DAC, PWM
- **Timing**: RTC, Timers, DMA
- **Digital I/O**: GPIO, Interrupts

**Key Features:**
- Platform-agnostic API
- STM32 reference implementations
- PC simulation layer for testing
- Configurable via `*_cfg.h` headers

**Example Usage:**
```c
#include "xy_hal_uart.h"

void *uart = xy_hal_uart_open(UART_PORT_1, 115200);
xy_hal_uart_send(uart, data, len, 1000);
xy_hal_uart_close(uart);
```

---

### 2. OS Abstraction Layer (OSAL)

**Location:** `components/osal/`

**Purpose:** Unified interface for RTOS operations

**Supported RTOS:**
- FreeRTOS
- RT-Thread
- Bare-metal (no RTOS)

**Abstractions:**
- Task/Thread management
- Semaphores, Mutexes, Message queues
- Timers and delays
- Memory management

**Key Features:**
- Single API for multiple RTOS
- Compile-time configuration
- Minimal overhead
- Fallback implementations for bare-metal

---

### 3. Cryptography Component

**Location:** `components/crypto/`

**Algorithms:**
- **Symmetric**: AES (ECB, CBC, CTR modes)
- **Hashing**: MD5, SHA-1, SHA-256
- **Authentication**: HMAC
- **Encoding**: Base64, Hex
- **Utilities**: CRC, Random number generation

**Features:**
- Optimized for embedded systems
- Configurable algorithm selection
- Hardware acceleration support (where available)
- Comprehensive test suite

---

### 4. Data Management (DM)

**Location:** `components/dm/`

**Capabilities:**
- **Storage**: EEPROM, NOR Flash, NAND Flash
- **Encoding**: TLV (Tag-Length-Value) protocol
- **NVM**: Non-volatile memory management
- **Wear Leveling**: Flash lifetime optimization

**Use Cases:**
- Configuration storage
- Calibration data persistence
- Firmware update staging
- User data backup

---

### 5. Network & Communication

**Location:** `components/net/`

**Protocols:**
- **MQTT**: IoT messaging (with TLS support)
- **Modbus**: Industrial protocol (RTU/TCP)
- **AT Commands**: Cellular modem interface
- **ISO7816**: SIM card communication

**Features:**
- Protocol stacks with minimal dependencies
- Configurable buffer sizes
- Error recovery mechanisms
- Example implementations

---

### 6. Sensor Framework

**Location:** `components/sensor/`

**Supported Sensors:**
- Temperature: ADT7420, TMP36
- Motion: Accelerometer, Gyroscope
- Light: APDS9960 (ambient light, proximity)
- Pressure, Humidity, Gas sensors

**Features:**
- Unified sensor interface
- Calibration support
- Data fusion algorithms
- Low-power operation modes
- DMA-based data collection

---

### 7. Logging System (xy_log)

**Location:** `components/trace/xy_log/`

**Purpose:** Unified logging across all components

**Log Levels:**
- `VERBOSE` - Detailed trace information
- `DEBUG` - Debug information
- `INFO` - Informational messages
- `WARN` - Warning messages
- `ERROR` - Error messages
- `NEVER` - Disable logging

**Features:**
- Per-module log level configuration
- Minimal runtime overhead
- Support for multiple output backends
- Timestamp and context information

**Usage:**
```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

xy_log_d("Debug message: %d\n", value);
xy_log_e("Error: operation failed\n");
```

---

### 8. Custom C Library (xy_clib)

**Location:** `components/clib/xy_clib/`

**Modules:**
- **String Operations**: Safe string handling
- **Math Utilities**: Fixed-point math, bit operations
- **Data Structures**: Lists, queues, trees
- **Encoding**: BCD, Hex, Base64 conversions
- **Filtering**: Digital filters (IIR, FIR)
- **Memory**: Pool allocators, safe malloc wrappers

**Benefits:**
- Embedded-optimized implementations
- Reduced dependency on libc
- Predictable memory usage
- Comprehensive test coverage

---

### 9. Power Management (PM)

**Location:** `components/pm/`

**Features:**
- Sleep mode management
- Power state transitions
- Wake-up source configuration
- Energy consumption monitoring

---

### 10. Additional Components

| Component | Location | Purpose |
|-----------|----------|---------|
| **Battery Management** | `components/Bank/` | Power bank, battery charging, gauge |
| **State Machine** | `components/sys/` | Hierarchical state machine framework |
| **Timer System** | `components/sys/xy_timer/` | Software timers, scheduling |
| **IPC** | `components/ipc/` | Inter-process communication |
| **FOTA** | `components/fota/` | Firmware over-the-air updates |
| **GUI** | `components/gui/` | Display and UI framework |
| **Kernel** | `components/kernel/` | Core kernel utilities |

---

## Component Ecosystem

### Component Dependency Graph

```
Application Projects
    ├── Power Bank (Battery Management)
    ├── Soldering Iron (Temperature Control)
    ├── USB Bridge (Multi-protocol)
    ├── Smart Card Bridge (ISO7816)
    ├── LCR Meter (Measurement)
    └── Multi-Stepper (Motor Control)
         │
         ├─→ Sensor Framework
         ├─→ Network (MQTT, Modbus, AT)
         ├─→ Cryptography
         ├─→ Data Management
         ├─→ Logging (xy_log)
         │
         ├─→ OSAL (FreeRTOS/RT-Thread/Bare-metal)
         │    └─→ HAL (UART, SPI, I2C, GPIO, etc.)
         │         └─→ Platform (STM32, PC Sim)
         │
         └─→ xy_clib (String, Math, Filters, etc.)
```

### Component Maturity Levels

- **Stable**: HAL, OSAL, Crypto, xy_clib, Logging
- **Production**: Network (MQTT, Modbus), Sensor, Data Management
- **Active Development**: FOTA, GUI, Power Management
- **Experimental**: Advanced filtering, AI/ML utilities

---

## Real-World Applications

### 1. Power Bank (Battery Management System)

**Features:**
- Multi-cell battery monitoring
- Charging/discharging control
- Fuel gauge algorithm
- Thermal management
- Over-current/over-voltage protection

**Components Used:**
- Sensor Framework (voltage, current, temperature)
- Power Management
- Data Management (calibration storage)
- Logging

**Status:** Production-ready

---

### 2. Soldering Iron (Temperature Control)

**Features:**
- PID temperature control
- Tip temperature monitoring
- Sleep mode management
- Power optimization

**Components Used:**
- HAL (ADC, PWM, Timer)
- Sensor Framework
- State Machine
- Logging

---

### 3. USB Bridge (Multi-Protocol)

**Features:**
- USB to SPI/I2C/UART conversion
- Real-time protocol translation
- PC library interface
- Multi-device support

**Components Used:**
- HAL (USB, SPI, I2C, UART)
- Data Management (configuration)
- Logging

---

### 4. Smart Card USB Bridge

**Features:**
- ISO7816 SIM card interface
- TLV protocol encoding
- USB communication
- Python test client

**Components Used:**
- Network (ISO7816)
- Data Management (TLV)
- HAL (UART, GPIO)
- Logging

---

### 5. LCR Meter (Measurement Device)

**Features:**
- Inductance, Capacitance, Resistance measurement
- Frequency analysis
- Data logging
- Calibration support

**Components Used:**
- Sensor Framework
- Data Management
- Logging

---

### 6. Multi-Stepper Motor Controller

**Features:**
- Multi-axis stepper control
- Acceleration profiling
- Position tracking
- Limit switch handling

**Components Used:**
- HAL (GPIO, Timer, PWM)
- State Machine
- Logging

---

## Development Standards

### Mandatory Coding Standards

All code **MUST** follow:

1. **[xy_code_style.md](xy_code_style.md)** - Detailed C coding conventions
2. **[RULEBOOK.md](RULEBOOK.md)** - Development rules and guidelines
3. **[.clang-format](.clang-format)** - Automated code formatting

### Quick Reference

| Standard | Requirement |
|----------|-------------|
| **Language** | C99 |
| **Indentation** | 4 spaces (no tabs) |
| **Naming** | Lowercase with underscores (`my_function`, `my_var`) |
| **Braces** | Same line as keywords (`if {`, `for {`) |
| **Logging** | Use `xy_log_*()`, never `printf()` |
| **Documentation** | Doxygen comments for all public functions |
| **Formatting** | Apply clang-format before commit |

### File Structure

```c
/**
 * @file module_name.c
 * @brief Brief description
 * @version X.Y.Z
 * @date YYYY-MM-DD
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
#include "module_name.h"

/**
 * @brief Function description
 * @param param1 Description
 * @return Description of return value
 */
int my_function(int param1) {
    xy_log_d("Entering function\n");
    return 0;
}
```

### Error Handling Convention

- **Return 0 or positive** for success
- **Return negative values** for errors (standardized error codes)
- **Use `xy_log_e()`** for error logging
- **Document error codes** in function comments

---

## Getting Started

### Prerequisites

- **Compiler**: GCC, Clang, or ARM toolchain (C99 compatible)
- **Build Tools**: CMake 3.10+ or GNU Make
- **Optional**: FreeRTOS SDK, RT-Thread SDK
- **Code Formatting**: clang-format

### Installation

```bash
# Clone the repository
git clone <repository-url>
cd XinYi

# Install dependencies (example for Ubuntu)
sudo apt-get install build-essential cmake clang-format

# Verify setup
make --version
cmake --version
clang-format --version
```

### Quick Build

```bash
# Build all components
make

# Build specific component
make crypto

# Run tests
make test

# Clean build artifacts
make clean
```

### First Project

```bash
# Create a new project directory
mkdir my_project
cd my_project

# Create main.c
cat > main.c << 'EOF'
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

int main(void) {
    xy_log_i("XinYi Framework initialized\n");
    return 0;
}
EOF

# Build with CMake
mkdir build && cd build
cmake ..
make
```

---

## Build System

### Build Options

#### Using Make

```bash
# Build all
make

# Build specific component
make crypto
make net
make sensor

# Build with specific target
make TARGET=stm32

# Run tests
make test

# Clean
make clean
make distclean
```

#### Using CMake

```bash
# Configure
mkdir build && cd build
cmake .. -DRTOS_BACKEND=freertos  # or baremetal/rtthread/cmsis_rtx

# Build all
make

# Build specific target
make xy_crypto
make xy_net

# Run tests
make test

# Install
make install
```

#### Using Smart Agent

```bash
# 使用智能代理系统进行构建和管理
./.qwen/smart_agent.sh dev create my_component  # 创建新组件
./.qwen/smart_agent.sh pm build my_component    # 构建组件
./.qwen/smart_agent.sh arch check               # 代码质量检查
./.qwen/smart_agent.sh test run all             # 运行所有测试
```

#### Using Build Scripts

```bash
# Cross-platform build script
./build.sh make all
./build.sh cmake all
./build.sh make test
```

### Configuration System (Kconfig)

Each component includes `Kconfig` for compile-time configuration:

```bash
# Interactive configuration
make menuconfig

# Set specific option
make CONFIG_CRYPTO_AES=y
```

### Component Structure

Each component includes:
- `CMakeLists.txt` - CMake configuration
- `Makefile` - GNU Make configuration
- `Kconfig` - Configuration options
- `README.md` - API documentation
- `*_cfg.h` - Compile-time settings
- `test/` - Unit tests

---

## Contributing Guidelines

### Before Submitting Code

1. **Read Documentation**
   - [RULEBOOK.md](RULEBOOK.md) - Development rules
   - [xy_code_style.md](xy_code_style.md) - Coding standards

2. **Format Code**
   ```bash
   clang-format -i your_file.c
   # Or use the script
   ./utils/script/format_staged.sh
   ```

3. **Add Documentation**
   - Doxygen comments for all public functions
   - File header with brief description
   - README.md for new components

4. **Test Changes**
   - Run unit tests: `make test`
   - Test on target platform
   - Verify no regressions

5. **Use Proper Logging**
   - Replace `printf()` with `xy_log_*()`
   - Set appropriate log levels
   - Include context in messages

### Code Review Checklist

- [ ] Follows coding standards (xy_code_style.md)
- [ ] Formatted with clang-format
- [ ] All functions documented with Doxygen
- [ ] Uses `xy_log_*()` for logging
- [ ] Error handling implemented
- [ ] Unit tests included
- [ ] No compiler warnings
- [ ] Tested on target platform

### Commit Message Format

```
[COMPONENT] Brief description

Detailed explanation of changes:
- What was changed
- Why it was changed
- How it was tested

Fixes: #issue_number
```

---

## Project Statistics

### Component Count

- **Core Components**: 15+
- **Supported Protocols**: 4 (MQTT, Modbus, AT, ISO7816)
- **Sensor Drivers**: 10+
- **Cryptographic Algorithms**: 8+
- **Real-World Projects**: 6+

### Code Metrics

- **Total Lines of Code**: ~50,000+
- **Test Coverage**: Comprehensive unit tests
- **Documentation**: Doxygen + Markdown
- **Supported Platforms**: STM32, RT-Thread, FreeRTOS, Bare-metal

### Development Activity

- **Active Components**: 15+
- **Stable Components**: 10+
- **Under Development**: 5+
- **Experimental**: 3+

---

## Roadmap & Future Enhancements

### Short Term (Next Release)

- [ ] Enhanced FOTA (Firmware Over-The-Air) updates
- [ ] GUI framework improvements
- [ ] Additional sensor drivers
- [ ] Performance optimizations

### Medium Term

- [ ] Machine learning utilities
- [ ] Advanced power management
- [ ] Enhanced security features
- [ ] Cloud connectivity improvements

### Long Term

- [ ] Multi-core support
- [ ] Real-time performance guarantees
- [ ] Advanced debugging tools
- [ ] Commercial support options

---

## Support & Resources

### Documentation

- **Main README**: [ReadMe.md](ReadMe.md)
- **Chinese README**: [ReadMe-cn.md](ReadMe-cn.md)
- **Development Rules**: [RULEBOOK.md](RULEBOOK.md)
- **Coding Standards**: [xy_code_style.md](xy_code_style.md)
- **Component Docs**: Individual README.md in each component

### Getting Help

1. Check component-specific README.md
2. Review example code in component directories
3. Check unit tests for usage patterns
4. Review RULEBOOK.md for common issues

### Community

- Issue tracking: [GitHub Issues]
- Discussions: [GitHub Discussions]
- Contributing: See Contributing Guidelines above

---

## License

[Specify your license here]

---

## Contact & Attribution

**Project Lead**: [Your Name/Organization]
**Contributors**: [List of contributors]
**Contact**: [Your contact information]

---

## Appendix: Quick Reference

### Common Tasks

#### Add a New Component

1. Create directory: `components/my_component/`
2. Add files: `CMakeLists.txt`, `Makefile`, `Kconfig`
3. Create headers in `inc/` directory
4. Create implementation in `src/` directory
5. Add README.md with API documentation
6. Add unit tests in `test/` directory

#### Add a New Sensor Driver

1. Create file: `components/sensor/sensor_<name>.c/h`
2. Implement sensor interface
3. Add calibration support
4. Create example usage
5. Add unit tests

#### Port to New Platform

1. Create platform directory: `bsp/xy_hal/<platform>/`
2. Implement HAL interfaces for your platform
3. Create platform-specific Makefile/CMakeLists.txt
4. Test all components on new platform
5. Document platform-specific notes

### Useful Commands

```bash
# Format all C files
find . -name "*.c" -o -name "*.h" | xargs clang-format -i

# Run all tests
make test

# Generate documentation
doxygen docs/doxygen.config

# Check code style
./utils/script/check_style.sh

# Build for specific platform
make TARGET=stm32f4

# Clean everything
make distclean
```

---

**End of Document**
