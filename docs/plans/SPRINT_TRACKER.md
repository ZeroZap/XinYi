# XinYi Sprint 跟踪看板

**建立日期**：2026-08-17
**当前阶段**：Sprint 1 — GUI 产品化纵切；Sprint 2 非实板前置并行推进
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
| S0-04 | P0 | 建立 Kconfig/CMake 配置组合矩阵 | DONE | Zero | S0-03（DONE） | [矩阵](../validation/kconfig-cmake-configuration-matrix.md)已覆盖 Display 全组合、all-off、Device/Crypto/DM/Sensor/Actuator-only 及 STM32U5 默认组合；STM32U5 clean root compile、条件默认值检查、Host 178/178、PC root build 与 `git diff --check` 通过 | `cc1b3b75`～`e4c0ff4c` | 2026-08-24 |
| S0-05 | P0 | 统一版本、tag、release note 与 workflow 触发 | DONE | Zero | S0-03（DONE） | `VERSION` 驱动 root CMake；Kconfig/public header 为受检镜像；canonical changelog、Known Limitations 与 fail-closed `vMAJOR.MINOR.PATCH` workflow 已建立；release facts 正/负向 probe、workflow YAML、Host 178/178、PC root build 与 `git diff --check` 通过 | `5b1943e1` | 2026-08-24 |
| S0-06 | P0 | 降级无证据的 production/security/hardware 宣称 | DONE | Zero | 组件证据台账 | 根/组件/HAL/Net/Modbus/OSAL/Crypto 公开 README 已降级到 Host/compile/hardware/security 分层口径并链接证据台账；CI capability-claim guard、release facts、workflow YAML、Host 178/178、PC root build、`git diff --check` 通过 | `73464378` | 2026-08-24 |
| S0-07 | P1 | GUI Sprint 1 任务细化与失败测试清单 | DONE | Zero | GUI 独立变更规则 | 审计计划已明确 backend 错误传播、SDL strict、字体/显示三组任务及验收；首个失败测试已于 Sprint 1 RED→GREEN | `e4faf3c3` | 2026-08-24 |
| S0-08 | P1 | STM32U5 HAL/HIL 夹具与记录模板准备 | DONE | Zero | S0-04（DONE）；实板执行仍依赖板卡/仪器 | 已建立 fail-closed record、夹具/接线清单及 GPIO/UART/I2C/SPI/IRQ/DMA normal/negative/recovery 场景；focused policy guard、Host 181/181、PC root build 与 `git diff --check` 通过；当前环境缺板卡、调试器、SDK checkout 与 ARM toolchain，分类保持 `BLOCKED_NO_HARDWARE` | `3e4c2c54` | 2026-08-25 |

### Sprint 0 退出条件

- [x] 本地 477 个提交与本轮文档提交已直接推送到 `origin/main`，本地/远端 SHA 一致（2026-08-23：`9cea83f0`，ahead/behind `0/0`）。
- [x] canonical Host gate 实跑 178/178 通过（2026-08-23，`make test-unit`）。
- [x] PC root build 通过（2026-08-23，Release、`BUILD_TESTS=OFF`）。
- [x] stale workflow 不再产生假绿（删除 `.github/workflows/ci.yml` 与 `ci-cd.yml`；canonical workflow 无 empty CTest/无说明 `|| true`）。
- [x] 版本和 release note 事实源唯一（`VERSION` + `docs/release/CHANGELOG.md`；2026-08-24）。
- [x] 公开组件能力声明已按证据层级降级，并由 CI claim guard 防止关键 README 回归到无条件 production-ready（2026-08-24）。
- [x] Sprint 1 GUI 工作项达到 `READY`，且 S1-01 已进入执行并闭环（2026-08-24：`e4faf3c3`）。

Sprint 0 于 2026-08-24 满足全部退出条件并关闭；S0-08 作为非退出门禁的 HIL 准备项滚入 Sprint 2 前置队列。

---

## 3. 当前 Sprint：Sprint 1

**周期**：2026-08-24 ～ 2026-09-06
**目标**：完成 GUI backend 可信错误边界、strict backend 选择与字体/单一显示纵切；GUI 提交不混入固件/HAL/FOTA。

