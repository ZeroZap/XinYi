# XinYi HAL 统一层

**版本**: 2.1
**日期**: 2026-11-02
**维护者**: XinYi Team
**状态**: Host (PC) 契约由 CTest 覆盖；逐平台实现/编译/QEMU/实板状态以证据矩阵为准，不在此处复述
**证据边界**: [`docs/validation/hal-platform-evidence-matrix.md`](../../docs/validation/hal-platform-evidence-matrix.md)

---

## 📖 概述

XinYi HAL 是应用/驱动与厂商 SDK 之间的唯一接口：上层调 `xy_hal_*`，平台后端把调用翻译成
STM32Cube / WCH / HC32 的调用。分层关系是

```text
application / driver -> xy_hal_*  -> components/hal/<platform>/  -> MCU vendor SDK
```

**本文所有代码块都来自 `components/hal/examples/hal_quickstart.c`**，该文件由 CTest
`hal_quickstart` 在 Host 上编译并运行（37 条断言）。文档与样例共用同一份事实源：某个入口改名
或消失，测试会红，而不是让 markdown 悄悄过期。

---

## 🎯 句柄模型：没有 `bind("GPIOA.5")`

HAL **不用字符串绑定外设**。每个入口的第一个参数就是平台外设本身：

| 外设 | 句柄类型 | 板上的值 | Host 上的值 |
|------|----------|----------|-------------|
| GPIO | `xy_hal_gpio_port_t` | `GPIOA` / `GPIOC`（厂商端口指针） | 一个 `struct xy_hal_gpio_port` 对象，带 `port_id` |
| UART | `void *` | `&huart2`（CubeMX 句柄） | PC 后端按句柄指针区分实例，任一独立对象即可 |
| I2C | `void *` | `&hi2c1` | 同上 |
| SPI | `void *` | `&hspi1` | 同上 |

主机侧“句柄是什么类型的对象”不是可以随手换的细节：`components/hal/PC/` 的后端按**句柄身份**
保存每个实例的状态，所以同一个端口必须始终用同一个句柄调用，并且在 Host 上不要用整数强转出来的
假指针。

---

## 🔧 支持的入口（以头文件为准）

`components/hal/inc/*.h` 里出现 `xy_hal_` 前缀的入口名共 **447 个**（GPIO 33、UART 14、I2C 12、
SPI 13、SYS 89，其余为 ADC/PWM/CAN/RTC/DMA/I2S/EXTI/FLASH/RNG/TIMER/WDG 等）。**声明不等于实现**：
哪个平台真的实现了哪个入口，由后端源码与证据矩阵说话，不由头文件说话。

四个总线最常用的入口：

```c
/* GPIO — components/hal/inc/xy_hal_gpio.h */
xy_hal_gpio_init(port, pin, &config);        xy_hal_gpio_deinit(port, pin);
xy_hal_gpio_write(port, pin, value);         xy_hal_gpio_read(port, pin);
xy_hal_gpio_toggle(port, pin);               xy_hal_gpio_write_batch(port, pin_mask, value_mask);
xy_hal_gpio_read_batch(port, pin_mask);      xy_hal_gpio_attach_irq(port, pin, mode, handler, arg);

/* UART — components/hal/inc/xy_hal_uart.h */
xy_hal_uart_init(handle, &config);           xy_hal_uart_deinit(handle);
xy_hal_uart_send(handle, data, len, timeout_ms);
xy_hal_uart_recv(handle, data, len, timeout_ms);
xy_hal_uart_send_dma(handle, data, len);     xy_hal_uart_recv_dma(handle, data, len);
xy_hal_uart_register_callback(handle, callback, arg);

/* I2C — components/hal/inc/xy_hal_i2c.h */
xy_hal_i2c_init(handle, &config);            xy_hal_i2c_deinit(handle);
xy_hal_i2c_mem_write(handle, dev_addr, mem_addr, data, len, timeout_ms);
xy_hal_i2c_mem_read(handle, dev_addr, mem_addr, data, len, timeout_ms);
xy_hal_i2c_master_transmit(handle, dev_addr, data, len, timeout_ms);
xy_hal_i2c_master_receive(handle, dev_addr, data, len, timeout_ms);

/* SPI — components/hal/inc/xy_hal_spi.h */
xy_hal_spi_init(handle, &config);            xy_hal_spi_deinit(handle);
xy_hal_spi_transmit(handle, data, len, timeout_ms);
xy_hal_spi_receive(handle, data, len, timeout_ms);
xy_hal_spi_transmit_receive(handle, tx_data, rx_data, len, timeout_ms);
```

