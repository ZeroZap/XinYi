# XinYi 项目概述

**版本**: 1.0.0  
**最后更新**: 2026-02-28  
**语言**: 简体中文

---

## 执行摘要

**XinYi** 是一个模块化、生产级的嵌入式 C 框架，专为资源受限的微控制器系统设计。它提供硬件、通信协议、密码学和 RTOS 管理的统一抽象层，使开发人员能够在多个平台上构建可移植、可维护的嵌入式应用。

### 核心特性

- **模块化架构**: 独立、可复用的组件，最小化耦合
- **跨平台支持**: STM32、RT-Thread、FreeRTOS、裸机环境
- **生产级代码**: 完善的错误处理、日志系统和文档
- **开发者友好**: 统一 API、丰富示例、清晰的编码规范
- **可扩展性**: 从简单传感器驱动到复杂 IoT 应用
- **硬件抽象层 (HAL)**: UART、SPI、I2C、PWM、RTC、定时器、DMA、GPIO 的可移植接口
- **多 RTOS 支持**: 统一 OSAL 层支持 FreeRTOS、RT-Thread 和裸机
- **通信协议**: MQTT、Modbus、AT 命令、ISO7816 (SIM 卡)
- **密码学**: AES、HMAC、RNG、CRC、Base64、MD5
- **数据管理**: EEPROM、NOR Flash、TLV 编码、NVM 存储
- **工具库**: 自定义 C 库 (xy_clib)、状态机、日志系统

---

## 项目愿景与目标

### 愿景

创建一个全面、标准化的嵌入式框架，在保持代码质量、可移植性和可维护性的同时，减少开发时间。

### 目标

1. **抽象**: 为硬件和 OS 特定操作提供统一接口
2. **可复用**: 实现代码在不同项目和平台间共享
3. **质量**: 执行一致的编码标准和全面的文档
4. **效率**: 最小化资源开销，最大化功能
5. **可访问性**: 降低嵌入式系统开发的入门门槛

---

## 架构概览

### 高层次架构

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层                                   │
│  (项目：电源银行、电烙铁、USB 桥接器等)                      │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│                   组件层                                    │
│  ┌──────────────┬──────────────┬──────────────┐            │
│  │   Crypto     │   Network    │   Sensors    │            │
│  │   Data Mgmt  │   Logging    │   Power Mgmt │            │
│  └──────────────┴──────────────┴──────────────┘            │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│              OS 抽象层 (OSAL)                               │
│  ┌──────────────┬──────────────┬──────────────┐            │
│  │  FreeRTOS    │  RT-Thread   │  Bare-Metal  │            │
│  └──────────────┴──────────────┴──────────────┘            │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│         硬件抽象层 (HAL)                                    │
│  ┌──────────────┬──────────────┬──────────────┐            │
│  │   UART/SPI   │   I2C/PWM    │   Timer/DMA  │            │
│  │   GPIO/RTC   │   ADC/DAC    │   Interrupt  │            │
│  └──────────────┴──────────────┴──────────────┘            │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│              SDK HAL 层                                     │
│  ┌──────────────┬──────────────┬──────────────┐            │
│  │   STM32      │   Other MCUs │   PC Sim     │            │
│  └──────────────┴──────────────┴──────────────┘            │
└─────────────────────────────────────────────────────────────┘
```

### 分层设计优势

| 层级 | 用途 | 优势 |
|------|------|------|
| **应用** | 项目特定逻辑 | 快速开发，代码复用 |
| **组件** | 领域特定功能 | 模块化，可测试性 |
| **OSAL** | OS 抽象 | 多 RTOS 支持 |
| **HAL** | 硬件抽象 | 平台可移植性 |
| **平台** | MCU 特定实现 | 硬件优化 |

---

## 核心组件

### 1. 硬件抽象层 (HAL)

**位置**: `components/hal/`

**用途**: 为硬件外设提供统一接口

**支持的接口**:
- **串行通信**: UART、SPI、I2C
- **模拟 I/O**: ADC、DAC、PWM
- **定时**: RTC、定时器、DMA
- **数字 I/O**: GPIO、中断

**关键特性**:
- 平台无关的 API
- STM32 参考实现
- 用于测试的 PC 仿真层
- 通过 `*_cfg.h` 头文件配置

**使用示例**:
```c
#include "xy_hal_uart.h"

