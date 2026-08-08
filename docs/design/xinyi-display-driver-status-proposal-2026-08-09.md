# XinYi Display Driver Status Sync Proposal (2026-08-09)

## 背景

旧完整度报告仍把 `display/` 描述为顶层 `components/display` 组件，且说 Kconfig/CMake/测试缺失。当前仓库事实已经变化：显示驱动实际位于 `components/drivers/display/`，并且根 `Kconfig`、`components/drivers/CMakeLists.txt` 与 `tests/unit/display/*` 已经提供主线护栏。

本 proposal 的目标不是立即重构显示目录或批量补齐 GUI，而是先固定当前 Display driver 的真实边界，避免后续闭环任务继续按旧报告重复选择“缺 Kconfig/CMake/测试”的基线补齐项。

## 当前事实

### 路径与构建

- 驱动根目录：`components/drivers/display/`
- 根构建入口：`components/drivers/CMakeLists.txt`
- 子目录构建入口：`components/drivers/display/CMakeLists.txt`
- 根 Kconfig 事实源：root `Kconfig` 中的 `DRIVER_DISPLAY` 及子选项：
  - `DRIVER_DISPLAY_OLED`
  - `DRIVER_DISPLAY_SSD1306`
  - `DRIVER_DISPLAY_LCD`
  - `DRIVER_DISPLAY_LCD_SPI`
  - `DRIVER_DISPLAY_LCD_I8080`
  - `DRIVER_DISPLAY_LCD_ST7789`
  - `DRIVER_DISPLAY_LED`
  - `DRIVER_DISPLAY_LED_SERIAL_RGB`
  - `DRIVER_DISPLAY_RGB`

`components/drivers/display/Kconfig` 仍是组件局部/历史配置候选，不应替代 root `Kconfig` 作为当前生成配置事实源。

### 已有 host CTest 护栏

`tests/unit/CMakeLists.txt` 已注册 5 个 display-focused host CTest：

| CTest | 覆盖范围 |
| --- | --- |
| `display_lcd` | LCD core、SPI、I8080、ST7789 host transaction/geometry contracts |
| `display_oled_ws2812` | SSD1306 OLED framebuffer/I2C flow 与 WS2812 GPIO/byte-order contracts |
| `display_rgb_matrix` | RGB matrix 坐标映射、亮度缩放、效果更新与 WS2812 backing contracts |
| `display_serial_rgb_headers` | serial RGB helper public headers self-containment |
| `display_led_driver` | LED driver GUI adapter forwarding/registry contracts |

这些测试说明 Display driver 已不是“无测试/无 CMake/Kconfig”的空白组件；后续只能按真实失败补小回归或补明确缺口。

## 当前已收敛内容与仍存在的风险

1. `components/drivers/display/README.md` 已收敛为实现状态表，区分 host-guarded 软件契约、未验证 panel/backlog 与真实硬件验证缺口。
2. `components/drivers/display/display.md` 已从空入口收敛为设计边界说明，指向 README 的详细源码/CTest 事实源。
3. Root `Kconfig` 与 `components/drivers/display/Kconfig` 的符号集合不完全一致；后者目前只能视为历史局部配置候选。
4. GUI 组件仍是单独的基础状态，不能因为 Display driver host CTest 存在就宣称 GUI 字体/控件/渲染全部闭环。

## 建议闭环顺序

1. **已完成**：记录本 status sync proposal，并在质量闭环文档中把 Display 从“严重不足/缺配置测试”修正为“driver host-guarded / README/display.md 已收敛”。
2. **已完成**：重写 `components/drivers/display/README.md` 的实现状态表，只保留已由源码和 CTest 证明的 SSD1306、LCD core/SPI/I8080/ST7789、WS2812、RGB Matrix、LED GUI adapter；把 MAX7219、Charlieplex、GUI effects/fonts 等移入 backlog/未验证范围。
3. **已完成**：改写 `components/drivers/display/display.md`，避免空文档入口误导。
4. 后续只有在新增 panel/interface、补真实硬件记录或发现具体 display CTest 失败时再推进；禁止一次性目录/API 大迁移。

## 验证命令

本 proposal 是文档/状态同步 slice，建议验证：

```bash
make test-unit

git diff --check
```

若后续触碰 display C/H 文件，应额外运行：

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_display_lcd test_display_oled_ws2812 test_display_rgb_matrix test_display_serial_rgb_headers test_display_led_driver -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(display_lcd|display_oled_ws2812|display_rgb_matrix|display_serial_rgb_headers|display_led_driver)$'
```
