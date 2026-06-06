# STM32F4 QEMU 增强测试工程

**目标**: 增强 STM32F4 QEMU 测试，验证更多外设功能

---

## 📋 测试工程列表

| 工程 | 功能 | 状态 | 固件大小 |
|------|------|------|---------|
| `olimex_stm32_h405_test` | GPIO LED 闪烁 | ✅ 完成 | 868 bytes |
| `uart_test` | USART1 串口通信 | ✅ 完成 | 1.7 KB |
| `timer_test` | TIM2 定时器中断 | ✅ 完成 | 1.4 KB |

---

## 🧪 UART 测试

### 功能
- ✅ USART1 配置 (PA9 TX / PA10 RX)
- ✅ 波特率 115200 8N1
- ✅ 字符收发
- ✅ 命令行交互 (help/hello/test)

### 编译运行
```bash
cd MCU/ST/STM32F4/STM32CubeF4/QEMU/uart_test

# 编译
source /home/eugene/zerozap/scripts/env.sh
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp \
    -mfpu=fpv4-sp-d16 -O0 -g3 \
    -o uart_test.elf src/main.c src/startup.c \
    -nostdlib -T stm32f405rg.ld

# 运行
qemu-system-arm -M olimex-stm32-h405 -nographic \
    -kernel uart_test.elf -semihosting
```

### 输出示例
```
========================================
  XinYi STM32F4 UART Test!
  MCU: STM32F405RG (Cortex-M4F)
  USART1 @ 115200 8N1
========================================

[INIT] Initializing USART1 (PA9/PA10)...
[INIT] UART initialized.

[TEST] UART Echo Test
[TEST] Type something and press Enter:

> > 
```

---

## 🧪 Timer 测试

### 功能
- ✅ TIM2 基本定时器
- ✅ 1kHz 中断频率
- ✅ NVIC 中断配置
- ✅ LED 闪烁 (500ms)
- ✅ 秒计数器输出

### 编译运行
```bash
cd MCU/ST/STM32F4/STM32CubeF4/QEMU/timer_test

# 编译
source /home/eugene/zerozap/scripts/env.sh
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp \
    -mfpu=fpv4-sp-d16 -O0 -g3 \
    -o timer_test.elf src/main.c src/startup.c \
    -nostdlib -T ../olimex_stm32_h405_test/stm32f405rg.ld

# 运行
qemu-system-arm -M olimex-stm32-h405 -nographic \
    -kernel timer_test.elf -semihosting
```

### 输出示例
```
========================================
  XinYi STM32F4 Timer Test!
  MCU: STM32F405RG (Cortex-M4F)
  TIM2 @ 1kHz Interrupt
  LED: PC13 (500ms blink)
========================================

[INIT] Initializing GPIOC (PC13)...
[INIT] GPIO initialized.

[INIT] Initializing TIM2 (1kHz)...
[INIT] Timer initialized.

[LOOP] Running timer interrupt test...
[INFO] LED will blink every 500ms

[TICK] 1 second elapsed (1000 ticks)
[TICK] 1 second elapsed (2000 ticks)
...
```

---

## 🔧 寄存器配置摘要

### USART1
| 寄存器 | 地址 | 说明 |
|--------|------|------|
| `USART1_CR1` | 0x4001100C | 控制寄存器 1 |
| `USART1_BRR` | 0x40011008 | 波特率寄存器 |
| `USART1_DR` | 0x40011004 | 数据寄存器 |
| `USART1_SR` | 0x40011000 | 状态寄存器 |

### TIM2
| 寄存器 | 地址 | 说明 |
|--------|------|------|
| `TIM2_CR1` | 0x40000000 | 控制寄存器 1 |
| `TIM2_DIER` | 0x4000000C | DMA/中断使能寄存器 |
| `TIM2_SR` | 0x40000010 | 状态寄存器 |
| `TIM2_CNT` | 0x40000024 | 计数器寄存器 |
| `TIM2_ARR` | 0x40000028 | 自动重装载寄存器 |
| `TIM2_PSC` | 0x4000002C | 预分频寄存器 |

### NVIC
| 寄存器 | 地址 | 说明 |
|--------|------|------|
| `NVIC_ISER` | 0xE000E100 | 中断使能寄存器 |
| `NVIC_ICER` | 0xE000E180 | 中断清除寄存器 |

---

## 📊 固件对比

| 工程 | 代码 | 数据 | BSS | 总计 |
|------|------|------|-----|------|
| GPIO | 868 B | 0 B | 8 KB | 9 KB |
| UART | 1.7 KB | 0 B | 8 KB | 10 KB |
| Timer | 1.4 KB | 0 B | 8 KB | 10 KB |

---

## 🚀 下一步

- [ ] **SPI 测试** - SPI1/SPI2 通信
- [ ] **I2C 测试** - I2C1/I2C2 通信
- [ ] **ADC 测试** - 模拟数字转换
- [ ] **PWM 测试** - 定时器 PWM 输出
- [ ] **HAL 集成** - 使用 STM32CubeF4 HAL 库

---

**状态**: ✅ 3 个测试工程完成  
**最后更新**: 2026-03-17
