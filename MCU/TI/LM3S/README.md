# TI LM3S6965 QEMU 支持

**目标**: 在 QEMU 上模拟验证 XinYi 核心库

---

## 📋 概述

TI Stellaris LM3S6965 是 QEMU 支持最完善的 Cortex-M3 开发板，适合用于：
- ✅ XinYi 核心库的单元测试
- ✅ HAL 驱动的功能验证
- ✅ 组件集成测试
- ✅ CI/CD 自动化测试

---

## 🏗️ 硬件规格 (QEMU 模拟)

| 特性 | 参数 |
|------|------|
| **内核** | ARM Cortex-M3 @ 50MHz |
| **Flash** | 256 KB |
| **SRAM** | 64 KB |
| **GPIO** | 48 引脚 (GPIOA-GPIOD) |
| **UART** | 4 通道 |
| **SSI (SPI)** | 2 通道 |
| **I2C** | 2 通道 |
| **Timer** | 4 个 16/32 位定时器 |
| **ADC** | 12 位，16 通道 |
| **PWM** | 6 通道 |

---

## 📁 目录结构

```
MCU/TI/LM3S/
├── README.md           # 本文档
├── lm3s6965.h          # 寄存器定义头文件
├── xy_hal_lm3s.c       # XinYi HAL 实现
├── xy_hal_lm3s.h       # HAL 接口头文件
├── linker.ld           # QEMU 链接脚本
└── examples/
    └── hello_qemu.c    # QEMU 半主机示例
```

---

## 🔧 编译配置

### CMake 选项
```cmake
# 启用 QEMU LM3S6965 目标
-DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake
-DXY_PLATFORM=LM3S6965
-DXY_PLATFORM_QEMU=ON
```

### 编译命令
```bash
cd /home/eugene/zerozap/XinYi
mkdir build_qemu && cd build_qemu

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
    -DXY_PLATFORM=LM3S6965 \
    -DXY_PLATFORM_QEMU=ON \
    -DCMAKE_BUILD_TYPE=Debug

make -j$(nproc)
```

---

## 🚀 运行测试

### 基本运行
```bash
qemu-system-arm \
    -M lm3s6965evb \
    -nographic \
    -kernel build_qemu/xinyi_lm3s.elf
```

### GDB 调试
```bash
# 终端 1: 启动 QEMU GDB 服务器
qemu-system-arm \
    -M lm3s6965evb \
    -nographic \
    -kernel build_qemu/xinyi_lm3s.elf \
    -s -S

# 终端 2: GDB 连接
arm-none-eabi-gdb build_qemu/xinyi_lm3s.elf
(gdb) target remote :1234
(gdb) continue
(gdb) break main
(gdb) next
```

### 半主机输出
```bash
qemu-system-arm \
    -M lm3s6965evb \
    -semihosting \
    -kernel build_qemu/xinyi_lm3s.elf
```

---

## 📝 QEMU 外设支持

| 外设 | QEMU 支持 | XinYi HAL | 状态 |
|------|----------|----------|------|
| GPIO | ✅ | ✅ | 可用 |
| UART | ✅ | ✅ | 可用 |
| SSI (SPI) | ✅ | ⏳ | 开发中 |
| I2C | ✅ | ⏳ | 开发中 |
| Timer | ✅ | ⏳ | 开发中 |
| ADC | ✅ | ❌ | 待实现 |
| PWM | ✅ | ❌ | 待实现 |

---

## 🧪 测试示例

### GPIO 测试
```c
#include "xy_hal_gpio.h"

int main(void)
{
    xy_hal_gpio_t led = xy_hal_gpio_bind("GPIOF.1");
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .speed = XY_HAL_GPIO_SPEED_HIGH
    };
    xy_hal_gpio_configure(led, XY_HAL_GPIO_PIN_1, &cfg);
    
    while (1) {
        xy_hal_gpio_toggle(led, XY_HAL_GPIO_PIN_1);
        xy_hal_delay_ms(500);
    }
}
```

### UART 测试 (半主机)
```c
#include <stdio.h>

int main(void)
{
    printf("XinYi on QEMU LM3S6965!\n");
    return 0;
}
```

---

## 📚 参考资料

- [QEMU lm3s6965evb 文档](https://www.qemu.org/docs/master/system/arm/stellaris.html)
- [TI LM3S6965 数据手册](https://www.ti.com/product/LM3S6965)
- [Stellaris 外设驱动库](https://www.ti.com/tool/SW-LM3S)

---

**状态**: 🟡 规划中  
**优先级**: P0 - 核心验证平台
