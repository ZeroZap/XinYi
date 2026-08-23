# XinYi Sprint 跟踪看板

**建立日期**：2026-08-17
**当前阶段**：Sprint 0 — 可信基线与治理门禁
**状态事实源**：本文件
**范围与验收事实源**：[全组件状态审计与 Sprint 计划](2026-08-17-component-audit-sprint-plan.md)
**质量流程事实源**：[组件设计与质量闭环](../design/xinyi-component-quality-loop.md)

> 本看板跟踪执行状态；详细背景、组件矩阵与验收定义保留在审计计划中，避免复制后漂移。

---

## 1. 状态规则

| 状态 | 含义 |
|---|---|
| `BACKLOG` | 已确认但尚未进入当前 Sprint |
| `READY` | 前置条件齐备，可以开工 |
| `IN_PROGRESS` | 当前正在执行；同一时间最多 2 个主要 slice |
| `BLOCKED` | 缺硬件、决策、依赖或验证环境；必须记录原因 |
| `VERIFYING` | 实现完成，正在执行 focused/full/target/HIL 验证 |
| `DONE` | 验收证据完整且已 path-limited commit |
| `CANCELLED` | 经决策取消；必须保留原因 |

### 更新要求

每个 slice 开始、验证、提交或阻塞时，在对应行更新：

- `状态`
- `负责人`
- `分支/提交`
- `证据/阻塞`
- `更新时间`

完成项必须链接到真实测试日志、validation record、PR 或 commit；不能只写“已完成”。

---

## 2. 当前 Sprint：Sprint 0

**周期**：2026-08-17 ～ 2026-08-23
**目标**：让分支、CI、版本和能力声明成为可信事实源。
**容量建议**：治理/CI 40%，GUI 40%，STM32U5 HIL 准备 20%。

### Sprint 0 看板

| ID | 优先级 | 工作项 | 状态 | 负责人 | 依赖 | 验收/证据 | 分支/提交 | 更新时间 |
|---|---:|---|---|---|---|---|---|---|
| S0-01 | P0 | 建立 Sprint 看板与组件证据台账 | DONE | Zero | 审计计划 | 看板、审计计划与证据台账已建立；`git diff --check` 通过 | `9cea83f0` | 2026-08-23 |
| S0-02 | P0 | 将本地 477 个提交直接推送 `origin/main`，以服务器作为单机开发备份 | DONE | Zero | SSH/远端权限 | 2026-08-23 实测 `HEAD`=`origin/main`=`9cea83f00ac661685f8d4b0384ff247fb4b87ac1`，ahead/behind `0/0`；无历史重写 | `9cea83f0` | 2026-08-23 |
| S0-03 | P0 | 收敛 canonical CI workflow | DONE | Zero | S0-02（DONE） | canonical `unit-tests.yml`：Host 178/178 + PC root build；删除 stale `ci.yml`/`ci-cd.yml`，移除过期 `-DPLATFORM`、empty root CTest 与无说明 `|| true` 路径；`git diff --check` | `045a9e56` | 2026-08-23 |
| S0-04 | P0 | 建立 Kconfig/CMake 配置组合矩阵 | IN_PROGRESS | Zero | S0-03（DONE） | [矩阵](../validation/kconfig-cmake-configuration-matrix.md)已建立；非法 Display 组合 fail-closed，OLED/SSD1306、LCD SPI/I8080/ST7789 与 LED/serial RGB 合法组合的生成值、focused targets 和归档 source inventory 通过；独立 RGB 已确认无实现源；全关/逐组件/Sensor/STM32U5 组合待闭环 | `cc1b3b75`, `5375c930`, `e918c375`, 本轮提交 | 2026-08-23 |
| S0-05 | P0 | 统一版本、tag、release note 与 workflow 触发 | BACKLOG | - | S0-03 | 单一版本事实源；tag 流程可验证；release note 路径存在 | - | 2026-08-17 |
| S0-06 | P0 | 降级无证据的 production/security/hardware 宣称 | BACKLOG | - | 组件证据台账 | README 声明逐项链接证据；Known Limitations 存在 | - | 2026-08-17 |
| S0-07 | P1 | GUI Sprint 1 任务细化与失败测试清单 | READY | - | GUI 独立变更规则 | backend 错误传播、SDL strict、字体/显示三组任务可直接执行 | - | 2026-08-17 |
| S0-08 | P1 | STM32U5 HAL/HIL 夹具与记录模板准备 | READY | - | 可用板卡/仪器 | GPIO/UART/I2C/SPI/IRQ/DMA 测试夹具清单和 record template | - | 2026-08-17 |

### Sprint 0 退出条件

- [x] 本地 477 个提交与本轮文档提交已直接推送到 `origin/main`，本地/远端 SHA 一致（2026-08-23：`9cea83f0`，ahead/behind `0/0`）。
- [x] canonical Host gate 实跑 178/178 通过（2026-08-23，`make test-unit`）。
- [x] PC root build 通过（2026-08-23，Release、`BUILD_TESTS=OFF`）。
- [x] stale workflow 不再产生假绿（删除 `.github/workflows/ci.yml` 与 `ci-cd.yml`；canonical workflow 无 empty CTest/无说明 `|| true`）。
- [ ] 版本和 release note 事实源唯一。
- [ ] 组件能力声明可追溯到证据台账。
- [ ] Sprint 1 GUI 工作项达到 `READY`。

