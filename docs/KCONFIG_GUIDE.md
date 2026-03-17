# XinYi Framework - Kconfig + CMake 配置系统

## 📋 概述

XinYi 现在使用类似 Zephyr/ESP-IDF 的 Kconfig + CMake 配置系统，提供：

- ✅ **统一的配置界面** - 单一 Kconfig 文件配置所有选项
- ✅ **跨平台支持** - PC (Windows/Linux/macOS) + 嵌入式 (STM32/WCH/HC32)
- ✅ **条件编译** - 根据配置自动启用/禁用功能
- ✅ **模块化构建** - 只编译需要的组件

## 🚀 快速开始

### PC 平台 (Linux/macOS)

```bash
cd XinYi
mkdir build && cd build
cmake ..
make
```

### PC 平台 (Windows + MinGW)

```bash
cd XinYi
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

### STM32 平台

```bash
cd XinYi
mkdir build_stm32 && cd build_stm32
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/platform/STM32U5.cmake ..
make
```

## ⚙️ 配置选项

### 平台选择

| 选项 | 说明 | 默认 |
|------|------|------|
| `PLATFORM_PC` | PC 模拟 (Windows/Linux/macOS) | ✅ (x86_64) |
| `PLATFORM_STM32U5` | STM32U5 系列 (Cortex-M33) | ❌ |
| `PLATFORM_STM32F4` | STM32F4 系列 (Cortex-M4) | ❌ |
| `PLATFORM_STM32F1` | STM32F1 系列 (Cortex-M3) | ❌ |
| `PLATFORM_WCH` | 沁恒 CH32 系列 | ❌ |
| `PLATFORM_HC32` | 华大 HC32 系列 | ❌ |

### GUI 子系统

| 选项 | 说明 | 依赖 |
|------|------|------|
| `GUI_ENABLED` | 启用 GUI 子系统 | - |
| `GUI_SDL` | SDL2 后端 (PC) | `GUI_ENABLED` + `PLATFORM_PC` |
| `GUI_TFT` | TFT LCD 后端 (嵌入式) | `GUI_ENABLED` + `PLATFORM_STM32` |
| `GUI_LVGL` | LVGL 集成 | `GUI_ENABLED` |
| `GUI_WIDGETS` | 内置控件库 | `GUI_ENABLED` |

### 核心组件

| 选项 | 说明 | 默认 |
|------|------|------|
| `COMPONENT_DEVICE` | 设备框架 | ✅ |
| `COMPONENT_CHARGER` | 充电管理 | ❌ |
| `COMPONENT_CRYPTO` | 加密库 | ✅ |
| `COMPONENT_SENSOR` | 传感器框架 | ✅ |

### 日志与调试

| 选项 | 说明 | 默认 |
|------|------|------|
| `LOG_ENABLED` | 日志系统 | ✅ |
| `LOG_LEVEL` | 日志级别 (0-4) | 3 (INFO) |
| `LOG_COLOR` | 彩色输出 | ✅ (PC) |
| `ASSERT_ENABLED` | 断言 | ✅ |

## 📁 生成的文件

构建时自动生成：

```
build/
├── .config           # 构建配置 (类似 Linux kernel)
├── include/
│   └── autoconf.h    # C 语言配置宏
└── config.cmake      # CMake 变量
```

### autoconf.h 示例

```c
#ifndef XY_AUTOCONF_H
#define XY_AUTOCONF_H

#define CONFIG_PLATFORM_PC 1
#define CONFIG_GUI_ENABLED 0
#define CONFIG_LOG_ENABLED 1
#define CONFIG_LOG_LEVEL 3
#define CONFIG_COMPONENT_DEVICE 1
#define CONFIG_COMPONENT_CRYPTO 1

#endif /* XY_AUTOCONF_H */
```

## 🔧 高级用法

### 自定义配置

1. 编辑 `Kconfig` 添加新选项
2. 运行 `cmake ..` 重新生成配置
3. 在代码中使用 `#if CONFIG_XXX` 条件编译

### 平台特定配置

```bash
# 启用 GUI + SDL2
cmake -DXY_GUI_ENABLED=y -DXY_GUI_SDL=y ..

# 启用所有组件
cmake -DXY_COMPONENT_CHARGER=y -DXY_COMPONENT_DM=y ..

# 调试构建
cmake -DCMAKE_BUILD_TYPE=Debug -DXY_BUILD_TESTING=y ..
```

### 跨平台 GUI 开发

```c
// 代码中根据配置选择后端
#if CONFIG_GUI_SDL
    #include "xy_gui_sdl.h"
    // SDL2 实现
#elif CONFIG_GUI_TFT
    #include "xy_gui_tft.h"
    // TFT 实现
#endif
```

## 📊 构建矩阵

| 平台 | GUI | 组件 | 状态 |
|------|-----|------|------|
| **Linux** | SDL2 | 全部 | ✅ |
| **macOS** | SDL2 | 全部 | ✅ |
| **Windows** | SDL2 | 全部 | ✅ |
| **STM32U5** | TFT | HAL + Device | 🟡 |
| **STM32F4** | TFT | HAL + Device | 🟡 |
| **WCH** | - | HAL | 🔴 |
| **HC32** | - | HAL | 🔴 |

## 🎯 下一步

1. ✅ Kconfig 配置系统
2. ✅ CMake 集成
3. ✅ 平台配置文件
4. 🟡 GUI SDL2 后端实现
5. 🟡 GUI TFT 后端实现
6. 🟡 自动化测试集成

---

**文档**: 参见 `docs/BUILD_SYSTEM.md`  
**示例**: 参见 `examples/cmake_*` 目录
