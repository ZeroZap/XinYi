# XinYi 组件完整度分析报告与修复计划

**版本**: 1.0.0
**日期**: 2026-05-01
**状态**: 分析完成，待修复

---

## 📊 分析范围

本报告分析了 XinYi 框架的所有主要组件，评估以下维度：

- 目录结构完整性
- 配置文件存在性（README.md, Kconfig, CMakeLists.txt）
- 核心实现完成度
- 示例代码
- 测试用例
- 文档质量

---

## 📋 组件评分总览

| 组件       | 完整度 | 状态     | Kconfig | README | 示例 | 测试 |
| ---------- | ------ | -------- | ------- | ------ | ---- | ---- |
| sensor     | 95% 🟢 | 优秀     | ✅      | ✅     | ✅   | ❌   |
| crypto     | host-guarded / 文档已补齐 | 需安全审查 | ✅      | ✅     | ⚠️   | ✅   |
| fota       | 90% 🟢 | 主线可用 / 硬件验证待证据 | ✅      | ✅     | ✅   | ✅   |
| dm         | host-guarded / 文档已补齐 | README 已收敛 | ✅      | ✅     | ⚠️   | ✅   |
| gui        | host-guarded core / 硬件待验证 | core/widget/event/theme 已有 CTest | ✅      | ✅     | ❌   | ✅   |
| pm         | host-guarded / 功耗待实证 | 文档已补齐 | ✅      | ✅     | ⚠️   | ✅   |
| net        | host-guarded / 硬件待验证 | LTE/CAN/MQTT/AT/Modbus 护栏已收敛 | ✅      | ✅     | ⚠️   | ✅   |
| pid        | 85% 🟢 | 可用     | ✅      | ✅     | ✅   | ✅   |
| display    | driver host-guarded / 硬件待验证 | README 已收敛 | ✅      | ✅     | ⚠️   | ✅   |
| actuator   | 80% 🟢 | 可用     | ✅      | ✅     | ✅   | ✅   |
| mux        | 100% 🟢 | 主线完善 | ✅      | ✅     | ✅   | ✅   |
| fuel_gauge | 90% 🟢 | 主线可用 / 硬件验证待证据 | ✅      | ✅     | ⚠️   | ✅   |
| charger    | 65% 🟡 | 已弃用   | ❌      | ✅     | ❌   | ❌   |

---

## 🔴 高优先级问题（必须修复）

### 1. display/ - 显示驱动组件（driver host-guarded / 硬件待验证）

#### 当前状态

- **当前路径事实源**：显示驱动位于 `components/drivers/display/`，不是旧报告中的空白顶层 `components/display/` 基线。
- **构建配置已存在**：root `Kconfig` 提供 `DRIVER_DISPLAY*` 选项，`components/drivers/CMakeLists.txt` 与 `components/drivers/display/CMakeLists.txt` 提供构建入口。
- **README 已收敛**：`components/drivers/display/README.md` 已改为状态表，明确区分 host-guarded 软件契约、未验证 panel/backlog 与真实硬件验证缺口。
- **host CTest 已存在**：`display_lcd`、`display_oled_ws2812`、`display_rgb_matrix`、`display_serial_rgb_headers`、`display_led_driver` 覆盖 LCD/OLED/WS2812/RGB Matrix/LED adapter 的当前软件契约。
- **边界**：这些测试不能替代 GUI fonts/widgets/rendering 闭环，也不能替代真实屏幕/LED 硬件验证记录。

#### 剩余修复计划

| 序号 | 任务 | 工作量 | 优先级 |
| ---- | ---- | ------ | ------ |
| D1   | 保持 README/display.md 与真实源码、root Kconfig、display CTest 同步 | - | 已完成 |
| D2   | 新增 MAX7219、Charlieplex、QSPI/RGB LCD 或新 panel 前先写独立 proposal，并补 focused host CTest | 2–4h/项 | 🟡 中 |
| D3   | 真实 OLED/LCD/LED 硬件验证记录 | 硬件驱动 | 🟡 中 |
| D4   | GUI fonts/effects/widgets/rendering 在 `components/gui` 独立推进，不混入 Display driver | 实证驱动 | 🟡 中 |

**预计剩余工时**: 实证驱动；不再按“缺 Kconfig/CMake/测试”的旧基线重复开工。

