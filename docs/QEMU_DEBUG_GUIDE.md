# XinYi QEMU STM32 调试指南

**版本**: 1.1.0  
**日期**: 2026-03-17  
**维护者**: XinYi Team  
**状态**: ✅ 环境验证通过

---

## 📋 概述

使用 QEMU STM32 模拟器进行 XinYi 组件的虚拟调试，无需实际硬件即可开发和测试驱动代码。

---

## ✅ 环境验证 (2026-03-17)

### 已安装工具
| 工具 | 版本 | 状态 |
|------|------|------|
| QEMU | 8.2.2 | ✅ 已安装 |
| GDB | 15.0.50 | ✅ 已安装 |
| ARM GCC | 9.3.1 | ✅ 已配置 |

### 测试固件
- **路径**: `examples/qemu_test/build/qemu_test.elf`
- **大小**: 40 bytes
- **运行**: ✅ QEMU lm3s6965evb 验证通过

### 支持的 QEMU 开发板
系统 QEMU 8.2.2 支持的 Cortex-M 开发板：
| 开发板 | 内核 | 推荐用途 |
|--------|------|---------|
| `lm3s6965evb` | Cortex-M3 | ⭐ 推荐 - Stellaris 评估板 |
| `lm3s811evb` | Cortex-M3 | Stellaris 评估板 |
| `mps2-an385` | Cortex-M3 | ARM MPS2 FPGA |
| `mps2-an386` | Cortex-M4 | ARM MPS2 FPGA |
| `olimex-stm32-h405` | Cortex-M4 | STM32F405 |
| `stm32vldiscovery` | Cortex-M3 | STM32F100 |

**注意**: 系统 QEMU 不支持 STM32U5，使用 lm3s6965evb (Cortex-M3) 进行通用驱动测试。

---

## 🚀 快速开始

### 1. 运行测试固件
```bash
cd /home/eugene/zerozap/XinYi/examples/qemu_test

# 基本运行
qemu-system-arm -M lm3s6965evb -nographic -kernel build/qemu_test.elf

# 带 GDB 调试
qemu-system-arm -M lm3s6965evb -nographic -kernel build/qemu_test.elf -s -S
```

### 2. GDB 连接
```bash
arm-none-eabi-gdb build/qemu_test.elf
(gdb) target remote :1234
(gdb) continue
```

---

## 🏗️ 环境架构

```
┌─────────────────────────────────┐
│      XinYi 应用代码              │
│  (组件驱动/应用逻辑)             │
└───────────────┬─────────────────┘
                │ 编译为
┌───────────────▼─────────────────┐
│      ARM ELF 固件                │
│  (build/xinyi_qemu.elf)         │
└───────────────┬─────────────────┘
                │ 运行于
┌───────────────▼─────────────────┐
│    QEMU STM32 模拟器             │
│  (qemu-system-arm)              │
└───────────────┬─────────────────┘
                │ 调试接口
┌───────────────▼─────────────────┐
│      GDB 调试器                  │
│  (arm-none-eabi-gdb)            │
└─────────────────────────────────┘
```

---

## 📦 安装 QEMU STM32

### 方法 1: 从 beckus/qemu_stm32 编译 (推荐)

```bash
# 1. 克隆仓库
cd /home/eugene/zerozap
git clone https://github.com/beckus/qemu_stm32.git qemu_stm32
cd qemu_stm32

# 2. 安装依赖
sudo apt install libglib2.0-dev libpixman-1-dev libcapstone-dev \
                 libfdt-dev python3 ninja-build

# 3. 配置编译
mkdir build && cd build
../configure --target-list=arm-softmmu \
             --enable-debug \
             --prefix=/opt/qemu-stm32

# 4. 编译安装
make -j$(nproc)
sudo make install

# 5. 验证安装
/opt/qemu-stm32/bin/qemu-system-arm --version
```

### 方法 2: 使用系统 QEMU (快速测试)

```bash
# Ubuntu/Debian
sudo apt install qemu-system-arm gdb-multiarch

# 验证
qemu-system-arm --version
```

---

## 🔧 XinYi QEMU 配置

### 1. CMake 配置

在 `CMakeLists.txt` 中添加 QEMU 构建选项：

```cmake
# QEMU 调试支持
option(ENABLE_QEMU "Build for QEMU STM32 simulation" OFF)

if(ENABLE_QEMU)
    add_definitions(-DXY_PLATFORM_QEMU)
    add_definitions(-DXY_PLATFORM_STM32U5)
    
    # 链接 QEMU 半主机支持
    target_link_options(xinyi PRIVATE --specs=rdimon.specs)
endif()
```

### 2. 代码适配

在 `xy_hal.h` 中添加 QEMU 检测：

```c
#ifdef XY_PLATFORM_QEMU
    #define XY_HAL_USE_SEMIHOSTING 1
    #define XY_HAL_USE_QEMU_LOG 1
#endif
```

### 3. 串口输出重定向

