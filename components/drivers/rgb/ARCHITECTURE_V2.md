# RGB LED 效果库架构重构

**版本**: 2.0.0  
**日期**: 2026-03-02

---

## 📋 重构目标

将 RGB LED 效果库按**显示模式**拆分，实现专业化、模块化设计。

---

## 🏗️ 新架构

```
xy_rgb/
├── core/                    # 核心层
│   ├── xy_rgb.h            # 主接口
│   ├── xy_rgb.c            # 核心实现
│   ├── xy_rgb_color.h      # 颜色工具
│   └── xy_rgb_color.c
│
├── drivers/                 # 驱动层
│   ├── xy_rgb_drv.h        # 驱动接口
│   ├── xy_rgb_gpio.c       # GPIO 驱动
│   ├── xy_rgb_spi.c        # SPI 驱动
│   └── xy_rgb_i2s.c        # I2S 驱动
│
├── shapes/                  # 形状层 (按物理布局)
│   ├── xy_rgb_line.h       # 直线模式
│   ├── xy_rgb_line.c
│   ├── xy_rgb_matrix.h     # 矩形矩阵
│   ├── xy_rgb_matrix.c
│   ├── xy_rgb_circle.h     # 圆形模式
│   ├── xy_rgb_circle.c
│   ├── xy_rgb_ring.h       # 环形模式
│   └── xy_rgb_ring.c
│
├── effects/                 # 效果层 (按视觉效果)
│   ├── xy_rgb_fx_static.h  # 静态效果
│   ├── xy_rgb_fx_static.c
│   ├── xy_rgb_fx_dynamic.h # 动态效果
│   ├── xy_rgb_fx_dynamic.c
│   ├── xy_rgb_fx_music.h   # 音乐效果
│   └── xy_rgb_fx_music.c
│
├── transform/               # 变换层 (3D/旋转)
│   ├── xy_rgb_3d.h         # 3D 变换
│   ├── xy_rgb_3d.c
│   ├── xy_rgb_rotate.h     # 旋转变换
│   └── xy_rgb_rotate.c
│
└── examples/                # 示例
    ├── demo_line.c         # 直线示例
    ├── demo_matrix.c       # 矩阵示例
    ├── demo_circle.c       # 圆形示例
    └── demo_3d.c           # 3D 示例
```

---

## 🎨 模式分类

### 1. Line 模式 (直线)

```
LED 布局：[0]─[1]─[2]─[3]─[4]─...
```

**适用场景**:
- LED 灯带
- 直线排列
- 条形装饰

**效果**:
- Scan (扫描)
- Chase (追逐)
- Comet (彗星)
- Meteor (流星)

---

### 2. Matrix 模式 (矩形)

```
LED 布局:
[0]  [1]  [2]  [3]
[4]  [5]  [6]  [7]
[8]  [9]  [10] [11]
...
```

**适用场景**:
- LED 点阵屏
- 矩形显示屏
- 文字显示

**效果**:
- Text Scroll (文字滚动)
- Plasma (等离子)
- Game of Life (生命游戏)
- Spectrum Analyzer (频谱)

---

### 3. Circle 模式 (圆形)

```
LED 布局:
      [0]
   [11] [1]
 [10]     [2]
 [9]       [3]
   [8]   [4]
      [5]
```

**适用场景**:
- 圆形 LED 环
- 时钟显示
- 仪表盘

**效果**:
- Clock (时钟)
- Gauge (仪表)
- Spiral (螺旋)
- VU Meter (音量表)

---

### 4. Ring 模式 (环形)

```
LED 布局 (多圈):
外圈：[0]─[1]─[2]─[3]
      │         │
内圈：[7]─[6]─[5]─[4]
```

**适用场景**:
- 多圈 LED 环
- 螺旋灯带
- 装饰灯

**效果**:
- Spiral In/Out (螺旋进出)
- Rotation (旋转)
- Wave (波浪)

---

### 5. 3D 模式 (立方体)

```
LED 布局 (4x4x4 立方体):
     顶层
   ┌─────┐
   │     │
   └─────┘
     中层
   ┌─────┐
   │     │
   └─────┘
     底层
```

**适用场景**:
- LED 立方体
- 3D 显示
- 体素显示

**效果**:
- Rain (雨滴)
- Fire (3D 火焰)
- Plasma (3D 等离子)
- Maze (3D 迷宫)

---

### 6. Rotate 模式 (旋转)

```
旋转变换:
原始：[0][1][2][3]
旋转 90°:
      [0]
      [1]
      [2]
      [3]
```

**效果**:
- Rotate CW (顺时针旋转)
- Rotate CCW (逆时针旋转)
- Flip H (水平翻转)
- Flip V (垂直翻转)

---

## 🔧 坐标映射系统

### 通用坐标接口

```c
/* 2D 坐标 */
typedef struct {
    int16_t x;
    int16_t y;
} point2d_t;

/* 3D 坐标 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} point3d_t;

/* 坐标映射接口 */
typedef struct {
    uint16_t (*get_led_count)(void);
    point2d_t (*get_2d_coord)(uint16_t led_index);
    uint16_t (*from_2d_coord)(point2d_t coord);
    point3d_t (*get_3d_coord)(uint16_t led_index);
    uint16_t (*from_3d_coord)(point3d_t coord);
} xy_rgb_mapping_t;
```