---

### 2. gui/ - 图形界面组件 (host-guarded core / 硬件待验证)

#### 问题描述

- **Kconfig/CMake 已接入**：`components/gui/CMakeLists.txt` 已存在，root `Kconfig` 当前提供 `GUI_ENABLED`、`GUI_SDL`、`GUI_TFT`、`GUI_LVGL` 与 `GUI_WIDGETS` 等生成配置入口。
- **核心护栏已存在**：`tests/unit/gui/test_gui_core.c`、`test_gui_widget_theme.c`、`test_gui_widgets.c` 已注册为 `gui_core` / `gui_widget_theme` / `gui_widgets` CTest，覆盖 core、widget、event、theme 与主要控件契约。
- **README 已同步当前 API**：`components/gui/README.md` 已改为显式 `xy_gui_t` context API 示例，避免旧 `xy_gui_init(display)` / 全局绘图调用误导。
- **剩余风险**：effects、fonts、display backend 与真实屏幕渲染仍需独立 proposal/验证；不能用当前 host CTest 替代硬件证据。

#### 修复计划

| 序号 | 任务 | 工作量 | 优先级 |
| ---- | ---- | ------ | ------ |
| G1   | 添加 Kconfig/CMake 基线 | - | 已完成 |
| G2   | core/widget/event/theme host CTest | - | 已完成 |
| G3   | README/API 示例同步 | - | 已完成 |
| G4   | effects header self-containment probe | 1–2h | 🟡 中 |
| G5   | display backend validation proposal | 1–2h | 🟡 中 |
| G6   | 字体资产/中文渲染验证计划与 focused test | 2–4h | 🟡 中 |

**预计剩余工时**: 实证驱动；仅在 effects/fonts/display backend 有明确验证目标时推进。

---

### 3. net/ - 网络组件 (host-guarded / 硬件待验证)

#### 问题描述

- **MQTT 主线已迁移**：活跃实现为 `src/xy_mqtt_client.c`/`src/xy_mqtt_client.h`，已覆盖 CONNECT/CONNACK、QoS0/1 publish、SUBACK/UNSUBACK、inbound publish callback 与 keepalive；旧 `xy_mqtt/` 子树仅作历史规划材料
- **CAN/LTE 状态分化**：CAN 已有 FIFO/timeout/output-preservation host coverage 且仍保持 direct opt-in；LTE 已有 fake AT seam、callback UART adapter、default-off HAL UART adapter、smoke skeleton 与 STM32U5 compile probe，但仍需真实 UART/modem/flow-control 硬件证据
- **统一 README 已补齐**：`components/net/README.md` 现为组件入口，包含模块状态、MQTT/CAN/LTE/AT 活跃入口与 focused verification
- **AT 模块混乱**：依赖多个第三方仓库，组织结构复杂

#### 修复计划

| 序号 | 任务                                                  | 工作量 | 优先级 |
| ---- | ----------------------------------------------------- | ------ | ------ |
| N1   | 维护活跃 MQTT client host coverage，后续只按真实失败补契约 | 1–2h/次 | 🟡 低  |
| N2   | CAN 继续保持 default-off/direct-opt-in，只有产品决策明确时再做 umbrella 默认接入 proposal | 实证驱动 | 🟡 中  |
| N3   | LTE 等待真实 UART/modem/flow-control 验证记录；host fake/HAL adapter 不替代硬件证据 | 硬件驱动 | 🔴 高  |
| N4   | 保持 `components/net/README.md` 与主线实现同步          | -      | 已完成 |
| N5   | 整理 AT 模块依赖边界，避免把 vendor-style 树默认编入主库 | 8h     | 🟡 中  |
| N6   | 补充活跃 API 的最小 host smoke 示例                    | 4h     | 🟡 中  |
| N7   | 针对 CAN/LTE/AT 的真实失败追加小回归测试                | 1–2h/次 | 🟡 中  |

**预计剩余工时**: 硬件验证/产品接入决策驱动；不再按“无 README/无测试/LTE 无 adapter”重复开工。

---

## 🟠 中优先级问题（应该修复）

### 4. actuator/ - 执行器组件 (80%)

#### 问题描述

