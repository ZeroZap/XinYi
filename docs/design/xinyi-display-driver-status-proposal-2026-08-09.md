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

## 仍存在的风险

1. `components/drivers/display/README.md` 仍包含较多超前/示意 API，例如 MAX7219 matrix scan、GUI effects/fonts、多个 LCD panel 的“✅”表述；这些并不全部等价于当前编译护栏。
2. `components/drivers/display/display.md` 是空文件，无法承担设计入口职责。
3. Root `Kconfig` 与 `components/drivers/display/Kconfig` 的符号集合不完全一致；后者目前只能视为历史局部配置候选。
4. GUI 组件仍是单独的基础状态，不能因为 Display driver host CTest 存在就宣称 GUI 字体/控件/渲染全部闭环。

## 建议闭环顺序

1. **本轮**：记录本 status sync proposal，并在质量闭环文档中把 Display 从“严重不足/缺配置测试”修正为“driver host-guarded / README 待收敛”。
2. 下一轮低风险 slice：重写 `components/drivers/display/README.md` 的实现状态表，只保留已由源码和 CTest 证明的 SSD1306、LCD core/SPI/I8080/ST7789、WS2812、RGB Matrix、LED GUI adapter；把 MAX7219、Charlieplex、GUI effects/fonts 等移入 backlog 或删除超前 ✅。
3. 再下一轮：删除或改写空的 `components/drivers/display/display.md`，避免空文档入口误导。
4. 只有在 README 收敛后，才考虑新的 display 示例 smoke 或 Kconfig 生成路径验证；禁止一次性目录/API 大迁移。

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