| ID | 优先级 | 工作项 | 状态 | 负责人 | 依赖 | 验收/证据 | 分支/提交 | 更新时间 |
|---|---:|---|---|---|---|---|---|---|
| S1-01 | P0 | GUI backend 错误传播 | DONE | Zero | S0-07（DONE） | RED probe 证明 init/clear/draw/fill/flush 吞错；修复后 `gui_core`、`gui_display_backend`、`gui_ssd1306_adapter` 3/3，Host 178/178、PC root build、`git diff --check` 通过；clear 失败不提交背景色，fallback 首错即停 | `e4faf3c3` | 2026-08-24 |
| S1-02 | P0 | SDL/backend strict selection | DONE | Zero | S1-01（DONE） | `GUI_SDL` 改为显式 opt-in；缺 SDL2 的 RED 配置曾错误成功，修复后 `CMAKE_DISABLE_FIND_PACKAGE_SDL2=TRUE` 配置按预期失败；默认配置生成 `CONFIG_GUI_SDL=OFF` 且 `xy_gui` 构建通过。仓库缺 SDL backend source，已显式 fail-closed 并记录为后续恢复项，不虚报 backend 可用 | `d2979d41` | 2026-08-25 |
| S1-03 | P1 | 字体清单、生成器与 Host snapshot 收口 | DONE | Zero | S1-01（DONE） | legacy 资产 review 为 `rejected-needs-regeneration`；已固定 OFL-1.1 Noto Sans CJK SC TTC SHA-256/index/license，将 15 个必需 UI glyph 按 deterministic snapshot 接入 active 16x16 table，并以 distinct/nonblank Host contract 守护；font focused 15/15、Host 179/179、PC root build、`git diff --check` 通过；其余 legacy 字体、视觉与实板状态不升级 | `ca96d82b`～`019d9206` | 2026-08-25 |
| S1-04 | P1 | SSD1306 单一显示纵切记录 | BLOCKED | - | 可用板卡/显示屏 | 板卡、接线、固件 SHA、init/fill/text/flush/error/re-init、帧时间和 RAM 记录 | 缺实板环境 | 2026-08-24 |
| S1-05 | P1 | 恢复 SDL2 backend source 与 Host contract | DONE | Zero | S1-02（DONE） | 新增 explicit-context SDL2 backend；fake seam 覆盖错误与 RGB565 contract；real-library `gui_sdl_runtime` 使用 dummy video driver 实跑 window/renderer/texture/fill/flush/event/deinit，并修复无 accelerated renderer 时缺少 software fallback 的 headless 初始化失败；Host 180/180、默认 PC root、SDL-enabled `xy_gui`、real SDL runtime 与 `git diff --check` 通过。不宣称人工视觉、性能或硬件证据 | `2fd4e668`～`a881eb55` | 2026-08-25 |
| S1-06 | P1 | SSD1306 实板验证记录与阻塞探测 | DONE | Zero | S1-03（DONE）；S1-04 硬件阻塞 | 已建立 fail-closed 记录，覆盖板卡/接线/SHA、init/fill/text/flush、NACK/timeout、re-init、帧时间与 RAM；2026-08-25 focused 2/2、Host 180/180、`git diff --check` 通过；`lsusb` 未发现开发板/调试器，且无 `/dev/ttyACM*`/`ttyUSB*`，故记录为 `BLOCKED_NO_HARDWARE`，不填写实板通过 | `f9ebc4a9` | 2026-08-25 |

---

## 4. 后续 Sprint 队列

### Sprint 2 前置看板

> Sprint 1 的 SSD1306 实板项仍受硬件阻塞；为避免空转，只推进不冒充实板证据的
> HAL/Device/Driver Host 前置。S2-2/S2-3 在 B1/B2 与 clean STM32U5 compile 补齐前不得标记
> `DONE`。