- **Kconfig 已接入但存在兼容符号**：根 `Kconfig` 同时保留 `COMPONENT_ACTUATOR` 和旧 `XY_ACTUATOR_ENABLE`，组件 CMake 接受两者生成变量
- **README.md 已补齐**：包含 API、Kconfig/CMake、测试入口和覆盖范围说明
- **示例已接入 build-guarded smoke**：`components/actuator/examples/example_relay_servo_pwm.c` 已通过 `actuator_example_relay_servo_pwm` CTest 保持 public API 对齐
- **测试已接入 active Unity/CTest**：`tests/unit/actuator/test_actuator_framework.c` 覆盖框架、relay/servo/PWM、批处理和 helper guard
- **目录结构扁平**：所有代码在根目录，未使用 src/ 子目录

#### 修复计划

| 序号 | 任务                 | 工作量 | 优先级 |
| ---- | -------------------- | ------ | ------ |
| A1   | 创建 README.md       | -      | 已完成 |
| A2   | 添加 Kconfig         | -      | 已完成 |
| A3   | 添加构建护栏示例代码 | -      | 已完成 |
| A4   | 维护/增强 host 单测  | -      | 持续维护 |
| A5   | 重构为 src/ 目录结构 | 4h     | 🟡 低  |

**预计剩余工时**: 4h（仅剩低优先级目录结构整理；README/Kconfig/示例/测试基线已关闭）

---

### 5. pid/ - PID 控制组件 (85%)

#### 问题描述

- **历史 auto-tune bug 已修复**：当前 `xy_pid_auto.c` 调用 `xy_pid_auto_calc_zn(tuner)` / `xy_pid_auto_calc_imc(tuner)`，未再出现旧 `ltc2945` 误引用
- **独立示例已补齐**：`components/pid/examples/` 下已有 basic、incremental、temperature、charging、auto_tune 示例源文件与示例 CMake 入口
- **单元测试已接入 active Unity/CTest**：`tests/unit/pid/test_pid_core.c` 与 `test_pid_auto.c` 覆盖核心 PID 与自整定契约，并已覆盖 tick wraparound 前进计算
- **示例 smoke 护栏已接入**：`pid_example_basic`、`pid_example_incremental`、`pid_example_auto_tune`、`pid_example_temperature`、`pid_example_charging` CTest 已将 `components/pid/examples/` 下全部示例编入 host unit suite，防止示例 API 漂移
- **剩余风险**：后续 PID 工作可转向小范围契约 hardening 或示例运行时输出约束，不再需要继续补示例编译入口

#### 修复计划

| 序号 | 任务                                      | 工作量 | 优先级 |
| ---- | ----------------------------------------- | ------ | ------ |
| P1   | 修复 xy_pid_auto_calc_zn 中的 ltc2945 bug | -      | 已完成 |
| P2   | 补充示例代码（独立文件）                  | -      | 已完成 |
| P3   | 添加单元测试                              | -      | 已完成 |
| P4   | 更新 README 与实际实现一致                | -      | 已完成 |
| P5   | 将基础示例纳入 host smoke CTest           | -      | 已完成 |
| P6   | 按需追加其它示例 host smoke CTest         | 1–2h/个 | 🟡 低  |

**预计工时**: 2h

---

### 6. mux/ - 多路复用组件 (100%)

#### 问题描述

- **公共头文件/API 已补齐**：`xy_mux.h` 与 GPIO/I2C/SPI/UART typed capability headers 已有实际 API 声明，旧“空头文件”问题关闭。
- **README 已补齐核心入口**：包含核心 manager、typed GPIO/I2C/SPI/UART API、配置结构、使用示例、CMake/Kconfig 状态与 host 验证契约。
- **active Unity/CTest 已接入**：`tests/unit/mux/` 下 `test_mux_core/gpio/i2c/spi/uart.c` 均已注册到主线 unit suite，当前库存显示 `mux` 组件 5 个 Unity 测试文件、0 个 raw assert、0 个 unwired 源文件。
- **build-guarded 示例已补齐**：`components/mux/examples/example_mux_basic.c` 已作为 `mux_example_basic` CTest 纳入 `make test-unit`，覆盖 GPIO/I2C/SPI/UART public API smoke。
- **组件构建入口已闭环**：`components/mux/CMakeLists.txt` 在 `COMPONENT_MUX` 默认启用时产出 `xy_mux` / `mux_component` target，根构建可发现并编译该组件。
- **剩余维护口径**：后续只围绕明确 packet/typed ops 契约 hardening、真实失败回归或硬件/上位机集成需求推进，不再重复补“无测试/无示例”基线。