### Line 映射

```c
static point2d_t line_get_2d(uint16_t index)
{
    return (point2d_t){index, 0};
}

static uint16_t line_from_2d(point2d_t coord)
{
    return coord.x;
}
```

### Matrix 映射

```c
static uint16_t g_matrix_width = 16;
static uint16_t g_matrix_height = 16;

static point2d_t matrix_get_2d(uint16_t index)
{
    return (point2d_t){
        index % g_matrix_width,
        index / g_matrix_width
    };
}

static uint16_t matrix_from_2d(point2d_t coord)
{
    return coord.y * g_matrix_width + coord.x;
}
```

### Circle 映射

```c
static uint16_t g_circle_count = 12;

static point2d_t circle_get_2d(uint16_t index)
{
    float angle = index * 2 * PI / g_circle_count;
    return (point2d_t){
        cosf(angle) * 50,
        sinf(angle) * 50
    };
}
```

---

## 📦 API 设计

### 初始化

```c
/* 基础初始化 */
int xy_rgb_init(xy_rgb_config_t *config);

/* 带形状初始化 */
int xy_rgb_init_shape(xy_rgb_config_t *config, 
                      xy_rgb_shape_t shape,
                      void *shape_params);
```

### 形状特定 API

```c
/* Line 模式 */
void xy_rgb_line_set_pixel(uint16_t x, rgb_color_t color);
void xy_rgb_line_fill(uint16_t start, uint16_t len, rgb_color_t color);

/* Matrix 模式 */
void xy_rgb_matrix_set_pixel(uint16_t x, uint16_t y, rgb_color_t color);
void xy_rgb_matrix_draw_line(uint16_t x0, uint16_t y0, 
                             uint16_t x1, uint16_t y1, 
                             rgb_color_t color);
void xy_rgb_matrix_draw_rect(uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h,
                             rgb_color_t color);
void xy_rgb_matrix_draw_text(const char *text, uint16_t x, uint16_t y);

/* Circle 模式 */
void xy_rgb_circle_set_angle(uint16_t angle, rgb_color_t color);
void xy_rgb_circle_draw_arc(uint16_t start_angle, uint16_t end_angle,
                            rgb_color_t color);

/* 3D 模式 */
void xy_rgb_3d_set_voxel(uint16_t x, uint16_t y, uint16_t z, 
                         rgb_color_t color);
void xy_rgb_3d_draw_line_3d(point3d_t p0, point3d_t p1,
                            rgb_color_t color);
```

### 变换 API

```c
/* 旋转 */
void xy_rgb_rotate_90_cw(void);
void xy_rgb_rotate_90_ccw(void);
void xy_rgb_rotate_180(void);

/* 翻转 */
void xy_rgb_flip_horizontal(void);
void xy_rgb_flip_vertical(void);

/* 3D 变换 */
void xy_rgb_3d_rotate_x(float angle);
void xy_rgb_3d_rotate_y(float angle);
void xy_rgb_3d_rotate_z(float angle);
```

---

## 🎯 效果分层

### Layer 1: 基础效果 (Base Effects)

```c
/* 不依赖形状 */
void xy_rgb_fx_static(rgb_color_t color);
void xy_rgb_fx_rainbow(void);
void xy_rgb_fx_gradient(rgb_color_t c1, rgb_color_t c2);
```

### Layer 2: 形状效果 (Shape Effects)

```c
/* 依赖形状 */
void xy_rgb_line_fx_scan(void);
void xy_rgb_matrix_fx_plasma(void);
void xy_rgb_circle_fx_clock(void);
void xy_rgb_3d_fx_rain(void);
```

### Layer 3: 变换效果 (Transform Effects)

```c
/* 带变换 */
void xy_rgb_matrix_fx_rotating_plasma(void);
void xy_rgb_3d_fx_rotating_rain(void);
```

---

## 📊 性能优化

### 双缓冲

```c
/* 帧缓冲区 */
typedef struct {
    rgb_color_t *buffer;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
} xy_rgb_framebuffer_t;

/* 双缓冲交换 */
void xy_rgb_swap_buffers(void);
```

### DMA 传输

```c
/* 使用 DMA 发送数据 */
void xy_rgb_show_dma(void);
```

### 查找表

```c
/* 预计算坐标映射 */
static uint16_t g_mapping_table[256];

void xy_rgb_init_mapping_table(void)
{
    for (uint16_t i = 0; i < 256; i++) {
        g_mapping_table[i] = map_function(i);
    }
}
```

---

## 🚀 开发计划

### 阶段 1: 核心层 (已完成)

- [x] 基础 RGB 库
- [x] 颜色工具
- [x] 驱动接口

### 阶段 2: 形状层 (进行中)

- [ ] Line 模式
- [ ] Matrix 模式
- [ ] Circle 模式
- [ ] Ring 模式

### 阶段 3: 3D 层 (计划)

- [ ] 3D 立方体
- [ ] 坐标映射
- [ ] 3D 效果

### 阶段 4: 变换层 (计划)

- [ ] 旋转
- [ ] 翻转
- [ ] 缩放

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
