# HAL 统一 API 文档

**版本**: 2.1
**日期**: 2026-11-02
**状态**: Host (PC) 契约由 CTest 覆盖；逐平台状态见
[平台实现与证据矩阵](../validation/hal-platform-evidence-matrix.md)

---

## 📋 概述

XinYi HAL (Hardware Abstraction Layer) 用一套 `xy_hal_*` 接口屏蔽 STM32 / WCH / HC32 与 PC 仿真
之间的差异。应用与驱动只包含 `components/hal/inc/` 的公共头，平台差异全部落在
`components/hal/<platform>/` 的后端里。

**本文的代码块与 `components/hal/examples/hal_quickstart.c` 是同一份事实源**，该样例由 CTest
`hal_quickstart` 在 Host 上编译运行（37 条断言）。文档里写的调用如果不再存在，测试会红。

---

## 🏗️ 架构设计

```text
┌─────────────────────────────────┐
│    应用层 / 组件层                │
└───────────────┬─────────────────┘
                │ 调用 xy_hal_*
┌───────────────▼─────────────────┐
│   HAL 公共接口                    │
│   components/hal/inc/xy_hal_*.h │
└───────────────┬─────────────────┘
                │ 平台后端实现
┌───────────────▼─────────────────┐
│  components/hal/<platform>/     │
│  PC/  stm32/{stm32f4,l4,u5}/    │
│  wch/ch32x/src/  hc32/          │
└───────────────┬─────────────────┘
                │ 厂商调用
┌───────────────▼─────────────────┐
│   MCU SDK (STM32Cube / WCH / HC32)│
└─────────────────────────────────┘
```

`components/hal/stm32/stm32_hal.h` 按族宏引入对应厂商头；`components/hal/PC/` 是 Host 契约后端，
只建模契约（参数校验、状态、返回值），不建模硬件时序。

---

## 🎯 句柄模型

HAL **没有字符串绑定**，也没有 `xy_hal_gpio_t` 这样的统一句柄类型。每个入口的第一个参数就是平台
外设本身：

| 外设 | 参数类型 | 板上传什么 | Host 上传什么 |
|------|----------|------------|---------------|
| GPIO | `xy_hal_gpio_port_t` | `GPIOA` / `GPIOC` | 带 `port_id` 的 `struct xy_hal_gpio_port` 对象 |
| UART | `void *` | `&huart2` | 任一独立句柄对象（后端按指针身份区分实例） |
| I2C | `void *` | `&hi2c1` | 同上 |
| SPI | `void *` | `&hspi1` | 同上 |

Host 后端按句柄身份保存实例状态，所以同一端口必须始终用同一个句柄对象；不要用整数强转出来的假指针。

---

## 🔧 支持的模块

`components/hal/inc/*.h` 中出现的 `xy_hal_` 入口名共 **447 个**：GPIO 33、UART 14、I2C 12、SPI 13、
SYS 89，其余分布在 ADC/PWM/CAN/DAC/RTC/DMA/I2S/EXTI/FLASH/RNG/TIMER/WDG/QSPI/LP_TIMER。

**声明不是实现。** 某个平台实现到哪一步（`SOURCE_PRESENT` / `PARTIAL_SOURCE` / `UNSUPPORTED` /
`HOST_CONTRACT` / `COMPILE_ONLY` / `QEMU_PARTIAL` / `BOARD_PENDING`）只以
[平台实现与证据矩阵](../validation/hal-platform-evidence-matrix.md) 为准；本文不复述百分比，
因为一个"100%"在没有 compile/runtime 记录时只是排版。

---

## 📖 API 使用示例

### GPIO

```c
#include "xy_hal.h"
#include "xy_hal_gpio.h"

struct xy_hal_gpio_port port_a = { .port_id = 0 };   /* Host 变体；板上直接传 GPIOA */
xy_hal_gpio_port_t port = &port_a;

xy_hal_gpio_config_t config = {
    .mode = XY_HAL_GPIO_MODE_OUTPUT,
    .pull = XY_HAL_GPIO_PULL_NONE,
    .otype = XY_HAL_GPIO_OTYPE_PP,
    .speed = XY_HAL_GPIO_SPEED_LOW,
    .alternate = 0,
};

xy_hal_gpio_init(port, 5, &config);
xy_hal_gpio_write(port, 5, 1);      /* 点亮 */
xy_hal_gpio_write(port, 5, 0);      /* 熄灭 */
xy_hal_gpio_toggle(port, 5);        /* 翻转 */
xy_hal_gpio_deinit(port, 5);
```