#### 修复计划

| 序号 | 任务 | 工作量 | 优先级 |
| ---- | ---- | ------ | ------ |
| M1   | 补充空头文件的实际 API 实现 | - | 已完成 |
| M2   | 补充示例代码并纳入 host smoke/compile 护栏 | - | 已完成 |
| M3   | 添加测试用例 | - | 已完成 |
| M4   | 完善 README.md | - | 已完成 |
| M5   | 维护 packet/typed ops 边界回归测试 | 1–2h/次 | 按真实失败触发 |

**预计剩余工时**: 按真实失败或新增集成需求维护，不再作为基线补齐 backlog。

---

### 7. fota/ - 固件升级组件 (90%)

**状态**: 主线可用 / 硬件验证待证据；`xy_fota` / `fota_component` 根构建入口、root `Kconfig` 的 `FOTA_*` 配置、`components/fota/README.md`、`fota_core` host Unity/CTest 和 `fota_smoke_example` host-safe public flow smoke 已闭环。`xy_fota_flash.h` 已提供 flash/NOR 抽象声明，external Flash 通过 `xy_fota_set_flash_ops()` / `xy_fota_set_backup_flash_ops()` callback seam 接入，不依赖不存在的 board-specific NOR 源文件；真实 bootloader/board NOR/hardware 结果仍必须来自板级验证记录。

#### 剩余修复计划

| 序号 | 任务 | 工作量 | 优先级 |
| ---- | ---- | ------ | ------ |
| F1   | README 使用指南 | - | 已完成 |
| F2   | `fota_core` host lifecycle/CRC/download/rollback/flash-op 契约测试 | - | 已完成 |
| F3   | 补充 host-safe build-guarded public example，使用 fake Flash callback，不访问真实硬件 | - | 已完成 |
| F4   | 若要接真实 external NOR backend，先提供 board/project 验证记录与独立 backend proposal | 4h | 🟡 低 |

**预计剩余工时**: 4h（仅剩真实 board/project backend proposal 与硬件验证记录；不再用 host fake smoke 替代硬件证据）

---

### 8. pm/ - 电源管理组件 (host-guarded / 功耗待实证)

#### 问题描述

- **README 已补齐并同步边界**：`components/pm/README.md` 记录 PM framework、host CTest 与 standalone Fuel Gauge 独立维护边界。
- **Kconfig/CMake 已存在**：`components/pm/Kconfig` 提供 `XY_PM_ENABLE`、charger/fuel-gauge 兼容选项，`components/pm/CMakeLists.txt` 产出 `xy_pm` target。
- **host CTest 已存在**：`pm_component` 覆盖 PM lifecycle、ADC fallback、charger state、PM-local fuel-gauge wrapper；`pm_platform_fallback` 覆盖 fallback platform/tick/charger hook 契约。
- **剩余风险**：`xy_pm_enter_sleep()` / `xy_pm_enter_shutdown()` 仍是框架入口；真实低功耗、charger GPIO、ADC 通道、电池曲线与整机功耗必须依赖 board/project 验证记录。
- **产品边界**：standalone `components/fuel_gauge/` 保持独立，不因 PM README 或 PM-local wrapper 覆盖而回并 PM。

#### 修复计划

| 序号 | 任务 | 工作量 | 优先级 |
| ---- | ---- | ------ | ------ |
| PM1  | 创建 README.md 并同步 host/硬件边界 | - | 已完成 |
| PM2  | 保持 Kconfig/CMake 与当前 PM framework 对齐 | - | 已完成 |
| PM3  | 维护 `pm_component` / `pm_platform_fallback` host CTest | - | 持续维护 |
| PM4  | 真实低功耗/charger GPIO/ADC/电池曲线验证记录 | 硬件驱动 | 🟡 中 |
| PM5  | 若要新增 board-specific PM backend，先写 proposal + host seam | 2–4h | 🟡 中 |

**预计剩余工时**: 硬件/板级实证驱动；不再按“缺 README/缺测试”的旧基线重复开工。

---

