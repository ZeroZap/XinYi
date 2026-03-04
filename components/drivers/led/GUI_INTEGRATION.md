# LED 组件与 GUI 关系重构

**日期**: 2026-03-02  
**原则**: LED 驱动为 GUI 服务，GUI 使用 LED 驱动

---

## ❌ 之前的问题

```
❌ LED 库独立存在
❌ 屏幕效果在 LED 库中
❌ GUI 和 LED 分离
```

**问题**:
1. **职责不清** - LED 驱动应该服务 GUI
2. **功能重复** - GUI 和 LED 都有屏幕效果
3. **架构混乱** - 用户不知道用哪个

---

## ✅ 新架构

```
components/
├── gui/                    # GUI 核心 (上层)
│   ├── xy_gui.h           # GUI 统一接口
│   ├── xy_gui_engine.h    # GUI 引擎
│   ├── xy_gui_font.h      # 字库
│   ├── xy_gui_widget.h    # 控件
│   └── xy_gui_effects.h   # GUI 效果
│
└── drivers/
    └── led/               # LED 驱动 (下层)
        ├── charlieplex/   # 查理复用驱动
        ├── matrix_scan/   # 矩阵扫描驱动
        └── serial_rgb/    # 串行 RGB 驱动
```

**关系**:
```
┌─────────────────────────────────┐
│       应用层 (Application)       │
└───────────────┬─────────────────┘
                │ 使用
┌───────────────▼─────────────────┐
│         GUI 核心层               │
│  xy_gui.h / xy_gui_engine.h    │
│  - 绘图 API                     │
│  - 控件系统                     │
│  - 动画效果                     │
│  - 字体渲染                     │
└───────────────┬─────────────────┘
                │ 使用驱动
┌───────────────▼─────────────────┐
│         LED 驱动层               │
│  - charlieplex (单色复用)       │
│  - matrix_scan (点阵扫描)       │
│  - serial_rgb (RGB 串行)        │
└─────────────────────────────────┘
```

---

## 📦 详细设计

### 1. GUI 核心层 (xy_gui.h)

**职责**: 提供统一 GUI 接口，不关心底层硬件

```c
/**
 * @brief GUI 显示设备抽象
 */
typedef struct {
    uint16_t width;
    uint16_t height;
    xy_gui_color_format_t format;
    
    // 驱动接口 (由 LED 驱动实现)
    void (*set_pixel)(int16_t x, int16_t y, uint32_t color);
    uint32_t (*get_pixel)(int16_t x, int16_t y);
    void (*flush)(void);
    void (*fill)(uint32_t color);
} xy_gui_display_t;

/**
 * @brief GUI 初始化
 */
int xy_gui_init(xy_gui_display_t *display);

/**
 * @brief 绘制像素
 */
void xy_gui_set_pixel(int16_t x, int16_t y, xy_gui_color_t color);

/**
 * @brief 绘制直线
 */
void xy_gui_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
                      xy_gui_color_t color);

/**
 * @brief 绘制矩形
 */
void xy_gui_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                      xy_gui_color_t color, bool filled);

/**
 * @brief 绘制圆形
 */
void xy_gui_draw_circle(int16_t x0, int16_t y0, int16_t radius,
                        xy_gui_color_t color, bool filled);

/**
 * @brief 绘制字符
 */
void xy_gui_draw_char(int16_t x, int16_t y, char c, xy_gui_color_t color);

/**
 * @brief 绘制字符串
 */
void xy_gui_draw_string(int16_t x, int16_t y, const char *str,
                        xy_gui_color_t color);

/**
 * @brief 刷新显示
 */
void xy_gui_flush(void);
```

---

### 2. LED 驱动层

#### 2.1 matrix_scan 驱动 (为 GUI 服务)

