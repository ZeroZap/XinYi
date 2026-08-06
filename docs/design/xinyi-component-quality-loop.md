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
| P1 | actuator | 执行器框架独立于 Sensor，已有测试入口 | 补 README/Kconfig/示例或增强测试 |
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
