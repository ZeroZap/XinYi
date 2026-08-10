# XinYi GUI Component Status

**状态**: host-guarded core / font engine host-guarded / SSD1306 adapter host-guarded / 显示硬件待实证<br>
**事实源日期**: 2026-08-11

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
| GUI ↔ Display backend bridge | `xy_gui.c`, `xy_gui.h`, fake `xy_gui_disp_drv_t` backend fixture, LED GUI display adapter fixture | `gui_display_backend` | host-safe bridge CTest 证明 `xy_gui_clear/draw_pixel/fill_rect/flush` 会把坐标、尺寸、颜色与调用次数转发到 fake display backend，并覆盖 `xy_gui_t` 经 LED GUI display adapter 驱动 host framebuffer/flush 的集成路径、两个 LED GUI adapter channel 的 framebuffer/flush 隔离与 per-driver enable/disable 行为；同时记录当前 backend 失败被 GUI core 归一化为 `XY_GUI_OK` 的现有 contract；仍不是任何真实屏幕硬件验证 |
| GUI ↔ SSD1306 adapter | `inc/xy_gui_ssd1306_adapter.h`, `src/xy_gui_ssd1306_adapter.c`, display SSD1306 public driver API | `gui_ssd1306_adapter` | host-safe adapter CTest 覆盖 bind guard、RGB565→mono 映射、draw-line/draw-rect/draw-char callback 转发、fill-rect clipping、flush 到 SSD1306 refresh I2C transaction、多 OLED instance slot 隔离，以及 slot exhaustion/reset 契约；仍不是真实 OLED/I2C 硬件验证 |
| Widget base + theme | `src/xy_gui_widget.c`, `src/xy_gui_theme.c`, `inc/xy_gui_widget.h`, `inc/xy_gui_theme.h` | `gui_widget_theme` | widget init/style/text/value/parent-child 与 theme register/apply/list/unregister contract 已由 host 测试守护 |
| Event + widgets | `src/xy_gui_event.c`, button/checkbox/label/progress/slider/container 源码与头文件 | `gui_widgets` | event queue/dispatch 与 button、checkbox/radio、label、progress、slider、container contract 已由 host 测试守护 |
| Effects public headers | `effects/xy_gui_effect*.h` | `gui_effects_headers` | effects 统一头与各效果头文件 self-containment、公共类型/函数签名编译契约已由 host 测试守护 |
| Basic effects algorithms | `effects/xy_gui_effect_{fade,blink,breath,slide,rotate}.c` | `gui_effects` | fade/blink/breath/slide/rotate create/update/getter、NULL guard、边界 duration/period 与基础 lifecycle contract 已由 host 测试守护 |
| LED-screen extended effects headers | `effects/xy_led_screen.h`, `effects/xy_gui_screen_fx.h` | `gui_led_screen_effects` | LED-screen framebuffer/effect-engine public types、枚举和函数签名已由 host include/compile CTest 守护；该目标不链接硬件、未覆盖实现算法，也不代表真实 LED/screen 效果验证 |
| RGB extended effects compile seam | `effects/xy_rgb_fx_{extended,music,matrix,3d}.c`, serial-RGB public headers, fake RGB-strip fixture | `gui_rgb_extended_effects_compile` | music extended implementation 已由 fake `xy_rgb_*` strip seam 与 test-owned `g_frame_count` 覆盖 setter/beat/frequency/autocorr/VU 基础路径；matrix/extended/3D implementation 已覆盖 2D matrix size/plasma、color-wipe/lightning seam 与 3D plasma 基础路径；该目标只证明 compile-boundary 与低风险调用契约，不代表视觉算法质量或硬件验证 |
| Bitmap font assets | `fonts/xy_font_8x16.c`, `fonts/xy_font_16x24.c`, `fonts/xy_font_chinese_16x16.c`, `fonts/font_manifest.json` | `gui_fonts`, `gui_font_manifest` | ASCII 8x16/16x24 与 Chinese 16x16 font handle、boundary lookup、NULL/空串 measurement、基础 UTF-8 中文宽度 contract 已由 host 测试守护；manifest smoke 额外固定当前 legacy asset 范围、provenance、duplicate/placeholder inventory；字体美术质量、完整中文字库与生成流程仍不在本结论内 |
| Font engine | `src/xy_font.c`, `inc/xy_font.h` | `gui_font_engine` | host CTest 覆盖 runtime font init、glyph lookup、multi-line measurement、draw char/string/aligned text framebuffer writes、NULL/unsupported char guards、cache disable/init/cache hit/LRU replacement/clear contract；仍不代表字库美术质量或真实屏幕渲染验证 |