### 9. dm/ - 数据管理组件 (host-guarded / 文档已补齐)

#### 当前状态

- **统一 README 已补齐**：`components/dm/README.md` 现在记录 root `COMPONENT_DM` / `xy_dm` 构建入口、子模块路径、host CTest 名称与硬件验证边界。
- **主线 host CTest 已存在**：`tests/unit/dm/` 下 `dm_base64`、`dm_tlv`、`dm_nvm`、`dm_factory`、`dm_fee`、`dm_corejson` 已纳入 `make test-unit`。
- **根构建入口已存在**：`components/dm/CMakeLists.txt` 在 `XY_COMPONENT_DM` 启用时产出 `xy_dm`，默认包含 FS/JSON abstraction，并按生成配置条件接入 NOR/FlashDB glue。
- **剩余风险**：FS/JSON abstraction 暂无 dedicated focused CTest；NOR/FlashDB 真实擦写、掉电与板级 timing 仍必须等待硬件验证记录，不能用 host fake 结果替代。

#### 剩余修复计划

| 序号 | 任务 | 工作量 | 优先级 |
| ---- | ---- | ------ | ------ |
| DM1  | 创建统一 `components/dm/README.md` 入口 | - | 已完成 |
| DM2  | 维护 `tests/unit/dm/` 6 个 host CTest 契约 | - | 持续维护 |
| DM3  | 为 FS/JSON abstraction 补 focused host CTest（仅在成为活跃 public dependency 时） | 2–4h | 🟡 中 |
| DM4  | NOR/FlashDB 真实硬件验证记录模板与 board log | 硬件驱动 | 🟡 中 |
| DM5  | 历史规划文档与当前 layout 的 docs-only reconciliation | 1–2h | 🟡 低 |

**预计剩余工时**: 实证/真实失败驱动；不再按“无统一 README/测试缺失”的旧基线重复开工。

---

### 10. crypto/ - 加密组件 (host-guarded / 文档已补齐)

#### 当前状态

- **统一 README 已补齐**：`components/crypto/README.md` 现在是组件闭环入口，记录 root `xy_tiny_crypto` target、root `COMPONENT_CRYPTO` 默认关闭策略、active CTest 与安全边界。
- **host CTest 已存在**：`tests/unit/crypto/` 下 10 个 Unity/CTest 目标覆盖 CRC、RNG/CSPRNG、Base64/Hex、MD5/SHA-256、AES/HMAC/SM3/SM4/ChaCha20、SM2、LWC/Ascon、Curve25519 generic 与 Cortex-M0 fallback。
- **实现位置已明确**：Base64/Hex、hash、AES/HMAC、SM 系列与 25519 等实现位于 module 目录和历史 `src/` aggregate copy 中；后续不应再按“Base64/Hex 源码位置不明”重复开工。
- **剩余风险**：重复源码 ownership、`xy_tiny_crypto` 历史 target 命名、placeholder-grade 算法安全等级、硬件加速与第三方来源审查仍需独立 proposal/验证，不能由 host CTest 直接替代。

#### 修复计划

| 序号 | 任务                                   | 工作量 | 优先级 |
| ---- | -------------------------------------- | ------ | ------ |
| C1   | 定位或补充 Base64/Hex 实现             | -      | 已完成 |
| C2   | 维护 10 个 active crypto host CTest     | -      | 持续维护 |
| C3   | 补安全等级/来源/provenance review plan | 1–2h   | 🟡 中  |
| C4   | 重复源码 ownership 与 target 命名 proposal | 1–2h | 🟡 低  |
| C5   | active public API 的 host-safe smoke 示例 | 1–2h/个 | 🟡 低  |

**预计剩余工时**: 实证/审查驱动；不再按“无 Base64/Hex / 无 SM 测试 / 无 README”旧基线重复开工。

---

## 🟡 低优先级问题（建议修复）

### 11. fuel_gauge/ - 电量计组件 (90%)

**状态**: 主线可用，保留 standalone API；已补齐组件 README/Kconfig/CMake、`xy_fuel_gauge` 根构建目标和 6 个 host Unity/CTest 目标。历史“已弃用”结论已过期，当前通过 `xy_fuel_gauge` 组件库接入主线；真实 SMBus/I2C 硬件验证仍必须等待板级证据。

