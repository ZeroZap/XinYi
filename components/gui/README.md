# XinYi GUI Component Status

**状态**: host-guarded core / 文档与显示硬件待收口<br>
**事实源日期**: 2026-08-09

本目录记录 `components/gui/` 的当前 GUI core、widget、event、theme 与扩展边界。它不是 Display driver 总说明，也不宣称真实屏幕硬件或完整 UI 产品已经验证通过。

## 当前边界

- GUI core 入口：`components/gui/xy_gui.h` / `components/gui/xy_gui.c`
- Widget/event/theme 入口：`components/gui/inc/xy_gui_*.h` / `components/gui/src/xy_gui_*.c`
- 根构建入口：`components/gui/CMakeLists.txt`
- 当前 host 单元测试事实源：`tests/unit/gui/*` 与 `tests/unit/CMakeLists.txt`

GUI core 使用显式 `xy_gui_t` context API；不要再使用旧文档中的全局 `xy_gui_init(display)`、`xy_gui_clear(color)`、`xy_gui_flush()` 风格示例。

## 已有源码与 host CTest 护栏

| 范围 | 源码路径 | Host CTest | 当前结论 |
| --- | --- | --- | --- |
| GUI core | `xy_gui.c`, `xy_gui.h` | `gui_core` | lifecycle、clear/flush、像素/线/矩形/字符/字符串和 object contract 已由 host 测试守护 |
| Widget base + theme | `src/xy_gui_widget.c`, `src/xy_gui_theme.c`, `inc/xy_gui_widget.h`, `inc/xy_gui_theme.h` | `gui_widget_theme` | widget init/style/text/value/parent-child 与 theme register/apply/list/unregister contract 已由 host 测试守护 |
| Event + widgets | `src/xy_gui_event.c`, button/checkbox/label/progress/slider/container 源码与头文件 | `gui_widgets` | event queue/dispatch 与 button、checkbox/radio、label、progress、slider、container contract 已由 host 测试守护 |
| Effects public headers | `effects/xy_gui_effect*.h` | `gui_effects_headers` | effects 统一头与各效果头文件 self-containment、公共类型/函数签名编译契约已由 host 测试守护；效果算法仍未标记为完整闭环 |

这些测试说明 GUI 已不是“完全无测试/待开发”的空白组件；后续只能按真实失败补小回归，或为尚未纳入测试的扩展写独立 proposal/CTest。

## 当前 API 示例

### 1. 初始化 explicit context GUI core

```c
#include "xy_gui.h"

static int display_init(void) { return XY_GUI_OK; }
static int display_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    (void)x;
    (void)y;
    (void)color;
    return XY_GUI_OK;
}
static int display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
    return XY_GUI_OK;
}
static int display_flush(void) { return XY_GUI_OK; }

xy_gui_t gui;
xy_gui_disp_drv_t drv = {
    .init = display_init,
    .draw_pixel = display_draw_pixel,
    .fill_rect = display_fill_rect,
    .flush = display_flush,
};

if (xy_gui_init(&gui, 128, 64, &drv) == XY_GUI_OK) {
    xy_gui_clear(&gui, XY_GUI_COLOR_BLACK);
    xy_gui_draw_pixel(&gui, 10, 10, XY_GUI_COLOR_RED);
    xy_gui_flush(&gui);
}
```

### 2. 使用 widget/event API

```c
#include "xy_gui_button.h"
#include "xy_gui_event.h"

static void on_button_click(xy_gui_widget_t *widget, xy_gui_event_t *event, void *user_data)
{
    (void)widget;
    (void)event;
    (void)user_data;
}

xy_gui_button_t button;
xy_gui_button_create(&button, 4, 4, 64, 24, "Run", XY_GUI_BUTTON_TOGGLE);
xy_gui_button_set_click_cb(&button, on_button_click, NULL);
xy_gui_button_trigger_click(&button);
```

### 3. 使用 theme API

```c
#include "xy_gui_theme.h"

xy_gui_theme_t light;
xy_gui_theme_create_light(&light);
xy_gui_theme_system_init();
xy_gui_theme_register(&light);
xy_gui_theme_apply("Light");
```

## 不应再宣称为已完成的内容

以下内容可能有历史说明、目录或部分源码，但当前没有等价的主线 CTest、真实屏幕日志或产品级验证记录，不能标记为完整：

- GUI effects/LED-screen/extended animation 全量效果闭环
- 字体资产完整度、中文字体渲染质量或字体生成流程闭环
- 与 LCD/OLED/LED matrix 真实硬件 backend 的板级渲染验证
- 触摸输入、窗口管理、多页面应用框架或完整 UI 产品流程
- 用 Display driver 的 host CTest 替代 GUI core/widget/display-backend 验证

这些项应进入 backlog 或单独 proposal；不要在 README 同步 slice 中批量改 effects、fonts 或 display backend。

## 验证命令

GUI 文档/测试收敛后至少运行：

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target \
  test_gui_core \
  test_gui_widget_theme \
  test_gui_widgets \
  test_gui_effects_headers \
  -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^gui_(core|widget_theme|widgets|effects_headers)$'

make test-unit

git diff --check
```

触碰 GUI C/H 文件时，应先运行上述 focused gate；若涉及 display backend，还需额外运行 display driver focused CTest，且不能把 host fake 结果写成真实硬件验证。

## 下一步 backlog

1. 为 GUI effects 算法写独立 focused CTest/proposal 前，先明确只覆盖 fade/blink/breath/slide/rotate 的 host-safe public contract，不把 LED-screen/硬件动画一起混入。
2. 为 GUI ↔ Display driver backend 写独立 validation proposal，区分 host fake、PC sim 与真实屏幕日志。
3. 若需要字体/中文渲染闭环，先固定字体资产范围与生成流程，再补 focused host CTest 或 snapshot smoke。
4. 只有在真实板级日志存在后，才更新硬件验证结论；当前状态保持 `host-guarded core / hardware validation pending`。
