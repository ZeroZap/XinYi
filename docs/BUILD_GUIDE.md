# XinYi Framework Build Guide

## 📁 Build Directory Structure

```
XinYi/
├── build/                    # 主构建目录 (PC)
├── build_full_test/         # 完整功能测试构建
├── build_stm32f4_test/      # STM32F4 测试构建
├── build_stm32f4_validation/ # STM32F4 验证构建
├── build_stm32u5_validation/ # STM32U5 验证构建
├── build_hc32l021_test/      # HC32 测试构建
└── build_atc_test/           # ATC 测试构建
```

## 🚀 Quick Build

### PC 平台 (推荐)
```bash
cd XinYi
rm -rf build && mkdir build
cd build
cmake .. -DXY_PLATFORM_PC=ON -DXY_CONFIG_SENSOR_ENABLED=ON \
         -DXY_CONFIG_ACTUATOR_ENABLED=ON -DXY_CONFIG_SMBUS_ENABLED=ON
make -j4
```

### STM32F4 平台
```bash
cd XinYi
rm -rf build_stm32f4_test && mkdir build_stm32f4_test
cd build_stm32f4_test
cmake .. -DXY_PLATFORM_STM32F4=ON -DXY_HAL_STM32=ON
make -j4
```

### STM32U5 平台
```bash
cd XinYi
rm -rf build_stm32u5_validation && mkdir build_stm32u5_validation
cd build_stm32u5_validation
cmake .. -DXY_PLATFORM_STM32U5=ON -DXY_HAL_STM32=ON
make -j4
```

## ⚙️ CMake 选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `XY_PLATFORM_PC` | PC 平台 | OFF |
| `XY_PLATFORM_STM32F4` | STM32F4 平台 | OFF |
| `XY_PLATFORM_STM32U5` | STM32U5 平台 | OFF |
| `XY_CONFIG_SENSOR_ENABLED` | 启用传感器组件 | ON |
| `XY_CONFIG_ACTUATOR_ENABLED` | 启用执行器组件 | ON |
| `XY_CONFIG_SMBUS_ENABLED` | 启用 SMBus/PMBus | ON |
| `XY_GUI_ENABLED` | 启用 GUI | ON |
| `XY_CONFIG_LOG_ENABLED` | 启用日志 | OFF |

## 📋 推荐的构建目录

| 用途 | 目录 | 说明 |
|------|------|------|
| 开发调试 | `build/` | PC 平台快速验证 |
| 功能测试 | `build_full_test/` | 完整功能测试 |
| STM32F4 | `build_stm32f4_test/` | F4 芯片测试 |
| STM32U5 | `build_stm32u5_validation/` | U5 芯片验证 |

## 🗑️ 清理旧构建

```bash
# 删除所有 build_* 目录
cd XinYi
for d in build_*; do [ -d "$d" ] && rm -rf "$d"; done

# 仅保留推荐目录
mkdir build build_stm32f4_test build_stm32u5_validation
```

## 🔧 常用构建命令

```bash
# 清理后重新构建
rm -rf build && mkdir build && cd build
cmake .. && make -j4

# 只构建特定目标
make xy_sensor
make xy_actuator
make xy_smbus

# 查看所有可用目标
make help
```

## 📁 输出文件

构建产物位于各 `build_*/` 目录下：
- `*.a` - 静态库文件
- `*.elf` / `*.bin` - 可执行文件 (嵌入式平台)
- `CMakeFiles/` - 编译中间文件

---
_最后更新: 2026-03-30_