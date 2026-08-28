# XinYi 组件证据台账

**建立日期**：2026-08-17
**维护入口**：[Sprint 跟踪看板](../plans/SPRINT_TRACKER.md)
**审计基线**：[全组件状态审计与 Sprint 计划](../plans/2026-08-17-component-audit-sprint-plan.md)

> 本文件记录“我们有什么证据”，不记录愿景。状态只能由真实执行、审查或硬件记录升级。

---

## 1. 证据等级

| 代码 | 证据 | 最低要求 |
|---|---|---|
| H0 | 无有效验证 | 仅源码、文档、stub 或未执行计划 |
| H1 | Host contract | focused CTest + full Host suite |
| C1 | Target compile | clean cross-compile；记录工具链、芯片、配置 |
| Q1 | QEMU runtime | 实际运行；标明测试内模拟部分 |
| B1 | Board smoke | 实板正常路径；记录板卡、接线、固件与日志 |
| B2 | Board negative/recovery | NACK/timeout/掉电/复位/错误恢复等负向证据 |
| P1 | Performance | 固定硬件、频率、编译参数、样本与统计 |
| S1 | Security review | 来源、许可证、威胁模型、实现审查、允许用途 |
| R1 | Release qualified | CI、目标构建、HIL、文档、制品和已知限制全部满足 |

规则：

- 后一级不能自动由前一级推导。
- Host fake、synthetic timing、compile-only 不得填写为 Board/Performance/Security。
- `pending`、`rejected` 和 `unsupported` 都是有效结论，优于虚假通过。

---

## 2. 当前组件证据矩阵