`pin` 合法范围 0..15；越界与 `NULL` 配置都返回 `XY_HAL_ERROR_INVALID_PARAM`，而不是被截断。
批量形式是 `xy_hal_gpio_write_batch(port, pin_mask, value_mask)` 与
`xy_hal_gpio_read_batch(port, pin_mask)`，空掩码被拒绝。

### UART

```c
#include "xy_hal_uart.h"

void *uart = platform_uart_handle();       /* 板上是 &huart2 */

xy_hal_uart_config_t config = {
    .baudrate = 115200,
    .wordlen = XY_HAL_UART_WORDLEN_8B,
    .stopbits = XY_HAL_UART_STOPBITS_1,
    .parity = XY_HAL_UART_PARITY_NONE,
    .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
    .mode = XY_HAL_UART_MODE_TX_RX,
};

xy_hal_uart_init(uart, &config);
xy_hal_uart_send(uart, (const uint8_t *)"Hello, XinYi!\n", 14, 100);
xy_hal_uart_recv(uart, data, sizeof(data), 100);
```

`timeout` 是整笔传输的毫秒预算。`baudrate == 0` 或枚举越界一律 `XY_HAL_ERROR_INVALID_PARAM`；
未初始化的句柄返回 `XY_HAL_ERROR_NOT_INIT`；重复 init 返回 `XY_HAL_ERROR_ALREADY_INIT`。

### I2C

```c
#include "xy_hal_i2c.h"

void *i2c = platform_i2c_handle();         /* 板上是 &hi2c1 */

xy_hal_i2c_config_t config = {
    .clock_speed = 400000,                 /* 400 kHz */
    .addr_mode = XY_HAL_I2C_ADDR_7BIT,
    .duty_cycle = XY_HAL_I2C_DUTY_2,
    .own_address = 0,
    .general_call_mode = 0,
};

uint8_t value = 0x10;
uint8_t data[6];

xy_hal_i2c_init(i2c, &config);
xy_hal_i2c_mem_write(i2c, 0x68, 0x0001, &value, 1, 100);   /* 寄存器 0x01 <- 0x10 */
xy_hal_i2c_mem_read(i2c, 0x68, 0x0001, data, sizeof(data), 100);
```

`dev_addr` 是 7 位未左移地址；`>= 0x80` 被拒绝。**10 位寻址没有任何后端实现**：
`xy_hal_i2c_init()` 对 `XY_HAL_I2C_ADDR_10BIT` 返回 `XY_HAL_ERROR_NOT_SUPPORTED`。

### SPI

```c
#include "xy_hal_spi.h"

void *spi = platform_spi_handle();         /* 板上是 &hspi1 */

xy_hal_spi_config_t config = {
    .mode = XY_HAL_SPI_MODE_0,
    .direction = XY_HAL_SPI_DIR_2LINES,
    .datasize = XY_HAL_SPI_DATASIZE_8BIT,
    .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
    .nss = XY_HAL_SPI_NSS_SOFT,
    .baudrate_prescaler = 8,
    .is_master = 1,
};

uint8_t tx_data[2] = { 0x9F, 0x00 };
uint8_t rx_data[2];

xy_hal_spi_init(spi, &config);
xy_hal_spi_transmit_receive(spi, tx_data, rx_data, sizeof(tx_data), 100);
```

`len` 一律按**字节**计；16 位数据宽度下奇数字节数被拒绝（硬件按帧计，半个帧不是一笔传输）。

### SYS

```c
#include "xy_hal_sys.h"

xy_hal_sys_clock_info_t info;
xy_hal_sys_get_clock_info(&info);
uint32_t now = xy_hal_sys_get_tick_count();
uint32_t reason = xy_hal_sys_get_reset_reason();
```

复位原因、芯片 ID、电源模式、tick 频率都在 `xy_hal_sys.h`；具体平台支持范围见证据矩阵。

---

## 🧯 错误处理

所有入口返回 `xy_hal_error_t`（`components/hal/inc/xy_hal_error.h`），数值与 `xy_error.h` 对齐：

