# XinYi 全组件状态审计与 Sprint 计划（2026-08-17）

> **定位**：本文件是组件组合级规划，不将 Host、交叉编译或 QEMU 结果升级为真实硬件、安全、性能或产品发布证据。

**目标**：基于当前源码、构建配置、测试、文档和近期活动，重新建立可信组件状态，并安排后续 Sprint。

**当前事实源**：`main@9cea83f0`；2026-08-23 校准时工作树干净，且本地 `main` 与 `origin/main` ahead/behind 为 `0/0`。当前执行状态由 [Sprint 跟踪看板](SPRINT_TRACKER.md) 维护，证据等级由 [组件证据台账](../validation/component-evidence-matrix.md) 维护。

---

## 1. 执行摘要

### 1.1 当前总体状态

XinYi 已不是“缺少组件骨架”的早期仓库，而是一个具有强 Host 回归基础、部分 PC/QEMU 可验证、但真实硬件和产品证据明显不足的嵌入式框架。

- PC 根构建：通过。
- PC Host 单元测试：实跑 **178/178 通过**。
- STM32F4 QEMU：实跑 **46/46 通过**；其中存在测试内模拟 HAL，不能视为生产 HAL 或真实板卡证明。
- CI：canonical `unit-tests.yml` 可运行；其余 workflow 存在过期参数、重复门禁或假绿风险。
- 发布：不具备统一 RC 条件；版本、release note、支持矩阵、安全声明与硬件证据不一致。
- 最大仓库治理风险：本地 `main` 比远端领先 **477 个提交**，审查、备份、协作和发布边界不可信。

### 1.2 成熟度口径

| 等级 | 含义 |
|---|---|
| A — 软件可用 | 主路径、构建和 focused test 完整；仍不自动代表硬件验证 |
| B — 基本可用 | 核心契约成立，但平台、并发、配置或覆盖仍有明显缺口 |
| C — 开发中 | 可运行子集存在，但关键路径仍含 stub、弱实现或缺少闭环 |
| D — 原型/禁用于产品 | 仅状态机、占位或实验能力；不能承担产品关键功能 |
| X — Deprecated | 不再新增能力，只维护迁移与兼容边界 |

### 1.3 结论

**软件层最成熟**：Device、IPC、MUX、PID、Clib、Fuel Gauge Host 契约。
**有较强 Host 护栏但产品证据不足**：Sensor、GUI、Net、Crypto、FOTA、DM、Display。
**主要基础风险**：HAL、SYS、PM、Secure FOTA、发布工程。
**建议项目描述**：`host-guarded development baseline`，不得泛化为“全平台 production-ready”。

---

## 2. 全组件状态矩阵