```c
/**
 * @brief 矩阵 LED 驱动 - 实现 GUI 显示接口
 */
typedef struct {
    xy_gui_display_t gui_display;  // GUI 显示接口
    
    // 矩阵特定参数
    uint8_t row_pins[MATRIX_MAX_ROWS];
    uint8_t col_pins[MATRIX_MAX_COLS];
    uint8_t num_rows;
    uint8_t num_cols;
    
    // 缓冲区
    uint32_t *frame_buffer;
    bool dirty;
} xy_matrix_led_driver_t;

/**
 * @brief 初始化矩阵驱动 (并注册到 GUI)
 */
int xy_matrix_led_init(xy_matrix_led_driver_t *drv,
                       uint8_t *row_pins, uint8_t num_rows,
                       uint8_t *col_pins, uint8_t num_cols,
                       void (*gpio_write)(uint8_t, bool),
                       void (*pwm_set)(uint8_t, uint8_t));

/**
 * @brief 获取 GUI 显示接口 (供 GUI 使用)
 */
xy_gui_display_t* xy_matrix_led_get_display(xy_matrix_led_driver_t *drv);
```

**使用方式**:
```c
// 1. 初始化矩阵驱动
xy_matrix_led_driver_t matrix_drv;
xy_matrix_led_init(&matrix_drv, row_pins, 8, col_pins, 8, gpio_write, pwm_set);

// 2. 获取 GUI 显示接口
xy_gui_display_t *display = xy_matrix_led_get_display(&matrix_drv);

// 3. 初始化 GUI
xy_gui_init(display);

// 4. 使用 GUI 绘图
xy_gui_draw_string(0, 0, "Hello", WHITE);
xy_gui_flush();
```

---

#### 2.2 serial_rgb 驱动 (为 GUI 服务)

```c
/**
 * @brief 串行 RGB 驱动 - 实现 GUI 显示接口
 */
typedef struct {
    xy_gui_display_t gui_display;  // GUI 显示接口
    
    // RGB LED 特定参数
    uint16_t num_leds;
    uint8_t pin;
    xy_rgb_led_type_t type;
    
    // 缓冲区
    xy_rgb_color_t *led_buffer;
    uint8_t *tx_buffer;
} xy_serial_rgb_driver_t;

/**
 * @brief 初始化串行 RGB 驱动 (并注册到 GUI)
 */
int xy_serial_rgb_init(xy_serial_rgb_driver_t *drv,
                       uint16_t num_leds,
                       uint8_t pin,
                       xy_rgb_led_type_t type);

/**
 * @brief 获取 GUI 显示接口 (供 GUI 使用)
 */
xy_gui_display_t* xy_serial_rgb_get_display(xy_serial_rgb_driver_t *drv);
```

**使用方式**:
```c
// 1. 初始化 RGB 驱动
xy_serial_rgb_driver_t rgb_drv;
xy_serial_rgb_init(&rgb_drv, 256, GPIO5, XY_RGB_LED_WS2812B);

// 2. 获取 GUI 显示接口
xy_gui_display_t *display = xy_serial_rgb_get_display(&rgb_drv);

// 3. 初始化 GUI
xy_gui_init(display);

// 4. 使用 GUI 绘图
xy_gui_draw_rect(0, 0, 16, 16, RED, true);
xy_gui_flush();
```

---

#### 2.3 charlieplex 驱动 (独立使用)

```c
/**
 * @brief 查理复用驱动 - 用于简单指示灯
 * 不提供 GUI 接口 (因为是单色，不适合 GUI)
 */
typedef struct {
    uint8_t io_pins[CHARLIE_IO_NUM];
    uint8_t num_ios;
    uint8_t brightness[CHARLIE_LED_NUM];
} xy_charlieplex_driver_t;

// 直接使用，不注册到 GUI
xy_charlieplex_init(&drv, io_pins, 4);
xy_charlieplex_set_brightness(&drv, 0, 128);
```

---

### 3. GUI 效果层 (xy_gui_effects.h)

**职责**: 提供 GUI 动画效果 (从原 LED 屏幕效果迁移)

```c
/**
 * @brief GUI 滚动效果
 */
void xy_gui_scroll_text(const char *text, int16_t x, int16_t y,
                        xy_gui_scroll_dir_t dir, uint8_t speed);

/**
 * @brief GUI 淡入效果
 */
void xy_gui_fade_in(uint8_t speed);

/**
 * @brief GUI 淡出效果
 */
void xy_gui_fade_out(uint8_t speed);

/**
 * @brief GUI 缩放效果
 */
void xy_gui_zoom(uint8_t scale, bool from_center);

/**
 * @brief GUI 波浪效果
 */
void xy_gui_wave(uint8_t amplitude, uint8_t frequency);

/**
 * @brief GUI 粒子效果
 */
void xy_gui_particles(xy_gui_particle_t type, uint8_t density);
```