这些测试说明 GUI 已不是“完全无测试/待开发”的空白组件；后续只能按真实失败补小回归，或为尚未纳入测试的 RGB 扩展实现效果、display-backend 写独立 proposal/CTest。

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
- 字体资产完整度、中文字体渲染质量、完整字库覆盖或字体生成流程闭环
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
  test_gui_display_backend \
  test_gui_ssd1306_adapter \
  test_gui_widget_theme \
  test_gui_widgets \
  test_gui_effects \
  test_gui_effects_headers \
  test_gui_led_screen_effects \
  test_gui_rgb_extended_effects_compile \
  test_gui_fonts \
  test_gui_font_manifest \
  test_gui_font_engine \
  -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^gui_(core|display_backend|ssd1306_adapter|widget_theme|widgets|effects|effects_headers|led_screen_effects|rgb_extended_effects_compile|fonts|font_manifest|font_generator_manifest|font_generator_output|font_engine)$'

make test-unit

git diff --check
```

触碰 GUI C/H 文件时，应先运行上述 focused gate；若涉及 display backend，还需额外运行 display driver focused CTest，且不能把 host fake 结果写成真实硬件验证。

## 下一步 backlog

1. GUI ↔ Display driver backend 已新增 host-safe `test_gui_display_backend` / `gui_display_backend` 与 `test_gui_ssd1306_adapter` / `gui_ssd1306_adapter` CTest：当前证明 `xy_gui_disp_drv_t` fake backend 转发 contract、LED GUI display adapter host framebuffer/flush 绑定路径、SSD1306 adapter 的 mono 映射/flush/多实例隔离/slot reset，以及失败归一化现状，不代表真实 LCD/OLED/LED matrix hardware validation。后续若要接更多具体 display driver adapter，应继续保持 host fake transport/framebuffer，不直接改 HAL/vendor 或默认启用硬件路径。
2. GUI LED-screen/RGB extended effects 已由 `docs/design/xinyi-gui-led-screen-effects-proposal-2026-08-09.md` 固定边界，且 `gui_led_screen_effects` 已先补 public-header self-containment CTest：当前只证明 `xy_led_screen.h` / `xy_gui_screen_fx.h` 的 host include/type/signature contract，不链接 LED-screen 实现、不代表 RGB 扩展算法或真实屏幕效果验证；若继续推进，应再补独立 host fake framebuffer implementation CTest。
3. RGB extended effect implementation (`xy_rgb_fx_extended/matrix/3d/music.c`) 已由 `docs/design/xinyi-gui-rgb-extended-effects-compile-proposal-2026-08-09.md` 明确为 compile-boundary 优先：当前 `gui_rgb_extended_effects_compile` 已纳入全部 4 个 RGB extended implementation 文件，确认 fake `xy_rgb_*` strip seam、test-owned `g_frame_count`、delay/color helper seam 可守护 music setter/beat/frequency/autocorr/VU、matrix size/plasma、extended color-wipe/lightning 与 3D plasma 基础路径；不要把本目标解读为视觉算法质量或硬件效果已验证。
4. GUI font asset manifest 已由 `gui_font_manifest` 追加 host CTest，证明当前 legacy asset 范围/重复/placeholder inventory 与 manifest contract 对齐；generator bootstrap 已追加 `gui_font_generator_manifest` / `gui_font_generator_output`，证明 manifest validation、deterministic summary 与 manifest-inventory header preview contract；若需要字体/中文渲染进一步闭环，下一步应实现 `.c/.h` 写入或 host snapshot review；不要把这些 host 测试等同于美术/字库质量验收。
5. 只有在真实板级日志存在后，才更新硬件验证结论；当前状态保持 `host-guarded core / hardware validation pending`。