I2C 的 `dev_addr` 是 **7 位、未左移**的地址（`0x68`，不是 `0xD0`）；越界地址在传输边界被拒绝。
10 位寻址在枚举里存在、但**没有任何后端实现**，`xy_hal_i2c_init()` 直接返回
`XY_HAL_ERROR_NOT_SUPPORTED`。

---

## 🚀 快速开始

### 1. GPIO：点亮一个 LED

```c
#include "xy_hal.h"
#include "xy_hal_gpio.h"

/* 板上用厂商端口；Host 上用一个句柄对象（本样例即 PC 变体） */
struct xy_hal_gpio_port port_a = { .port_id = 0 };
xy_hal_gpio_port_t port = &port_a;

xy_hal_gpio_config_t output_cfg = {
    .mode = XY_HAL_GPIO_MODE_OUTPUT,
    .pull = XY_HAL_GPIO_PULL_NONE,
    .otype = XY_HAL_GPIO_OTYPE_PP,
    .speed = XY_HAL_GPIO_SPEED_LOW,
    .alternate = 0,
};

if (xy_hal_gpio_init(port, 5, &output_cfg) != XY_HAL_OK) {
    /* 每个入口都返回 xy_hal_error_t，没有“无返回值”的配置调用 */
}
xy_hal_gpio_write(port, 5, 1);   /* 点亮 */
xy_hal_gpio_toggle(port, 5);     /* 翻转 */
xy_hal_gpio_deinit(port, 5);
```

### 2. UART：阻塞收发

```c
#include "xy_hal_uart.h"

/* 板上是 &huart2 */
void *uart = platform_uart_handle();

xy_hal_uart_config_t cfg = {
    .baudrate = 115200,
    .wordlen = XY_HAL_UART_WORDLEN_8B,
    .stopbits = XY_HAL_UART_STOPBITS_1,
    .parity = XY_HAL_UART_PARITY_NONE,
    .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
    .mode = XY_HAL_UART_MODE_TX_RX,
};

xy_hal_uart_init(uart, &cfg);           /* baudrate == 0 会被拒绝，不会悄悄用默认值 */
xy_hal_uart_send(uart, (const uint8_t *)"Hello UART!", 11, 100);
xy_hal_uart_recv(uart, buf, sizeof(buf), 100);
```

### 3. I2C：读写一个传感器寄存器

```c
#include "xy_hal_i2c.h"

/* 板上是 &hi2c1 */
void *i2c = platform_i2c_handle();

xy_hal_i2c_config_t cfg = {
    .clock_speed = 400000,             /* 400 kHz */
    .addr_mode = XY_HAL_I2C_ADDR_7BIT,
    .duty_cycle = XY_HAL_I2C_DUTY_2,
    .own_address = 0,
    .general_call_mode = 0,
};

uint8_t value = 0x55;
uint8_t readback = 0;

xy_hal_i2c_init(i2c, &cfg);
xy_hal_i2c_mem_write(i2c, 0x68, 0x0000, &value, 1, 100);   /* 寄存器地址 0x00 <- 0x55 */
xy_hal_i2c_mem_read(i2c, 0x68, 0x0000, &readback, 1, 100);
```

完整样例（含 SPI 全双工、SYS tick/时钟查询、以及上面每条拒绝路径）见
`components/hal/examples/hal_quickstart.c`。

---

## 🧯 错误码

所有入口返回 `xy_hal_error_t`（`components/hal/inc/xy_hal_error.h`），数值与 `xy_error.h` 对齐，
可直接跨层比较：

```c
xy_hal_error_t ret = xy_hal_gpio_write(port, 5, 1);
if (ret != XY_HAL_OK) {
    /* XY_HAL_ERROR_INVALID_PARAM / XY_HAL_ERROR_NOT_INIT / XY_HAL_ERROR_TIMEOUT / ... */
}
```

`XY_HAL_OK` 为 0，其余为负值：`XY_HAL_ERROR`、`XY_HAL_ERROR_INVALID_PARAM`、
`XY_HAL_ERROR_TIMEOUT`、`XY_HAL_ERROR_BUSY`、`XY_HAL_ERROR_NOT_SUPPORTED`、
`XY_HAL_ERROR_NOT_FOUND`、`XY_HAL_ERROR_NO_MEMORY`、`XY_HAL_ERROR_NO_RESOURCE`、
`XY_HAL_ERROR_IO`、`XY_HAL_ERROR_FAIL`、`XY_HAL_ERROR_CRC`、`XY_HAL_ERROR_OVERFLOW`、
`XY_HAL_ERROR_NOT_INIT`、`XY_HAL_ERROR_ALREADY_INIT`。