| 组件/领域 | 等级 | 已有证据 | 主要缺口/阻塞 | 下一动作 |
|---|---:|---|---|---|
| `clib` | A- | `clib_component`、alloc shim；字符串/内存/stdio/ctype/ring/filter/sort | MCU heap、代码尺寸、math/stdlib 深覆盖不足；存在 warning | 保持稳定，补 warning 与 MCU size 记录 |
| `kernel/OSAL` | B | bare-metal Host 契约；AutoTask/SysMon/bootreason CTest | RTOS 调度、ISR/event、低功耗唤醒和 bootreason 板级证据不足；bare-metal thread 仍弱 | 选一个 RTOS 后端做并发与 ISR 实证 |
| `hal` | C | PC simulation、Host `hal_pc`、STM32F4 QEMU | README 过度宣称；STM32U5 I2S stub、STM32F4 SPI/Timer 多处 unsupported、HC32 GPIO 不完整；缺真实 IRQ/DMA/timeout | 建 HAL×平台×外设×证据矩阵，并完成 STM32U5 最小实板集 |
| `device` | A- | registry/lifecycle/I/O/PM/bus helper/async；6 个 focused CTest | RTOS 并发、IRQ/DMA、真实 PM backend 未证 | 作为硬件纵切统一入口，不再另造生命周期 |
| `drivers/display` | B | LCD/OLED/WS2812/RGB/LED 5 个 Host CTest | panel/LED 实物、时序、DMA、帧率、长期稳定性未证；部分 backlog 仅声明 | 与 GUI 分离，先完成 SSD1306 或 ST7789 实板纵切 |
| `drivers/storage` | B- | 24xx EEPROM Host transaction contract | 写周期、NACK polling、掉电、寿命与实物证据不足 | 纳入 I2C 硬件纵切及掉电测试 |
| `drivers/sensor`（新 Device 模型） | B | SHT30/MPU6050/BMP280/ADS1115 与 Device/PC HAL 集成测试 | 仅少量 active 驱动；与 legacy Sensor 双轨并存 | 定义新驱动准入与 legacy 迁移模板 |
| `drivers/power/wireless/system` | D/C | 目录与部分配置入口 | active 源很少或为空，历史声明大于实现 | 不扩张宣称；按产品需求逐项 proposal |
| `sensor` legacy `sensor_*` | B | 根 `sensor_component` 实际编译 framework 与约 55 个 `sensor_*.c`；大量 focused CTest 覆盖解析、边界和 I/O 失败 | 当前产品主路径，但部分 stub/简单驱动的测试不等于完整数据手册实现；fusion 部分路径仍返回 `SENSOR_ENOSYS`；多数无实板证据 | 冻结新增 legacy；逐批迁移到 Device 模型，保留兼容层 |
| `sensor` 新 `xy_*` | B-/C | 约 23 个 `src/xy_*.c`，多个环境/IMU/光学/存储 Host 测试 | 当前根 `components/sensor/CMakeLists.txt` 未纳入这些新实现，即“测试过但未进入根产品库”；MLX90614 EEPROM 写未实现 | 先建立 active-source manifest，再迁移高价值驱动并验证进入根 target |
| `drivers/sensor` 第三路径 | C | SHT30/MPU6050/BMP280/ADS1115 共 4 个独立驱动及部分 Device 集成测试 | 与 legacy/new 路径形成第三套同名实现；近 90 天基本停滞，生命周期与 active ownership 不统一 | 纳入 canonical Sensor API 决策，禁止继续形成第四套生命周期 |
| `actuator` | A- | relay/servo/PWM/batch/callback 与示例 CTest | 自建 registry/lifecycle，尚未接入 Device；真实 PWM/timer/GPIO、故障态和安全默认值未证 | 增加 Device adapter；随 HAL 纵切补实板证据 |
| `fuel_gauge` | A-/B | core + BQ27Z746/BQ40Z50/MAX17043/BQ27Z561 共 6 个 Host CTest | SMBus clock stretching、放电期 NACK/retry、告警写入、板级日志未证；security AES 当前存在明文透传风险 | 保持 standalone、不回并 PM；AES passthrough 改 fail-closed 或接可信 provider；安排硬件验证 |
| `charger` | X/C | 旧 BQ25620 实现、Host CTest 与弃用文档 | 弃用文档推荐迁往实际为空的 `drivers/power/charger`，事实冲突；无热保护/充电故障实证 | 先纠正文档与 ownership；只维护迁移，禁止新功能 |
| `analog_devices` | B- | ADC/DAC helper、MCP3008、HX711；root build 与 Host test | 模拟值/Host seam 不能证明 ADC 精度、SPI 时序或标定 | 随具体板卡验证，不扩成通用生态包 |
| `mux` | A- | GPIO/I2C/SPI/UART typed ops，5 个 CTest + 示例 | 并发、真实总线切换和上位机集成未证 | 维护模式；仅按真实失败补回归 |
| `pid` | A- | core/auto-tune、5 个示例、focused CTest | 实际控制对象、采样抖动、饱和/抗积分工程参数未证 | 维护模式；产品接入时做 HIL tuning |
| `trace` | B | raw/format/macro/weak sink Host CTest | 动态等级与宏过滤不完全一致；UART/RTT/ITM、ISR/并发、丢日志策略未证 | 修正等级语义并纳入 RTOS 并发验证 |
| `ipc` | A- | pipe/broker/MQ/observer/event-group 5 个 CTest | 多线程/ISR stress、内存压力、配置 ownership 未证 | 选 FreeRTOS/RT-Thread 做并发实证 |
| `sys` | C | timer/state-machine CTest | reset/chip ID/MAC/bootreason 多为 weak/no-op，缺 BSP strong override | 与 STM32U5 板级 Sprint 一起闭环 |
| `dm` | B | Base64/TLV/NVM/Factory/FEE/coreJSON 6 个 CTest | FS/JSON dedicated test 缺失；Flash/NOR 掉电、磨损、布局迁移未证 | 补 FS/JSON focused test；设计掉电注入验证 |
| `pm` | C | lifecycle、fallback、charger state Host CTest | sleep/shutdown 仍偏入口；真实功耗、唤醒源、ADC/charger GPIO、电池曲线未证 | 按既定路线后置，待 HAL/SYS 稳定再推进 |
| `crypto` | B（契约）/D（安全） | 算法向量/API、ownership/manifest、约 30 项 Host/plan/compile policy 测试 | provenance 多为 review-pending；SM2/ECDSA security-rejected；无常数时间证明、真实 MCU 性能/硬件加速证据 | 产品级重建：先算法清单、来源、安全等级和 provider 边界，再优化 |
| `gui` | B（Host）/C（产品） | core/widget/event/theme/effects/fonts/snapshot/backend/SSD1306 adapter，约 23 项测试 | backend 错误传播、真实显示、帧率/RAM/视觉质量/输入链路不足；SDL 可静默降级 | 下一功能 Sprint 主线；GUI 独立提交，不混固件/HAL/FOTA |
| `net` | B（协议）/C（产品） | MQTT/CAN/Modbus/ISO7816/AT/SMBus/PMBus，约 18 项测试 | LTE recv/URC 未闭环；无 modem/RTS-CTS/注册/PDP/长稳；无统一 SAL；AT 多实现 ownership 不清 | 无明确产品硬件前保持 opt-in；先收敛配置和 ownership |
| `fota` | B（Host）/D（安全产品） | core state/CRC/flash callback/backup/restore，2 个 Host test | update 流程模拟；patch callback no-op；boot handoff/confirm/anti-rollback/掉电恢复未实现；依赖被拒绝 ECDSA | 默认禁用 Secure FOTA；先定义可信 signature provider 和真实 bootloader 协议 |
| `third_party` | B（集成） | FlashDB/TinyUSB/NimBLE submodule | SDK submodule部分未初始化；许可证/SBOM/版本固定与产品选择未统一 | Release 前做 SBOM、许可证与 submodule reproducibility |
| `examples/projects` | C | 部分示例有 Host smoke | 存在过期平台变量、未受 CI 保护或不可编译项目；STM32U5 FOTA 项目有明确缺陷 | 建 supported/archive 清单，只保护少量官方样例 |
| CI/Docs/Release | C | canonical Unit Tests workflow；docs workflow 基础 | 多 workflow 冲突；过期 `-DPLATFORM`、empty-CTest/`|| true` 假绿、版本与 release note 漂移 | 先建立可信门禁，再谈 RC |

