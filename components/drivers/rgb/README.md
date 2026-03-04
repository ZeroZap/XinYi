# RGB LED 效果库使用指南

**版本**: 1.0.0  
**日期**: 2026-03-02

---

## 📋 概述

XY_RGB 是一个功能丰富的 RGB LED 效果库，参考 WS2812FX/FastLED/WLED 设计，支持 WS2812/SK6812 等可寻址 RGB LED。

---

## 🚀 快速开始

### 1. 硬件连接

```
MCU GPIO5 ──┬── LED DIN
            │
            └── 330Ω 电阻 (推荐)

VCC ────────┬── LED VCC (5V)
            │
           1000μF 电容

GND ───────┴── LED GND
            │
           MCU GND
```

### 2. 基础示例

```c
#include "xy_rgb.h"

#define NUM_LEDS    30
#define DATA_PIN    5

int main(void) {
    /* 配置 */
    xy_rgb_config_t config = {
        .num_leds = NUM_LEDS,
        .brightness = 128,
        .drv_type = XY_RGB_DRV_GPIO,
    };
    
    xy_rgb_gpio_config_t gpio_cfg = {
        .data_pin = DATA_PIN,
        .color_order = XY_RGB_GRB,
    };
    config.drv_handle = &gpio_cfg;
    
    /* 初始化 */
    xy_rgb_init(&config);
    
    /* 设置颜色 */
    xy_rgb_set_all((rgb_color_t){255, 0, 0});  /* 红色 */
    xy_rgb_show();
    
    while (1) {
        /* 主循环 */
    }
}
```

---

## 🎨 效果使用

### 静态效果

```c
/* 彩虹效果 */
xy_rgb_set_effect(FX_RAINBOW);
xy_rgb_set_effect_params(128, 128);  /* 速度，强度 */

while (1) {
    xy_rgb_service();  /* 更新效果 */
    xy_rgb_show();     /* 显示 */
    xy_os_delay(20);   /* 控制 FPS */
}
```

### 动态效果

```c
/* 追逐灯效果 */
xy_rgb_set_effect(FX_CHASE);
xy_rgb_set_effect_params(200, 128);  /* 快速 */

while (1) {
    xy_rgb_service();
    xy_rgb_show();
    xy_os_delay(20);
}
```

### 火焰效果

```c
/* 火焰效果 */
xy_rgb_set_effect(FX_FIRE);
xy_rgb_set_effect_params(128, 200);  /* 高强度 */

while (1) {
    xy_rgb_service();
    xy_rgb_show();
    xy_os_delay(30);
}
```

---

## 📦 分段管理

### 创建分段

```c
/* 创建 3 个独立分段 */
int seg1 = xy_rgb_create_segment(0, 10);    /* LED 0-9 */
int seg2 = xy_rgb_create_segment(10, 20);   /* LED 10-19 */
int seg3 = xy_rgb_create_segment(20, 30);   /* LED 20-29 */
```

### 设置分段效果

```c
/* 不同分段不同效果 */
xy_rgb_set_segment_effect(seg1, FX_RAINBOW);
xy_rgb_set_segment_effect(seg2, FX_CHASE);
xy_rgb_set_segment_effect(seg3, FX_BREATH);

/* 设置分段颜色 */
xy_rgb_set_segment_colors(seg1,
    (rgb_color_t){255, 0, 0},   /* 主颜色 */
    (rgb_color_t){0, 0, 0},     /* 次颜色 */
    (rgb_color_t){0, 0, 0});    /* 第三颜色 */
```

### 分段参数

```c
/* 设置速度 */
xy_rgb_set_segment_params(seg1, 200, 128);  /* 速度，强度 */

/* 设置方向 */
xy_rgb_set_segment_reverse(seg1, true);

/* 使能/禁用 */
xy_rgb_set_segment_enabled(seg1, false);
```

---

## 🌈 颜色工具

### RGB/HSV 转换

```c
/* RGB 转 HSV */
rgb_color_t rgb = {255, 0, 0};
hsv_color_t hsv = xy_rgb_to_hsv(rgb);

/* HSV 转 RGB */
hsv_color_t hsv = {0, 255, 255};  /* 红色 */
rgb_color_t rgb = xy_hsv_to_rgb(hsv);
```

### 颜色混合

