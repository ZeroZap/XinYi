# LED 组件最终分类方案

**日期**: 2026-03-02  
**原则**: 按**驱动方式**分类，而不是 LED 类型

---

## 🤔 问题分析

### 之前分类的问题

```
❌ mono_led/     - 单色 LED
❌ rgb_led/      - RGB LED  
❌ matrix_led/   - 矩阵 LED
```

**问题**:
1. **分类标准不统一** - 有的按颜色，有的按结构
2. **GPIO 矩阵无处安放** - 既不是查理复用，也不是 RGB
3. **容易混淆** - 矩阵 LED 可以是单色也可以是 RGB

---

## ✅ 新分类方案

### 按驱动方式分类 (3 大类)

```
components/drivers/led/
├── charlieplex/        # 查理复用驱动 (高效 IO 复用)
├── matrix_scan/        # 矩阵扫描驱动 (GPIO/SPI/I2C)
└── serial_rgb/         # 串行 RGB 驱动 (WS2812/SK6812)
```

---

## 📊 详细分类

### 1. charlieplex/ - 查理复用驱动

**特点**: N 个 IO 控制 N*(N-1) 个 LED

```
IO 数    LED 数
2        2
3        6
4        12
5        20
6        30
8        56
```

**适用**:
- ✅ 单色 LED 阵列
- ✅ 指示灯面板
- ✅ 低功耗显示

**不适用**:
- ❌ RGB LED (需要独立控制 3 通道)
- ❌ 高亮度应用

**驱动文件**:
```
charlieplex/
├── inc/
│   ├── xy_charlieplex.h      # 核心驱动
│   └── xy_charlieplex_fx.h   # 效果库
├── src/
│   ├── xy_charlieplex.c
│   └── xy_charlieplex_fx.c
└── README.md
```

---

### 2. matrix_scan/ - 矩阵扫描驱动

**特点**: 行/列扫描，支持多种接口

**子分类**:

```
matrix_scan/
├── gpio_matrix/      # GPIO 直接驱动
├── spi_matrix/       # SPI 驱动 (MAX7219)
├── i2c_matrix/       # I2C 驱动 (HT16K33)
└── rgb_matrix/       # RGB 矩阵 (STM32 DMA)
```

**GPIO 矩阵示例**:
```
        列 (GPIO 输出/PWM)
        COL0  COL1  COL2
         │     │     │
ROW0 ────●─────●─────●────
         │     │     │
ROW1 ────●─────●─────●────
         │     │     │
```

**适用**:
- ✅ 单色点阵 (1 色/3 色/7 色)
- ✅ RGB 点阵 (共阴/共阳)
- ✅ 大屏幕 (级联)

**驱动文件**:
```
matrix_scan/
├── inc/
│   ├── xy_matrix.h           # 统一接口
│   ├── xy_matrix_gpio.h      # GPIO 矩阵
│   ├── xy_matrix_spi.h       # SPI 矩阵
│   ├── xy_matrix_i2c.h       # I2C 矩阵
│   └── xy_matrix_rgb.h       # RGB 矩阵
├── src/
│   ├── xy_matrix.c
│   ├── xy_matrix_gpio.c
│   ├── xy_matrix_spi.c
│   ├── xy_matrix_i2c.c
│   └── xy_matrix_rgb.c
├── font/
│   └── xy_matrix_font.c      # 字库
└── README.md
```

---

### 3. serial_rgb/ - 串行 RGB 驱动

**特点**: 串行数据，单线控制多个 RGB LED

**支持芯片**:
- WS2812B / WS2811
- SK6812 / SK6812-RGBW
- APA102 / SK9822 (SPI 型)
- WS2813 / WS2815 (双信号备份)

**驱动方式**:
```
serial_rgb/
├── gpio_bitbang/   # GPIO 位模拟 (通用)
├── spi_dma/        # SPI+DMA (高性能)
├── i2s_dma/        # I2S+DMA (ESP32)
└── rmt/            # RMT 外设 (ESP32 专用)
```

**适用**:
- ✅ LED 灯带
- ✅ LED 灯条
- ✅ RGB 装饰灯
- ✅ 大型显示屏

**驱动文件**:
```
serial_rgb/
├── inc/
│   ├── xy_serial_rgb.h       # 统一接口
│   ├── xy_serial_rgb_bb.h    # 位模拟
│   ├── xy_serial_rgb_spi.h   # SPI 驱动
│   └── xy_serial_rgb_i2s.h   # I2S 驱动
├── src/
│   ├── xy_serial_rgb.c
│   ├── xy_serial_rgb_bb.c
│   ├── xy_serial_rgb_spi.c
│   └── xy_serial_rgb_i2s.c
├── effects/
│   ├── xy_rgb_fx_basic.c     # 基础效果
│   ├── xy_rgb_fx_advanced.c  # 高级效果
│   └── xy_rgb_palettes.c     # 调色板
└── README.md
```

---

## 📋 完整架构