void *uart = xy_hal_uart_open(UART_PORT_1, 115200);
xy_hal_uart_send(uart, data, len, 1000);
xy_hal_uart_close(uart);
```

---

### 2. OS 抽象层 (OSAL)

**位置**: `components/kernel/osal/`

**用途**: 为 RTOS 操作提供统一接口

**支持的 RTOS**:
- FreeRTOS
- RT-Thread
- 裸机 (无 RTOS)

**抽象内容**:
- 任务/线程管理
- 信号量、互斥量、消息队列
- 定时器和延时
- 内存管理

**关键特性**:
- 单一 API 支持多种 RTOS
- 编译时配置
- 最小开销
- 裸机的后备实现

---

### 3. 密码学组件

**位置**: `components/crypto/`

**算法**:
- **对称加密**: AES (ECB、CBC、CTR 模式)
- **哈希**: MD5、SHA-1、SHA-256
- **认证**: HMAC
- **编码**: Base64、Hex
- **工具**: CRC、随机数生成

**特性**:
- 针对嵌入式系统优化
- 可配置的算法选择
- 硬件加速支持（如可用）
- 全面的测试套件

---

### 4. 数据管理 (DM)

**位置**: `components/dm/`

**功能**:
- **存储**: EEPROM、NOR Flash、NAND Flash
- **编码**: TLV (标签 - 长度 - 值) 协议
- **NVM**: 非易失性内存管理
- **磨损均衡**: Flash 寿命优化

**用例**:
- 配置存储
- 校准数据持久化
- 固件更新暂存
- 用户数据备份

---

### 5. 网络与通信

**位置**: `components/net/`

**协议**:
- **MQTT**: IoT 消息（支持 TLS）
- **Modbus**: 工业协议（RTU/TCP）
- **AT 命令**: 蜂窝调制解调器接口
- **ISO7816**: SIM 卡通信

**特性**:
- 最小依赖的协议栈
- 可配置的缓冲区大小
- 错误恢复机制
- 示例实现

---

### 6. 传感器框架

**位置**: `components/sensor/`

**支持的传感器**:
- 温度：ADT7420、TMP36
- 运动：加速度计、陀螺仪
- 光线：APDS9960（环境光、接近）
- 压力、湿度、气体传感器

**特性**:
- 统一的传感器接口
- 校准支持
- 数据融合算法
- 低功耗操作模式
- 基于 DMA 的数据采集

---

### 7. 日志系统 (xy_log)

**位置**: `components/trace/xy_log/`

**用途**: 跨所有组件的统一日志

**日志级别**:
- `VERBOSE` - 详细跟踪信息
- `DEBUG` - 调试信息
- `INFO` - 信息性消息
- `WARN` - 警告消息
- `ERROR` - 错误消息
- `NEVER` - 禁用日志

**特性**:
- 每模块日志级别配置
- 最小运行时开销
- 支持多种输出后端
- 时间戳和上下文信息

**使用**:
```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

xy_log_d("调试消息：%d\n", value);
xy_log_e("错误：操作失败\n");
```

---

### 8. 自定义 C 库 (xy_clib)

**位置**: `components/clib/xy_clib/`

**模块**:
- **字符串操作**: 安全字符串处理
- **数学工具**: 定点数学、位操作
- **数据结构**: 列表、队列、树
- **编码**: BCD、Hex、Base64 转换
- **滤波**: 数字滤波器 (IIR、FIR)
- **内存**: 池分配器、安全的 malloc 包装

**优势**:
- 针对嵌入式优化的实现
- 减少对 libc 的依赖
- 可预测的内存使用
- 全面的测试覆盖

---

### 9. 电源管理 (PM)

**位置**: `components/pm/`

**功能**:
- 睡眠模式管理
- 电源状态转换
- 唤醒源配置
- 能耗监控

---

### 10. 其他组件

| 组件 | 位置 | 用途 |
|------|------|------|
| **电池管理** | `components/Bank/` | 电源银行、电池充电、电量计 |
| **状态机** | `components/sys/` | 分层状态机框架 |
| **定时器系统** | `components/sys/xy_timer/` | 软件定时器、调度 |
| **IPC** | `components/ipc/` | 进程间通信 |
| **FOTA** | `components/fota/` | 固件无线升级 |
| **GUI** | `components/gui/` | 显示和 UI 框架 |
| **内核** | `components/kernel/` | 核心内核工具 |

---

## 组件生态系统

### 组件依赖图

```
应用项目
    ├── 电源银行 (电池管理)
    ├── 电烙铁 (温度控制)
    ├── USB 桥接器 (多协议)
    ├── 智能卡桥接器 (ISO7816)
    ├── LCR 表 (测量)
    └── 多步进电机 (电机控制)
         │
         ├─→ 传感器框架
         ├─→ 网络 (MQTT、Modbus、AT)
         ├─→ 密码学
         ├─→ 数据管理
         ├─→ 日志 (xy_log)
         │
         ├─→ OSAL (FreeRTOS/RT-Thread/裸机)
         │    └─→ HAL (UART、SPI、I2C、GPIO 等)
         │         └─→ 平台 (STM32、PC 仿真)
         │
         └─→ xy_clib (字符串、数学、滤波器等)