| 序号 | 任务 | 工作量 | 优先级 |
| ---- | ---- | ------ | ------ |
| FG1  | 保持 standalone `components/fuel_gauge` 与 PM 旧路径边界清晰 | - | 已完成 |
| FG2  | 维护 driver/core/board-smoke host coverage：`fuel_gauge_core`、`fg_bq27z746`、`fg_bq40z50`、`fg_max17043`、`fg_bq27z561`、`fuel_gauge_smbus_hardware_smoke_example` | - | 已完成 |
| FG3  | 补充真实 SMBus/I2C 硬件验证记录：clock stretching、放电期 NACK/retry、告警阈值硬件编程；记录必须保持 pending，直到有真实板级日志 | 2h | 🟡 低 |
| FG4  | 后续如迁移到 `components/drivers/power/`，先产出兼容 proposal，不直接批量移动 API/目录 | 2h | 🟡 低 |

---

### 12. charger/ - 充电器组件 (65%)

**状态**: 已弃用，v3.0 将移除

| 序号 | 任务                              | 工作量 | 优先级 |
| ---- | --------------------------------- | ------ | ------ |
| CH1  | 确认迁移到 drivers/power/charger/ | -      | 已完成 |
| CH2  | 添加 DEPRECATED.md 说明           | 1h     | 🟡 低  |

---

## 📊 修复工时汇总

| 优先级   | 组件       | 工时     |
| -------- | ---------- | -------- |
| 🟡 中    | display    | 实证驱动 |
| 🔴 高    | gui        | 48h      |
| 🔴 高    | net        | 52h      |
| 🟠 中    | actuator   | 20h      |
| 🟠 中    | pid        | 2h       |
| 🟠 中    | mux        | 22h      |
| 🟠 中    | fota       | 6h       |
| 🟠 中    | pm         | 实证驱动 |
| 🟠 中    | dm         | 25h      |
| 🟠 中    | crypto     | 审查驱动 |
| 🟡 低    | fuel_gauge | 4h       |
| 🟡 低    | charger    | 1h       |
| **总计** |            | **304h** |

---

## 🎯 修复执行计划

### 第一阶段（1-2 周）- 清理和文档

1. 继续保持 Display driver README/display.md 与真实源码、root Kconfig、host CTest 同步；新 panel/interface 先 proposal 后 CTest
2. 为仍缺失 README 的组件添加文档（PM 已完成）
3. 添加 Kconfig 到缺失的组件

### 第二阶段（3-4 周）- 核心功能修复

1. 将低风险示例纳入 host smoke 构建护栏
2. 补充 mux/ 空头文件的 API
3. 补充 fota/ 的 xy_fota_flash.h
4. Crypto 后续先做安全/provenance review 或 duplicate-source ownership proposal，不直接批量整理代码组织

### 第三阶段（5-8 周）- 示例和测试

1. 为所有组件添加示例代码
2. 为所有组件添加测试用例
3. 补充 net/ 的 MQTT 实现

### 第四阶段（9-12 周）- 完善功能

1. 完成 gui/ 的 effects/ 和 fonts/
2. 按 proposal 与 focused CTest 推进 Display 新 panel/interface 或真实硬件验证记录
3. 完成 net/ 的 CAN 和 LTE 模块

---

## 📁 生成的文档

本报告基于以下并行分析结果生成：

- `components/sensor/` - 传感器组件分析 (95%)
- `components/fuel_gauge/` - 电量计组件分析 (70%)
- `components/charger/` - 充电器组件分析 (65%)
- `components/drivers/display/` - 显示驱动状态同步（driver host-guarded / README 已收敛 / 硬件待验证）
- `components/gui/` - 图形界面分析 (40%)
- `components/pm/` - 电源管理状态同步（host-guarded / 功耗待实证）
- `components/dm/` - 数据管理分析 (70%)
- `components/crypto/` - 加密组件状态同步（host-guarded / 文档已补齐 / 安全审查待证据）
- `components/net/` - 网络组件分析 (50%)
- `components/actuator/` - 执行器分析 (60%)
- `components/fota/` - 固件升级分析 (75%)
- `components/pid/` - PID 控制分析 (70%)
- `components/mux/` - 多路复用分析 (45%)

---

**维护者**: XinYi Team
**许可证**: Apache License 2.0
