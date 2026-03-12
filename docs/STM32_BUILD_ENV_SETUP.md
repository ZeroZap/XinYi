# STM32 编译环境部署指南

**XinYi Framework - STM32 开发环境配置**

---

## 📋 环境概述

- **工具链**: Arm GNU Toolchain 15.2.1 (arm-none-eabi-gcc)
- **工具链路径**: `/home/eugene/Tools/arm-gnu-toolchain/bin/`
- **CMake 工具链文件**: `cmake/arm-gcc.cmake`
- **支持设备**: STM32F4 系列 (可扩展)

---

## 🔧 工具链配置

### 1. 工具链位置

```bash
/home/eugene/Tools/arm-gnu-toolchain/bin/
├── arm-none-eabi-gcc      # C 编译器
├── arm-none-eabi-g++      # C++ 编译器
├── arm-none-eabi-ar       # 归档工具
├── arm-none-eabi-objcopy  # 格式转换
├── arm-none-eabi-objdump  # 反汇编
├── arm-none-eabi-size     # 代码大小分析
└── arm-none-eabi-gdb      # 调试器
```

### 2. CMake 工具链文件

位置：`cmake/arm-gcc.cmake`

主要配置：
- 目标系统：Generic (裸机)
- 处理器：ARM Cortex-M
- 编译器标志：`-Wall -Wextra -ffreestanding -nostdlib`
- 优化级别：Release (-Os) / Debug (-O0 -g3)

---

## 📁 项目结构

```
XinYi/
├── cmake/
│   └── arm-gcc.cmake          # ARM GCC 工具链配置
├── projects/
│   └── stm32_test/            # STM32 测试项目
│       ├── CMakeLists.txt     # 项目构建配置
│       ├── STM32F407VGTx_FLASH.ld  # 链接脚本
│       ├── src/
│       │   ├── main.c         # 主程序
│       │   └── startup_stm32f407xx.c  # 启动文件
│       └── build/             # 构建输出目录
└── docs/
    └── STM32_BUILD_ENV_SETUP.md  # 本文档
```

---

## 🚀 构建流程

### 方法 1: 使用 CMake (推荐)

```bash
cd /home/eugene/zerozap/XinYi/projects/stm32_test

# 配置
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-gcc.cmake -DCMAKE_BUILD_TYPE=Release

# 编译
make

# 输出文件
# - stm32_test.elf  (ELF 格式，用于调试)
# - stm32_test.bin  (二进制，用于烧录)
# - stm32_test.hex  (Intel HEX，用于烧录)
```

### 方法 2: 使用项目构建脚本

```bash
cd /home/eugene/zerozap/XinYi

# 使用 Make
make BUILD_TYPE=release

# 使用 CMake
./build.sh cmake all
```

---

## 📊 构建输出示例

```
[ 33%] Building C object CMakeFiles/stm32_test.dir/src/main.c.obj
[ 66%] Building C object CMakeFiles/stm32_test.dir/src/startup_stm32f407xx.c.obj
[100%] Linking C executable stm32_test.elf
Generating stm32_test.bin
Generating stm32_test.hex
Printing size information
   text	   data	    bss	    dec	    hex	filename
    352	      0	   1536	   1888	    760	stm32_test.elf
```

---

## 🔍 代码大小分析

使用 `arm-none-eabi-size` 工具分析：

```bash
arm-none-eabi-size build/stm32_test.elf

# 输出示例：
#    text	   data	    bss	    dec	    hex	filename
#     352	      0	   1536	   1888	    760	stm32_test.elf
```

- **text**: 代码段大小 (Flash)
- **data**: 已初始化数据 (Flash + RAM)
- **bss**: 未初始化数据 (RAM)

---

## 🎯 目标设备配置

当前测试项目针对 **STM32F407VGTx**:

- **Flash**: 1024KB (0x08000000)
- **RAM**: 128KB (0x20000000)
- **内核**: Cortex-M4
- **FPU**: 硬件浮点 (FPv4-SP-D16)

修改 `projects/stm32_test/CMakeLists.txt` 可适配其他设备：

```cmake
set(STM32_DEVICE "STM32F407xx" CACHE STRING "STM32 device")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
```

---

## 🛠️ 常见问题

### 1. 找不到编译器

```bash
# 检查工具链路径
ls /home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-gcc

# 添加到 PATH (可选)
export PATH="/home/eugene/Tools/arm-gnu-toolchain/bin:$PATH"
```

### 2. CMake 配置失败

```bash
# 清理重新配置
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-gcc.cmake
```

### 3. 链接错误

检查链接脚本是否与目标设备匹配：
- Flash 起始地址
- RAM 大小
- 栈和堆大小

---

## 📝 下一步

1. **添加更多组件**: 将 XinYi Framework 的组件集成到项目中
2. **调试支持**: 配置 OpenOCD 或 J-Link 进行硬件调试
3. **CI/CD**: 添加自动化构建和测试流程
4. **文档完善**: 为每个组件添加使用说明

---

**最后更新**: 2026-03-12  
**维护者**: XinYi Team