```

### 组件成熟度级别

- **稳定**: HAL、OSAL、Crypto、xy_clib、日志
- **生产**: 网络 (MQTT、Modbus)、传感器、数据管理
- **活跃开发**: FOTA、GUI、电源管理
- **实验性**: 高级滤波、AI/ML 工具

---

## 实际应用案例

### 1. 电源银行 (电池管理系统)

**功能**:
- 多节电池监控
- 充放电控制
- 电量计算法
- 热管理
- 过流/过压保护

**使用的组件**:
- 传感器框架（电压、电流、温度）
- 电源管理
- 数据管理（校准存储）
- 日志

**状态**: 生产就绪

---

### 2. 电烙铁 (温度控制)

**功能**:
- PID 温度控制
- 烙铁头温度监控
- 睡眠模式管理
- 电源优化

**使用的组件**:
- HAL (ADC、PWM、定时器)
- 传感器框架
- 状态机
- 日志

---

### 3. USB 桥接器 (多协议)

**功能**:
- USB 转 SPI/I2C/UART转换
- 实时协议转换
- PC 库接口
- 多设备支持

**使用的组件**:
- HAL (USB、SPI、I2C、UART)
- 数据管理（配置）
- 日志

---

### 4. 智能卡 USB 桥接器

**功能**:
- ISO7816 SIM 卡接口
- TLV 协议编码
- USB 通信
- Python 测试客户端

**使用的组件**:
- 网络 (ISO7816)
- 数据管理 (TLV)
- HAL (UART、GPIO)
- 日志

---

### 5. LCR 表 (测量设备)

**功能**:
- 电感、电容、电阻测量
- 频率分析
- 数据记录
- 校准支持

**使用的组件**:
- 传感器框架
- 数据管理
- 日志

---

### 6. 多步进电机控制器

**功能**:
- 多轴步进控制
- 加速度曲线
- 位置跟踪
- 限位开关处理

**使用的组件**:
- HAL (GPIO、定时器、PWM)
- 状态机
- 日志

---

## 开发标准

### 强制编码标准

所有代码**必须**遵循:

1. **[xy_code_style.md](docs/rules/100-code_style/xy_code_style.md)** - 详细的 C 编码约定
2. **[RULEBOOK.md](docs/rules/RULEBOOK.md)** - 开发规则和指南
3. **[.clang-format](.clang-format)** - 自动化代码格式化

### 快速参考

| 标准 | 要求 |
|------|------|
| **语言** | C99 |
| **缩进** | 4 空格 (无制表符) |
| **命名** | 下划线小写 (`my_function`, `my_var`) |
| **括号** | 与关键字同行 (`if {`, `for {`) |
| **日志** | 使用 `xy_log_*()`, 禁止 `printf()` |
| **文档** | 所有公共函数使用 Doxygen 注释 |
| **格式化** | 提交前应用 clang-format |

### 文件结构

```c
/**
 * @file module_name.c
 * @brief 简要描述
 * @version X.Y.Z
 * @date YYYY-MM-DD
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
#include "module_name.h"

/**
 * @brief 函数描述
 * @param param1 描述
 * @return 返回值描述
 */
int my_function(int param1) {
    xy_log_d("进入函数\n");
    return 0;
}
```

### 错误处理约定

- **返回 0 或正值** 表示成功
- **返回负值** 表示错误 (标准化错误码)
- **使用 `xy_log_e()`** 记录错误日志
- **在函数注释中记录错误码**

---

## 入门指南

### 前置条件

- **编译器**: GCC、Clang 或 ARM 工具链 (C99 兼容)
- **构建工具**: CMake 3.10+ 或 GNU Make
- **可选**: FreeRTOS SDK、RT-Thread SDK
- **代码格式化**: clang-format

### 安装

```bash
# 克隆仓库
git clone <repository-url>
cd XinYi

# 安装依赖 (Ubuntu 示例)
sudo apt-get install build-essential cmake clang-format

# 验证环境
make --version
cmake --version
clang-format --version
```

### 快速构建

```bash
# 构建所有组件
make

# 构建特定组件
make crypto

# 运行测试
make test

# 清理构建产物
make clean
```

### 第一个项目

```bash
# 创建新项目目录
mkdir my_project
cd my_project

# 创建 main.c
cat > main.c << 'EOF'
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

int main(void) {
    xy_log_i("XinYi 框架已初始化\n");
    return 0;
}
EOF

# 使用 CMake 构建
mkdir build && cd build
cmake ..
make
```

---

## 构建系统

### 构建选项

#### 使用 Make

```bash
# 构建所有
make

# 构建特定组件
make crypto
make net
make sensor

# 使用特定目标构建
make TARGET=stm32

# 运行测试
make test

# 清理
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
# 跨平台构建脚本
./build.sh make all
./build.sh cmake all
./build.sh make test
```

### 配置系统 (Kconfig)

每个组件都包含 `Kconfig` 用于编译时配置:

```bash
# 交互式配置
make menuconfig

# 设置特定选项
make CONFIG_CRYPTO_AES=y
```

### 组件结构

每个组件包含:
- `CMakeLists.txt` - CMake 配置
- `Makefile` - GNU Make 配置
- `Kconfig` - 配置选项
- `README.md` - API 文档
- `*_cfg.h` - 编译时设置
- `test/` - 单元测试

---

## 贡献指南

### 提交代码前

1. **阅读文档**
   - [RULEBOOK.md](docs/rules/RULEBOOK.md) - 开发规则
   - [xy_code_style.md](docs/rules/100-code_style/xy_code_style.md) - 编码标准

2. **格式化代码**
   ```bash
   clang-format -i your_file.c
   # 或使用脚本
   ./utils/script/format_staged.sh
   ```

3. **添加文档**
   - 所有公共函数的 Doxygen 注释
   - 文件头部的简要描述
   - 新组件的 README.md

4. **测试变更**
   - 运行单元测试：`make test`
   - 在目标平台测试
   - 验证无回归

5. **使用正确的日志**
   - 用 `xy_log_*()` 替换 `printf()`
   - 设置适当的日志级别
   - 包含上下文信息

### 代码审查清单

- [ ] 遵循编码标准 (xy_code_style.md)
- [ ] 使用 clang-format 格式化
- [ ] 所有函数都有 Doxygen 文档
- [ ] 使用 `xy_log_*()` 进行日志记录
- [ ] 实现错误处理
- [ ] 包含单元测试
- [ ] 无编译器警告
- [ ] 在目标平台测试

### 提交信息格式

```
[组件] 简要描述

变更详细说明:
- 变更了什么
- 为什么变更
- 如何测试

Fixes: #issue_number
```

---

## 项目统计

### 组件数量

- **核心组件**: 15+
- **支持的协议**: 4 (MQTT、Modbus、AT、ISO7816)
- **传感器驱动**: 10+
- **加密算法**: 8+
- **实际项目**: 6+

### 代码指标

- **总代码行数**: ~50,000+
- **测试覆盖**: 全面的单元测试
- **文档**: Doxygen + Markdown
- **支持平台**: STM32、RT-Thread、FreeRTOS、裸机

### 开发活动

- **活跃组件**: 15+
- **稳定组件**: 10+
- **开发中**: 5+
- **实验性**: 3+

---

## 路线图与未来增强

### 短期 (下个版本)

- [ ] 增强的 FOTA (固件无线升级) 更新
- [ ] GUI 框架改进
- [ ] 额外的传感器驱动
- [ ] 性能优化

### 中期

- [ ] 机器学习工具
- [ ] 高级电源管理
- [ ] 增强的安全功能
- [ ] 云连接改进

### 长期

- [ ] 多核支持
- [ ] 实时性能保证
- [ ] 高级调试工具
- [ ] 商业支持选项

---

## 支持与资源

### 文档

- **主 README**: [ReadMe.md](ReadMe.md)
- **中文 README**: [ReadMe-cn.md](ReadMe-cn.md)
- **开发规则**: [RULEBOOK.md](docs/rules/RULEBOOK.md)
- **编码标准**: [xy_code_style.md](docs/rules/100-code_style/xy_code_style.md)
- **组件文档**: 各组件目录中的 README.md

### 获取帮助

1. 查看组件特定的 README.md
2. 查看组件目录中的示例代码
3. 查看单元测试了解使用模式
4. 查看 RULEBOOK.md 了解常见问题

### 社区

- 问题跟踪：[GitHub Issues]
- 讨论：[GitHub Discussions]
- 贡献：参见上方的贡献指南

---

## 附录：快速参考

### 常见任务

#### 添加新组件

1. 创建目录：`components/my_component/`
2. 添加文件：`CMakeLists.txt`、`Makefile`、`Kconfig`
3. 在 `inc/` 目录创建头文件
4. 在 `src/` 目录创建实现
5. 添加带 API 文档的 README.md
6. 在 `test/` 目录添加单元测试

#### 添加新传感器驱动

1. 创建文件：`components/sensor/sensor_<name>.c/h`
2. 实现传感器接口
3. 添加校准支持
4. 创建使用示例
5. 添加单元测试

#### 移植到新平台

1. 创建平台目录：`bsp/xy_hal/<platform>/`
2. 为平台实现 HAL 接口
3. 创建平台特定的 Makefile/CMakeLists.txt
4. 在新平台测试所有组件
5. 记录平台特定的注意事项

### 有用的命令

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
make TARGET=stm32f4

# 清理所有
make distclean
```

---

**文档结束**
