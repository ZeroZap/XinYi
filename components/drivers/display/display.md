# XinYi Display Driver Design Note

**状态**: driver host-guarded / hardware validation pending<br>
**范围**: `components/drivers/display/`

Display driver 当前是驱动层集合，不是 GUI 总组件。它提供 OLED/LCD/LED/RGB-matrix 等硬件驱动入口，并通过 `tests/unit/display/*` 的 focused CTest 保护 host 可验证契约。

## 当前已守护范围

| 方向 | 当前护栏 |
| --- | --- |
| LCD core/SPI/I8080/ST7789 | `display_lcd` |
| SSD1306 OLED + WS2812 | `display_oled_ws2812` |
| RGB Matrix + WS2812 backing | `display_rgb_matrix` |
| Serial RGB helper public headers | `display_serial_rgb_headers` |
| LED GUI adapter | `display_led_driver` |

详细路径、源码和风险边界见同目录 `README.md`。

## 设计边界

- Display driver 可以被 GUI、示例或应用使用，但不拥有 GUI widget/font/effect 的整体设计闭环。
- Host CTest 只能证明驱动 API、transaction order、buffer/geometry/color conversion 等软件契约；不能替代真实屏幕、LED 灯带或板级电气验证。
- 新增 panel/interface/backend 前，应先补 proposal 和 focused host CTest，避免直接把未验证路径标记为完整。

## 不在本轮闭环范围

- MAX7219 / Charlieplex 的完整驱动闭环
- QSPI/RGB LCD interface
- ST7735/ILI9341/ILI9488/GC9A01 等未验证 panel
- GUI fonts/effects/widgets/rendering
- 真实硬件验证记录