---

## 3. 核心架构与依赖判断

### 3.1 真实依赖链

```text
OSAL + HAL
    -> Device
        -> Drivers / Sensor / Actuator / Fuel Gauge
            -> GUI / Net / FOTA / Product Projects

Clib + Trace + IPC + SYS + DM 为横切基础设施
Crypto 是 FOTA/安全通信的安全依赖，但当前安全等级不足
PM 依赖 HAL + OSAL + Device + SYS 的真实板级能力
```

### 3.2 Sensor 实际为三轨

```text
legacy sensor_*  -> 当前根 sensor_component 主体
new src/xy_*     -> 多数有独立 Host 测试，但未进入根 sensor target
drivers/sensor   -> 4 个独立驱动，形成第三套实现路径
```

因此近期最高优先级不是新增驱动，而是决定 canonical API、建立逐芯片 ownership/active-source 清单，并用 SHT30、MPU6050、ADS1115 做单一路径迁移试点。

### 3.3 配置事实源漂移

已发现典型不一致：Sensor、Fuel Gauge、MUX、Display 的 Kconfig 符号与 CMake 判断存在前缀差异；Actuator、Charger 缺少一致的独立开关，Analog 历史开关未可靠控制 target。当前“能构建”部分依赖根默认值，不能证明 menuconfig 组合正确。

