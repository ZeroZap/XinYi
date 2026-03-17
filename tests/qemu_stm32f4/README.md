# STM32F4 QEMU 测试工程

**目标**: 在 QEMU 上验证 STM32F4 系列 MCU

---

## 📋 支持的开发板

| QEMU 开发板 | MCU | Flash | RAM | 状态 |
|-----------|-----|-------|-----|------|
| `olimex-stm32-h405` | STM32F405RG | 1024KB | 192KB | ✅ 已验证 |

---

## 📁 测试工程

### olimex_stm32_h405_test

**功能**: GPIO LED 闪烁测试

**文件结构**:
```
olimex_stm32_h405_test/
├── src/
│   ├── main.c                 # 主程序 (GPIO + 半主机输出)
│   └── startup.c              # 启动代码 + 向量表
├── stm32f405rg.ld             # 链接脚本
└── olimex_stm32_h405.elf      # 编译产物
```

**编译命令**:
```bash
cd /home/eugene/zerozap/XinYi/MCU/ST/STM32F4/QEMU/olimex_stm32_h405_test

source /home/eugene/zerozap/scripts/env.sh
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
    -O0 -g3 -o olimex_stm32_h405.elf src/main.c src/startup.c \
    -nostdlib -T stm32f405rg.ld
```

**运行命令**:
```bash
# 基本运行
qemu-system-arm -M olimex-stm32-h405 -nographic \
    -kernel olimex_stm32_h405.elf -semihosting

# GDB 调试
qemu-system-arm -M olimex-stm32-h405 -nographic \
    -kernel olimex_stm32_h405.elf -semihosting -s -S

# 另一终端连接 GDB
arm-none-eabi-gdb olimex_stm32_h405.elf
(gdb) target remote :1234
(gdb) continue
```

**输出示例**:
```
========================================
  XinYi on QEMU OLIMEX-STM32-H405!
  MCU: STM32F405RG (Cortex-M4F)
  Test: GPIO LED Blink
========================================

[INIT] Configuring GPIOC (PC13)...
[INIT] GPIO initialized.

[LOOP] Starting LED blink (PC13)...

  [LED] GREEN ON (PC13)
  [LED] GREEN OFF
  [LED] GREEN ON (PC13)
  [LED] GREEN OFF
  ...
```

---

## 🔧 寄存器定义

### RCC (复位和时钟控制)
- 基地址：`0x40023800`
- `RCC_AHB1ENR` - AHB1 外设时钟使能寄存器
- `RCC_AHB1ENR_GPIOCEN` - GPIOC 时钟使能位 (bit 2)

### GPIOC
- 基地址：`0x40020800`
- `GPIOC_MODER` - 端口模式寄存器
- `GPIOC_OTYPER` - 输出类型寄存器
- `GPIOC_OSPEEDR` - 输出速度寄存器
- `GPIOC_PUPDR` - 上拉/下拉寄存器
- `GPIOC_ODR` - 输出数据寄存器
- `GPIOC_BSRR` - 置位/复位寄存器

### LED 引脚
- **绿色 LED**: PC13

---

## 📊 固件信息

| 项目 | 数值 |
|------|------|
| 代码大小 | ~868 bytes |
| 栈大小 | 8KB |
| 编译器 | ARM GCC 9.3.1 |
| 优化等级 | -O0 (Debug) |
| FPU | Cortex-M4F (softfp) |

---

## 🔍 STM32F4 vs STM32F1 差异

| 特性 | STM32F1 | STM32F4 |
|------|---------|---------|
| 内核 | Cortex-M3 | Cortex-M4F |
| FPU | ❌ | ✅ |
| GPIO 时钟 | APB2ENR | AHB1ENR |
| GPIO 配置 | CRL/CRH | MODER/OTYPER/OSPEEDR/PUPDR |
| 编译选项 | `-mcpu=cortex-m3` | `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16` |

---

## 🚀 下一步

1. **UART 测试** - 使用 USART1/USART2/USART3
2. **Timer 测试** - 基本/高级定时器
3. **SPI/I2C 测试** - 通信接口
4. **HAL 集成** - 使用 STM32CubeF4 HAL 库
5. **FPU 测试** - 浮点运算验证

---

**状态**: ✅ 运行成功  
**最后更新**: 2026-03-17