---

## 🖥️ 平台支持与证据

逐平台、逐外设的 implementation / unsupported / Host / compile / QEMU / Board 状态，以
[HAL 平台实现与证据矩阵](../../docs/validation/hal-platform-evidence-matrix.md)为事实源。

关键边界：

- STM32U5 的基础 wrapper 存在，但没有可升级为板级结论的证据；I2S 明确不支持。
- STM32F4 的 Timer 仍是显式 unsupported；STM32L4 多数外设复用 F4 wrapper，GPIO/I2C 为部分实现。
- WCH CH32V30x 的部分外设有实板 smoke，但未覆盖完整外设矩阵；HC32L021 目前主要为 GPIO。
- PC simulation 和 QEMU 结果不能替代目标平台 clean compile 或实板运行。

---

## 📊 测试

Host 侧 HAL 相关 CTest（`make test-unit` 的一部分）：

| CTest | 覆盖内容 |
|-------|----------|
| `hal_pc` | PC 后端的 GPIO/UART/I2C/SPI/SYS/RTC/DMA/Timer/IRQ 契约 |
| `hal_quickstart` | 本文与 `docs/hal/HAL_UNIFIED_API.md` 教的样例，37 条断言 |
| `hal_pc_coverage` / `hal_pc_coverage_unit` | PC 后端入口覆盖清单 + 检查器自测 |
| `hal_platform_build_contract` | 平台分支必须定义 `STM32_HAL_ENABLED`（否则 wrapper 编成空对象） |
| `hal_dma_contract` / `hal_timer_irq_contract` / `hal_stub_returns` | DMA、Timer IRQ、stub 返回值契约（各带 `_unit` 自测） |

只用 Host 跑一条：

```bash
cmake --build build/tests/unit --target test_hal_quickstart -j"$(nproc)"
ctest --test-dir build/tests/unit -R '^hal_quickstart$' --output-on-failure
```

旧的 `tests/hal/*` 独立 GPIO smoke 入口已合并到 active `tests/unit/hal_component/test_hal_pc.c`。

---

## 🔧 编译配置

平台由根 CMake 的 `HAL_PLATFORM` 缓存变量选择（取值与 `components/hal/CMakeLists.txt` 一致）：

```bash
make                                   # 默认 PC
make HAL_PLATFORM=STM32L4              # Pandora 参考板
make HAL_PLATFORM=STM32U5
make HAL_PLATFORM=STM32F4              # 可用 BUILD_TESTS=ON
make HAL_PLATFORM=WCH
make HAL_PLATFORM=HC32
```

直接 CMake 等价写法：

```bash
cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pc -j"$(nproc)"
```

Kconfig 侧的 HAL 符号是 `HAL_PC`、`HAL_STM32`、`HAL_WCH`、`HAL_HC32` 与
`HAL_USE_STANDARD_API`（由平台选择项 `select`，生成名带 `CONFIG_` 前缀）。

---

## 📁 目录结构

```text
components/hal/
├── inc/                  # 公共头文件（唯一 API 事实源）
├── PC/                   # PC simulation 后端（Host 契约）
├── stm32/
│   ├── stm32f4/          # STM32F4 wrapper
│   ├── stm32l4/          # STM32L4 wrapper（Pandora）
│   ├── stm32u5/          # STM32U5 wrapper
│   └── stm32_hal.h       # 族宏 -> vendor 头的公共入口
├── wch/ch32x/src/        # CH32V30x wrapper
├── hc32/                 # hc32l021 / hc32l19x wrapper
├── examples/             # hal_quickstart.c（CTest `hal_quickstart` 编译并运行）
└── CMakeLists.txt        # 按 HAL_PLATFORM 选源与定义
```

---

## 🔗 相关文档

- [HAL 统一 API 文档](../../docs/hal/HAL_UNIFIED_API.md)
- [HAL 平台实现与证据矩阵](../../docs/validation/hal-platform-evidence-matrix.md)
- [组件证据矩阵](../../docs/validation/component-evidence-matrix.md)
- [HAL 移植指南](../../docs/hardware/porting.md)
- [API 参考](../../docs/API_REFERENCE.md)

---

## 📝 维护约定

代码块改动必须同时改 `components/hal/examples/hal_quickstart.c` 并跑 `hal_quickstart`；
`tests/unit/support/check_doc_api_symbols.py`（CTest `doc_api_symbols`）会拒绝本文里出现
“任何头都没声明过的 `xy_*` 名字”或“指向不存在路径的仓内引用”。

**最后更新**: 2026-11-02
**许可证**: Apache License 2.0
