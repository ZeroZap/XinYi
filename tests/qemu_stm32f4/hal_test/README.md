# XinYi HAL 统一 API 测试

**平台**: STM32F405 on QEMU  
**测试范围**: GPIO/UART 统一 API  
**状态**: ✅ 11 个测试全部通过

---

## 📋 测试说明

### 测试的 HAL API

#### GPIO 统一 API
```c
xy_hal_gpio_t xy_hal_gpio_bind(const char *name);
int xy_hal_gpio_configure(xy_hal_gpio_t dev, uint8_t pin, const xy_hal_gpio_config_t *config);
int xy_hal_gpio_write(xy_hal_gpio_t dev, uint8_t pin, uint8_t value);
int xy_hal_gpio_read(xy_hal_gpio_t dev, uint8_t pin);
int xy_hal_gpio_toggle(xy_hal_gpio_t dev, uint8_t pin);
```

#### UART 统一 API
```c
xy_hal_uart_t xy_hal_uart_bind(const char *name);
int xy_hal_uart_configure(xy_hal_uart_t dev, const xy_hal_uart_config_t *config);
int xy_hal_uart_write(xy_hal_uart_t dev, const uint8_t *data, uint32_t length);
```

---

## 🚀 快速开始

```bash
cd tests/qemu_stm32f4/hal_test
./run_test.sh
```

---

## 📊 测试结果

```
[TEST] HAL GPIO Bind...
  ✓ PASS: Bind GPIOA.5
  ✓ PASS: Bind GPIOC.13

[TEST] HAL GPIO Configure...
  ✓ PASS: Configure GPIOA.5 as output

[TEST] HAL GPIO Write/Read...
  ✓ PASS: Write GPIOA.5 high
  ✓ PASS: Write GPIOA.5 low

[TEST] HAL GPIO Toggle...
  ✓ PASS: Toggle GPIOC.13

[TEST] HAL GPIO LED Blink (PC13)...
  ✓ PASS: LED blink 3 times

[TEST] HAL UART Bind...
  ✓ PASS: Bind UART1

[TEST] HAL UART Configure...
  ✓ PASS: Configure UART1 115200 8N1

[TEST] HAL UART Write...
HAL UART Test Message  ✓ PASS: UART write test message

[TEST] HAL Combined Workflow...
HAL Workflow Test
  ✓ PASS: Combined workflow completed

╔══════════════════════════════════════════╗
║  Test Summary                            ║
╚══════════════════════════════════════════╝
  Total: 11
  PASS:  11
  FAIL:  0

>>> ALL TESTS PASSED <<<
```

---

## 📁 文件结构

```
hal_test/
├── src/
│   ├── main.c              # HAL 测试代码 (13KB)
│   └── startup.c           # 启动代码
├── stm32f405rg.ld          # 链接脚本
├── hal_test.elf            # 固件 (4.2KB)
└── run_test.sh             # 自动化脚本
```

---

## 🎯 测试覆盖

| 模块 | 测试项 | 状态 |
|------|--------|------|
| **GPIO Bind** | GPIOA.5/GPIOC.13 | ✅ |
| **GPIO Configure** | 输出模式配置 | ✅ |
| **GPIO Write** | 高/低电平 | ✅ |
| **GPIO Toggle** | 翻转操作 | ✅ |
| **GPIO LED** | LED 闪烁 | ✅ |
| **UART Bind** | UART1 | ✅ |
| **UART Configure** | 115200 8N1 | ✅ |
| **UART Write** | 数据发送 | ✅ |
| **综合工作流** | GPIO+UART | ✅ |

---

## 🔧 HAL 统一架构

```
应用层
    │
    ▼
┌─────────────────┐
│  xy_hal_gpio_t  │ ← 统一句柄类型
│  xy_hal_uart_t  │
└────────┬────────┘
         │
    ┌────┴────┐
    ▼         ▼
┌───────┐ ┌───────┐
│STM32U5│ │ WCH   │ ← 平台实现
│  HAL  │ │  HAL  │
└───────┘ └───────┘
```

---

## 📝 添加新 HAL 测试

### 1. 实现测试函数

```c
static void test_hal_new_feature(void)
{
    TEST_START("HAL New Feature");
    
    /* 测试代码 */
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.1");
    TEST_ASSERT(gpio != 0, "Bind GPIOA.1");
}
```

### 2. 注册测试

```c
int main(void)
{
    test_hal_gpio_bind();
    test_hal_new_feature(); /* 添加新测试 */
    
    /* 输出总结... */
}
```

---

## 📈 平台支持

| 平台 | GPIO | UART | SPI | I2C |
|------|------|------|-----|-----|
| **STM32U5** | ✅ | ✅ | ✅ | ✅ |
| **WCH CH32U5** | ✅ | ✅ | ✅ | ✅ |
| **HC32 L021** | ✅ | ✅ | ✅ | ✅ |
| **QEMU STM32F4** | ✅ | ✅ | ⏳ | ⏳ |

---

**最后更新**: 2026-03-17  
**测试结果**: ✅ 11/11 通过
