# XinYi GUI ↔ SSD1306 Display Adapter Proposal（2026-08-10）

## 目标

在不改变 `XY_NET/GUI/Display` 既有默认策略、不接触 HAL/vendor 树、不声称真实硬件验证的前提下，给 GUI core 到 Display SSD1306 driver 的桥接方式定边界。当前 GUI 的 `test_gui_display_backend` 已证明：

- `xy_gui_t` 可以通过 `xy_gui_disp_drv_t` 回调转发 clear/draw/fill/flush；
- LED driver 的 `xy_gui_display_t` adapter 可以把 GUI display 抽象绑定到 host fake framebuffer；
- 这些都是 host contract，不等于 OLED/LCD/LED matrix 真实硬件验证。

下一步若要把 GUI 画到 SSD1306，应新增一个 **display-driver-specific adapter**，而不是让 GUI core 直接 include `xy_oled_ssd1306.h` 或让 SSD1306 driver 依赖 GUI core。

## 当前事实源

- GUI 抽象：`components/gui/xy_gui.c` + `components/gui/inc/xy_gui.h` 使用 `xy_gui_disp_drv_t`。
- 旧 display 抽象：`components/gui/inc/xy_gui_display.h` 提供 `xy_gui_display_t`，目前 LED adapter 使用固定 slot/registry 方式解决 C 函数指针无 `user_data` 的限制。
- SSD1306 driver：`components/drivers/display/oled/ssd1306/xy_oled_ssd1306.{h,c}` 已有 host CTest `display_oled_ws2812`，覆盖 init、pixel/line/refresh、buffer 和 I2C transaction contract。
- GUI display bridge：`tests/unit/gui/test_gui_display_backend.c` 当前只覆盖 generic fake backend + LED GUI adapter，不覆盖 SSD1306-specific binding。

## 设计边界

### 不做

1. 不把 `xy_oled_ssd1306_t` 放入 GUI core；GUI core 仍只认识 `xy_gui_disp_drv_t`。
2. 不把真实 I2C/HAL vendor 初始化塞进 GUI 层；SSD1306 adapter 只调用现有 SSD1306 public API。
3. 不改变 `xy_gui_display_t` 旧抽象的 ABI，除非另开 API migration proposal。
4. 不把 host fake SSD1306 测试结果写成真实 OLED 硬件验证。

### 推荐新增的小 adapter

建议新增独立文件（后续 slice）：

```text
components/gui/inc/xy_gui_ssd1306_adapter.h
components/gui/src/xy_gui_ssd1306_adapter.c
```

Public API 形状建议保持极窄：

```c
int xy_gui_ssd1306_bind(xy_gui_disp_drv_t *out_drv, xy_oled_ssd1306_t *oled);
```

语义：

- `out_drv == NULL` 或 `oled == NULL` 返回 `XY_GUI_INVALID_PARAM`；
- 成功后填充 `out_drv->draw_pixel`、`out_drv->fill_rect`、`out_drv->flush`；
- `draw_pixel` 将 RGB565/非零颜色映射为 SSD1306 `true`，`XY_GUI_COLOR_BLACK` 映射为 `false`；
- `fill_rect` 通过裁剪后多次调用 `xy_oled_ssd1306_draw_pixel()`，避免在 adapter 中直接访问 buffer 布局；
- `flush` 调用 `xy_oled_ssd1306_refresh()`；
- 若继续沿用无 `user_data` 的 `xy_gui_disp_drv_t`，adapter 需要像 LED adapter 一样使用小型 static registry/slot，确保多个 OLED 实例隔离；不要使用单一全局 `g_oled` 导致多屏串扰。

## Host CTest 计划

后续实现 slice 应新增 `test_gui_ssd1306_adapter`，路径限定为：

```text
tests/unit/gui/test_gui_ssd1306_adapter.c
tests/unit/CMakeLists.txt
components/gui/inc/xy_gui_ssd1306_adapter.h
components/gui/src/xy_gui_ssd1306_adapter.c
```

建议覆盖：

1. bind guard：NULL `out_drv` / NULL `oled` 不产生可调用回调。
2. draw_pixel：GUI red/white 写 SSD1306 buffer bit，black 清 bit，越界由 SSD1306 public API 保持 no-op。
3. fill_rect：矩形被裁剪到 SSD1306 buffer 范围内，宽高为 0/负数不写。
4. flush：调用 SSD1306 refresh，host fake I2C 捕获 column/page/data transaction。
5. 多实例隔离：两个 OLED + 两个 `xy_gui_disp_drv_t` slot 不应互相写 buffer/flush。

## 验证命令

实现 slice 后至少运行：

```bash
cmake --build build/tests/unit --target test_gui_ssd1306_adapter -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^gui_ssd1306_adapter$'
cd /home/eugene/zerozap/XinYi && make test-unit
git diff --check
```

若只做本 proposal 文档，不应声称 adapter 已实现；只需 `git diff --check` 和可选 full unit gate 证明仓库测试基线仍可运行。

## 风险与回滚

- 风险：后续若用单一 static OLED 指针实现 adapter，会破坏多屏隔离；必须用 slot/registry 或先扩展 `xy_gui_disp_drv_t` 的 user-data 机制（另开 proposal）。
- 风险：SSD1306 是 1-bit display，GUI RGB565 到 mono 的映射必须被测试固定，避免后续误以为支持灰度或 RGB。
- 回滚：删除上述 adapter 文件与 `test_gui_ssd1306_adapter` CMake 入口即可，不影响现有 `gui_display_backend`、`display_oled_ws2812` 或 LED adapter。