Sprint 0 应加入配置矩阵：全关、逐组件开启、Display 子功能以及 Sensor legacy/new 兼容模式。

### 3.4 必须避免的错误路线

1. 不再按“缺 README/缺 CMake/缺测试”重复补已经闭环的组件。
2. 不以 Host fake、compile-only 或 QEMU 模拟宣称真实硬件通过。
3. 不继续扩张 legacy `sensor_*`；新驱动必须进入 Device 模型或提供明确兼容层。
4. 不将 Fuel Gauge 回并 PM；保持 standalone ownership。
5. 不将 GUI 文件与固件/HAL/FOTA 改动混在同一提交。
6. 不以现有 ECDSA/SM2 placeholder 承担 Secure FOTA、签名或身份认证。
7. 不在缺安全/provenance/benchmark 证据时直接做 Crypto 汇编优化并宣传性能。
8. 不以 178 项测试全绿掩盖 workflow、版本、硬件证据和远端落后问题。

---

## 4. 风险排序

| 顺位 | 风险 | 影响 | 处置 |
|---:|---|---|---|
| 1 | Secure FOTA 链接 security-rejected ECDSA | 可产生虚假安全感与产品安全事故 | 默认 fail-closed/feature-off；定义 production signature provider |
| 2 | HAL 文档成熟度高于源码和板级证据 | 所有上层硬件组件结论失真 | 建证据矩阵，先 STM32U5 GPIO/UART/I2C/SPI/IRQ/DMA |
| 3 | `main` 尚未推送的 477 个提交 | 单机故障会造成代码丢失 | 本轮文档提交后直接推送 `origin/main`，并核对本地/远端 SHA |
| 4 | 多套 CI 可能假绿 | 回归和 release gate 不可信 | 合并 canonical workflow，禁止 empty CTest/`|| true` |
| 5 | Sensor 双轨和 AT 多实现 | API/ownership/构建漂移 | 冻结旧入口，建立 active-source/ownership manifest |
| 6 | SYS/PM/DM 关键行为仍为 hook/stub/simulation | 复位、功耗、数据一致性无法产品化 | 板级 backend + 掉电/功耗实证 |
| 7 | README/版本/发布宣传过强 | 用户和集成方误判能力 | 建能力矩阵与 Known Limitations，证据驱动措辞 |

---

## 5. Sprint 安排

### Sprint 0（1 周）：建立可信基线与治理门禁 — P0

**目标**：先让“绿灯、版本和能力声明”可信，避免在错误基线上继续扩张。

#### S0-1 单机开发远端备份

**路径**：Git history、`origin/main`。
**任务**：
- 当前只有这一台 PC 开发，无其他设备或开发者并行同步；`origin` 作为服务器备份和恢复事实源。
- 完成本轮 path-limited 文档提交后，将现有 477 个本地提交直接 fast-forward 推送到 `origin/main`。
- 推送后核对本地 `HEAD`、`origin/main` 和远端 `refs/heads/main` SHA；不得 force-push 或重写历史。
- 后续每个验证通过的本地提交直接推送，避免再次积累大量仅存于单机的提交。

**验收**：工作树干净；本地/远端 SHA 一致；ahead/behind 为 `0/0`；远端可读取最新提交。

#### S0-2 CI 单一事实源

**路径**：`.github/workflows/unit-tests.yml`、`ci.yml`、`ci-cd.yml`、docs/status workflows。
**任务**：
- 保留 canonical `cmake -S tests/unit` + 178 CTest gate。
- 增加明确 PC root build 与目标平台 compile matrix。
- 删除/停用过期 `-DPLATFORM=`、empty-CTest、无条件 `|| true`。
- docs strict build；静态分析若非 blocking 必须明确标注 informational。