```c
/* 混合两种颜色 */
rgb_color_t red = {255, 0, 0};
rgb_color_t blue = {0, 0, 255};
rgb_color_t purple = xy_color_blend(red, blue, 128);  /* 50% 混合 */
```

### 彩虹颜色

```c
/* 生成彩虹颜色 */
for (uint8_t hue = 0; hue < 255; hue++) {
    rgb_color_t color = xy_rainbow_color(hue);
    /* 使用颜色 */
}
```

---

## 🎯 效果列表

### 静态效果 (3 种)

| 效果 | 说明 | 参数 |
|------|------|------|
| **Static** | 静态颜色 | 颜色 |
| **Rainbow** | 彩虹 | 速度 |
| **Gradient** | 渐变 | 颜色 1, 颜色 2 |

### 动态效果 (10 种)

| 效果 | 说明 | 参数 |
|------|------|------|
| **Scan** | 扫描灯 | 速度 |
| **Chase** | 追逐灯 | 速度 |
| **Fade** | 淡入淡出 | 速度 |
| **Blink** | 闪烁 | 速度 |
| **Twinkle** | 闪烁星光 | 强度 |
| **Comet** | 彗星 | 速度 |
| **Fire** | 火焰 | 强度 |
| **Water** | 水流 | 速度 |
| **Breath** | 呼吸 | 速度 |
| **Meteor** | 流星 | 速度 |

### 音乐效果 (2 种)

| 效果 | 说明 | 输入 |
|------|------|------|
| **Spectrum** | 频谱分析 | 频谱数据 |
| **VU Meter** | 音量表 | 音量级别 |

---

## 🔧 驱动支持

### GPIO 位 bang

```c
xy_rgb_gpio_config_t gpio_cfg = {
    .data_pin = 5,
    .color_order = XY_RGB_GRB,
};
```

**特点**:
- ✅ 兼容性好
- ⚠️ 速度较慢
- ⚠️ 占用 CPU

### SPI 驱动

```c
xy_rgb_spi_config_t spi_cfg = {
    .spi_handle = hspi1,
    .color_order = XY_RGB_GRB,
};
```

**特点**:
- ✅ 速度快
- ✅ 不占用 CPU
- ⚠️ 需要 SPI 支持

### I2S 驱动

```c
xy_rgb_i2s_config_t i2s_cfg = {
    .i2s_handle = hi2s1,
    .color_order = XY_RGB_GRB,
};
```

**特点**:
- ✅ 速度最快
- ✅ 适合大量 LED
- ⚠️ 需要 I2S 支持

---

## 📊 性能优化

### 亮度控制

```c
/* 全局亮度 */
xy_rgb_set_brightness(128);  /* 50% 亮度 */

/* 降低亮度可减少功耗 */
```

### FPS 控制

```c
/* 目标 30 FPS */
#define TARGET_FPS  30
#define FRAME_TIME  (1000 / TARGET_FPS)

while (1) {
    uint32_t start = xy_os_tick_get();
    
    xy_rgb_service();
    xy_rgb_show();
    
    /* 等待下一帧 */
    while ((xy_os_tick_get() - start) < FRAME_TIME);
}
```

### 内存优化

```c
/* 减少最大 LED 数 */
#define XY_RGB_MAX_LEDS  64  /* 默认 256 */

/* 减少最大分段数 */
#define XY_RGB_MAX_SEGMENTS  4  /* 默认 8 */
```

---

## 🐛 常见问题

### Q: LED 颜色不正确

**A**: 检查颜色顺序设置

```c
/* WS2812B 通常是 GRB */
.color_order = XY_RGB_GRB;

/* SK6812 可能是 RGB */
.color_order = XY_RGB_RGB;
```

### Q: 效果闪烁

**A**: 检查时序和电源

```c
/* 确保电源充足 */
/* 添加 1000μF 电容 */
/* 检查数据信号质量 */
```

### Q: 效果不流畅

**A**: 降低 LED 数量或提高 FPS

```c
/* 减少 LED 数量 */
#define NUM_LEDS  60  /* 而不是 300 */

/* 提高 FPS */
xy_os_delay(10);  /* 100 FPS */
```

---

## 📚 参考资源

- [WS2812 数据手册](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
- [FastLED 库](https://github.com/FastLED/FastLED)
- [WLED 项目](https://github.com/Aircoookie/WLED)
- [WS2812FX 库](https://github.com/kitesurfer1404/WS2812FX)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