---

## 📊 完整架构图

```
┌─────────────────────────────────────────────────────────┐
│                  应用层 (Application)                    │
│  main.c / app.c                                         │
└────────────────────┬────────────────────────────────────┘
                     │ 使用
┌────────────────────▼────────────────────────────────────┐
│                    GUI 核心层                            │
│  xy_gui.h / xy_gui_engine.h / xy_gui_effects.h         │
│  ┌───────────────────────────────────────────────────┐  │
│  │ 绘图 API  │ 控件系统 │ 动画效果 │ 字体渲染 │       │  │
│  └───────────────────────────────────────────────────┘  │
└────────────────────┬────────────────────────────────────┘
                     │ 使用驱动接口
        ┌────────────┼────────────┬────────────────────┐
        │            │            │                    │
┌───────▼───────┐ ┌──▼────────┐ ┌─▼──────────┐ ┌─────▼──────┐
│ charlieplex/  │ │matrix_    │ │serial_     │ │ 其他显示   │
│ 查理复用      │ │scan/      │ │rgb/        │ │ 驱动       │
│               │ │矩阵扫描    │ │RGB 串行     │ │            │
│ 单色指示灯    │ │点阵屏      │ │LED 灯带     │ │ LCD/OLED   │
└───────────────┘ └───────────┘ └────────────┘ └────────────┘
```

---

## 🔧 使用示例

### 示例 1: 8x8 点阵屏显示

```c
#include "xy_gui.h"
#include "xy_matrix_led.h"

// 矩阵驱动
xy_matrix_led_driver_t matrix_drv;
uint8_t row_pins[8] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
uint8_t col_pins[8] = {PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7};

// 1. 初始化矩阵驱动
xy_matrix_led_init(&matrix_drv, row_pins, 8, col_pins, 8, 
                   gpio_write, pwm_set);

// 2. 获取 GUI 显示接口
xy_gui_display_t *display = xy_matrix_led_get_display(&matrix_drv);

// 3. 初始化 GUI
xy_gui_init(display);

// 4. 使用 GUI 绘图
xy_gui_clear();
xy_gui_draw_string(0, 0, "Hi", WHITE);
xy_gui_draw_circle(4, 4, 2, RED, false);
xy_gui_flush();

// 5. 使用 GUI 效果
xy_gui_scroll_text("Hello World", 0, 0, SCROLL_LEFT, 1);
```

---

### 示例 2: WS2812 RGB 灯阵

```c
#include "xy_gui.h"
#include "xy_serial_rgb.h"

// RGB 驱动
xy_serial_rgb_driver_t rgb_drv;

// 1. 初始化 RGB 驱动 (16x16=256 LED)
xy_serial_rgb_init(&rgb_drv, 256, GPIO5, XY_RGB_LED_WS2812B);

// 2. 获取 GUI 显示接口
xy_gui_display_t *display = xy_serial_rgb_get_display(&rgb_drv);

// 3. 初始化 GUI
xy_gui_init(display);

// 4. 使用 GUI 绘图
xy_gui_gradient(0, 0, 15, 15, RED, BLUE);
xy_gui_flush();

// 5. 使用 GUI 效果
xy_gui_plasma_effect(100);
```

---

### 示例 3: 查理复用指示灯

```c
#include "xy_charlieplex.h"

// 查理复用不提供 GUI 接口，直接使用
xy_charlieplex_driver_t charlie_drv;
uint8_t io_pins[4] = {PA0, PA1, PA2, PA3};

xy_charlieplex_init(&charlie_drv, io_pins, 4);

// 直接控制 LED
xy_charlieplex_set_brightness(&charlie_drv, 0, 128);
xy_charlieplex_effect_breath(&charlie_drv, 0, 1000);
```

---

## ✅ 优势

1. **职责清晰** - GUI 负责绘图，LED 负责驱动
2. **易于扩展** - 新增显示设备只需实现 GUI 接口
3. **用户友好** - 统一使用 xy_gui.h 接口
4. **代码复用** - GUI 效果可用于任何显示设备

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
