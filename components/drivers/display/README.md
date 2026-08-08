# XinYi Display Driver Status

**状态**: driver host-guarded / README 收敛中<br>
**事实源日期**: 2026-08-09

本目录记录 `components/drivers/display/` 的当前实现与验证边界。它不是 GUI 组件总说明，也不宣称真实显示硬件已经通过验证。

## 当前边界

- 驱动目录：`components/drivers/display/`
- 根构建入口：`components/drivers/CMakeLists.txt`
- Display 子构建入口：`components/drivers/display/CMakeLists.txt`
- 当前根 Kconfig 事实源：root `Kconfig` 中的 `DRIVER_DISPLAY*` 选项
- 当前 host 单元测试事实源：`tests/unit/display/*` 与 `tests/unit/CMakeLists.txt`

`components/drivers/display/Kconfig` 保留为历史/局部配置候选；生成配置仍以 root `Kconfig` 为准。

## 已有源码与 host CTest 护栏

| 范围 | 源码路径 | Host CTest | 当前结论 |
| --- | --- | --- | --- |
| LCD core | `lcd/xy_lcd.c`, `lcd/xy_lcd.h` | `display_lcd` | core 初始化、几何与通用绘制契约已由 host 测试守护 |
| LCD SPI | `lcd/xy_lcd_spi.c`, `lcd/xy_lcd_spi.h` | `display_lcd` | SPI transaction、RGB565 byte order、reset/backlight/DMA 路径已由 host 测试守护 |
| LCD I8080 | `lcd/xy_lcd_i8080.c`, `lcd/xy_lcd_i8080.h` | `display_lcd` | 并口写/read/strobe 顺序已由 host 测试守护 |
| ST7789 | `lcd/xy_lcd_st7789.c`, `lcd/xy_lcd_st7789.h` | `display_lcd` | offset/window/draw-pixel/inversion 契约已由 host 测试守护 |
| SSD1306 OLED | `oled/ssd1306/xy_oled_ssd1306.c`, `oled/ssd1306/xy_oled_ssd1306.h` | `display_oled_ws2812` | framebuffer、I2C init/write/refresh/clear 契约已由 host 测试守护 |
| WS2812 | `led_drivers/serial_rgb/xy_ws2812.c`, `led_drivers/serial_rgb/xy_ws2812.h` | `display_oled_ws2812`, `display_rgb_matrix` | GPIO bitbang、brightness、RGB/GRB/RGBW byte-order 契约已由 host 测试守护 |
| RGB Matrix | `led_drivers/serial_rgb/xy_rgb_matrix.c`, `led_drivers/serial_rgb/xy_rgb_matrix.h` | `display_rgb_matrix` | 坐标映射、亮度缩放、基础效果更新契约已由 host 测试守护 |
| Serial RGB helper headers | `led_drivers/serial_rgb/xy_rgb_*.h`, `xy_serial_rgb.h` | `display_serial_rgb_headers` | public header self-containment 已由 host 测试守护 |
| LED GUI adapter | `led_drivers/xy_led_driver.c`, `led_drivers/xy_led_driver.h` | `display_led_driver` | adapter registry 与 `set_pixel/get_pixel/fill_rect/flush` forwarding 已由 host 测试守护 |

## 不应再宣称为已完成的内容

以下内容在本目录中可能存在历史头文件、计划文档或示意代码，但当前没有等价的主线 CTest/构建护栏，不能标记为“完整”或“硬件已验证”：

- MAX7219 / matrix-scan 点阵驱动闭环
- Charlieplex 驱动闭环
- QSPI/RGB LCD interface 闭环
- ST7735、ILI9341、ILI9488、GC9A01 等 panel 驱动闭环
- GUI effects、GUI fonts、GUI widget/rendering 闭环
- 真实 OLED/LCD/LED 硬件验证结果

这些项应进入 backlog 或单独 proposal；不要用当前 display driver host CTest 代替 GUI 或真实硬件证据。

## 验证命令

Display driver 文档/测试收敛后至少运行：

```bash
make test-unit

git diff --check
```

触碰 Display C/H 文件时，应先运行 focused gate：

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target \
  test_display_lcd \
  test_display_oled_ws2812 \
  test_display_rgb_matrix \
  test_display_serial_rgb_headers \
  test_display_led_driver \
  -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(display_lcd|display_oled_ws2812|display_rgb_matrix|display_serial_rgb_headers|display_led_driver)$'
```

## 下一步 backlog

1. 用真实源码/CTest 继续收敛 `display.md`，避免空文档入口误导。
2. 若需要启用 MAX7219、Charlieplex 或新 panel，先写独立 proposal，再补 focused host CTest。
3. 若需要 GUI 字体/控件/渲染闭环，应在 `components/gui` 与 `tests/unit/gui` 方向独立推进，不混入 Display driver 提交。
