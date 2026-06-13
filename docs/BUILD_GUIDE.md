# XinYi Framework Build Guide

## 📁 Build Directory Structure

```
XinYi/
└── build/
    ├── pc/                  # 默认 PC Release 构建
    ├── stm32f4/             # STM32F4 构建
    ├── stm32u5/             # STM32U5 构建
    ├── wch/                 # WCH/CH32 构建
    ├── hc32/                # HC32 构建
    └── tests/unit/          # PC 单元测试构建
```

## 🚀 Quick Build

### PC 平台 (推荐)
```bash
cd XinYi
make
```

### STM32F4 平台
```bash
cd XinYi
make HAL_PLATFORM=STM32F4
```

### STM32U5 平台
```bash
cd XinYi
make HAL_PLATFORM=STM32U5
```

## ⚙️ CMake 选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `HAL_PLATFORM` | 平台：`PC` / `STM32F4` / `STM32U5` / `WCH` / `HC32` | `PC` |
| `BUILD_TYPE` | `Release` / `Debug` | `Release` |
| `BUILD_TESTS` | 是否启用根 CMake 测试 | `OFF` |
| `FOTA` | 是否启用 FOTA | `OFF` |
| `BUILD_ROOT` | 统一构建根目录 | `build` |
| `BUILD_DIR` | CMake 输出目录 | `build/<platform>` |

## 📋 推荐的构建目录

| 用途 | 目录 | 说明 |
|------|------|------|
| PC 默认构建 | `build/pc/` | `make` |
| STM32F4 | `build/stm32f4/` | `make HAL_PLATFORM=STM32F4` |
| STM32U5 | `build/stm32u5/` | `make HAL_PLATFORM=STM32U5` |
| WCH/CH32 | `build/wch/` | `make HAL_PLATFORM=WCH` |
| HC32 | `build/hc32/` | `make HAL_PLATFORM=HC32` |
| PC 单元测试 | `build/tests/unit/` | `make test-unit` |
| STM32F4 QEMU | `build/qemu/stm32f4/` | `make test-qemu` |
| CH32V QEMU | `build/qemu/ch32v/` | `make test-qemu-ch32v` |

更完整的构建入口和输出目录说明见 `docs/BUILD_PROCESS_OUTPUTS.md`。

## 🗑️ 清理旧构建

```bash
# 删除统一 build 目录、旧 build_* 目录、examples/tests/components 下常见嵌套 build 目录
cd XinYi
make distclean
```

## 🔧 常用构建命令

```bash
# 清理后重新构建
make distclean
make

# 只构建特定目标
cmake --build build/pc --target xy_sensor
cmake --build build/pc --target xy_actuator

# 查看所有可用目标
make help
```

## 📁 输出文件

构建产物默认位于 `build/<platform>/` 目录下：
- `*.a` - 静态库文件
- `*.elf` / `*.bin` - 可执行文件 (嵌入式平台)
- `CMakeFiles/` - 编译中间文件

---
_最后更新: 2026-03-30_
