# LED 组件重构方案

**日期**: 2026-03-02  
**目标**: 按 LED 类型分离为 3 个独立子组件

---

## 📊 问题分析

### 当前问题

1. **查理复用只支持单色** - 原始文档设计用于单色 LED
2. **RGB LED 需要独立驱动** - WS2812/SK6812 等是串行数据
3. **混在一起导致复杂度高** - 代码耦合，难以维护

### 解决方案

**按 LED 类型分离为 3 个独立子组件**:

```
components/drivers/led/
├── mono_led/          # 单色 LED (含查理复用)
├── rgb_led/           # RGB LED (WS2812/SK6812)
└── matrix_led/        # 矩阵 LED (点阵屏)
```

---

## 🏗️ 新架构

### 1. mono_led/ - 单色 LED 驱动

**适用**: 普通单色 LED、查理复用 LED

```
mono_led/
├── inc/
│   ├── xy_mono_led.h         # 基础单色 LED
│   ├── xy_mono_charlieplex.h # 查理复用驱动
│   └── xy_mono_matrix.h      # 单色矩阵
├── src/
│   ├── xy_mono_led.c
│   ├── xy_mono_charlieplex.c
│   └── xy_mono_matrix.c
└── README.md
```

**特性**:
- ✅ 基础 GPIO 控制
- ✅ 查理复用 (N IO 控制 N*(N-1) LED)
- ✅ PWM 亮度调节
- ✅ 单色矩阵扫描

**效果**:
- 呼吸灯
- 流水灯
- 闪烁
- 矩阵效果

---

### 2. rgb_led/ - RGB LED 驱动

**适用**: WS2812B、SK6812、APA102 等可寻址 RGB LED

```
rgb_led/
├── inc/
│   ├── xy_rgb_led.h          # RGB LED 核心
│   ├── xy_rgb_color.h        # 颜色工具
│   ├── xy_rgb_segment.h      # 分段管理
│   ├── xy_rgb_fx.h           # 效果接口
│   └── xy_rgb_drv.h          # 驱动接口 (SPI/I2S/RMT)
├── src/
│   ├── xy_rgb_led.c
│   ├── xy_rgb_color.c
│   ├── xy_rgb_segment.c
│   ├── xy_rgb_fx.c
│   └── xy_rgb_drv_*.c
├── effects/
│   ├── xy_rgb_fx_basic.c     # 基础效果
│   ├── xy_rgb_fx_extended.c  # 扩展效果
│   ├── xy_rgb_fx_noise.c     # 噪声效果
│   └── xy_rgb_palettes.c     # 调色板
└── README.md
```

**特性**:
- ✅ WS2812B/SK6812 支持
- ✅ 串行数据协议
- ✅ GRB/RGB 色彩顺序
- ✅ 分段效果
- ✅ 50+ 效果库
- ✅ 噪声算法
- ✅ 50 种调色板

**效果**:
- 彩虹/渐变/扫描
- 火焰/流星/彗星
- 等离子/噪声
- 音乐响应

---

### 3. matrix_led/ - 矩阵 LED 驱动

**适用**: LED 点阵屏、MAX7219、HT16K33

```
matrix_led/
├── inc/
│   ├── xy_matrix_led.h       # 矩阵核心
│   ├── xy_matrix_font.h      # 字库
│   ├── xy_matrix_graphics.h  # 图形 API
│   └── xy_matrix_spi.h       # SPI 驱动 (MAX7219)
├── src/
│   ├── xy_matrix_led.c
│   ├── xy_matrix_font.c
│   ├── xy_matrix_graphics.c
│   └── xy_matrix_spi.c
└── README.md
```

**特性**:
- ✅ 8x8/16x16 点阵
- ✅ SPI/I2C 接口
- ✅ 字库支持
- ✅ 图形绘制
- ✅ 滚动显示

**效果**:
- 文字滚动
- 图形动画
- 时钟显示
- 频谱分析

---

## 📦 组件对比

| 特性 | mono_led | rgb_led | matrix_led |
|------|----------|---------|------------|
| **LED 类型** | 单色 LED | RGB LED | 点阵屏 |
| **驱动方式** | GPIO/PWM | 串行数据 | SPI/I2C |
| **查理复用** | ✅ | ❌ | ❌ |
| **WS2812** | ❌ | ✅ | ❌ |
| **亮度控制** | PWM | 数据级 | PWM |
| **效果数** | 10+ | 50+ | 20+ |
| **IO 占用** | 少 | 1-2 个 | 2-4 个 |
| **最大 LED** | 56 (查理) | 1000+ | 256 (16x16) |

---

## 🔧 使用示例

### mono_led 示例

```c
#include "xy_mono_charlieplex.h"

// 4 IO 控制 12 个 LED
xy_charlieplex_t charlie;
uint8_t io_pins[] = {PA0, PA1, PA2, PA3};

xy_charlieplex_init(&charlie, io_pins, set_io_state, set_pwm);

// 设置单个 LED 亮度
xy_charlieplex_set_brightness(&charlie, 0, 128);

// 呼吸灯效果
xy_charlieplex_effect_breath(&charlie, 0, 1000);
```

### rgb_led 示例

```c
#include "xy_rgb_led.h"

// WS2812B 灯带
xy_rgb_led_t strip;
xy_rgb_led_config_t cfg = {
    .num_leds = 30,
    .type = XY_RGB_LED_WS2812B,
    .pin = GPIO5,
};

xy_rgb_led_init(&strip, &cfg);

// 设置彩虹效果
xy_rgb_led_set_effect(&strip, FX_RAINBOW);
xy_rgb_led_service(&strip);
xy_rgb_led_show(&strip);
```

### matrix_led 示例

```c
#include "xy_matrix_led.h"

// 8x8 点阵屏 (MAX7219)
xy_matrix_t matrix;
xy_matrix_spi_config_t cfg = {
    .width = 8,
    .height = 8,
    .spi_handle = hspi1,
};

xy_matrix_spi_init(&matrix, &cfg);

// 显示文字
xy_matrix_draw_string(&matrix, 0, 0, "Hello");
xy_matrix_show(&matrix);
```

---

## 📊 迁移计划

### 阶段 1: 分离 mono_led

- [ ] 创建 mono_led 目录
- [ ] 迁移查理复用代码
- [ ] 迁移单色矩阵代码
- [ ] 更新文档

### 阶段 2: 分离 rgb_led

- [ ] 创建 rgb_led 目录
- [ ] 迁移 WS2812 驱动
- [ ] 迁移效果库
- [ ] 迁移噪声引擎
- [ ] 迁移调色板

### 阶段 3: 创建 matrix_led

- [ ] 创建 matrix_led 目录
- [ ] 实现 MAX7219 驱动
- [ ] 实现字库
- [ ] 实现图形 API

### 阶段 4: 清理与整合

- [ ] 删除旧代码
- [ ] 更新依赖
- [ ] 测试所有组件
- [ ] 完善文档

---

## 🎯 优势

1. **清晰分离** - 每个组件职责单一
2. **独立维护** - 互不影响
3. **按需使用** - 只引入需要的组件
4. **易于扩展** - 新增 LED 类型不影响其他

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
