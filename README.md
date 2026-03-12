# XinYi (芯一)

> **嵌入式系统框架** - 为 STM32/CH32 打造的统一开发平台

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-STM32%20%7C%20CH32-orange.svg)](docs/hardware/)
[![Build](https://img.shields.io/badge/build-make-green.svg)](docs/getting-started/installation.md)

---

## 📋 目录

- [简介](#简介)
- [特性](#特性)
- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [硬件支持](#硬件支持)
- [文档](#文档)
- [贡献](#贡献)
- [许可](#许可)

---

## 简介

**XinYi (芯一)** 是 ZeroZap 组织开发的嵌入式系统框架，为 STM32 和 CH32 系列 MCU 提供统一的开发平台。

### 设计目标

- **统一抽象**: 屏蔽不同 MCU 系列的差异，提供一致的 API
- **模块化设计**: 按需选择组件，灵活配置
- **AI 友好**: 支持 AI Agent 自动化开发和代码生成
- **生产就绪**: 经过实际项目验证的稳定框架

### 核心组件

```
XinYi
├── OSAL          # 操作系统抽象层 (支持 RT-Thread, Zephyr, Bare-metal)
├── HAL           # 硬件抽象层 (统一外设接口)
├── Components    # 可复用组件 (FOTA, Logger, Shell 等)
├── BSP           # 板级支持包 (开发板配置)
└── Tools         # 开发工具链 (构建、调试、烧录)
```

---

## 特性

### ✨ 核心特性

- **多 MCU 支持**: STM32F1/F4/G0, CH32V30X/X03X
- **多 OS 后端**: RT-Thread, Zephyr, FreeRTOS, Bare-metal
- **统一 API**: 一套代码，多平台运行
- **组件化架构**: 按需选择，灵活配置
- **Kconfig 配置**: 类似 Linux 内核的配置系统

### 🛠️ 内置组件

| 组件 | 描述 | 状态 |
|------|------|------|
| **FOTA** | 固件无线升级 | 🟢 稳定 |
| **Logger** | 日志系统 (多级别、多输出) | 🟢 稳定 |
| **Shell** | 命令行交互界面 | 🟢 稳定 |
| **KV** | 键值存储 (Flash/EEPROM) | 🟢 稳定 |
| **Timer** | 软件定时器 | 🟢 稳定 |
| **WorkQueue** | 工作队列 | 🟡 开发中 |

### 🔧 开发工具

- **统一构建系统**: 基于 Make/CMake
- **代码生成器**: 自动生成初始化代码
- **调试支持**: GDB, J-Link, ST-Link
- **CI/CD**: 自动化构建和测试

---

## 快速开始

### 环境要求

- **工具链**: ARM GCC (arm-none-eabi-gcc)
- **构建工具**: Make, CMake
- **调试工具**: OpenOCD (可选)
- **操作系统**: Linux, macOS, Windows (WSL)

### 安装步骤

```bash
# 1. 克隆仓库
git clone https://github.com/ZeroZap/XinYi.git
cd XinYi

# 2. 配置工具链 (如未在全局路径)
export CROSS_COMPILE=arm-none-eabi-

# 3. 配置项目
make menuconfig

# 4. 构建
make

# 5. 烧录 (需要硬件)
make flash
```

详细安装指南见 [安装文档](docs/getting-started/installation.md)。

### 最小示例

```c
#include <xy_os.h>
#include <xy_log.h>

int main(void) {
    // 初始化日志系统
    xy_log_init();
    
    // 初始化 OS
    xy_os_init();
    
    LOG_I("XinYi 启动!");
    
    // 创建任务
    xy_os_thread_create("task1", task1_entry, NULL, 512, 5);
    
    // 启动调度器
    xy_os_start();
    
    return 0;
}
```

---

## 项目结构

```
XinYi/
├── components/           # 可复用组件
│   ├── fota/            # FOTA 升级
│   ├── logger/          # 日志系统
│   ├── shell/           # Shell 组件
│   ├── kv/              # 键值存储
│   └── ...
├── core/                # 核心框架
│   ├── osal/            # OS 抽象层
│   ├── hal/             # HAL 抽象层
│   └── include/         # 公共头文件
├── bsp/                 # 板级支持包
│   ├── stm32/           # STM32 系列
│   │   ├── stm32f103x8/
│   │   ├── stm32f407vg/
│   │   └── ...
│   └── ch32/            # CH32 系列
│       ├── ch32v307vc/
│       ├── ch32x035c8/
│       └── ...
├── docs/                # 文档
│   ├── getting-started/ # 入门指南
│   ├── api/             # API 参考
│   ├── guides/          # 使用指南
│   └── hardware/        # 硬件支持
├── examples/            # 示例代码
├── tools/               # 开发工具
├── Kconfig              # 配置菜单
├── Makefile             # 构建系统
└── README.md            # 本文件
```

---

## 硬件支持

### 支持的 MCU 系列

| 系列 | MCU 型号 | 状态 | BSP |
|------|---------|------|-----|
| **STM32F1** | STM32F103x8, STM32F103xC | 🟢 稳定 | ✅ |
| **STM32F4** | STM32F407VG, STM32F429ZI | 🟢 稳定 | ✅ |
| **STM32G0** | STM32G071RB | 🟡 开发中 | 🔄 |
| **CH32V30x** | CH32V307VC, CH32V305RB | 🟢 稳定 | ✅ |
| **CH32X03x** | CH32X035C8, CH32X035F8 | 🟢 稳定 | ✅ |

### 支持的开发板

| 开发板 | MCU | 状态 |
|--------|-----|------|
| **XinBoard Std** | STM32F103xC | 🟢 稳定 |
| **XinBoard Pro** | STM32F407VG | 🟢 稳定 |
| **Blue Pill** | STM32F103C8 | 🟢 稳定 |
| **Nucleo-F429ZI** | STM32F429ZI | 🟡 开发中 |
| **CH32V307VCT6** | CH32V307VC | 🟢 稳定 |

查看完整的 [硬件支持列表](docs/hardware/supported-boards.md)。

---

## 文档

### 入门指南

- [安装指南](docs/getting-started/installation.md)
- [快速开始](docs/getting-started/quickstart.md)
- [第一个项目](docs/getting-started/first-project.md)
- [常见问题](docs/getting-started/faq.md)

### API 参考

- [OSAL API](docs/api/osal.md)
- [HAL API](docs/api/hal.md)
- [组件 API](docs/api/components.md)

### 使用指南

- [配置系统](docs/guides/kconfig.md)
- [添加新组件](docs/guides/add-component.md)
- [移植到新 MCU](docs/guides/port-to-new-mcu.md)
- [调试技巧](docs/guides/debugging.md)

### 硬件文档

- [支持的 MCU](docs/hardware/supported-mcus.md)
- [支持的开发板](docs/hardware/supported-boards.md)
- [原理图](docs/hardware/schematics/)

---

## 贡献

欢迎贡献代码、文档和建议!

### 贡献方式

1. **报告 Bug**: 在 GitHub Issues 中创建 issue
2. **功能建议**: 在 GitHub Discussions 中讨论
3. **代码贡献**: 提交 Pull Request
4. **文档改进**: 修正拼写错误、补充说明

### 开发流程

```bash
# 1. Fork 仓库
# 2. 创建功能分支
git checkout -b feature/your-feature

# 3. 开发和测试
# 4. 提交变更
git commit -m "feat(component): add new feature"

# 5. 推送到分支
git push origin feature/your-feature

# 6. 创建 Pull Request
```

详见 [贡献指南](docs/contribute/how-to-contribute.md)。

### 代码规范

- 遵循 [C 编码规范](docs/rules/c-coding-style.md)
- 提交信息遵循 [Conventional Commits](docs/rules/commit-message.md)
- 所有代码通过 CI 检查

---

## 社区

- **GitHub**: https://github.com/ZeroZap/XinYi
- **Discord**: [加入讨论](https://discord.gg/zerozap)
- **文档**: https://zerozap.github.io/XinYi

---

## 许可

XinYi 采用 **Apache License 2.0** 许可。

详见 [LICENSE](LICENSE) 文件。

---

## 致谢

感谢以下项目和组织:

- [RT-Thread](https://github.com/RT-Thread/rt-thread) - RTOS 灵感来源
- [Zephyr](https://github.com/zephyrproject-rtos/zephyr) - 配置系统设计
- [ARM](https://www.arm.com) - CMSIS 标准
- [WCH](https://www.wch.cn) - CH32 系列 MCU

---

**维护者**: ese (嵌入式系统工程师)  
**联系方式**: Discord @ese  
**最后更新**: 2026-03-12