| 值 | 名称 |
|----|------|
| 0 | `XY_HAL_OK` |
| -1 | `XY_HAL_ERROR` |
| -2 | `XY_HAL_ERROR_INVALID_PARAM` |
| -3 | `XY_HAL_ERROR_TIMEOUT` |
| -4 | `XY_HAL_ERROR_BUSY` |
| -5 | `XY_HAL_ERROR_NOT_SUPPORTED` |
| -6 | `XY_HAL_ERROR_NOT_FOUND` |
| -7 | `XY_HAL_ERROR_NO_MEMORY` |
| -8 | `XY_HAL_ERROR_NO_RESOURCE` |
| -9 | `XY_HAL_ERROR_IO` |
| -10 | `XY_HAL_ERROR_FAIL` |
| -11 | `XY_HAL_ERROR_CRC` |
| -12 | `XY_HAL_ERROR_OVERFLOW` |
| -13 | `XY_HAL_ERROR_NOT_INIT` |
| -14 | `XY_HAL_ERROR_ALREADY_INIT` |

```c
xy_hal_error_t ret = xy_hal_i2c_mem_write(i2c, 0x68, 0x01, &value, 1, 100);
if (ret != XY_HAL_OK) {
    /* 按上面的枚举分支处理，不要只看 ret < 0 就当作“总线坏了” */
}
```

---

## 🧪 测试验证

### Host（`make test-unit` 的一部分）

| CTest | 内容 |
|-------|------|
| `hal_pc` | PC 后端 GPIO/UART/I2C/SPI/SYS/RTC/DMA/Timer/IRQ 契约 |
| `hal_quickstart` | 本文与 `components/hal/README.md` 的样例，37 条断言 |
| `hal_platform_build_contract` | 平台分支必须定义 `STM32_HAL_ENABLED`，否则 wrapper 编成空对象而 gate 仍绿 |
| `hal_pc_coverage` / `hal_dma_contract` / `hal_timer_irq_contract` / `hal_stub_returns` | 覆盖清单与契约检查（各带 `_unit` 自测） |

### QEMU（`make test-qemu`）

`tests/qemu_stm32f4/` 下有 `hal_test` 等目标，属于**部分**证据（覆盖若干外设场景），不能替代目标
平台 clean compile 或实板运行。运行方式：

```bash
cd tests/qemu_stm32f4 && make list && make hal_test
```

---

## 📦 编译配置

### 平台选择（根 CMake）

```bash
make                                   # 默认 PC
make HAL_PLATFORM=STM32L4
make HAL_PLATFORM=STM32U5
make HAL_PLATFORM=STM32F4
make HAL_PLATFORM=WCH
make HAL_PLATFORM=HC32
```

直接 CMake：

```bash
cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pc -j"$(nproc)"
```

`HAL_PLATFORM` 是 CMake 缓存变量，不是 Kconfig 符号；`components/hal/CMakeLists.txt` 按它的取值
选源文件与编译定义（STM32F4 与 STM32L4 两个分支都必须带 `STM32_HAL_ENABLED`，否则复用/专属的
wrapper 会编成空对象）。

### Kconfig

```text
menu "HAL Configuration"
config HAL_PC
config HAL_STM32
config HAL_WCH
config HAL_HC32
config HAL_USE_STANDARD_API
endmenu
```

平台选择项会 `select` 对应的 `HAL_*`；生成名带 `CONFIG_` 前缀（`CONFIG_HAL_PC` 等）。

---

## 📁 目录结构

```text
components/hal/
├── inc/                  # 公共 API（唯一事实源）
│   ├── xy_hal.h
│   ├── xy_hal_gpio.h
│   ├── xy_hal_uart.h
│   ├── xy_hal_i2c.h
│   ├── xy_hal_spi.h
│   ├── xy_hal_sys.h
│   └── ...
├── PC/                   # PC simulation 后端（Host 契约）
├── stm32/stm32f4/        # STM32F4 wrapper
├── stm32/stm32l4/        # STM32L4 wrapper（Pandora 参考板）
├── stm32/stm32u5/        # STM32U5 wrapper
├── wch/ch32x/src/        # CH32V30x wrapper
├── hc32/hc32l021/        # HC32L021 wrapper
├── hc32/hc32l19x/        # HC32L19x wrapper
└── examples/             # hal_quickstart.c（CTest `hal_quickstart`）
```

---

## 🔗 相关文档

- [HAL 组件说明](../../components/hal/README.md)
- [平台实现与证据矩阵](../validation/hal-platform-evidence-matrix.md)
- [HAL 移植指南](../hardware/porting.md)
- [API 参考](../API_REFERENCE.md)

---

**最后更新**: 2026-11-02
**维护者**: XinYi Team
