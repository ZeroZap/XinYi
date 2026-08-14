# XinYi 组件设计与质量闭环

**状态**: 设计阶段 / 持续演进  
**目标**: 让 XinYi 组件从“集思广益 → 设计审查 → 小步开发 → 单元测试 → 优化提交 → 新组件纳入闭环”持续滚动，而不是一次性补丁式开发。

---

## 1. 闭环原则

1. **设计先行**：大接口、大目录结构、大职责边界变更先形成 proposal，不直接批量改代码。
2. **小步可验证**：每次执行只选择一个可落地 slice，改动范围必须可说明、可测试、可回滚。
3. **组件自洽**：组件至少具备 README/Kconfig/CMake/核心实现/示例/测试中的关键入口；缺项要进入 backlog。
4. **测试护栏**：新增或修改组件必须优先补 host Unity/CTest 单元测试；不把 GUI 杂项变更混入组件测试提交。
5. **路径限定提交**：提交只 stage 本轮明确触碰路径，仓库既有脏文件不得顺手带入。
6. **新增组件自动纳入**：发现新的 `components/*` 或 `tests/unit/*` 目标后，要更新组件地图、缺口列表和下一步建议。

---

## 2. 每日设计巡检（集思广益）

每日巡检只做分析和建议，默认不修改源码。

输入：
- `components/COMPONENT_COMPLETENESS_ANALYSIS.md`
- `docs/component-roadmap.md`
- `docs/design/unit-test-inventory.md`
- 当前 `components/`、`tests/unit/`、`CMakeLists.txt`、`Kconfig` 状态

输出应包含：

```text
今日组件观察：
1. 新增/变化组件：...
2. 当前最大设计空洞：...
3. 职责边界风险：...
4. 测试薄弱区：...
5. 今晚建议执行的 1 个小 slice：...
6. 不建议今晚碰的风险项：...
```

选择 slice 的优先级：

1. 已有测试框架、可快速验证的组件缺口。
2. 明显 bug 或 stub，但接口边界清晰。
3. README/Kconfig/CMake 缺失导致组件不可发现。
4. 架构不清但影响面大的项，只产出 proposal，不直接改实现。

---

## 3. 持续组件开发闭环

组件开发闭环是持续滚动任务；每日巡检只提供观察与候选项，不是等到晚间才开始开发。每次自动运行都只能选择一个小目标，流程固定：

1. 读取最近巡检建议和当前仓库状态。
2. 确认不触碰已有无关脏文件，尤其是 `tools/xy_host_tools/gui/z_serial_app.py` 这类 GUI 脏文件。
3. 选择一个路径限定 slice，例如：
   - 给某组件补 README/Kconfig/CMake 入口；
   - 给已有实现补 1 组 host 单元测试；
   - 修复一个明确 bug 并补回归测试；
   - 给设计不清项写 proposal 文档。
4. 修改代码/文档。
5. 执行验证：优先 `make test-unit`；若耗时或受环境阻塞，至少运行相关 CTest/构建命令并记录阻塞。
6. `git diff --check`。
7. 只 stage 本轮文件并提交；若验证失败，不提交，改为汇报 blocker。
8. 汇报：改动路径、验证输出、提交哈希或阻塞原因、下一 slice 建议。

---

## 4. 当前优先 backlog

| 优先级 | 方向 | 依据 | 推荐动作 |
| --- | --- | --- | --- |
| P1 | display | Display driver 已同步为 `components/drivers/display/` host-guarded；README/display.md 与旧完整度报告已收敛，硬件仍待实证 | 不再按“缺 Kconfig/CMake/测试”重复开工；只在新增 panel/interface 前写 proposal + focused host CTest，或补真实硬件验证记录 |
| P0 | net | MQTT client 与 README 已主线化，但 CAN 默认接入和 LTE transport 设计仍未闭环，测试已有 11 个 | 优先为 LTE transport 写 proposal；CAN 只在明确默认接入策略后推进 |
| P1 | actuator | 执行器框架独立于 Sensor，README/Kconfig/示例/host CTest 基线已闭环 | 仅按真实 helper/typed-ops 失败补小回归；不再重复补基线 |
| P1 | mux | 多个子接口已有测试但 API 边界需确认 | 先补测试覆盖与 README 对齐 |
| P1 | pid | `pid_core`/`pid_auto` 与 PID 示例 smoke 已有主线 CTest 护栏；近期已补 auto-tune 边界回归 | 暂不按“明显 bug”重复开工；仅在新增实证失败时补最小回归测试 |
| P1 | fuel_gauge | SMBus clock stretching/NACK 重试需关注 | 补边界测试，不重构大接口 |
| P1 | pm | PM README、Kconfig/CMake 与 `pm_component`/`pm_platform_fallback` host CTest 已闭环；真实低功耗/charger GPIO/ADC 仍待板级实证 | 不再按缺 README/缺测试开工；只推进硬件验证记录或明确 stub 失败的最小回归 |
| P2 | sensor | legacy `sensor_*` tail host CTest 已进入收口状态，详见 `docs/design/xinyi-sensor-tail-host-coverage-closure-2026-07-25.md` | 不再盲目新增尾部目标；只做现有 target 的具体契约 hardening |

### 2026-08-04 状态同步