**验收命令**：
```bash
make test-unit
cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pc -j$(nproc)
git diff --check
```
**预期**：178/178 通过；PC build 通过；workflow 不存在假绿路径。

#### S0-3 能力声明与版本事实源

**路径**：根 `README.md`、各组件 README、版本文件、release workflow。
**任务**：
- 将 production-ready、secure、hardware acceleration、hardware validated 降级为证据驱动表述。
- 建 `docs/validation/component-evidence-matrix.md` 与 `docs/release/known-limitations.md`。
- 统一版本、tag、release note 路径和触发条件。

**验收**：每项能力可追溯到 Host/cross-compile/QEMU/HIL/security review 中的一类证据。

---

### Sprint 1（2 周）：GUI 产品化纵切 — P0/P1

**目标**：延续 GUI 主线，但只做一个可验证、独立于固件杂项的显示纵切。

#### S1-1 Backend 错误传播

**路径**：`components/gui/xy_gui.c`、`components/gui/src/`、`tests/unit/gui/`。
**任务**：先写失败测试，再修复 init/draw/fill/flush 对 backend 错误的传播；禁止吞错返回 `XY_GUI_OK`。

**验收**：focused GUI CTest + 全量 178 CTest 通过；错误输出和 framebuffer 状态保持契约明确。

#### S1-2 SDL/backend strict selection

**路径**：`components/gui/CMakeLists.txt`、GUI Kconfig、backend tests。
**任务**：用户显式启用 SDL/backend 时，缺依赖必须 fail-fast，不能静默降级。

**验收**：有依赖路径成功；缺依赖路径配置失败且错误可读。

#### S1-3 字体与显示适配器闭环

**路径**：`components/gui/fonts/`、`xy_gui_ssd1306_adapter.*`、snapshot tests。
**任务**：固定中文/ASCII glyph 清单、生成器可复现性、Host snapshot；准备 SSD1306 实板验证记录模板。

**验收**：生成树无漂移；snapshot 稳定；GUI 变更单独提交，不触碰无关 GUI host tool 或 FOTA/HAL 文件。

#### S1-4 实板显示证据（硬件可用时）

**范围**：只选 SSD1306 或 ST7789 一种。
**验收**：init、fill、text、flush、错误注入、复位重入；记录板卡、接线、固件 commit、串口日志、帧时间与 RAM 峰值。无硬件则保持 `pending`，不得用 Host snapshot 替代。

---

### Sprint 2（2 周）：HAL/Device/Driver 最小硬件纵切 — P0

**目标**：建立第一条真实证据链，而不是继续增加模拟测试数量。

#### S2-1 HAL 证据矩阵

**路径**：`components/hal/README.md`、新 validation matrix。
**任务**：逐平台列 GPIO/UART/I2C/SPI/Timer/I2S/DMA 的 implementation、unsupported、Host、compile、QEMU、HIL 状态。

**验收**：STM32U5/F4/L4/WCH/HC32 不再以笼统“生产就绪”描述。

#### S2-2 STM32U5 基础外设

**范围**：GPIO、UART、I2C、SPI、IRQ callback、timeout/error mapping、至少一条 DMA。
**验收**：真实板卡日志/逻辑分析记录；negative path 包含 NACK、timeout、bus reset/re-init。

#### S2-3 Device→Driver 纵切

**推荐二选一**：
- I2C → `xy_device_bus_helpers` → SSD1306；或
- I2C → `xy_device_bus_helpers` → 24xx EEPROM。

**验收**：注册、初始化、正常事务、NACK、timeout、重入；Host focused + STM32U5 compile + 实板记录三层证据均存在。

#### S2-4 SYS 强 backend

**路径**：`components/sys/xy_sys/`、board/project override。
**任务**：reset、reset cause/bootreason、chip ID；不要把 weak no-op 留给受支持板卡。

