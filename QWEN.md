# XinYi Embedded Framework - QWEN.md

## 项目概述

**XinYi** 是一个模块化、生产级的嵌入式 C 框架，专为资源受限的微控制器系统设计。它提供硬件、通信协议、密码学和 RTOS 管理的统一抽象层，使开发人员能够在多个平台上构建可移植、可维护的嵌入式应用。

### 核心特性

- **模块化架构**: 独立、可复用的组件，最小化耦合
- **跨平台支持**: STM32、RT-Thread、FreeRTOS、裸机环境
- **生产级代码**: 完善的错误处理、日志系统和文档
- **硬件抽象层 (HAL)**: UART、SPI、I2C、PWM、RTC、定时器、DMA、GPIO
- **多 RTOS 支持**: 统一 OSAL 层支持 FreeRTOS、RT-Thread 和裸机
- **通信协议**: MQTT、Modbus、AT 命令、ISO7816 (SIM 卡)
- **密码学**: AES、HMAC、RNG、CRC、Base64、MD5
- **数据管理**: EEPROM、NOR Flash、TLV 编码、NVM 存储
- **工具库**: 自定义 C 库 (xy_clib)、状态机、日志系统

---

## 项目结构

```
XinYi/
├── components/          # 核心组件目录
│   ├── crypto/         # 密码学 (AES, HMAC, CRC, Base64, MD5)
│   ├── xy_clib/        # 自定义 C 库 (字符串、数学、数据结构)
│   ├── dm/             # 数据管理 (EEPROM, Flash, TLV, NVM)
│   ├── net/            # 网络协议 (MQTT, Modbus, AT, ISO7816)
│   ├── device/         # 设备驱动
│   ├── trace/          # 跟踪和日志 (xy_log)
│   ├── osal/           # OS 抽象层 (FreeRTOS, RT-Thread, 裸机)
│   ├── hal/            # 硬件抽象层 (UART, SPI, I2C, GPIO, etc.)
│   ├── Bank/           # 电池管理
│   ├── sensor/         # 传感器框架
│   ├── ipc/            # 进程间通信
│   ├── time_tick/      # 时间刻度
│   ├── xy_key/         # 按键处理
│   ├── xy_state_machine/ # 状态机
│   ├── fota/           # 固件空中升级
│   ├── kernel/         # 内核工具
│   ├── misc/           # 杂项工具
│   ├── pm/             # 电源管理
│   ├── xfer/           # 数据传输
│   └── xy_code_style/  # 代码风格规范
├── MCU/                # MCU SDK 子仓库
│   ├── HC/             # HC32 系列
│   ├── ST/             # STM32 系列
│   └── WCH/            # 沁恒系列
├── projects/           # 应用项目
│   ├── Bank/           # 电源银行
│   ├── Soldering Iron/ # 电烙铁
│   ├── USBBridge/      # USB 桥接器
│   ├── SmartCard_USB_Bridge/ # 智能卡 USB 桥接器
│   ├── LCRMeter/       # LCR 表
│   ├── MultiStepper/   # 多步进电机控制器
│   └── ...
├── docs/               # 文档
│   └── rules/
│       └── 100-code_style/
│           └── xy_code_style.md  # 代码风格规范
├── utils/              # 工具脚本
│   └── script/
├── CMakeLists.txt      # CMake 构建配置
├── Makefile            # GNU Make 构建配置
├── Kconfig             # Kconfig 配置系统
├── build.sh            # Linux/Mac 构建脚本
├── build.bat           # Windows 构建脚本
└── ReadMe.md           # 主文档
```

---

## 构建和运行

### 前置条件

- **编译器**: GCC、Clang 或 ARM 工具链 (C99 兼容)
- **构建工具**: CMake 3.10+ 或 GNU Make
- **可选**: FreeRTOS SDK、RT-Thread SDK
- **代码格式化**: clang-format

### 构建命令

#### 使用 Make

```bash
# 构建所有组件
make

# 构建特定组件
make crypto
make net
make sensor

# 运行测试
make test

# 清理构建产物
make clean
make distclean
```

#### 使用 CMake

```bash
# 配置
mkdir build && cd build
cmake ..

# 构建所有
make

# 构建特定目标
make xy_crypto
make xy_net

# 运行测试
make test

# 安装
make install
```

#### 使用构建脚本

```bash
# Windows
build.bat make all
build.bat cmake all

# Linux/Mac
./build.sh make all
./build.sh cmake test
```

### 配置系统 (Kconfig)

```bash
# 交互式配置
make menuconfig

# 设置特定选项
make CONFIG_CRYPTO_AES=y
```

---

## 开发规范

### 编码标准

所有代码 **必须** 遵循:

1. **[xy_code_style.md](docs/rules/100-code_style/xy_code_style.md)** - 详细 C 编码规范
2. **[.clang-format](.clang-format)** - 自动化代码格式化
3. **[.editorconfig](.editorconfig)** - 编辑器配置

### 快速参考

| 规范 | 要求 |
|------|------|
| **语言** | C99 |
| **缩进** | 4 空格 (禁用制表符) |
| **命名** | 小写 + 下划线 (`my_function`, `my_var`) |
| **括号** | 与关键字同行 (`if {`, `for {`) |
| **日志** | 使用 `xy_log_*()`，禁止 `printf()` |
| **文档** | 所有公共函数使用 Doxygen 注释 |
| **格式化** | 提交前应用 clang-format |

### 代码风格示例

```c
/**
 * @file module_name.c
 * @brief 模块简要描述
 * @version X.Y.Z
 * @date YYYY-MM-DD
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
#include "module_name.h"

/**
 * @brief 函数描述
 * @param param1 参数描述
 * @return 返回值描述
 */
int my_function(int param1) {
    xy_log_d("进入函数\n");
    
    if (condition) {
        // 处理逻辑
    }
    
    return 0;
}
```