- PID 不再作为“先验明显 bug”候选：当前 `tests/unit/pid/test_pid_core.c`、`tests/unit/pid/test_pid_auto.c` 以及 PID 示例 smoke 目标已经在 `tests/unit/CMakeLists.txt` 中注册，后续只根据真实失败补回归。
- 下一轮更适合选择 MUX/Net/Fuel Gauge 中仍有测试契约缺口的单一目标；Net 侧避免重复追逐已完成的 MQTT/README 基线，优先收敛 LTE transport proposal 或 CAN 默认接入策略；若只做测试风格维护，优先选择已稳定通过的 focused CTest，避免混入大接口迁移。

### 2026-08-05 状态同步

- 已新增 `docs/design/xinyi-net-lte-transport-proposal-2026-08-05.md`，明确 LTE 在 `XY_NET_ENABLE_LTE=0` 下继续保持 direct-opt-in/stub 状态，下一步应先补 fake transport host coverage，再考虑 UART/HAL 或 `xy_net` umbrella enablement。
- 已新增 `docs/design/xinyi-net-can-enablement-proposal-2026-08-05.md`，明确 CAN 虽已有 `test_can` host 护栏，但仍应保持 `XY_NET_ENABLE_CAN=0` / direct-opt-in；下一步先补 CAN timeout/output-preservation/FIFO-overflow contract，再做 feature-gated `xy_net` 接入。

### 2026-08-06 状态同步

- LTE fake transport host coverage 已在 `tests/unit/net/test_lte.c` 中落地：当前覆盖 AT/CSQ/SIM/attach/PDP/send/read-style helper 的传输失败与状态/输出保持契约；后续不应重复“补 fake transport”作为下一步。
- CAN component-edge hardening 已在 `tests/unit/net/test_can.c` 与 `test_can_public_header` 中落地：当前覆盖 timeout counter/output preservation、FIFO overflow accounting、invalid FIFO/frame guards 与 unregister callback suppression；下一步应优先做 explicit feature-gated `xy_net` build/probe，而不是继续重复同一批 CAN 边界测试。
- Net explicit feature-gated umbrella probe 已在 `test_net_feature_gated_umbrella` 中落地：`xy_net_config.h` 默认值保持 CAN/LTE off，但允许测试/消费者用编译定义显式 opt-in；`xy_net.h` 仅在 `XY_NET_ENABLE_CAN/LTE=1` 时导出对应 public headers。
- LTE UART/AT adapter 仍处设计阶段，已新增 `docs/design/xinyi-net-lte-uart-at-adapter-proposal-2026-08-06.md`：建议下一步只做 callback-backed adapter + focused host CTest，不直接接 vendor HAL，也不改变 `XY_NET_ENABLE_LTE=0` 默认策略。若继续实现，应严格限定在 `xy_lte_uart_adapter.{h,c}` 与 `test_lte_uart_adapter`。
- LTE callback-backed UART adapter 已按上述限定范围落地：`xy_lte_uart_adapter.{h,c}` 与 `test_lte_uart_adapter` 只使用回调 seam，不包含 vendor/HAL UART；后续若继续推进，应在保持 `XY_NET_ENABLE_LTE=0` 默认关闭的前提下，先验证 LTE core 绑定该 adapter 的端到端 AT command contract，再设计真实 HAL UART binding。
- 已新增 `docs/design/xinyi-net-lte-hal-uart-binding-proposal-2026-08-06.md`，明确下一步 HAL UART binding 应作为独立 default-off adapter：只引用公开 HAL UART API、先用 host fake 覆盖 timeout/error/短写归一化，再做 STM32U5 compile probe；仍不允许直接把 LTE 设为 `XY_NET_ENABLE_LTE=1` 默认导出。
- LTE HAL UART binding 已按 default-off 独立 adapter 落地：`xy_lte_hal_uart_adapter.{h,c}` 只引用公开 `xy_hal_uart_*` API，`test_lte_hal_uart_adapter` 覆盖 init/transport guard、send/recv/flush timeout 与错误归一化、rx buffer clamp，以及 LTE core `xy_lte_check()` 绑定路径；后续应先补 STM32U5 compile probe/真实硬件验证记录，再考虑任何 umbrella enablement。
- LTE HAL UART binding 的 STM32U5 compile probe 已通过：`make HAL_PLATFORM=STM32U5 -j$(nproc)` 完成现有 `xy_net` 静态库构建，proposal 已记录这是 compile-only 结果；下一步不应重复做 host/fake adapter，而应在保持 default-off 的前提下补真实 UART/调制解调器硬件验证记录或板级 flow-control 设计。
- 已新增 `docs/design/xinyi-net-lte-hardware-validation-plan-2026-08-06.md`，把 LTE 后续工作收束为硬件验证计划：真实 modem 记录必须区分 compile-only 与 board UART/flow-control/AT/SIM/signal 证据；在记录存在前仍保持 `XY_NET_ENABLE_LTE=0` direct-opt-in。
- 已新增 `docs/validation/xinyi-net-lte-hardware-validation-record-template-2026-08-06.md`，把 LTE 真实硬件验证证据格式固定为 pending/compile-only/hardware-failed/hardware-passed-* 分级；后续只能用真实板级 UART/modem 日志填写，不允许用 host fake 或 STM32U5 compile-only 结果替代硬件证据。
- 已新增 `test_lte_hal_uart_smoke_example` build-guarded smoke skeleton：它只验证 LTE core 绑定 HAL UART adapter 的 AT 成功/modem-absent timeout 路径与 default-off 策略，不伪造真实硬件记录；后续真实板级验证仍需按 validation record template 填写 UART/modem 证据。
- 已新增 `docs/design/xinyi-net-lte-board-flow-control-design-2026-08-06.md`，把真实 LTE 板级验证前的 UART/RTS/CTS/电源时序边界固定下来：`components/net` 继续只负责 transport/HAL UART adapter，UART pinmux、PWRKEY/RESET、RTS/CTS 选择与硬件日志必须由 board/project smoke 记录。后续若无真实板卡证据，应保持 validation record pending，不允许用 host fake/compile-only 填硬件结果。
- 已新增 `docs/design/xinyi-fuel-gauge-smbus-hardware-validation-plan-2026-08-06.md`，把 Fuel Gauge 后续工作从继续堆 host fake 测试收束到真实 SMBus/I2C 板级验证：BQ40Z50/BQ27Z* 等 host CTest 仍作为契约护栏，但 clock stretching、放电期 transient NACK/retry、快照保持在真实硬件上的结论必须用 board log/trace 记录，不能用 fake-I2C 输出替代。
- 已新增 `docs/validation/xinyi-fuel-gauge-smbus-hardware-validation-record-template-2026-08-06.md`，固定 Fuel Gauge 真实 SMBus 硬件验证证据格式：结果从 `pending` 开始，必须记录板卡/电池包/总线配置、init/fetch 日志、retry/NACK 计数、snapshot-preservation 与可选逻辑分析仪 trace；host fake-I2C 或 compile-only 结果不能提升为硬件通过。

