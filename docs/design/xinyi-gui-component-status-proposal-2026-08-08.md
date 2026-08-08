# XinYi GUI Component Status Proposal (2026-08-08)

## Slice

把 GUI 从旧路线图里的“40% / 字体控件渲染待补”拆成可验证的小闭环：先确认当前已经存在的 host 护栏，再限定下一步只做 display-independent 的文档/测试同步，不直接批量重构 GUI 目录或引入新图形框架。

## 当前事实

- `components/gui/` 已有根 `xy_gui.c`、widget/theme/event、button/checkbox/label/progress/slider/container、font 与 draw 相关实现。
- `tests/unit/CMakeLists.txt` 已注册 3 个 GUI CTest：
  - `gui_core`：核心绘图与 object contract。
  - `gui_widget_theme`：widget base/theme contract。
  - `gui_widgets`：event queue/dispatch 与多个控件 contract。
- 当前 GUI 仍不应被标为“主线完善”：
  - `components/gui/README.md` 仍保留较早期 API 示例，例如 `xy_gui_init(display)`、`xy_gui_clear(color)` 这类与当前 host CTest 使用的 explicit context API 不完全一致的写法。
  - effects/LED screen/extended FX 子目录包含较多显示/硬件相关扩展，尚未全部纳入 active host CTest。
  - 没有真实显示设备或板级渲染记录；当前证据主要是 display-free host 单元测试。

## 建议状态

GUI 应从“基础/待开发”改为：

```text
host-guarded core / 文档与显示硬件待收口
```

含义：

1. core/widget/event 层已有 host CTest 护栏，不应再按“完全无测试”处理。
2. README/API 示例需要按当前 public header 与 CTest 契约同步。
3. effects、字体扩展、真实 display backend 仍需独立 slice，不应与 core 文档同步混在一个提交里。

## 下一步可执行 slice

优先级从低风险到高风险：

1. **README/API 同步 slice**：只改 `components/gui/README.md` 与必要的 docs/component status 表，使用当前 `xy_gui_t` context API、registered CTest 名称和 default-off 硬件边界替换旧示例。
2. **effects header self-containment probe**：新增一个只 include GUI effects public headers 的 host CTest，先暴露 header/API drift，不改具体效果算法。
3. **display backend validation proposal**：在没有真实屏幕/驱动证据前，只写 proposal 明确 LCD/OLED/LED matrix 与 GUI core 的边界。

## 本轮不做

- 不移动 `components/gui/` 目录结构。
- 不批量改 effects/LED screen 实现。
- 不引入 LVGL/uGUI 等外部 GUI 框架。
- 不声称 GUI 已经硬件验证通过。

## 验证锚点

本 proposal 对应的后续代码/文档 slice 应至少运行：

```bash
cmake --build build/tests/unit --target test_gui_core test_gui_widget_theme test_gui_widgets -j$(nproc)
cd build/tests/unit && ctest -R '^gui_(core|widget_theme|widgets)$' --output-on-failure
make test-unit
git diff --check
```