**验收**：上电/软件复位/看门狗复位至少三类原因可区分并保存日志。

---

### Sprint 3（2 周）：Crypto 产品级重建 Phase 1 + Secure FOTA 隔离 — P0

**目标**：先恢复安全边界，再决定实现和优化；不是继续堆 benchmark policy 测试。

#### S3-1 Crypto 产品算法清单

**路径**：`components/crypto/crypto_review_manifest.json`、source ownership map、README。
**任务**：将算法分为 production candidate、legacy compatibility、test-only、security-rejected；记录来源、许可证、实现 owner、side-channel 目标和允许用途。

**验收**：SM2/ECDSA placeholder 明确无法被 production feature 选择；review-pending 不得变成 approved。

#### S3-2 Signature provider 边界

**路径**：Crypto public API、FOTA signature seam、focused tests。
**任务**：定义可替换 provider；FOTA 不直接依赖 placeholder ECDSA；缺 production provider 时 Secure FOTA 配置必须 fail-closed。

**验收**：默认构建不会产生“形式上验证成功”的 secure update；负向测试覆盖无 provider、坏签名、错误 key ID、回滚版本。

#### S3-3 单算法重建试点

**推荐**：SHA-256/HMAC 或 Ed25519/Curve25519 中只选一个明确产品需求区域。
**任务**：来源审查、已知向量、API/error contract、内存擦除、目标平台 compile；ASM 仅在 C fallback 与真实 MCU benchmark 后进入。

**验收**：安全结论与性能结论分离；Host correctness 不等于 constant-time/security approval。

#### S3-4 FOTA 状态机去模拟化设计

**路径**：`components/fota/`、`projects/stm32u5_fota/`、bootloader contract 文档。
**任务**：定义 image format、download/install、boot handoff、mark-valid、rollback、anti-rollback、断电恢复；修复示例 compile blocker并纳入 compile gate。

**验收**：项目可编译；尚未完成的硬件步骤返回明确错误，不允许成功 no-op。

---

### Sprint 4（2 周）：Sensor 双轨收敛与数据可靠性 — P1

**目标**：停止驱动数量扩张，统一入口并建立数据/存储可靠性。

#### S4-1 Sensor active-source manifest

**路径**：`components/sensor/CMakeLists.txt`、`components/drivers/sensor/`、root Kconfig。
**任务**：列明 legacy/new/active/deprecated/test-only；禁止 recursive inventory 被误认为 active product support。

**验收**：每个 active driver 有 owner、public API、构建开关、focused test、硬件状态。

#### S4-2 迁移 2–3 个高价值驱动

**推荐**：一个环境传感器、一个 IMU、一个 ADC/电源监测器。
**任务**：迁移到 Device 模型，兼容 wrapper 保留 legacy API；测试 normal/error/output-preservation/re-init。

**验收**：无 duplicate lifecycle；同一驱动只存在一个 active implementation owner。

#### S4-3 DM 掉电与布局恢复

**路径**：`components/dm/`、`tests/unit/dm/`。
**任务**：先补 FS/JSON focused test，再增加可注入掉电的 NVM/Flash 测试：erase/write interruption、CRC/metadata corruption、restart recovery、layout migration。

**验收**：恢复策略和数据丢失边界有明确断言；真实 NOR/FlashDB 仍需板级记录。

#### S4-4 Fuel Gauge 硬件证据

**任务**：验证 SMBus clock stretching、放电期 NACK/retry、告警阈值编程、snapshot 原子性。

**验收**：真实逻辑分析/串口记录；继续保持 standalone，不回并 PM。

---

### Sprint 5（2 周）：RTOS 并发、网络/电源按需求验证 — P1/P2

**目标**：补上 Host 单线程契约之外的运行时证据；PM 按既定路线后置。

#### S5-1 单一 RTOS 后端

**选择**：FreeRTOS 或 RT-Thread，只选一个作为本轮 reference backend。
**覆盖**：OSAL thread/event、IPC MQ/broker、Trace 多任务、Device registry/PM 并发。