### 2026-08-07 状态同步

- 巡检 backlog 中的 A3（“root `tests/CMakeLists.txt` AT 路径未被 `make test` 覆盖”）经复查已不适用：当前仓库没有 `tests/CMakeLists.txt`，AT client/server 已作为 `tests/unit/CMakeLists.txt` 中的 `at_client` / `at_server` Unity CTest 纳入 `make test-unit`。后续不应为不存在的 root AT suite 新增 `test-at` 入口；若未来重新引入 root `BUILD_TESTING=ON` 测试树，应先更新 `AGENTS.md`/Makefile 事实源。
- 巡检 backlog 中的 A10（`components/clib/xy_clib/xy_config copy.h` 重复文件）已由 `784e4b66 test: prune stale clib config copy` 关闭；后续不应重复选择该清理项。
- Fuel Gauge SMBus 硬件验证已具备 build-guarded host smoke skeleton：`test_fuel_gauge_smbus_hardware_smoke_example` 只覆盖 BQ40Z50/BQ27Z561 fake-I2C board-flow 契约与 snapshot preservation，不把 host fake 结果提升为硬件证据。后续应继续保持 `docs/validation/xinyi-fuel-gauge-smbus-hardware-validation-record-template-2026-08-06.md` 为 pending，直到有真实板级 SMBus/I2C 日志。

### 2026-08-08 状态同步

- 已新增 `docs/design/xinyi-mux-component-status-sync-2026-08-08.md`，把 MUX 从旧完整度报告中的 45%/无示例/无测试同步为 100% 主线完善：`xy_mux`/`mux_component` 根构建入口、`mux_core/gpio/i2c/spi/uart/example_basic` host CTest 与 README 验证契约均已闭环；后续只按真实 packet/typed ops 失败或新增集成需求维护，不再重复作为基线补齐 backlog。
- 已同步 Actuator 基线状态：`components/actuator/README.md`、根 `Kconfig` 的 `COMPONENT_ACTUATOR`/`XY_ACTUATOR_ENABLE` 兼容入口、`xy_actuator`/`actuator_component` CMake target、`test_actuator_framework` 与 `actuator_example_relay_servo_pwm` host CTest 均已存在。后续不应再按“缺 README/Kconfig/示例/测试”选择 Actuator slice，只在真实 helper/typed-ops 失败或新增硬件适配需求时补小回归；低优先级 `src/` 目录整理需单独 proposal/迁移验证。
- 已同步 Fuel Gauge 组件完整度：`xy_fuel_gauge`/`fuel_gauge_component` 根构建目标、standalone README/Kconfig/CMake、`fuel_gauge_core`/4 个芯片驱动 CTest 与 `fuel_gauge_smbus_hardware_smoke_example` 均已闭环；完整度报告更新为 90% 主线可用/硬件验证待证据。后续不应继续堆等价 fake-I2C 证明，应等待真实 SMBus/I2C 板级日志或只补与真实失败对应的最小回归。
- 已复查 repo audit A9：`components/clib/xy_clib/Kconfig` 中的 `XY_XY_CLIB_ENABLE` 不是当前根 Kconfig 事实源，PC 生成配置不导出该符号；`xy_clib` 作为核心 runtime 仍由根 `CMakeLists.txt` 无条件加入。A9 已标记 obsolete，后续不应把该 stale nested switch 直接接成可关闭选项，除非先设计完整的核心依赖/禁用模型。

- IPC 已从旧路线图“65%/消息队列待完善”同步为 host-guarded：`components/ipc/README.md` 记录了 pipe/broker/message queue/observer/event-group 的 active CTest 契约。IPC config ownership 已由 `docs/design/xinyi-ipc-component-config-proposal-2026-08-08.md` 明确并完成 post-contract sync：当前保持 always-discoverable core component，不新增 root `COMPONENT_IPC`；后续 IPC 工作只在真实 consumer 需要 generated config、线程/ISR event backend 证据，或具体 observer/event 回归失败时推进，不再按“event-group proposal/observer CTest/无消息队列测试/缺 config 决策”重复开工。

