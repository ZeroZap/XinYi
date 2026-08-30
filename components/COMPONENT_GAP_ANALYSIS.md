# 组件差距历史清单

**原始日期**：2026-03-05
**事实校准**：2026-08-30
**状态**：历史差距清单（非当前执行计划）

> 本文保留早期生态差距调查的主题，但不作为当前组件成熟度、产品优先级或支持状态的事实源。
> 当前执行状态以 `docs/plans/SPRINT_TRACKER.md` 为准，能力证据以
> `docs/validation/component-evidence-matrix.md` 为准，root source/config ownership 以 Kconfig、
> CMake 与对应 manifest 为准。Host/PC/QEMU/compile-only 不构成实板、安全、性能或 production-ready 证据。

---

## 1. 为什么降级为历史清单

原报告用目录存在、源码数量和静态百分比推导“完成度”，并据此给出长期集成顺序。后续审计证明
这种口径会混淆以下不同事实：

- source inventory 与 root product target；
- focused Host contract 与真实板卡行为；
- compile/QEMU reachability 与 runtime/HIL；
- 算法向量与安全审查；
- 框架 API 与具体产品需求。

因此删除旧的完成度百分比、组件数量预测和“完整/完善”结论。本文剩余条目只表示曾被识别的
候选能力，不代表已批准 backlog，也不要求为了凑齐生态而集成。

---

## 2. 当前已校准边界

| 领域 | 当前事实边界 | 当前事实源 |
|---|---|---|
| HAL / Device | PC Host contract、部分 QEMU/target compile；目标板 IRQ/DMA/timeout/recovery 仍 pending | `docs/validation/hal-platform-evidence-matrix.md` |
| Sensor | Device model 为 canonical migration destination；legacy 仅保留明确 compatibility boundary；experimental source 不等于 product-linked | `docs/validation/sensor-active-source-manifest.md` |
| GUI / Display | GUI backend、字体 subset、SDL headless 与显示 adapter 有 Host contract；视觉、帧率、RAM 和屏幕实板仍 pending | `docs/validation/component-evidence-matrix.md` |
| Net | root Kconfig 选择的协议默认关闭；active AT/MQTT owner 已冻结；modem/CAN controller/长稳与实板 pending | `components/net/README.md` |
| DM | FS/JSON/NVM 有 Host contract；真实 Flash 掉电、擦写粒度、寿命和板级恢复 pending | `docs/validation/xinyi-dm-power-loss-validation-record.md` |
| Crypto / FOTA | Host fail-closed contract；security provenance/provider、bootloader、board Flash 与实板 pending | `docs/validation/component-evidence-matrix.md` |
| OSAL / RTOS | Bare-metal 有 Host contract；FreeRTOS 有 STM32U5 source/static-library compile 前置；scheduler/ISR/concurrency runtime pending | `docs/validation/reference-rtos-decision.md` |
| Charger / Fuel Gauge | BQ25620 standalone owner 与 Fuel Gauge standalone ownership 已冻结；充电安全、SMBus 与实板 pending | `components/charger/README.md` |

---

## 3. 历史候选能力目录

以下主题只在明确产品需求、owner、许可证、资源预算和验证环境齐备后进入 Sprint。

### 3.1 网络与连接

- TCP/IP、HTTP/HTTPS、WebSocket、DNS、NTP；
- Wi-Fi、LoRa、NRF24L01；
- BLE/GATT/GAP、NFC；
- USB Device/Host、CDC/HID/MSC。

约束：不得因为 third-party/submodule 或协议源码存在就宣称 product-linked。Net/USB/BLE 的具体
选择必须先明确硬件、transport、root Kconfig/CMake owner 和 B1/B2 验收。

### 3.2 存储与文件系统

- FatFS、LittleFS、SPIFFS；
- SD/SDIO；
- NOR/EEPROM 的写周期、掉电恢复、磨损与迁移。

约束：DM 抽象层和 Host fault injection 不等于具体文件系统或 Flash 硬件已验证。引入第三方
文件系统前须记录 license、版本固定、block-device owner 和 recovery matrix。

### 3.3 显示与音频

- LVGL、TFT、E-Ink、LED Matrix；
- I2S、PDM、Audio Codec。

约束：优先完成单一真实显示/音频纵切，不为扩大“驱动数量”新增平行生命周期。视觉、时序、
DMA、帧率、内存和音质必须分别记录，Host snapshot 不能替代。

### 3.4 控制、电源与调试

- Encoder、Stepper、FOC；
- Low Power、Wake Source、Power Domain；
- SEGGER RTT、ITM、CoreSight。

约束：PID、PM、Trace 或 HAL API 存在只说明框架入口存在。控制稳定性、功耗、唤醒成功率、
日志吞吐与丢失策略须在选定 board/RTOS 上验证。

### 3.5 安全与传感器算法

- TLS/SSL、TrustZone、Secure Boot、Key Manager；
- AHRS、Kalman、Complementary filtering。

约束：Crypto Host vector 不构成 security review；Sensor 算法输出不构成精度/校准批准。安全能力
必须有 provenance、license、threat model、provider、key provisioning 和 side-channel 边界；融合
算法必须有数据集、误差指标和目标设备记录。

---

## 4. 候选能力进入 Sprint 的准入条件

候选项只有同时满足下列条件才可从本历史清单进入 tracker：

1. 有明确产品用例和目标硬件，不以“业界项目通常包含”为理由；
2. 已指定唯一 implementation owner、public API 与 root Kconfig/CMake 选择门；
3. 已检查仓库现有 dormant/experimental/source ownership，避免第四套生命周期；
4. 已定义 focused RED→GREEN contract 和适用的 Host/target/HIL gate；
5. third-party 依赖有固定版本、许可证与可重现获取路径；
6. 文档声明按 H1/C1/Q1/B1/B2/P1/S1/R1 分层，不从低层证据自动升级。

未满足条件的条目保持历史候选，不自动排期，也不影响当前 Sprint 的完成度。

---

## 5. 当前执行入口

- Sprint 状态与优先级：`docs/plans/SPRINT_TRACKER.md`
- 组件审计与 DoD：`docs/plans/2026-08-17-component-audit-sprint-plan.md`
- 组件证据等级：`docs/validation/component-evidence-matrix.md`
- Known limitations：`docs/release/known-limitations.md`
- Root build facts：`AGENTS.md`、`Makefile`、`CMakeLists.txt`、`Kconfig`

后续更新不得恢复静态“完成度”百分比，或用目录/源码/测试数量替代可追溯证据。