**验收**：并发 stress、ISR→task、timeout、资源耗尽、shutdown/re-init；有真实板卡或可信 RTOS 仿真记录。

#### S5-2 Net 产品选择门

**任务**：收敛 AT 实现 ownership、MQTT/CAN/LTE Kconfig；无明确 modem/产品需求则 LTE 保持 default-off。

**若推进 LTE，验收必须包括**：UART/RTS-CTS、电源时序、SIM、attach、PDP、TCP/UDP、URC、断线恢复和长稳；Host adapter 不算完成。

#### S5-3 PM（后置）

**任务**：STM32U5 sleep/wakeup backend、唤醒源、ADC/charger GPIO、功耗记录。

**验收**：至少 active/sleep 两档稳定功耗、唤醒成功率和复位原因；无仪器数据则保持开发中。

---

### Sprint 6（1–2 周）：Release Candidate — P1

**进入条件**：Sprint 0 门禁可信；目标组件硬件证据达标；Crypto/FOTA 安全声明无虚假；远端与本地历史风险已收敛。

**任务**：
- 只选择真正受支持的平台、组件、examples/projects。
- 历史/不可编译样例迁入 archive 或标 unsupported。
- 生成 SBOM、第三方许可证、release notes、Known Limitations、checksum/signature。
- 固定可复现构建环境和 submodule revisions。

**RC 验收**：
- canonical Host suite 全绿；
- PC + 支持 MCU compile matrix 全绿；
- 指定 reference board HIL 全绿；
- 文档能力声明逐项有证据；
- 无 security-rejected implementation 被产品配置选中；
- release artifact 可从 clean checkout 重建。

---

## 6. 推荐优先级与容量分配

### 最近一个开发周期

1. **40%：Sprint 0 CI/事实源/分支治理**
2. **40%：Sprint 1 GUI backend 错误传播、strict backend、字体/显示纵切**
3. **20%：为 Sprint 2 准备 STM32U5 HAL/HIL 夹具和证据模板**

### 后续顺序

```text
可信门禁
  -> GUI 继续
  -> HAL/Device 真实硬件纵切
  -> Crypto 产品级重建 + Secure FOTA 隔离
  -> Sensor/DM/Fuel Gauge 收敛
  -> RTOS/Net/PM 按产品需求实证
  -> Release Candidate
```

这与当前产品路线一致：**GUI 继续；PM 后置；Crypto 产品级重建而非整体否定；Release 最后处理。**

---

## 7. Definition of Done

任何组件只有满足相应层级，才能使用对应声明：

| 声明 | 最低证据 |
|---|---|
| Host-guarded | focused CTest + full Host suite + diff/format gate |
| Target-compilable | clean cross-compile + toolchain/chip/config 记录 |
| QEMU-validated | QEMU 实际运行；明确哪些 HAL 为模拟 |
| Hardware-validated | 实板、连接、固件 commit、日志、negative paths |
| Performance-validated | 固定硬件/频率/编译参数/样本与统计；不可用 synthetic timing 替代 |
| Security-reviewed | 来源/许可证/威胁模型/算法与实现审查/已知向量/side-channel 边界 |
| Production-ready | 上述相关层级 + HIL/长稳/错误恢复/发布制品与已知限制 |

每个代码任务必须：
1. 先写失败测试或明确验证 probe；
2. focused gate 通过；
3. `make test-unit` 通过；
4. 目标平台 compile/HIL 按任务层级通过；
5. `git diff --check`；
6. path-limited commit，不夹带无关 GUI 或其他脏文件。

---

## 8. 本次审计边界

- 本次结论基于当前源码、README/Kconfig/CMake、`tests/unit`、CI、近期 Git 活动和实际构建/测试。
- 178/178 Host 与 46/46 QEMU 是可信执行结果，但不代表全部真实硬件通过。
- 未发现足够的统一 HIL、功耗、总线、掉电恢复或安全审查证据，因此相关状态保持 pending/development。
- 本文只建立规划，不修改组件源码，不宣称新的硬件或安全结论。