### 2026-08-08 FOTA external-flash build closure

- FOTA external-flash Kconfig combination is now buildable without a non-existent board-specific NOR source: `components/fota/CMakeLists.txt` no longer appends missing `src/xy_fota_nor.c` when `FOTA_EXTERNAL_FLASH=ON;NOR_FLASH_ENABLED=ON`; it relies on the existing flash-op/backup-op hooks and records the focused probe in `docs/design/xinyi-fota-external-flash-build-closure-2026-08-08.md`.
- Verified slice: focused `fota_core` CTest, PC `xy_fota` build with external flash overrides, full `make test-unit`, and `git diff --check`.
- FOTA now has a root `components/fota/README.md` covering active root Kconfig symbols, `xy_fota`/`fota_component` target ownership, flash-op/external-backup hooks, focused verification commands, and the boundary that board NOR backends/hardware logs remain outside the platform-independent core. Remaining low-risk FOTA backlog is a build-guarded host-safe public example, not another README or fake external-flash compile proof.
- FOTA host-safe public example is now build-guarded as `test_fota_smoke_example` / `fota_smoke_example`: it exercises the documented init → flash-op registration → download → finish → update flow plus single-slot external-backup callback policy with fake Flash callbacks only. It does not claim bootloader, board NOR, or real hardware validation; next FOTA work should wait for real board/bootloader evidence or a specific failure.

### 2026-08-09 Display driver status proposal

- Display driver 的当前事实源不是旧报告里的顶层 `components/display/`，而是 `components/drivers/display/` 加 root `Kconfig` 的 `DRIVER_DISPLAY*` 选项、`components/drivers/CMakeLists.txt` source filter，以及 `tests/unit/display/*` 的 5 个 focused CTest。
- 已新增 `docs/design/xinyi-display-driver-status-proposal-2026-08-09.md`，把 Display 从“缺 Kconfig/CMake/测试”的旧基线候选改为“driver host-guarded / README 已收敛”。`components/drivers/display/README.md` 与 `display.md` 已同步为真实源码、root Kconfig、5 个 focused display CTest 与硬件待验证边界；后续不应重复按空白组件补基线，只在新增 panel/interface、真实硬件记录或具体 display CTest 失败时推进。
- GUI 仍保持单独基础状态；Display driver 的 host CTest 不能替代 GUI 字体/控件/渲染闭环，也不能作为真实显示硬件验证记录。

### 2026-08-09 PM component status sync

- PM 不再适合作为“缺 README/缺测试”的旧基线候选：`components/pm/README.md` 已同步为 host-guarded / 功耗待实证状态，`components/pm/Kconfig` 与 `components/pm/CMakeLists.txt` 已存在，`pm_component` / `pm_platform_fallback` 已在主线 `make test-unit` 中守护 PM lifecycle、ADC fallback、charger state、platform tick/charger hook 契约。
- PM 后续工作应等待真实低功耗、charger GPIO、ADC channel、电池曲线或整机功耗日志，或只按具体 stub/board backend 失败补最小回归；standalone `components/fuel_gauge/` 继续独立维护，不应回并 PM。

### 2026-08-09 GUI effects host coverage

- GUI basic effects 已新增 `gui_effects` host CTest：覆盖 `xy_gui_effect_{fade,blink,breath,slide,rotate}.c` 的 create/update/getter、NULL guard、duration/period 边界、基础 lifecycle 与工具函数 contract；`components/gui/README.md` 与 unit-test inventory 已同步。
- 本结论只适用于基础 effects 算法，不替代 LED-screen/RGB 扩展动画、字体/中文渲染、GUI↔Display backend 或真实屏幕硬件验证。后续若继续 GUI，应优先选择字体资产/渲染 proposal 或 display-backend validation proposal，而不是重复基础 effects host coverage。

### 2026-08-09 GUI display-backend boundary proposal

- 已新增 `docs/design/xinyi-gui-display-backend-validation-proposal-2026-08-09.md`，把 GUI core 的 `xy_gui_disp_drv_t` 回调、`xy_gui_display.h` 抽象接口、Display driver focused CTest 与真实屏幕硬件证据分层固定下来。
- 当前 `gui_core`/`gui_widgets`/`gui_fonts`/`gui_effects*` 仍只能证明 display-independent host contract；`display_*` CTest 只能证明各 display driver 的 host fake transaction contract。二者都不能替代 GUI ↔ Display backend bridge CTest 或真实屏幕日志。
- GUI ↔ Display backend bridge CTest 已落地并扩展到 LED GUI display adapter host 绑定：`test_gui_display_backend` / `gui_display_backend` 使用 host fake `xy_gui_disp_drv_t` backend 与 fake LED driver framebuffer，覆盖 `xy_gui_clear/draw_pixel/fill_rect/flush` 的坐标、尺寸、颜色、调用次数转发、`xy_gui_t` 经 LED adapter 驱动 host framebuffer/flush 的路径、两个 LED GUI adapter channel 的 framebuffer/flush 隔离与 per-driver enable/disable 行为，以及当前 backend error 被 GUI core 归一化为 `XY_GUI_OK` 的现有 contract。该测试仍不代表真实 LCD/OLED/LED matrix 硬件验证。
- 后续若继续推进 GUI ↔ Display，应优先做具体 display driver adapter 的 host fake framebuffer/transport 绑定，或等待真实屏幕硬件日志；不要直接改 HAL/vendor 或声称 hardware validation passed。

