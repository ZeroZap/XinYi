# STM32 Test Project - XinYi Framework

快速验证 STM32 编译环境的测试项目。

## 🎯 功能

- LED Blink 测试 (PA5 引脚)
- STM32F407VGTx 目标设备
- 完整的启动文件和链接脚本

## 🚀 快速开始

```bash
# 进入项目目录
cd /home/eugene/zerozap/XinYi/projects/stm32_test

# 构建
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-gcc.cmake -DCMAKE_BUILD_TYPE=Release
make

# 输出
# - stm32_test.elf  (调试用)
# - stm32_test.bin  (烧录用)
# - stm32_test.hex  (烧录用)
```

## 📊 构建输出示例

```
[100%] Linking C executable stm32_test.elf
Generating stm32_test.bin
Generating stm32_test.hex
Printing size information
   text	   data	    bss	    dec	    hex	filename
    352	      0	   1536	   1888	    760	stm32_test.elf
```

## 🔧 自定义

### 修改目标设备

编辑 `CMakeLists.txt`:

```cmake
set(STM32_DEVICE "STM32F407xx")  # 修改为你的设备
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb")
```

### 修改 LED 引脚

编辑 `src/main.c`:

```c
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))
GPIOA_ODR |= (1UL << 5);   // 修改 5 为你的引脚号
```

## 📁 文件结构

```
stm32_test/
├── CMakeLists.txt              # 构建配置
├── STM32F407VGTx_FLASH.ld      # 链接脚本
├── README.md                   # 本文档
├── src/
│   ├── main.c                  # 主程序
│   └── startup_stm32f407xx.c   # 启动文件
└── build/                      # 构建输出
```

## 📖 相关文档

- [STM32 编译环境部署指南](../../docs/STM32_BUILD_ENV_SETUP.md)
- [XinYi Framework 主文档](../../ReadMe.md)

---

**XinYi Framework** | 2026