| ID | 优先级 | 工作项 | 状态 | 负责人 | 依赖 | 验收/证据 | 分支/提交 | 更新时间 |
|---|---:|---|---|---|---|---|---|---|
| S2-01 | P0 | HAL 平台实现与证据矩阵 | DONE | Zero | S0-08（DONE） | STM32U5/F4/L4/WCH/HC32 的 implementation/unsupported/Host/compile/QEMU/HIL 边界已逐项记录；未升级实板声明 | `fef4f1fe` | 2026-08-26 |
| S2-02 | P0 | STM32U5 GPIO/UART/I2C/SPI/IRQ/DMA 实板基础外设 | BLOCKED | - | 板卡、调试器、SDK/toolchain、夹具/仪器 | fail-closed HIL 记录已建立；当前缺 B1/B2 原始日志与 capture | 缺实板环境 | 2026-08-26 |
| S2-03 | P0 | I2C→Device helper→24xx/SSD1306 纵切 | BLOCKED | Zero | S2-02；Host 前置已完成 | 24xx 已覆盖 timeout/NACK 错误传播与 re-init；SSD1306 已覆盖 helper/首命令失败清理与停止副作用；focused、Host 182/182、PC root build、`git diff --check` 通过。尚缺 clean STM32U5 compile 与 B1/B2 | `72391b51`、`6ec6081c` | 2026-08-26 |
| S2-04 | P1 | SYS reset/bootreason/chip-ID strong backend | BLOCKED | Zero | 参考板/board ownership 决策 | Host fail-closed 子项完成：默认 reset/reboot-reason/chip-ID/MAC/version 不再返回伪成功，focused `sys_core`、Host 183/183、PC root build 与 `git diff --check` 通过；strong backend 与上电/软件/看门狗复位 B1/B2 仍缺参考板决策和实板 | 本次 path-limited 提交 | 2026-08-26 |

### 后续 Sprint 队列

| Sprint | 周期 | 目标 | 进入条件 | 当前状态 |
|---|---:|---|---|---|
| Sprint 1 | 2 周 | GUI backend 错误传播、strict backend、字体与单一显示纵切 | Sprint 0 门禁可信 | IN_PROGRESS |
| Sprint 2 | 2 周 | STM32U5 HAL→Device→Driver 最小实板证据链 | [HAL 平台实现与证据矩阵](../validation/hal-platform-evidence-matrix.md)已建立；Host 前置推进中，HIL 夹具仍缺 | IN_PROGRESS（Host 前置）/BLOCKED（实板） |
| Sprint 3 | 2 周 | Crypto 产品级重建 Phase 1；Secure FOTA fail-closed | 安全算法清单和 provider 决策 | BACKLOG |
| Sprint 4 | 2 周 | Sensor 三轨收敛、DM 掉电测试、Fuel Gauge 实板 | canonical Sensor API 决策 | BACKLOG |
| Sprint 5 | 2 周 | 单一 RTOS 并发验证；Net/PM 按产品需求推进 | reference RTOS/board 决策 | BACKLOG |
| Sprint 6 | 1–2 周 | Release Candidate | 目标平台 HIL、安全边界和发布门禁达标 | BLOCKED |

---

## 5. 决策与阻塞日志

| 日期 | ID | 类型 | 内容 | 所需决策/解除条件 | 状态 |
|---|---|---|---|---|---|
| 2026-08-17 | D-001 | 决策 | Sensor 实际有 legacy/new/drivers 三条实现路径 | 选择 canonical API；建议 `xy_sensor_device_t` + Device adapter | OPEN |
| 2026-08-17 | D-002 | 安全阻塞 | Fuel Gauge security AES 存在明文透传风险 | 改 fail-closed 或接入已审查 provider | OPEN |
| 2026-08-17 | D-003 | 安全阻塞 | Secure FOTA 依赖 security-rejected ECDSA placeholder | production signature provider 未落地前保持 feature-off | OPEN |
| 2026-08-17 | D-004 | 仓库策略 | XinYi 当前仅此 PC 开发，无其他设备并行同步；`origin` 用作服务器备份 | 本地 path-limited commit 后直接推送 `origin/main`；不需要为多设备同步保留审查缓冲 | CLOSED |
| 2026-08-17 | D-005 | 硬件阻塞 | 缺统一 STM32U5 HIL/总线/功耗证据 | 明确参考板、仪器、接线和记录位置 | OPEN |
| 2026-08-17 | D-006 | 路线决策 | GUI 继续、PM 后置、Crypto 产品级重建、Release 最后 | 已纳入 Sprint 顺序 | CLOSED |

---

## 6. 周度更新模板

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

### 2026-08-24 Sprint 0 收口 / Sprint 1 启动