### 2026-08-10 GUI SSD1306 adapter proposal

- 已新增 `docs/design/xinyi-gui-ssd1306-display-adapter-proposal-2026-08-10.md`，把 GUI core、旧 `xy_gui_display_t`、LED GUI adapter 与 SSD1306 display driver 的职责边界拆开：GUI core 不直接依赖 `xy_oled_ssd1306_t`，SSD1306 adapter 只调用 display driver public API，真实 I2C/HAL/vendor 与硬件验证仍在 adapter 之外。
- 后续若扩展，应严格限定在 `xy_gui_ssd1306_adapter.{h,c}` + `test_gui_ssd1306_adapter`，当前已覆盖 bind guard、RGB565→mono 映射、draw-line/draw-rect/draw-char callback 转发、fill-rect clipping、flush I2C transaction、多实例 slot 隔离，以及 slot exhaustion/reset 契约；不要用单一全局 OLED 指针造成多屏串扰，也不要把 host fake 结果升级为真实 OLED 验证。

### 2026-08-11 GUI font engine closure

- GUI font engine 已从“字体/中文渲染待补”推进为 host-guarded：`gui_fonts` 覆盖 8x16/16x24/Chinese 16x16 bitmap asset lookup 与 measurement contract，`gui_font_engine` 覆盖 runtime font init、glyph lookup、draw/cache/multi-line measurement、unsupported/null guards、load-stub guard 与 wide-glyph row bounds。
- 本结论仍只代表 host-side 字体资产/引擎契约；不代表完整中文字库、美术质量、真实屏幕渲染或 GUI↔Display 硬件验证已通过。后续若继续字体方向，应先写字体资产范围/生成流程或真实渲染 snapshot proposal，不再重复补同类 font engine guard。
- 已新增 `docs/design/xinyi-gui-font-asset-generation-proposal-2026-08-11.md`，把字体方向的下一步收束为四级证据：当前 contract-guarded assets、可复现 manifest/generator、host framebuffer snapshot review、真实屏幕硬件记录。后续不要再把 `gui_fonts` / `gui_font_engine` 等同于完整中文字库或美术验收；若继续实现，应先做 current-asset manifest/generator 与 manifest consistency host smoke，不要同轮 bulk-import 完整 CJK 字库。
- 已新增 `components/gui/fonts/font_manifest.json` 与 `gui_font_manifest` host CTest，先把当前 legacy ASCII/Chinese bitmap asset 的范围、provenance、duplicate/placeholder inventory 固定为可验证事实；`components/gui/fonts/tools/README.md` 只记录 deterministic generator 要求，尚未实现生成器或导入完整 CJK 字库。后续字体方向应推进 generator 或 snapshot review，而不是重复补字体表边界测试。
- 已新增 `components/gui/fonts/tools/generate_bitmap_font.py` generator bootstrap 与 `gui_font_generator_manifest`/`gui_font_generator_output` CTest：当前做 manifest schema/range/source-file/known-inventory validation、deterministic summary，以及 manifest-inventory generated-header preview/self-test；不导入外部字体、不改写 legacy bitmap table、不声称字体美术或真实屏幕验证。后续 generator slice 应在此基础上实现可复现 `.c/.h` 写入或 host snapshot review。
- Font generator 已补 `--write-manifest-header` 与 `gui_font_generator_write` write-path CTest：当前可把 manifest-inventory header 明确写到指定输出路径，并验证写出内容与 preview 完全一致；仍不把生成文件纳入源码、不生成 glyph bitmap、不替代 snapshot/hardware validation。
- 已新增 `docs/design/xinyi-gui-font-glyph-generation-proposal-2026-08-11.md`，把下一步真实 `.c/.h` glyph generation 收束为独立 metadata-first slice：先为 manifest 增加 output/source/mode/license 字段与 `gui_font_generator_glyph_metadata` smoke，再考虑写出/提交生成表；不得同轮 bulk-import 完整 CJK 字库或声称视觉/硬件验收。
- Font generator 已补 `--write-glyph-preview`、`gui_font_generator_glyph_write` 与 `gui_font_generator_glyph_compile`：当前可把 legacy-passthrough glyph header/source preview 写入指定输出路径，并在临时 generated tree 中用 `gcc -std=c99 -Wall -Wextra -Werror` 编译预览 source；仍不提交生成表、不导入完整 CJK 字库、不替代 framebuffer snapshot 或真实屏幕硬件验证。
- 已新增 `docs/design/xinyi-gui-font-framebuffer-snapshot-proposal-2026-08-11.md`，把字体方向下一步从继续补 API/generator guard 收束为 host framebuffer snapshot review：只渲染当前 manifest-declared ASCII/Chinese UI 样本，检查 deterministic framebuffer/checksum/ASCII-art metadata，不提交生成 glyph byte 表、不导入完整 CJK、不声明真实屏幕硬件通过。
- GUI font snapshot 已新增 `gui_font_snapshot` host CTest：使用 runtime font engine 渲染 host-only framebuffer，锁定 ASCII-art/checksum metadata、unknown glyph output-preservation 与 clipping guard；该结果仍只是 deterministic host snapshot，不代表字体美术验收或真实屏幕硬件验证通过。
- 已新增 `docs/validation/xinyi-gui-font-rendering-hardware-validation-record-template-2026-08-11.md`，把 GUI 字体/显示后续证据固定为 pending/compile-only/host-snapshot-only/hardware-failed/hardware-passed-* 分级；后续不能用 `gui_font_snapshot` checksum、fake display backend CTest 或生成器 preview 替代真实屏幕照片/日志。
- 已新增 `docs/design/xinyi-gui-font-generated-preview-checkin-policy-2026-08-11.md`，把后续是否提交 `components/gui/fonts/generated/*` 收束为 legacy-passthrough preview 的独立政策：只有在 generator 能 byte-for-byte 复现、focused CTest/compile gate 证明、且不改变 runtime lookup/不导入新字库/不宣称硬件通过时，才允许单独提交生成预览文件；下一步不应直接 bulk-import CJK 或把 generated preview 当成美术验收。
- 已按上述政策提交 `components/gui/fonts/generated/*` legacy-passthrough generated preview，并新增 `gui_font_generator_checked_in_preview` CTest：当前只证明 checked-in generated preview 与 manifest/generator byte-for-byte 可复现且 generated `.c` 可 C99 编译；runtime font lookup、legacy bitmap tables、字体美术质量、完整 CJK 导入与真实屏幕验证均未因此改变。后续字体方向不应重复补 generator reproducibility guard，应转向 license/provenance review、host snapshot 人审流程或真实屏幕日志。
- 已新增 `docs/design/xinyi-gui-font-license-provenance-review-plan-2026-08-11.md`，把字体资产 license/provenance review 从 generator/preview 工作中拆出：当前 manifest 中 `project-review-pending` 必须保持到有来源、许可兼容性、placeholder/duplicate 处理与 review note 证据；后续低风险实现应先补 review_status/validation-template/manifest smoke，而不是直接改成 license-approved 或导入外部字体。
- Font license/provenance review 已补机器护栏：`font_manifest.json` 新增 `review_status` pending block，`docs/validation/xinyi-gui-font-license-provenance-review-record-template-2026-08-11.md` 固定人工审查证据格式，`gui_font_license_manifest` CTest 会阻止 license/source_license 在缺少 review record 前被静默改为 approved。后续若继续字体资产方向，应填写真实来源/许可审查记录或推进 host snapshot 人审流程，不应用 generator/preview/snapshot checksum 替代 license 证据。
- 已新增 `docs/validation/xinyi-gui-font-host-snapshot-review-record-template-2026-08-11.md` 与 `gui_font_snapshot_review_manifest` host smoke：`font_manifest.json` 现在把 host snapshot 人审状态作为独立 pending evidence tier 记录，明确不能替代 license/provenance approval、generated glyph byte check-in 或真实屏幕硬件验证。后续若继续字体方向，应填写真实 snapshot 人审 artifact/结论，或在有真实来源证据后推进 license/provenance review；不要再用 generator/preview/focused CTest 结果替代人工/硬件证据。

