# XinYi GUI ↔ Display Backend Validation Proposal (2026-08-09)

## Slice

固定 GUI core 与 Display driver 之间的验证边界，避免把现有 display-free `gui_*` host CTest 或 display driver CTest 误写成“GUI 已经完成真实显示后端验证”。本 proposal 只写设计与验证计划，不改 GUI/Display runtime 实现，不迁移目录，也不引入外部 GUI 框架。

## 当前事实

- `components/gui/xy_gui.h` 的主线 core API 使用 `xy_gui_t` + `xy_gui_disp_drv_t` explicit context：GUI 通过 `draw_pixel`、`fill_rect`、`flush` 等回调向下游显示层输出。
- `components/gui/inc/xy_gui_display.h` 仍是较轻量的抽象显示接口，当前没有主线 CTest 证明它与 `xy_gui_disp_drv_t` 或 `components/drivers/display/*` 的桥接关系。
- `tests/unit/gui/test_gui_core.c` 使用 FFF callback fake 证明 GUI core 会调用显示回调，但 fake 不代表任何真实 LCD/OLED/LED matrix driver。
- `tests/unit/display/*` 证明 SSD1306、LCD SPI/I8080/ST7789、WS2812、RGB Matrix、LED GUI adapter 等 display driver 的 host contract，但这些测试没有绑定 `xy_gui_t` 渲染路径。
- 没有真实屏幕、板级 frame capture、逻辑分析仪 trace 或 framebuffer snapshot 记录，因此当前 GUI 硬件验证状态必须保持 pending。

## 边界定义

| 层级 | 已有证据 | 仍缺证据 |
| --- | --- | --- |
| GUI core | `gui_core` 覆盖 lifecycle、draw/clear/flush/object callback contract | 与具体 display driver 的 adapter/bridge contract |
| GUI widget/font/effects | `gui_widgets`、`gui_widget_theme`、`gui_fonts`、`gui_effects*` 覆盖 display-independent contract | 真实屏幕上的布局、字体美术质量、刷新时序 |
| Display driver | `display_*` focused CTest 覆盖各驱动 host fake transaction contract | 作为 GUI backend 被 `xy_gui_t` 调用的集成路径 |
| Board hardware | 无 | LCD/OLED/LED matrix 实物日志、scope/LA trace、照片或 frame capture |

## 推荐下一步：host-safe bridge CTest

如果后续要推进 GUI ↔ Display backend，不应先改真实 HAL 或把 GUI 标记为硬件通过；建议添加一个 default-off、host-safe 的 bridge CTest：

1. 新增小型 adapter/test fixture，将 `xy_gui_disp_drv_t` 回调转发到一个 fake display backend（例如 fake `set_pixel/fill_rect/flush` 或 display driver adapter seam）。
2. 覆盖 `xy_gui_clear()`、`xy_gui_draw_pixel()`、`xy_gui_fill_rect()`、`xy_gui_flush()` 的坐标/颜色/调用次数转发 contract。
3. 覆盖 backend 返回失败时的归一化策略；若当前 public API 不传播 backend error，应先记录现有 contract，不在同一 slice 中大改 API。
4. 若绑定已有 display driver（SSD1306/LCD/RGB matrix），只使用 host fake transport/Framebuffer，不调用真实 HAL 或 vendor SDK。
5. 更新 `components/gui/README.md` 和 `docs/design/unit-test-inventory.md`，明确该测试仍是 host bridge，不是 hardware validation。

候选目标名：

```text
test_gui_display_backend / gui_display_backend
```

## 验证命令建议

文档 proposal slice：

```bash
make test-unit
git diff --check
```

未来 bridge CTest slice：

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_gui_display_backend -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^gui_display_backend$'
make test-unit
git diff --check
```

## 本轮明确不做

- 不编辑 `MCU/` 或 `third_party/` vendor 树。
- 不把 `XY_GUI_ENABLE` 或任一 display driver 默认策略改成硬件启用。
- 不迁移 `components/gui/` / `components/drivers/display/` 目录。
- 不把 host fake、PC sim 或 compile-only 结果升级为真实屏幕验证。
- 不触碰 `tools/xy_host_tools/gui/z_serial_app.py` 等 host GUI 工具脏文件。

## 回滚方式

本 proposal 是单文件文档 slice；若方向需要调整，直接 revert 本提交或删除 `docs/design/xinyi-gui-display-backend-validation-proposal-2026-08-09.md` 即可，不影响 firmware/runtime/test 代码。