- Sprint：Sprint 0 退出条件全部完成；Sprint 1 已启动。
- 完成：S0-04（`cc1b3b75`～`e4c0ff4c`）、S0-05（`5b1943e1`）、S0-06（`73464378`）、S0-07 与 S1-01（`e4faf3c3`）。
- 未完成：S0-08 滚入 Sprint 2 前置队列；S1-04 因缺实板环境保持 `BLOCKED`，Host 证据不替代实板。
- 验证：GUI focused 3/3；Host 178/178；PC Release root build；`git diff --check`。以上仅构成 Host/PC 证据。
- 仓库：本轮开始 `main` 与 `origin/main` 为 `0/0`、SHA 同为 `73464378c9ddcfbc0e28fccf00e468c15caf85d0`；本轮提交将在看板校准后直接推送并复核。
- 下一步：S1-03 已将 OFL-1.1 Noto Sans CJK SC 的 15 个必需 UI glyph 接入 active 16x16 table，并保留其 Host-only、未视觉/实板批准边界；其余 legacy 字体与 provenance 仍 pending。S1-04 继续等待实板；S1-05 已补 real SDL2 dummy-video headless runtime，下一 slice 应准备 S1-04 SSD1306 实板记录/夹具，不以 SDL headless 结果替代显示硬件证据。

### 2026-08-25 Sprint 1 / Sprint 2 前置校准

- 完成：S0-08 已建立 STM32U5 HAL/HIL fail-closed 记录；S2-1 平台实现与证据矩阵已建立，明确 PC Host、STM32U5 source/compile-only、STM32F4 部分 QEMU、STM32L4 wrapper 复用及 WCH/HC32 source/unsupported 边界。
- 阻塞：S1-04 与 Sprint 2 实板纵切仍缺板卡、调试器、夹具、仪器、STM32U5 SDK checkout 与 ARM toolchain；保持 `BLOCKED_NO_HARDWARE`，Host/compile/QEMU 不替代实板。
- 仓库：本轮开始本地 `main` 领先 `origin/main` 2 个提交；已 fast-forward push，并以 `git ls-remote` 核对两端 SHA=`c8ac794f45acc1203be549b48620cfa51e5ce3de`、ahead/behind `0/0`。
- 下一步：硬件阻塞未解除时，继续 S2-3 I2C→Device→Driver 的 Host error/re-init contract；硬件到位后执行 SSD1306 和 STM32U5 HAL 记录中的 B1/B2 场景。

### 2026-08-26 Sprint 2 非实板前置推进

- 完成：S2-1 HAL 平台实现与证据矩阵提交 `fef4f1fe` 已推送并与 `origin/main` 同步；S2-3 选择 I2C→Device helper→24xx EEPROM，新增 transmit/receive timeout、失败时不延时假成功及 re-init recovery Host 契约。
- 修复：24xx read/write-page 过去忽略 `xy_i2c_device_write()` 错误，可能在地址阶段 NACK/timeout 后继续读取或返回写入成功；现原样传播 Device I/O 错误。
- 验证：focused `storage_eeprom_24xx` 1/1、Host 182/182、PC Release root build 与 `git diff --check` 通过；只构成 Host/PC 证据。
- 阻塞：STM32U5 compile/board 和 EEPROM 写保护、write-cycle polling、掉电恢复仍待 SDK/toolchain、板卡与夹具。

### 2026-08-26 Sprint 2 SSD1306 Host 前置推进

- 完成：S2-3 的另一条推荐纵切 I2C→Device helper→SSD1306 新增 init 错误传播契约；I2C helper 初始化或首条面板命令失败时返回原始 Device 错误，释放并清空 framebuffer，且不继续延时/发送命令。
- RED：focused `display_oled_ws2812` 证明 init command I/O 失败此前仍返回 `XY_DEVICE_OK`。
- 验证：focused `display_oled_ws2812` + `gui_ssd1306_adapter` 2/2、Host 全量、PC Release root build 与 `git diff --check`；只构成 Host/PC 证据。
- 阻塞：SSD1306 NACK/timeout/re-init 的实板 B2、帧时间与 RAM 仍待板卡、调试器、接线和仪器。

### 2026-08-26 Sprint 2 SYS fail-closed 前置推进

- 完成：S2-4 最小可验收 Host 子项；新增 `sys_core` focused CTest，默认 weak/no-board reset、reboot-reason、chip-ID、MAC 与版本查询不再返回伪成功，统一返回 `XY_ERROR_NOT_SUPPORTED` 并保留调用方输出。
- RED：focused `sys_core` 3/3 用例最初全部失败（expected `-3`, was `0`）；实现后 1/1 CTest 通过。
- 验证：focused `sys_core` 1/1、Host 183/183、PC Release root build 与 `git diff --check` 通过；只构成 Host/PC fail-closed 证据。
- 阻塞：参考板和 board ownership 尚未决策，strong reset/bootreason/chip-ID backend 及上电/软件/看门狗复位 B1/B2 保持 `BLOCKED`。