### 2026-08-11 DM component status sync

- DM 已新增统一入口 `components/dm/README.md`，把 `COMPONENT_DM` / `xy_dm` root build、Base64/TLV/NVM/Factory/FEE/coreJSON 的 6 个 focused CTest、FS/JSON abstraction backlog，以及 NOR/FlashDB 硬件验证边界固定为当前事实源。
- `components/COMPONENT_COMPLETENESS_ANALYSIS.md` 与 `docs/component-roadmap.md` 已同步：DM 不再按“无统一 README/测试不足”旧基线重复开工；后续只在 FS/JSON abstraction 成为活跃 public dependency、NOR/FlashDB 有真实板级证据，或现有 `dm_*` CTest 暴露具体失败时推进。

### 2026-08-12 Trace component status sync

- Trace 已新增统一入口 `components/trace/README.md`，把 `xy_trace` root build、`xy_log` public logging API、弱 `xy_log_char()` 输出 seam、nested Kconfig 非根事实源边界，以及 `trace_component` host CTest 固定为当前事实源。
- Trace 后续不应再按“缺 README/无测试”旧基线重复开工；只有在真实 backend、动态等级运行时过滤策略、或 shell/command runtime 需求明确时，才先写 proposal 并补 focused 回归。

### 2026-08-12 Net component status sync

- Net 组件完整度报告与 `components/net/README.md` 已同步到当前事实：CAN/MQTT/AT/Modbus 维持 host CTest 护栏，LTE 已有 fake AT seam、callback UART adapter、default-off HAL UART adapter、smoke skeleton 与 STM32U5 compile probe。
- Net 后续不应再按“缺 README/缺测试/LTE 无 adapter”重复开工；CAN/LTE umbrella 默认导出仍需产品决策或真实 UART/modem/flow-control 硬件证据，host fake/HAL adapter smoke 不能替代硬件验证记录。

### 2026-08-12 Crypto component status sync