### 错误处理规范

- **返回 0 或正值** 表示成功
- **返回负值** 表示错误 (标准化错误码)
- **使用 `xy_log_e()`** 记录错误日志
- **在注释中记录错误码**

### 提交前检查清单

- [ ] 遵循编码规范 (xy_code_style.md)
- [ ] 使用 clang-format 格式化
- [ ] 所有函数有 Doxygen 文档
- [ ] 使用 `xy_log_*()` 进行日志记录
- [ ] 实现错误处理
- [ ] 包含单元测试
- [ ] 无编译器警告
- [ ] 在目标平台测试

---

## 组件详情

### 1. 硬件抽象层 (HAL)

**位置:** `components/hal/`

**支持的接口:**
- **串行通信**: UART、SPI、I2C
- **模拟 I/O**: ADC、DAC、PWM
- **定时**: RTC、定时器、DMA
- **数字 I/O**: GPIO、中断

**支持平台:**
- STM32 (完整实现)
- HC32、WCH (占位符)
- PC 仿真层 (开发测试)

### 2. OS 抽象层 (OSAL)

**位置:** `components/osal/`

**支持的 RTOS:**
- FreeRTOS
- RT-Thread
- 裸机 (无 RTOS)

**抽象内容:**
- 任务/线程管理
- 信号量、互斥量、消息队列
- 定时器和延时
- 内存管理

### 3. 密码学组件

**位置:** `components/crypto/`

**算法:**
- **对称加密**: AES (ECB, CBC, CTR 模式)
- **哈希**: MD5、SHA-1、SHA-256
- **认证**: HMAC
- **编码**: Base64、Hex
- **工具**: CRC、随机数生成

### 4. 数据管理 (DM)

**位置:** `components/dm/`

**功能:**
- **存储**: EEPROM、NOR Flash、NAND Flash
- **编码**: TLV (标签 - 长度 - 值) 协议
- **NVM**: 非易失性内存管理
- **磨损均衡**: Flash 寿命优化

### 5. 网络与通信

**位置:** `components/net/`

**协议:**
- **MQTT**: IoT 消息 (支持 TLS)
- **Modbus**: 工业协议 (RTU/TCP)
- **AT 命令**: 蜂窝调制解调器接口
- **ISO7816**: SIM 卡通信

### 6. 传感器框架

**位置:** `components/sensor/`

**支持的传感器:**
- 温度：ADT7420、TMP36
- 运动：加速度计、陀螺仪
- 光线：APDS9960 (环境光、接近)
- 压力、湿度、气体传感器

### 7. 日志系统 (xy_log)

**位置:** `components/trace/xy_log/`

**日志级别:**
- `VERBOSE` - 详细跟踪信息
- `DEBUG` - 调试信息
- `INFO` - 信息性消息
- `WARN` - 警告消息
- `ERROR` - 错误消息
- `NEVER` - 禁用日志

**使用示例:**
```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

xy_log_d("调试消息：%d\n", value);
xy_log_e("错误：操作失败\n");
```

### 8. 自定义 C 库 (xy_clib)

**位置:** `components/clib/xy_clib/`

**模块:**
- **字符串操作**: 安全字符串处理
- **数学工具**: 定点数学、位操作
- **数据结构**: 列表、队列、树
- **编码**: BCD、Hex、Base64 转换
- **滤波**: 数字滤波器 (IIR, FIR)
- **内存**: 池分配器、安全 malloc 包装

---

## 应用项目

| 项目 | 描述 | 状态 |
|------|------|------|
| **Power Bank** | 电池管理系统 | 生产就绪 |
| **Soldering Iron** | 温度控制烙铁 | 生产就绪 |
| **USB Bridge** | USB 转 SPI/I2C/UART | 生产就绪 |
| **Smart Card Bridge** | ISO7816 SIM 卡接口 | 生产就绪 |
| **LCR Meter** | 电感/电容/电阻测量 | 生产中 |
| **Multi-Stepper** | 多轴步进电机控制 | 生产中 |

---

## 许可证

本项目采用 **Apache License 2.0** 许可证。

---

## 相关文档

- **主文档**: [ReadMe.md](ReadMe.md)
- **代码风格**: [docs/rules/100-code_style/xy_code_style.md](docs/rules/100-code_style/xy_code_style.md)
- **HAL 文档**: [components/hal/README.md](components/hal/README.md)
- **项目列表**: [projects/Projects.md](projects/Projects.md)
- **MCU 支持**: [MCU/MCU.md](MCU/MCU.md)

---

## 常用命令

```bash
# 格式化所有 C 文件
find . -name "*.c" -o -name "*.h" | xargs clang-format -i

# 运行所有测试
make test

# 生成文档
doxygen docs/doxygen.config

# 检查代码风格
./utils/script/check_style.sh

# 为特定平台构建
make TARGET=stm32

# 完全清理
make distclean
```

---

## 架构层次

```
┌─────────────────────────────────────────┐
│           应用层 (Projects)              │
│  电源银行 | 电烙铁 | USB 桥接器 | ...      │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│           组件层 (Components)            │
│  Crypto | Network | Sensor | Data Mgmt  │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│        OS 抽象层 (OSAL)                  │
│  FreeRTOS | RT-Thread | Bare-Metal      │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│       硬件抽象层 (HAL)                   │
│  UART | SPI | I2C | GPIO | Timer | ...  │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│          平台层 (MCU SDK)                │
│  STM32 | HC32 | WCH | PC Sim            │
└─────────────────────────────────────────┘
```