| 组件 | Host | Compile/QEMU | Board | Security/Performance | 当前允许声明 | 下一证据 |
|---|---|---|---|---|---|---|
| Clib | H1 | PC build | pending | pending | Host 软件可用 | MCU size/heap/性能记录 |
| Kernel/OSAL | H1（bare-metal） | Q1 部分 | pending | pending | bare-metal Host 契约 | 单一 RTOS 并发/ISR 板级记录 |
| HAL | H1（PC） | Q1 部分；[HAL 平台实现与证据矩阵](hal-platform-evidence-matrix.md)逐平台列明 source/unsupported/compile 状态 | [STM32U5 HAL/HIL 记录](xinyi-stm32u5-hal-hardware-validation-record.md)为 `BLOCKED_NO_HARDWARE`（2026-08-25） | pending | PC simulation、部分 QEMU；不得从 Host/QEMU/compile 升级实板声明 | 恢复 SDK/toolchain 后重跑 U5 clean compile；按记录补 GPIO/UART/I2C/SPI/IRQ/DMA B1/B2 原始日志与 capture |
| Device | H1 | PC build | pending | n/a | 软件层 registry/lifecycle 可用 | 与真实 Driver 的 B1/B2 纵切 |
| Display drivers | H1（SSD1306 init/refresh error propagation、GUI adapter；LCD/LED transaction contracts） | PC build | pending；[SSD1306 实板记录](xinyi-display-hardware-validation-record.md)为 `BLOCKED_NO_HARDWARE`（2026-08-25） | pending | Host transaction contract；SSD1306 init 失败不保留 framebuffer 或继续延时假成功；不得从 Host/compile 升级实板或性能声明 | 按记录补 SSD1306 板卡/接线/SHA、NACK/timeout/re-init、帧时间与 RAM 证据 |
| Storage/24xx | H1（page split、I2C error propagation、re-init recovery） | PC build | pending | pending | fake-I2C Device→Driver 契约；失败事务不假成功且不进入 write delay | 写保护、write-cycle polling、掉电 B2 |
| Sensor legacy | H1 | PC build；[active-source manifest](sensor-active-source-manifest.md) 记录 55 个 root sources | pending | pending | `legacy-active-root` 当前根库 Host 契约；冻结新增型号 | 逐芯片 compatibility wrapper + 代表芯片 B1/B2 |
| Sensor new `xy_*` | H1（独立测试） | [manifest](sensor-active-source-manifest.md) 记录 20 个 `experimental-test-only` sources，未进入根 Sensor target；SHT30/ADS1115/MPU6050 duplicate test-local implementations 已移除 | pending | pending | 仅 test-local Host 实验实现；不得因 focused test 宣称 product-linked | 继续冻结新增并迁移高价值 owner |
| Drivers Sensor | H1（少量；SHT30、ADS1115 与 MPU6050 Device owners 已覆盖 focused transaction/error/output-preservation contracts） | PC build；[manifest](sensor-active-source-manifest.md) 记录 SHT30/MPU6050/BMP280/ADS1115 四个 `device-active-root` sources；SHT30、ADS1115 与 MPU6050 canonical owners 同时进入 `xy_drivers` ownership 与 root `sensor_component` | pending | pending | Device-model canonical migration destination；SHT30/ADS1115/MPU6050 各只有一个实现 owner | 三个迁移 owner 的 B1/B2；评估 BMP280 duplicate lifecycle |
| Actuator | H1 | PC build | pending | safety pending | Host 框架可用 | Device adapter + PWM/GPIO B1/B2 |
| Fuel Gauge | H1 | PC build | pending | AES passthrough rejected/pending | Host 驱动契约 | AES fail-closed；SMBus B1/B2 |
| Charger | H1（单芯片） | PC build | pending | safety pending | legacy/迁移状态 | ownership 决策；充电/热故障 B1/B2 |
| Analog Devices | H1 | PC build | pending | calibration pending | active 3-source Host 契约 | MCP3008/HX711 实测与标定 |
| MUX | H1 | PC build | pending | protocol security pending | Host typed ops 可用 | Device adapter/真实跨接口验证 |
| PID | H1 | PC build | pending | performance pending | Host 算法可用 | plant simulation/HIL、抖动和饱和恢复 |
| Trace | H1 | PC build | pending | throughput pending | Host weak sink/format contract | UART/RTT/ITM、并发和丢日志策略 |
| IPC | H1 | Q1 间接/部分 | pending | concurrency pending | 单线程/Host 契约可用 | RTOS 并发/ISR stress |
| SYS | H1（timer/SM；默认系统 API fail-closed） | PC build | pending | n/a | timer/state-machine Host 契约；无 board backend 时 reset/identity/version 查询明确返回 unsupported 且保留输出 | reset/bootreason/chip ID strong backend + B1/B2 |
| DM | H1（6 目标） | PC build | pending | durability pending | Host 数据格式/NVM 契约 | FS/JSON test + 掉电/布局恢复 B2 |
| PM | H1 | PC build | pending | power pending | Host framework/fallback | sleep/wakeup/ADC/charger GPIO 功耗记录 |
| Crypto | H1（契约；SHA-256/HMAC zero-length 与工作状态清理） | C1 部分；本轮仅 PC Crypto target compile | pending | product-classification/owner/origin/license/side-channel/allowed-use 清单已机器守护；provenance 均 review-pending；SM2/ECDSA rejected | contract-guarded；SHA-256/HMAC 已补 API/error/memory-hygiene Host contract，但 production-candidate 仍不等于安全、constant-time、provenance、MCU 或性能批准 | SHA-256/HMAC 外部 provenance/license、独立审计、target compile、fuzz/side-channel；reviewed signature provider |
| GUI | H1（backend 错误传播；SDL fake + real-library headless contract；字体 source-table review；licensed required-UI subset active） | PC build；SDL2 opt-in 缺依赖 fail-closed；canonical CI 用真实 SDL2 headers/library 编译链接 backend，并以 dummy video driver 实跑 window/renderer/texture/fill/flush/event/deinit；fake seam 覆盖错误路径 | pending | legacy 字体视觉 `rejected-needs-regeneration` 且 provenance pending；OFL-1.1 Noto Sans CJK SC 的 15 个 required UI glyph 已按 pinned Host snapshot 接入 active 16x16 table，但尚未视觉/实板批准；performance pending | Host GUI/font/backend contract；SDL headless runtime 仅为 PC runtime evidence，不是人工视觉、性能或屏幕硬件证据；required UI subset 为 distinct/nonblank active table；其余 legacy 16x24/中文 placeholder 不得作为最终产品字体 | 人工视觉审查；替换其余 legacy table；屏幕 B1/P1 |
| Net | H1 | LTE C1 部分 | pending | long-run pending | Host 协议与 adapter 契约 | modem UART/flow-control/PDP/URC B1/B2 |
| FOTA | H1（状态机 + signature-provider/boot-handoff/delta/boot-confirm fail-closed；双副本 metadata journal partial/corrupt/read-error recovery、generation 半范围歧义与 equal-generation split-brain 拒绝、公开 backend validation、有限 boot-attempt policy、commit/load 与公开状态转换的一致性校验、active-version rollback-floor 与 backend 地址范围校验、callback adapter） | PC/FOTA target build；`projects/stm32u5_fota/main.c` 以 Arm GNU 15.2 Cortex-M33 `-fsyntax-only -Werror` 通过，仅为源级 C1 前置，不是可链接/可烧录镜像 | pending | Secure FOTA blocked；无 approved provider | Host 原型；STM32U5 skeleton 在注册 callback 和进入事件循环前显式验证 metadata backend，默认未绑定 `.ops` 时 fail-closed；journal 持久化 pending version/slot/attempt count；两份有效 record 的 generation 恰差 `2^31`，或 generation 相同但受 CRC 保护的 payload 冲突时，load/commit 均 fail-closed，load 保持输出且 commit 不擦写 Flash；handoff/confirm/boot-attempt callback 均执行 load→状态转换→commit；已有 pending candidate 时重复 handoff/stage 返回 `XY_FOTA_IN_PROGRESS` 且保持 durable attempt count，防止重复请求绕过 bounded rollback；commit、load 及公开 stage/attempt/confirm 均拒绝非法 slot、未知 flags、pending 字段不一致、同槽候选、`active_version < min_version` 与低于 rollback floor 的候选状态，非法内存状态不被修改，CRC 正确但 payload 语义非法的 newest copy 不会被提升；backend 在任何 Flash I/O 前拒绝会令双槽/record 地址发生 `uint32_t` wrap 的布局；boot-attempt 只有 commit 成功才输出 rollback 决策；commit marker 最后写且 marker/write/read-back 验证失败时擦除目标副本；任一 journal slot read error 不降级为 empty/corrupt；confirm 后才推进 active/min version；缺 signature provider、handoff、delta patch、confirm callback 或有效 metadata backend 均 fail-closed | 为 project skeleton 提供 board-owned Flash ops/保留区；真实 bootloader 调用 boot-attempt adapter；真实掉电矩阵；reviewed provider + key provisioning + production patch algorithm + bootloader link gate 与 B1/B2 |
| CI/Release | canonical Host gate + PC root build 可用（2026-08-26：185/185） | PC build；[Kconfig/CMake 配置矩阵](kconfig-cmake-configuration-matrix.md)已建立，all-off 配置不再泄漏关闭组件 target，Device/Crypto/DM/Sensor/Actuator-only 组合的生成值、focused target 与归档对象已验证；Sensor/Actuator 的两个 framework 开关分别映射到各自相同 root target，独立 Device-driver 路径未混入；非法 Display 子功能组合 fail-closed，OLED/SSD1306、LCD SPI/I8080/ST7789 与 LED/serial RGB 合法组合已验证 source selection；无实现源的 standalone RGB 配置已移除；STM32U5 默认组合曾用 `arm-none-eabi-gcc` clean compile，并验证 STM32U5 与 FS/FlashDB 条件默认值 | n/a | supply-chain pending | development CI only；`unit-tests.yml` 为单一可信 Host/PC gate；`VERSION`、canonical changelog、Known Limitations 与 fail-closed tag workflow 已统一；根与关键组件公开 README 已按本台账降级，CI capability-claim guard 禁止无条件 production-ready 回归；所有 target compile/tag/release 均不构成安全、持久性或硬件证据 | SBOM、可复现制品与 release-qualified HIL R1 |