- Crypto 已新增统一入口 `components/crypto/README.md`，把当前 root auto-discovered target `xy_tiny_crypto`、root `COMPONENT_CRYPTO` 默认关闭策略、10 个 active `tests/unit/crypto` focused CTest、历史 `src/` 与 module-directory duplicate source ownership 风险，以及“host CTest 不等于安全审计/硬件加速验证”的边界固定为事实源。
- `components/COMPONENT_COMPLETENESS_ANALYSIS.md` 与 `docs/component-roadmap.md` 已同步：Crypto 不再按“Base64/Hex 源码位置不明 / SM 测试不足 / 无 README”旧基线重复开工；后续应优先做 security/provenance review、duplicate-source ownership proposal、或 active public API 的 host-safe smoke 示例，不直接批量整理源码目录。
- Crypto security/provenance review 已补机器护栏：`components/crypto/crypto_review_manifest.json` 记录每个算法区域的 contract tests、root/runtime source 与 focused-test source、review-pending 状态与允许用途；`crypto_review_manifest` CTest 会阻止没有 review record 的算法被静默改成 provenance/security approved。SM2 已新增 explicit `security-rejected` review record，限定为 test-only/compatibility-only placeholder，不允许用于 production signing/verification/encryption/key exchange/authentication。MD5/SHA-256/HMAC、AES/SM3/SM4/ChaCha20、lightweight crypto、Curve25519/Ed25519 generic 与 Cortex-M0 已新增 `security-reviewed-limited` boundary record；provenance、side-channel/constant-time、RNG/hash dependency、target assembly/hardware evidence 与 source-ownership reconciliation 继续 pending。后续应继续补真实 review record 或 source ownership map，不把该 manifest 视为安全审计结论。
- `crypto_cipher_hmac` 的 RFC 8439 Poly1305/ChaCha20-Poly1305 AEAD hardening 已落地：测试覆盖 Poly1305 RFC tag、AEAD ciphertext/tag、invalid-parameter guard 与 tampered-tag 输出保持，修复范围限定在 module-source `xy_chacha/xy_chacha20_poly1305.c`，未做 duplicate-source 整理、root target rename、安全审计状态升级或 `COMPONENT_CRYPTO` 默认启用。后续 Crypto 仍应优先做 security/provenance review、source ownership reconciliation proposal，或等待真实 consumer failure；不要把该 host CTest 当成安全审计结论。
- 已新增 `docs/design/xinyi-crypto-duplicate-source-reconciliation-proposal-2026-08-13.md`，把 duplicate-source 收敛策略限定为分组小步：优先让 `xy_tiny_crypto` 复用 module-directory 源并逐组更新 source map/manifest/checker，禁止一次性删除 `components/crypto/src/`、改名 root target、默认启用 Crypto 或把 host CTest 提升为安全/来源审查结论。Base64/Hex、CRC/BLAKE2、RNG/CSPRNG、MD5/HMAC/AES 的 module-source ownership 与 stale duplicate pruning 已闭环。
- ChaCha20-Poly1305 split ownership 已按 `docs/design/xinyi-crypto-chacha-root-module-reconciliation-proposal-2026-08-13.md` 收敛为 root compact compatibility wrapper + module RFC8439 arithmetic implementation：`crypto_root_target_smoke` 覆盖 root `ciphertext || tag` API 与 auth-failure 输出保持，`crypto_cipher_hmac` 覆盖 module RFC vectors，`crypto_review_manifest` 同步 source map/manifest 边界。后续不应重复做 wrapper CTest 或委托实现；只能在真实 consumer failure、stale reference、或安全/来源审查证据出现时补最小 slice，且不得直接删除 `src/xy_chacha20poly1305.c` 或把 host CTest 升级为安全审查结论。
- 已新增 `docs/design/xinyi-crypto-lwc-root-ownership-proposal-2026-08-14.md`，把 Ascon/TinyJAMBU/Photon-Beetle 从“umbrella header 可见”与“focused `crypto_lwc` 可测”之间的 runtime ownership 边界固定下来：当前仍是 `focused-test-only-until-root-ownership-decided`，若未来 consumer 需要 root `xy_tiny_crypto` 链接 LWC，必须先补 root-link smoke，再在同一小 slice 更新 CMake/source map/manifest；不得把 host CTest 或 header include 视为安全/来源审查或默认启用证据。
- LWC root ownership 已按上述 proposal 的小步路径落地：`xy_tiny_crypto` 现在显式链接 Ascon/TinyJAMBU/Photon-Beetle module sources，`crypto_root_target_smoke` 覆盖最小 aggregate-link/API flow，source map 与 review manifest 已同步为 `root-runtime-module-source-limited`；这仍不代表 provenance/security/hardware approval，也不改变 `COMPONENT_CRYPTO` default-off 策略。后续 Crypto 不应重复做 LWC root-link smoke，只能按真实 consumer failure 或真实 review/provenance 证据补最小 slice。
- 已新增 `docs/design/xinyi-crypto-curve25519-root-ownership-proposal-2026-08-14.md`，明确 Curve25519/Ed25519 与 LWC 不同：虽然 `xy_tiny_crypto.h` 有 X25519/Ed25519 声明，但 `xy_25519.c`/M0 material 仍保持 focused-test-only，依赖 RNG/SHA-512 seam 且 Ed25519 verify/security/provenance 证据不足。推荐继续 Option A（不接入 root runtime）直到出现真实 root consumer 或产品安全决策；后续若要接 root，必须另做 root-link/unsupported-wrapper 小 slice，并更新 source map、manifest、root smoke 与真实验证。
- Curve25519 root ownership 已补 policy smoke：`crypto_curve25519_root_policy` 会检查 `xy_tiny_crypto` 未静默链接 `xy_25519`/M0 sources，manifest runtime_sources 保持 empty，source map/proposal 保留 focused-test-only 与 no-security-claim 边界。后续不应重复做该 policy guard；若出现真实 root consumer，应按 proposal Option B/C 单独实现并补 root-link/unsupported-wrapper smoke。
- SHA-256/HMAC root ownership 已完成 module-source follow-up：`xy_tiny_crypto` 现在显式链接 `components/crypto/xy_hmac/xy_sha256.c` 与 `xy_hmac.c`，已 prune byte-identical historical `src/xy_sha256_hmac.c`，并继续排除旧 API `src/xy_sha256.c`；`crypto_root_target_smoke`/`crypto_hash`/`crypto_cipher_hmac`/`crypto_review_manifest` 共同守护 root 与 focused CTest 使用同一 SHA-256/HMAC module source。后续不应重复做 SHA-256 module-source 接入；只能在真实 consumer failure 或后续 provenance/security 证据出现时补最小 slice。
- Crypto benchmark harness 已补 host timing bounds 护栏：`crypto_benchmark_manifest.json` 现在显式记录默认 CTest 1 iteration、最大 1000 iterations、最大 4096B 输入与 no-performance-threshold policy，`crypto_benchmark_manifest` smoke 会同时校验 record template 的 bounded wording。后续若继续 benchmark，只能按真实 opt-in record 或算法 API timing plumbing 小步推进，不得把默认 smoke 当成性能结论。

