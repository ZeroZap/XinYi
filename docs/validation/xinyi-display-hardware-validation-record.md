# XinYi SSD1306 单一显示硬件验证记录

**建立日期**：2026-08-25  
**当前状态**：`BLOCKED_NO_HARDWARE`  
**范围**：Sprint 1 S1-04，SSD1306 单一显示纵切  
**证据边界**：本记录只接受真实板卡和显示屏证据；Host、SDL、QEMU 与 compile-only 结果不能升级为实板、性能或产品证据。

## 1. 当前阻塞探测

2026-08-25 在开发主机执行：

```text
lsusb
# 仅发现主机内置/通用 USB hub、指纹、蓝牙和摄像头；未发现开发板或调试器。

/dev/ttyACM* + /dev/ttyUSB*
# (none)
```

因此本轮未烧录固件、未连接 SSD1306、未填写任何通过结果。解除阻塞需要可识别的目标板/调试器、SSD1306 屏幕和明确接线。

## 2. 运行身份

| 字段 | 必填值 |
|---|---|
| 状态 | `PENDING` / `RUNNING` / `FAILED` / `PASSED_BASIC` / `PASSED_STRESS` |
| 操作者与日期 | pending |
| Git SHA | pending；必须是已推送的完整 SHA |
| 板卡/修订版/MCU | pending |
| 调试器与烧录工具版本 | pending |
| SSD1306 模块、分辨率与 I2C 地址 | pending |
| 供电电压 | pending |
| 固件工程/构建命令 | pending |
| 原始日志与照片目录 | pending；不得只填最终布尔值 |

## 3. 接线与配置

| 字段 | 必填值 |
|---|---|
| I2C 实例 | pending |
| SCL/SDA 引脚与复用 | pending |
| RST/其他控制引脚 | pending / not used |
| 上拉电阻 | pending |
| 总线频率 | pending |
| GUI adapter/driver 路径 | `xy_gui_ssd1306_adapter` + `drivers/display/oled/ssd1306`，或记录实际替代路径 |
| 编译器、芯片宏、优化级别 | pending |

## 4. 前置软件门禁

实板运行前记录以下命令的原始结果；通过只代表 Host/compile 层级：

```bash
cmake --build build/tests/unit --target test_gui_ssd1306_adapter test_display_oled_ws2812 -j$(nproc)
ctest --test-dir build/tests/unit --output-on-failure -R '^(gui_ssd1306_adapter|display_oled_ws2812)$'
make test-unit
make HAL_PLATFORM=STM32U5 -j$(nproc)
git diff --check
```

| 门禁 | 结果/日志 |
|---|---|
| GUI→SSD1306 adapter focused | pending |
| SSD1306 driver focused | pending |
| 全量 Host | pending |
| 目标固件 clean compile | pending |
| 固件 map/RAM 记录 | pending |

## 5. 必测步骤

每项必须记录 API 序列、返回码、串口/总线日志、屏幕照片或采集物，以及实际结果。

| ID | 场景 | 最低验收 | 实际结果/证据 |
|---|---|---|---|
| DISP-01 | init | 初始化成功；记录探测地址与初始化事务 | pending |
| DISP-02 | fill/clear | 全黑、全亮、棋盘或条纹图案正确，无明显残影 | pending |
| DISP-03 | text | ASCII 与当前 required UI 中文 glyph 可辨识、对齐、裁剪符合 Host contract | pending |
| DISP-04 | flush | framebuffer 刷新到屏幕，返回码与总线结果一致 | pending |
| DISP-05 | error injection | 断开设备或制造 NACK/timeout；错误向 GUI 调用方传播，不报告假成功 | pending |
| DISP-06 | re-init | 失败后恢复接线/总线并 deinit→init，显示恢复 | pending |
| DISP-07 | frame timing | 固定图案至少 100 次刷新，记录时钟、样本数、min/median/p95/max | pending |
| DISP-08 | RAM | 记录链接 map 的静态 RAM、栈测量方法和峰值；不得估算 | pending |
| DISP-09 | stress（可选） | 记录时长、刷新次数、总线/显示错误和最终状态 | pending |

## 6. 结果分类

只能选择一个：

- `BLOCKED_NO_HARDWARE`：缺目标板、调试器、显示屏或接线；当前分类。
- `COMPILE_ONLY`：目标构建通过，但未运行真实屏幕。
- `HARDWARE_FAILED`：已运行实板且失败，原始证据已附。
- `PASSED_BASIC`：DISP-01～DISP-06 与 DISP-08 均有真实实板证据。
- `PASSED_PERFORMANCE`：`PASSED_BASIC` 且 DISP-07 有固定硬件/频率/构建参数与统计。
- `PASSED_STRESS`：`PASSED_PERFORMANCE` 且 DISP-09 通过。

**当前分类**：`BLOCKED_NO_HARDWARE`

## 7. 声明限制与签署

在至少达到 `PASSED_BASIC` 前，GUI/SSD1306 只能声明 Host contract 和已实际完成的 target compile，不得声明 hardware-validated。只有 `PASSED_PERFORMANCE` 才能引用本记录中的帧时间；单次测量、Host timing 或 SDL dummy-video 结果不能替代屏幕性能证据。

| 字段 | 值 |
|---|---|
| 执行人 | pending |
| 复核人 | pending |
| 结论日期 | pending |
| 关联 issue/commit | pending |
| 未关闭异常 | 缺实板环境 |