---

## 3. 证据记录索引

### 已存在模板/记录

- LTE：`docs/validation/xinyi-net-lte-hardware-validation-record-template-2026-08-06.md`
- Fuel Gauge：`docs/validation/xinyi-fuel-gauge-smbus-hardware-validation-record-template-2026-08-06.md`
- GUI 字体硬件：`docs/validation/xinyi-gui-font-rendering-hardware-validation-record-template-2026-08-11.md`
- GUI snapshot review：`docs/validation/xinyi-gui-font-host-snapshot-review-record-template-2026-08-11.md`
- Crypto security/provenance：`docs/validation/xinyi-crypto-security-provenance-review-record-template-2026-08-12.md`
- Crypto benchmark：`docs/validation/xinyi-crypto-benchmark-record-template-2026-08-14.md`

### Sprint 0 待补模板

- [x] `docs/validation/xinyi-stm32u5-hal-hardware-validation-record.md`（2026-08-25 已建立；当前 `BLOCKED_NO_HARDWARE`，有 focused policy guard，未产生实板通过证据）
- [x] `docs/validation/xinyi-display-hardware-validation-record.md`（2026-08-25 已建立；当前 `BLOCKED_NO_HARDWARE`，未产生实板通过证据）
- [ ] `docs/validation/xinyi-dm-power-loss-validation-record.md`
- [x] `docs/release/known-limitations.md`
- [ ] `docs/release/release-checklist.md`

---

## 4. 更新规则

组件发生以下变化时必须同步本台账：

1. 新增/删除 root target 或 active source；
2. 新增 focused CTest、compile probe、QEMU 或 HIL；
3. Kconfig 默认值或 public export 改变；
4. security/provenance 状态改变；
5. README 出现 production、secure、hardware、performance 等能力声明；
6. validation record 从 `pending` 变为 passed/failed/rejected。

每次更新写明：

```text
日期：
组件：
旧等级 -> 新等级：
证据路径/命令：
适用范围：
仍不允许宣称：
提交：
```