---

## 3. 后续 Sprint 队列

| Sprint | 周期 | 目标 | 进入条件 | 当前状态 |
|---|---:|---|---|---|
| Sprint 1 | 2 周 | GUI backend 错误传播、strict backend、字体与单一显示纵切 | Sprint 0 门禁可信 | BACKLOG |
| Sprint 2 | 2 周 | STM32U5 HAL→Device→Driver 最小实板证据链 | HIL 夹具可用；HAL 证据矩阵建立 | BACKLOG |
| Sprint 3 | 2 周 | Crypto 产品级重建 Phase 1；Secure FOTA fail-closed | 安全算法清单和 provider 决策 | BACKLOG |
| Sprint 4 | 2 周 | Sensor 三轨收敛、DM 掉电测试、Fuel Gauge 实板 | canonical Sensor API 决策 | BACKLOG |
| Sprint 5 | 2 周 | 单一 RTOS 并发验证；Net/PM 按产品需求推进 | reference RTOS/board 决策 | BACKLOG |
| Sprint 6 | 1–2 周 | Release Candidate | 目标平台 HIL、安全边界和发布门禁达标 | BLOCKED |

---

## 4. 决策与阻塞日志

| 日期 | ID | 类型 | 内容 | 所需决策/解除条件 | 状态 |
|---|---|---|---|---|---|
| 2026-08-17 | D-001 | 决策 | Sensor 实际有 legacy/new/drivers 三条实现路径 | 选择 canonical API；建议 `xy_sensor_device_t` + Device adapter | OPEN |
| 2026-08-17 | D-002 | 安全阻塞 | Fuel Gauge security AES 存在明文透传风险 | 改 fail-closed 或接入已审查 provider | OPEN |
| 2026-08-17 | D-003 | 安全阻塞 | Secure FOTA 依赖 security-rejected ECDSA placeholder | production signature provider 未落地前保持 feature-off | OPEN |
| 2026-08-17 | D-004 | 仓库策略 | XinYi 当前仅此 PC 开发，无其他设备并行同步；`origin` 用作服务器备份 | 本地 path-limited commit 后直接推送 `origin/main`；不需要为多设备同步保留审查缓冲 | CLOSED |
| 2026-08-17 | D-005 | 硬件阻塞 | 缺统一 STM32U5 HIL/总线/功耗证据 | 明确参考板、仪器、接线和记录位置 | OPEN |
| 2026-08-17 | D-006 | 路线决策 | GUI 继续、PM 后置、Crypto 产品级重建、Release 最后 | 已纳入 Sprint 顺序 | CLOSED |

---

## 5. 周度更新模板

每周结束时在本文件追加一节：

```markdown
### YYYY-MM-DD Sprint 周报

- Sprint：Sprint N
- 完成：Sx-xx（commit / validation record）
- 未完成：Sx-xx（原因）
- 新阻塞：D-xxx
- 验证：focused / full Host / target compile / QEMU / HIL
- 仓库：branch、ahead/behind、dirty paths
- 下一周：最多 3 个主要 slice
```

### 2026-08-17 建档基线

- Host：178/178 CTest 通过。
- PC 根构建：通过。
- STM32F4 QEMU：46/46 通过；含测试内模拟 HAL，不是实板证据。
- 分支：`main`；2026-08-23 已将本地累计提交推送至 `origin/main`，同步状态 `0/0`，两端 SHA 均为 `9cea83f00ac661685f8d4b0384ff247fb4b87ac1`。
- 仓库：审计前干净；建档提交为 `9cea83f0`。
- 当前重点：Sprint 0 治理门禁、GUI Sprint 1 准备、STM32U5 HIL 准备。

### 2026-08-23 Sprint 0 周报

- Sprint：Sprint 0（未完成项滚入下一周期，保持原 ID 与依赖顺序）。
- 完成：S0-01（`9cea83f0`）、S0-02（`02a9be45` 校准记录）、S0-03（`045a9e56`）。
- 未完成：S0-04～S0-08；S0-04 已建立配置矩阵并修复 override 绕过父依赖的 fail-open 风险，其余组合及版本/声明治理、GUI 准备和 HIL 模板顺延。
- 新阻塞：无；既有 D-001/D-002/D-003/D-005 保持 OPEN。
- 验证：`make test-unit` 178/178；PC Release root build 通过；workflow YAML 解析通过；`git diff --check` 通过。以上不构成实板、安全或产品证据。
- 仓库：`main`；本地/`origin/main` ahead/behind `0/0`；工作树干净。
- 下一周期：S0-04 Kconfig/CMake 配置组合矩阵 → S0-05 版本/release 事实源 → S0-06 能力声明与 Known Limitations。