```
components/drivers/led/
│
├── charlieplex/              # 查理复用
│   ├── inc/
│   │   ├── xy_charlieplex.h
│   │   └── xy_charlieplex_fx.h
│   ├── src/
│   │   ├── xy_charlieplex.c
│   │   └── xy_charlieplex_fx.c
│   └── README.md
│
├── matrix_scan/              # 矩阵扫描
│   ├── inc/
│   │   ├── xy_matrix.h
│   │   ├── xy_matrix_gpio.h
│   │   ├── xy_matrix_spi.h
│   │   ├── xy_matrix_i2c.h
│   │   └── xy_matrix_rgb.h
│   ├── src/
│   │   ├── xy_matrix.c
│   │   ├── xy_matrix_gpio.c
│   │   ├── xy_matrix_spi.c
│   │   ├── xy_matrix_i2c.c
│   │   └── xy_matrix_rgb.c
│   ├── font/
│   │   └── xy_matrix_font.c
│   └── README.md
│
└── serial_rgb/               # 串行 RGB
    ├── inc/
    │   ├── xy_serial_rgb.h
    │   ├── xy_serial_rgb_bb.h
    │   ├── xy_serial_rgb_spi.h
    │   └── xy_serial_rgb_i2s.h
    ├── src/
    │   ├── xy_serial_rgb.c
    │   ├── xy_serial_rgb_bb.c
    │   ├── xy_serial_rgb_spi.c
    │   └── xy_serial_rgb_i2s.c
    ├── effects/
    │   ├── xy_rgb_fx_basic.c
    │   ├── xy_rgb_fx_advanced.c
    │   └── xy_rgb_palettes.c
    └── README.md
```

---

## 🎯 分类对比

| 特性 | charlieplex | matrix_scan | serial_rgb |
|------|-------------|-------------|------------|
| **驱动方式** | 三态 IO 复用 | 行/列扫描 | 串行数据 |
| **IO 占用** | 极少 | 中等 | 极少 |
| **最大 LED** | 56 (8 IO) | 256 (16x16) | 1000+ |
| **支持单色** | ✅ | ✅ | ❌ |
| **支持 RGB** | ❌ | ✅ | ✅ |
| **支持灰度** | PWM | PWM | 数据级 |
| **复杂度** | 低 | 中 | 低 |
| **成本** | 最低 | 中 | 高 |
| **典型应用** | 指示灯 | 点阵屏 | 装饰灯带 |

---

## 🔧 使用示例

### 查理复用

```c
#include "xy_charlieplex.h"

// 4 IO 控制 12 个单色 LED
xy_charlieplex_t ctx;
uint8_t io_pins[] = {PA0, PA1, PA2, PA3};

xy_charlieplex_init(&ctx, io_pins, set_io, set_pwm);
xy_charlieplex_set_brightness(&ctx, 0, 128);  // 设置 LED0 亮度
```

### GPIO 矩阵

```c
#include "xy_matrix_gpio.h"

// 8x8 单色点阵 (GPIO 直接驱动)
xy_matrix_gpio_t mtx;
uint8_t row_pins[] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
uint8_t col_pins[] = {PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7};

xy_matrix_gpio_init(&mtx, row_pins, 8, col_pins, 8);
xy_matrix_draw_char(&mtx, 0, 0, 'A');
```

### SPI 矩阵 (MAX7219)

```c
#include "xy_matrix_spi.h"

// 4 个 MAX7219 级联 (32x8 点阵)
xy_matrix_spi_t mtx;
xy_matrix_spi_config_t cfg = {
    .spi_handle = hspi1,
    .num_devices = 4,
};

xy_matrix_spi_init(&mtx, &cfg);
xy_matrix_scroll_text(&mtx, "Hello World");
```

### 串行 RGB (WS2812B)

```c
#include "xy_serial_rgb.h"

// WS2812B 灯带 (30 个 LED)
xy_serial_rgb_t strip;
xy_serial_rgb_config_t cfg = {
    .num_leds = 30,
    .type = XY_SERIAL_RGB_WS2812B,
    .pin = GPIO5,
};

xy_serial_rgb_init(&strip, &cfg);
xy_serial_rgb_set_effect(&strip, FX_RAINBOW, 128, 128);
```

---

## 📈 选择指南

### 问：我要控制 12 个单色 LED，最少 IO 口

**答**: 使用 `charlieplex` - 4 个 IO 控制 12 个 LED

### 问：我要做 8x8 点阵屏显示文字

**答**: 使用 `matrix_scan/spi_matrix` - MAX7219 模块

### 问：我要做 RGB 氛围灯带

**答**: 使用 `serial_rgb` - WS2812B 灯带

### 问：我要做大型 RGB 显示屏

**答**: 使用 `matrix_scan/rgb_matrix` - 定制 RGB 矩阵

### 问：我要用 GPIO 直接驱动 8x8 单色点阵

**答**: 使用 `matrix_scan/gpio_matrix` - GPIO 扫描

---

## ✅ 最终优势

1. **分类清晰** - 按驱动方式，不混淆
2. **覆盖全面** - 所有 LED 类型都有归属
3. **易于扩展** - 新增驱动方式不影响现有
4. **用户友好** - 根据需求快速选择

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
