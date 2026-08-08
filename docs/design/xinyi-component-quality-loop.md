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
| P0 | display | 完整度报告显示实现/配置严重不足；但风险较高 | 先写/更新 proposal，避免直接大改 |
| P0 | net | MQTT client 与 README 已主线化，但 CAN 默认接入和 LTE transport 设计仍未闭环，测试已有 11 个 | 优先为 LTE transport 写 proposal；CAN 只在明确默认接入策略后推进 |
| P1 | actuator | 执行器框架独立于 Sensor，README/Kconfig/示例/host CTest 基线已闭环 | 仅按真实 helper/typed-ops 失败补小回归；不再重复补基线 |
| P1 | mux | 多个子接口已有测试但 API 边界需确认 | 先补测试覆盖与 README 对齐 |
| P1 | pid | `pid_core`/`pid_auto` 与 PID 示例 smoke 已有主线 CTest 护栏；近期已补 auto-tune 边界回归 | 暂不按“明显 bug”重复开工；仅在新增实证失败时补最小回归测试 |
| P1 | fuel_gauge | SMBus clock stretching/NACK 重试需关注 | 补边界测试，不重构大接口 |
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

- IPC 已从旧路线图“65%/消息队列待完善”同步为 host-guarded：`components/ipc/README.md` 记录了 pipe/broker/message queue 的 active CTest 契约与 event-group/observer 后续边界。IPC config ownership 已由 `docs/design/xinyi-ipc-component-config-proposal-2026-08-08.md` 明确：当前保持 always-discoverable core component，不新增 root `COMPONENT_IPC`；后续 IPC 工作应优先写 event-group proposal、为 observer 补 focused CTest，或按真实失败补最小回归，不再按“无 README/无消息队列测试/缺 config 决策”重复开工。

### 2026-08-08 FOTA external-flash build closure

- FOTA external-flash Kconfig combination is now buildable without a non-existent board-specific NOR source: `components/fota/CMakeLists.txt` no longer appends missing `src/xy_fota_nor.c` when `FOTA_EXTERNAL_FLASH=ON;NOR_FLASH_ENABLED=ON`; it relies on the existing flash-op/backup-op hooks and records the focused probe in `docs/design/xinyi-fota-external-flash-build-closure-2026-08-08.md`.
- Verified slice: focused `fota_core` CTest, PC `xy_fota` build with external flash overrides, full `make test-unit`, and `git diff --check`.
- FOTA now has a root `components/fota/README.md` covering active root Kconfig symbols, `xy_fota`/`fota_component` target ownership, flash-op/external-backup hooks, focused verification commands, and the boundary that board NOR backends/hardware logs remain outside the platform-independent core. Remaining low-risk FOTA backlog is a build-guarded host-safe public example, not another README or fake external-flash compile proof.
- FOTA host-safe public example is now build-guarded as `test_fota_smoke_example` / `fota_smoke_example`: it exercises the documented init → flash-op registration → download → finish → update flow plus single-slot external-backup callback policy with fake Flash callbacks only. It does not claim bootloader, board NOR, or real hardware validation; next FOTA work should wait for real board/bootloader evidence or a specific failure.

### 2026-08-09 Display driver status proposal

- Display driver 的当前事实源不是旧报告里的顶层 `components/display/`，而是 `components/drivers/display/` 加 root `Kconfig` 的 `DRIVER_DISPLAY*` 选项、`components/drivers/CMakeLists.txt` source filter，以及 `tests/unit/display/*` 的 5 个 focused CTest。
- 已新增 `docs/design/xinyi-display-driver-status-proposal-2026-08-09.md`，把 Display 从“缺 Kconfig/CMake/测试”的旧基线候选改为“driver host-guarded / README 待收敛”。后续不应重复按空白组件补基线；更合适的下一步是收敛 `components/drivers/display/README.md` 中超前的 MAX7219、Charlieplex、GUI effects/fonts、未验证 panel ✅ 表述。
- GUI 仍保持单独基础状态；Display driver 的 host CTest 不能替代 GUI 字体/控件/渲染闭环，也不能作为真实显示硬件验证记录。

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
