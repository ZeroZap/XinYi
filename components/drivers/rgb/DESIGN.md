# RGB LED 效果库设计文档

**版本**: 1.0.0  
**日期**: 2026-03-02  
**参考**: WS2812FX / FastLED / WLED

---

## 📋 概述

基于 WS2812/FastLED/WLED 的 RGB LED 效果库，支持 WS2812/SK6812 等地址able RGB LED，提供丰富的视觉效果。

---

## 🏗️ 架构设计

### 分层架构

```
┌─────────────────────────────────────────┐
│          应用层 (Application)            │
│   用户代码 / 效果组合 / 场景模式         │
├─────────────────────────────────────────┤
│          效果层 (Effects)                │
│   静态效果 | 动态效果 | 过渡效果         │
├─────────────────────────────────────────┤
│          引擎层 (Engine)                 │
│   效果调度 | 亮度控制 | 颜色映射         │
├─────────────────────────────────────────┤
│          驱动层 (Driver)                 │
│   SPI | I2S | GPIO | 硬件抽象            │
└─────────────────────────────────────────┘
```

### 模块化设计

```
xy_rgb/
├── inc/
│   ├── xy_rgb.h           # 主接口
│   ├── xy_rgb_drv.h       # 驱动接口
│   ├── xy_rgb_fx.h        # 效果接口
│   ├── xy_rgb_color.h     # 颜色工具
│   └── xy_rgb_segment.h   # 分段管理
├── src/
│   ├── xy_rgb.c           # 核心实现
│   ├── xy_rgb_spi.c       # SPI 驱动
│   ├── xy_rgb_i2s.c       # I2S 驱动
│   ├── xy_rgb_fx.c        # 效果实现
│   ├── xy_rgb_color.c     # 颜色工具
│   └── xy_rgb_segment.c   # 分段管理
└── examples/
    └── demo.c             # 演示程序
```

---

## 🎨 效果分类

### 1. 静态效果 (Static)

| 效果 | 说明 |
|------|------|
| **Static** | 静态颜色 |
| **Rainbow** | 彩虹色 |
| **Gradient** | 渐变色 |

### 2. 动态效果 (Dynamic)

| 效果 | 说明 |
|------|------|
| **Scan** | 扫描灯 |
| **Chase** | 追逐灯 |
| **Fade** | 淡入淡出 |
| **Blink** | 闪烁 |
| **Twinkle** | 闪烁星光 |
| **Comet** | 彗星效果 |
| **Fire** | 火焰效果 |
| **Water** | 水流效果 |

### 3. 音乐效果 (Audio Reactive)

| 效果 | 说明 |
|------|------|
| **VU Meter** | 音量表 |
| **Spectrum** | 频谱分析 |
| **Beat** | 节拍检测 |

---

## 🎯 核心功能

### 1. LED 分段 (Segments)

```c
/* 支持多个独立分段 */
typedef struct {
    uint16_t start;     /* 起始 LED */
    uint16_t stop;      /* 结束 LED */
    uint16_t speed;     /* 速度 */
    uint16_t intensity; /* 强度 */
    rgb_color_t color1; /* 主颜色 */
    rgb_color_t color2; /* 次颜色 */
    uint8_t effect;     /* 效果 ID */
} xy_rgb_segment_t;
```

### 2. 颜色空间

```c
/* 支持多种颜色空间 */
typedef struct {
    uint8_t r, g, b;    /* RGB */
    uint8_t h, s, v;    /* HSV */
    uint8_t h, s, l;    /* HSL */
} rgb_color_t;
```

### 3. 效果引擎

```c
/* 效果调度器 */
typedef struct {
    xy_rgb_segment_t *segments;
    uint16_t num_segments;
    uint32_t last_update;
    uint16_t fps;
} xy_rgb_engine_t;
```

---

## 📦 API 设计

### 初始化

```c
/* 初始化 RGB 库 */
int xy_rgb_init(xy_rgb_config_t *config);

/* 配置 */
typedef struct {
    uint16_t num_leds;      /* LED 数量 */
    uint8_t brightness;     /* 亮度 (0-255) */
    xy_rgb_drv_type_t type; /* 驱动类型 */
} xy_rgb_config_t;
```

### 基础控制

```c
/* 设置亮度 */
void xy_rgb_set_brightness(uint8_t brightness);

/* 设置所有 LED 颜色 */
void xy_rgb_set_all(rgb_color_t color);

/* 设置单个 LED 颜色 */
void xy_rgb_set_pixel(uint16_t index, rgb_color_t color);

/* 显示更新 */
void xy_rgb_show(void);
```

### 效果控制

```c
/* 设置效果 */
void xy_rgb_set_effect(uint8_t effect_id);

/* 设置效果参数 */
void xy_rgb_set_effect_params(uint16_t speed, uint16_t intensity);

/* 效果循环 */
void xy_rgb_service(void);
```

### 分段控制

```c
/* 创建分段 */
int xy_rgb_create_segment(uint16_t start, uint16_t stop);

/* 设置分段效果 */
void xy_rgb_set_segment_effect(uint8_t seg_id, uint8_t effect_id);

/* 删除分段 */
void xy_rgb_delete_segment(uint8_t seg_id);
```

---

## 🎨 效果实现

### 1. 扫描灯 (Scan)

```
LED:  [0][1][2][3][4][5][6][7]
T0:   [R][ ][ ][ ][ ][ ][ ][ ]
T1:   [ ][R][ ][ ][ ][ ][ ][ ]
T2:   [ ][ ][R][ ][ ][ ][ ][ ]
```

### 2. 追逐灯 (Chase)

```
LED:  [0][1][2][3][4][5][6][7]
T0:   [R][R][R][ ][ ][ ][ ][ ]
T1:   [ ][R][R][R][ ][ ][ ][ ]
T2:   [ ][ ][R][R][R][ ][ ][ ]
```

### 3. 彗星效果 (Comet)

```
LED:  [0][1][2][3][4][5][6][7]
T0:   [W][ ][ ][ ][ ][ ][ ][ ]
T1:   [B][W][ ][ ][ ][ ][ ][ ]
T2:   [B][B][W][ ][ ][ ][ ][ ]
```

### 4. 火焰效果 (Fire)

```
使用噪声算法模拟火焰跳动
每个 LED 亮度随机变化
颜色从红到黄渐变
```

---

## 🔧 驱动支持

### 1. GPIO 位 bang

```c
/* 适用于所有 MCU */
/* 速度较慢，但兼容性好 */
```

### 2. SPI 驱动

```c
/* 使用硬件 SPI */
/* 速度快，但需要 SPI 支持 */
```

### 3. I2S 驱动

```c
/* 使用 I2S 外设 */
/* 速度最快，适合大量 LED */
```

---

## 📊 性能指标

| 指标 | 目标 |
|------|------|
| **最大 LED 数** | 1000+ |
| **刷新率** | 30-60 FPS |
| **内存占用** | < 3KB (100 LED) |
| **效果数量** | 20+ |

---

## 🚀 开发计划

### 阶段 1: 核心层 (2 小时)

- [ ] 基础数据结构
- [ ] 颜色空间转换
- [ ] 驱动接口定义

### 阶段 2: 驱动层 (2 小时)

- [ ] GPIO 位 bang 驱动
- [ ] SPI 驱动
- [ ] I2S 驱动

### 阶段 3: 效果层 (3 小时)

- [ ] 静态效果 (5 种)
- [ ] 动态效果 (10 种)
- [ ] 效果调度器

### 阶段 4: 应用层 (1 小时)

- [ ] 分段管理
- [ ] 示例程序
- [ ] 文档

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
