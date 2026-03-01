# XinYi 开发者入门指南

**版本**: 1.0  
**最后更新**: 2026-03-01  
**目标读者**: 新加入 XinYi 项目的开发者

---

## 📋 目录

1. [快速开始](#1-快速开始)
2. [项目结构](#2-项目结构)
3. [开发环境搭建](#3-开发环境搭建)
4. [第一个 XinYi 项目](#4-第一个-xinyi-项目)
5. [学习路径](#5-学习路径)
6. [常见问题](#6-常见问题)

---

## 1. 快速开始

### 1.1 5 分钟上手

```bash
# 1. 克隆项目
git clone https://github.com/ZeroZap/XinYi.git
cd XinYi
git submodule update --init --recursive

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置 CMake
cmake .. -DBUILD_TESTING=ON

# 4. 构建
make -j$(nproc)

# 5. 运行测试
make test
```

### 1.2 最小代码示例

```c
/* main.c */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"
#include "xy_os.h"

int main(void) {
    xy_log_i("Hello, XinYi!\n");
    
    xy_os_kernel_init();
    
    xy_log_i("XinYi initialized!\n");
    
    while (1) {
        xy_os_delay(1000);
        xy_log_i("Tick...\n");
    }
    
    return 0;
}
```

---

## 2. 项目结构

### 2.1 核心目录

```
XinYi/
├── components/          # 核心组件
│   ├── kernel/         # 内核组件
│   │   ├── osal/       # OS 抽象层
│   │   └── misc/       # 内核杂项
│   ├── hal/            # 硬件抽象层
│   ├── clib/           # C 标准库
│   ├── crypto/         # 密码学
│   ├── dm/             # 数据管理
│   ├── net/            # 网络协议
│   ├── device/         # 设备管理
│   └── ...
├── tests/              # 统一测试目录
├── projects/           # 应用项目
├── third_party/        # 第三方库
└── docs/               # 文档
```

### 2.2 组件结构

每个组件遵循统一结构：

```
components/<name>/
├── inc/               # 公共头文件
├── src/               # 源文件
├── tests/             # 单元测试（可选）
├── docs/              # 组件文档
├── CMakeLists.txt     # CMake 配置
├── Kconfig            # Kconfig 配置
└── README.md          # 组件说明
```

---

## 3. 开发环境搭建

### 3.1 必需工具

| 工具 | 版本 | 用途 |
|------|------|------|
| GCC | 9.0+ | 编译器 |
| CMake | 3.10+ | 构建系统 |
| Git | 2.0+ | 版本控制 |
| Python | 3.8+ | 工具脚本 |

### 3.2 可选工具

| 工具 | 用途 |
|------|------|
| STM32CubeIDE | STM32 开发 |
| VS Code + PlatformIO | 跨平台开发 |
| Doxygen | API 文档生成 |
| clang-format | 代码格式化 |

### 3.3 Windows 环境

```powershell
# 1. 安装 MSYS2
# 2. 安装工具链
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-python

# 3. 克隆项目
git clone https://github.com/ZeroZap/XinYi.git
```

### 3.4 Linux 环境

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi cmake git python3

# Fedora
sudo dnf install arm-none-eabi-gcc-cs cmake git python3
```

### 3.5 macOS 环境

```bash
# 使用 Homebrew
brew install arm-none-eabi-gcc cmake git python3
```

---

## 4. 第一个 XinYi 项目

### 4.1 创建项目目录

```bash
mkdir my_first_project
cd my_first_project
```

### 4.2 创建 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyFirstProject C)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)

# 添加 XinYi 组件
set(XINYI_ROOT "../")  # XinYi 项目根目录
include_directories(
    ${XINYI_ROOT}/components/trace/xy_log/inc
    ${XINYI_ROOT}/components/kernel/osal/inc
)

# 添加源文件
add_executable(main main.c)

# 链接库（如果需要）
target_link_libraries(main)
```

### 4.3 创建 main.c

```c
/**
 * @file main.c
 * @brief My First XinYi Project
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"
#include "xy_os.h"

static void led_blink_task(void *arg) {
    (void)arg;
    
    while (1) {
        xy_log_i("LED Blink!\n");
        xy_os_delay(500);
    }
}

int main(void) {
    /* 初始化日志 */
    xy_log_i("=== My First XinYi Project ===\n");
    
    /* 初始化 OS */
    xy_os_kernel_init();
    
    /* 创建 LED 闪烁任务 */
    xy_os_thread_t led_thread;
    static uint8_t led_stack[512];
    
    xy_os_thread_create(
        &led_thread,
        "LED",
        led_blink_task,
        NULL,
        5,              /* 优先级 */
        led_stack,
        sizeof(led_stack)
    );
    
    /* 启动内核 */
    xy_os_kernel_start();
    
    return 0;
}
```

### 4.4 构建和运行

```bash
# 构建
mkdir build && cd build
cmake ..
make

# 运行（PC 仿真）
./main

# 输出:
# === My First XinYi Project ===
# [INFO] LED Blink!
# [INFO] LED Blink!
# ...
```

---

## 5. 学习路径

### 5.1 新手阶段 (1-2 周)

#### 第 1 周：基础了解

- [ ] 阅读 [项目概述](docs/getting-started/introduction.md)
- [ ] 学习 [代码风格规范](docs/rules/C_Coding_Standard_Full.md)
- [ ] 运行示例项目
- [ ] 理解项目结构

#### 第 2 周：组件学习

- [ ] 学习 OSAL 组件（OS 抽象层）
- [ ] 学习 HAL 组件（硬件抽象层）
- [ ] 学习 CLib 组件（C 标准库）
- [ ] 编写简单测试

### 5.2 进阶阶段 (1-2 月)

#### 第 1 月：深入组件

- [ ] 学习 DM 组件（数据管理）
- [ ] 学习 NET 组件（网络协议）
- [ ] 学习 Crypto 组件（密码学）
- [ ] 贡献第一个 PR

#### 第 2 月：实际应用

- [ ] 创建完整项目
- [ ] 添加新设备驱动
- [ ] 优化现有代码
- [ ] 编写技术文档

### 5.3 专家阶段 (3-6 月)

#### 技能提升

- [ ] 移植到新平台
- [ ] 设计新组件
- [ ] 性能优化
- [ ] 代码审查

#### 贡献方向

- [ ] 修复 Bug
- [ ] 新功能开发
- [ ] 文档完善
- [ ] 社区建设

---

## 6. 常见问题

### 6.1 构建问题

**Q: CMake 找不到编译器**

```bash
# 检查编译器
which arm-none-eabi-gcc

# 设置环境变量
export PATH=$PATH:/path/to/gcc/bin
```

**Q: 链接错误 - 找不到 xy_log**

```cmake
# 确保包含正确的头文件路径
include_directories(${XINYI_ROOT}/components/trace/xy_log/inc)

# 添加源文件
target_sources(main PRIVATE
    ${XINYI_ROOT}/components/trace/xy_log/src/xy_log.c
)
```

### 6.2 代码问题

**Q: 如何设置日志级别**

```c
/* 在文件开头设置 */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

/* 可用级别 */
XY_LOG_LEVEL_VERBOSE   /* 详细 */
XY_LOG_LEVEL_DEBUG     /* 调试 */
XY_LOG_LEVEL_INFO      /* 信息 */
XY_LOG_LEVEL_WARN      /* 警告 */
XY_LOG_LEVEL_ERROR     /* 错误 */
XY_LOG_LEVEL_NEVER     /* 禁用 */
```

**Q: 如何使用 OSAL 任务**

```c
#include "xy_os.h"

/* 1. 定义任务函数 */
static void my_task(void *arg) {
    while (1) {
        /* 任务代码 */
        xy_os_delay(100);
    }
}

/* 2. 创建任务 */
xy_os_thread_t thread;
static uint8_t stack[1024];

xy_os_thread_create(
    &thread,
    "MyTask",
    my_task,
    NULL,
    5,          /* 优先级 */
    stack,
    sizeof(stack)
);

/* 3. 启动内核 */
xy_os_kernel_start();
```

### 6.3 调试技巧

**使用日志调试**

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

int my_function(int param) {
    xy_log_d("Enter: param=%d\n", param);
    
    /* 函数代码 */
    
    xy_log_d("Exit: result=%d\n", result);
    return result;
}
```

**使用断言**

```c
#include "xy_assert.h"

int process_data(uint8_t *data, size_t len) {
    XY_ASSERT(data != NULL);
    XY_ASSERT(len > 0);
    
    /* 处理数据 */
}
```

---

## 📚 相关文档

| 文档 | 说明 |
|------|------|
| [项目概述](docs/getting-started/introduction.md) | 项目愿景和架构 |
| [快速入门](docs/getting-started/quickstart.md) | 5 分钟上手 |
| [代码风格](docs/rules/C_Coding_Standard_Full.md) | 完整编码规范 |
| [组件文档](docs/components/index.md) | 各组件详细说明 |
| [API 参考](docs/api/index.md) | API 文档 |

---

## 🤝 获取帮助

- 📚 [文档索引](docs/README.md)
- ❓ [常见问题](docs/about/faq.md)
- 💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