---

### 2026-08-11 Device component status sync

- Device 已新增统一入口 `components/device/README.md`，把当前 split ownership 固定为事实源：`src/xy_device.c` 负责 lifecycle/dispatch，`xy_device_core.c` 负责 static-array registry，`src/xy_device_bus_helpers.c` 负责 I2C/SPI/UART/GPIO compatibility helpers，`src/xy_device_pm.c`/`src/xy_device_async.c` 分别负责 PM 与 optional async helper。
- Device 当前由 `device_framework`、`spi_device`、`auto_register`、`device_async_helper`、`device_registry_example`、`device_driver_template` 等 host CTest 守护；后续不应再按旧路线图“Device 70% / 设备注册待完善”重复开工，只在真实 helper/group API/PM backend 失败时补最小回归或先写 proposal。

### 2026-08-12 Crypto security/provenance review policy

- 已新增 `docs/design/xinyi-crypto-security-provenance-review-plan-2026-08-12.md` 与 `docs/validation/xinyi-crypto-security-provenance-review-record-template-2026-08-12.md`，把 Crypto 后续工作从“host CTest 已过所以安全可用”的误区收束为分级证据：当前只能声称 `contract-guarded`，provenance/security/hardware 结论都必须有单独审查记录或真实硬件证据。
- 初始 matrix 已标记 SM2 placeholder-grade、MD5 legacy/integrity-only、CSPRNG entropy-source non-goal、Curve25519 M0 upstream/assembly TODO 等审查重点；本轮不移动 `src/`/module duplicate source，不改变 `COMPONENT_CRYPTO` 默认关闭策略，也不引入外部 crypto 库。
- 后续 Crypto 低风险 slice 应优先做 source ownership map 或单算法 review manifest smoke；不要直接批量整理 duplicate source 或把 focused CTest 结果升级为安全审计结论。

### 2026-08-13 Kernel service roadmap sync

- Kernel Service 不再适合作为旧路线图中的“60% / 系统监控/定时器待完善”基线候选：`components/kernel/README.md` 与 `docs/design/xinyi-kernel-host-guard-status-sync-2026-08-08.md` 已记录 `osal_baremetal`、`kernel_autotask`、`kernel_sysmon`、`bootreason_check` 四个主线 host CTest 护栏。
- `docs/component-roadmap.md` 已同步为 host-guarded / backend 与板级实证待补；后续 Kernel 方向应只按真实 OSAL backend 调度/ISR/低功耗、SysMon RTOS telemetry、bootreason 板级来源等证据或具体失败推进，不再重复补等价 host stub 或按旧“无监控/无定时器测试”开工。

---

## 5. 周度架构回顾

每周输出一次组件地图：

```text
组件成熟度：成熟 / 可用但缺测试 / 草稿 / 设计待定 / 废弃候选
本周新增组件：...
本周关闭缺口：...
下周建议：...
需要 Eugene 决策的问题：...
```

周度回顾只汇总和提出决策点，不自动做破坏性重构。

---

## 6. 提交与汇报格式

每次自动开发完成后汇报：

```text
XinYi 组件闭环完成：<slice 名称>
- 改动：<路径列表>
- 验证：<真实命令 + 关键结果>
- 提交：<hash 或 未提交原因>
- 未触碰：<已存在脏文件>
- 下一步：<建议 slice>
```

提交信息建议：

```text
docs: add XinYi component quality loop
feat: add actuator component config entry
fix: repair pid auto tuning regression
测试相关：test: add <component> host coverage
```

---

## 7. 安全边界

- 不自动编辑 `MCU/`、`third_party/` vendor 树。
- 不把无关 GUI 变更与组件单测/框架提交混在一起。
- 不在设计不清时批量移动目录或重命名公共 API。
- 不伪造验证结果；命令失败必须如实汇报并尝试替代验证。
- cron 任务不得递归创建新的 cron/kanban 工作流。