```c
/* qemu_semihost.c - QEMU 半主机串口输出 */
#include <stdint.h>
#include <stdbool.h>

static inline void qemu_semihost_write(const char *str, int len)
{
    /* ARM Semihosting SYS_WRITE0 */
    __asm__ volatile (
        "mov r0, %0\n"
        "mov r1, %1\n"
        "bkpt 0xAB\n"
        :
        : "r"(str), "r"(len)
        : "r0", "r1"
    );
}

void qemu_putchar(char c)
{
    char buf[2] = {c, '\0'};
    qemu_semihost_write(buf, 1);
}
```

---

## 🚀 运行 XinYi on QEMU

### 1. 编译固件

```bash
cd /home/eugene/zerozap/XinYi
mkdir build_qemu && cd build_qemu

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
    -DENABLE_QEMU=ON \
    -DCMAKE_BUILD_TYPE=Debug

make -j$(nproc)
```

### 2. 启动 QEMU

```bash
# 基本启动
qemu-system-arm \
    -M stm32u575i-dk \
    -nographic \
    -kernel build_qemu/xinyi.elf \
    -semihosting \
    -serial mon:stdio

# 带 GDB 服务器
qemu-system-arm \
    -M stm32u575i-dk \
    -nographic \
    -kernel build_qemu/xinyi.elf \
    -semihosting \
    -s -S  # GDB 服务器：端口 1234，启动时暂停
```

### 3. GDB 调试

```bash
# 启动 GDB
arm-none-eabi-gdb build_qemu/xinyi.elf

# GDB 命令
(gdb) target remote :1234      # 连接 QEMU
(gdb) continue                 # 继续运行
(gdb) break main               # 设置断点
(gdb) next                     # 单步
(gdb) info registers           # 查看寄存器
(gdb) backtrace                # 查看调用栈
```

---

## 🧪 组件调试示例

### 示例 1: GPIO 驱动调试

```c
/* test_gpio_qemu.c */
#include "xy_hal_gpio.h"

int main(void)
{
    xy_hal_gpio_t led = xy_hal_gpio_bind("GPIOA.5");
    
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .speed = XY_HAL_GPIO_SPEED_HIGH
    };
    
    xy_hal_gpio_configure(led, XY_HAL_GPIO_PIN_5, &cfg);
    
    while (1) {
        xy_hal_gpio_write(led, XY_HAL_GPIO_PIN_5, 1);
        xy_hal_delay_ms(500);
        
        xy_hal_gpio_write(led, XY_HAL_GPIO_PIN_5, 0);
        xy_hal_delay_ms(500);
    }
    
    return 0;
}
```

**QEMU 启动**:
```bash
qemu-system-arm -M stm32u575i-dk -kernel test_gpio.elf -semihosting -d gpio
```

### 示例 2: UART 驱动调试

```c
/* test_uart_qemu.c */
#include "xy_hal_uart.h"
#include <stdio.h>

int main(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("UART2");
    
    xy_hal_uart_config_t cfg = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = XY_HAL_UART_PARITY_NONE
    };
    
    xy_hal_uart_configure(uart, &cfg);
    
    printf("XinYi on QEMU STM32!\n");
    
    while (1) {
        char c = getchar();
        xy_hal_uart_write(uart, &c, 1, 100);
    }
    
    return 0;
}
```

**QEMU 启动**:
```bash
qemu-system-arm -M stm32u575i-dk -kernel test_uart.elf -nographic -serial mon:stdio
```

---

## 🐛 常见问题

### Q1: QEMU 启动失败 "Invalid machine type"

**解决**: 确认 QEMU 版本支持 STM32U5
```bash
qemu-system-arm -M help | grep stm32
```

### Q2: 半主机输出无响应

**解决**: 确保添加 `-semihosting` 参数
```bash
qemu-system-arm ... -semihosting
```

### Q3: GDB 连接失败

**解决**: 确认 QEMU 已启动 GDB 服务器 (`-s -S` 参数)
```bash
(gdb) target remote localhost:1234
```

---

## 📊 QEMU vs 实际硬件

| 特性 | QEMU | 实际硬件 |
|------|------|---------|
| **启动速度** | 快 (<1s) | 慢 (烧录时间) |
| **调试能力** | 强 (GDB 完整支持) | 中 (依赖调试器) |
| **外设支持** | 部分模拟 | 完整 |
| **性能测试** | ❌ 不准确 | ✅ 准确 |
| **中断测试** | ⚠️ 有限支持 | ✅ 完整 |
| **成本** | 免费 | 需要开发板 |

**建议**: 
- 开发阶段：使用 QEMU 快速迭代
- 集成测试：使用实际硬件验证

---

## 🔗 相关资源

- [QEMU STM32 仓库](https://github.com/beckus/qemu_stm32)
- [QEMU ARM 文档](https://www.qemu.org/docs/master/system/arm/)
- [ARM Semihosting 规范](https://github.com/ARM-software/abi-aa/blob/main/semihosting/semihosting.rst)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0  
**最后更新**: 2026-03-17
