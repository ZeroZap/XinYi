# XinYi 组件证据台账

**建立日期**：2026-08-17
**维护入口**：[Sprint 跟踪看板](../plans/SPRINT_TRACKER.md)
**审计基线**：[全组件状态审计与 Sprint 计划](../plans/2026-08-17-component-audit-sprint-plan.md)

> 本文件记录“我们有什么证据”，不记录愿景。状态只能由真实执行、审查或硬件记录升级。
>
> **当前平台边界（2026-09-04）**：Pandora STM32L475VE 是正式 reference board 与 Sprint 1–4
> 实板验收基线；STM32U5/M33/TrustZone 仅作后续 enhancement compile compatibility，不再阻塞
> 基础验收。SSD1306 deferred，不选择、不推进。无人值守证据仅允许自动构建/测试、ST-Link
> 烧录/复位与独立 UART 采集；人工接线、按键、目视确认和物理故障注入不得作为自动闭环步骤。

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
| Kernel/OSAL | H1（bare-metal） | Q1 部分；[FreeRTOS reference](reference-rtos-decision.md) 已有 project-owned config、pinned V10.4.6 Cortex-M33/CM4F ports；root U5/L4 opt-in 可构建 `freertos_kernel` + `xy_osal` | Pandora STM32L475VE 已取得 scheduler/delay、task-context binary semaphore/message queue/event flags/mutex、SysTick ISR 与 TIM6 peripheral IRQ→OSAL semaphore→task、TIM6 interrupt disable→timeout→restart recovery B2、no-wait memory-pool/queue exhaustion→recovery→delete/recreate、depth-one queue 100-tick blocking timeout、2-producer/2-consumer queue 唯一性及 16 条 payload 的 consumer 归属（9/7）、120 秒 bounded stress，以及逐周期严格覆盖 OSAL→IPC Broker→PM tick→Device registry→CLIB Trace sink→IPC delivery 的跨组件 B1 | pending | bare-metal Host 契约；STM32U5 仍为 compile-only；Pandora 仅证明声明区间内的 task primitives、两个明确 ISR-to-task 路径、受控 TIM6 recovery、资源恢复、blocking-timeout、bounded 2P/2C queue、stress 与跨组件纵切，不是物理故障注入、性能、公平性、多小时耐久或完整产品 RTOS 资格 | 补其他外设 IRQ 负向/恢复、PM sleep/wakeup 与长时间耐久 |
| HAL / Storage | H1（PC；canonical W25Q128 command/error/page-boundary contract；FOTA adapter range/page-split/error contract） | Q1 部分；通用 QSPI HAL API、STM32L4 backend、canonical W25Q128 Device driver 与 FOTA flash-ops adapter；PC/L4/U5 compile | Pandora U9 W25Q128 的 JEDEC、erase、single/quad program/read、recovery 已迁移为 canonical path；FOTA adapter 已在独立 4 KiB candidate 区完成 300-byte erase/write/read B1 | pending | 允许声明 Pandora canonical storage 与 bounded FOTA candidate storage B1/B2；不升级完整 image/CRC/signature、bootloader handoff、掉电/NRST、耐久、性能或 secure FOTA | FOTA core download→CRC validation；掉电/NRST恢复；冻结 legacy Sensor W25Q 与 DM NOR placeholder |
| Device | H1 | PC/L4/U5 build | Pandora board-owned software-I2C → HAL API → canonical I2C Device helper → AHT10 已取得匹配 `b94fc3c2` 固件身份、Flash write/verify/read-back 与 13 个完整 ACK/测量周期的 B1 | n/a | registry/lifecycle 与 I2C helper 软件契约；Pandora 仅证明 board software-I2C/AHT10 正常路径，不等于 hardware-I2C peripheral、任意 Driver 或 B2 | AHT10 NACK→recovery B2；hardware-I2C peripheral 与另一个 canonical Device driver B1/B2 |
| Display drivers | H1（SSD1306 init/refresh error propagation、GUI adapter；LCD/LED transaction contracts） | PC build | pending；SSD1306 实板项自 2026-09-04 `deferred` | pending | Host transaction contract；SSD1306 init 失败不保留 framebuffer 或继续延时假成功；deferred 项不得阻塞 Pandora 非显示工作，也不得从 Host/compile 升级实板或性能声明 | 暂不推进 SSD1306；未来产品决策重新启用后再定义 B1/P1 |
| Storage/24xx | H1（page split、I2C error propagation、re-init recovery） | PC build | pending | pending | fake-I2C Device→Driver 契约；失败事务不假成功且不进入 write delay | 写保护、write-cycle polling、掉电 B2 |
| Sensor legacy | H1 | PC build；[active-source manifest](sensor-active-source-manifest.md) 记录 55 个 root sources | pending | pending | `legacy-active-root` 当前根库 Host 契约；冻结新增型号 | 逐芯片 compatibility wrapper + 代表芯片 B1/B2 |
| Sensor new `xy_*` | H1（独立测试） | [manifest](sensor-active-source-manifest.md) 记录 20 个 `experimental-test-only` sources，未进入根 Sensor target；SHT30/ADS1115/MPU6050 duplicate test-local implementations 已移除 | pending | pending | 仅 test-local Host 实验实现；不得因 focused test 宣称 product-linked | 继续冻结新增并迁移高价值 owner |
| Drivers Sensor | H1（SHT30、ADS1115、MPU6050 与 BMP280 Device owners 已覆盖 focused transaction/error/output-preservation contracts） | PC build；[manifest](sensor-active-source-manifest.md) 记录四个 `device-active-root` sources，且四个 canonical owners 均进入 `xy_drivers` 与 root `sensor_component`；BMP280 legacy lifecycle 仍冻结兼容，未引用的第四生命周期已移除，tracked smart-hygrometer example 使用 canonical owner | pending | pending | Device-model canonical migration destination；四个 owner 的 root/source 边界已明确 | 四个 owner 的 B1/B2；逐步收敛 legacy compatibility wrappers |
| Actuator | H1 | PC build | pending | safety pending | Host 框架可用 | Device adapter + PWM/GPIO B1/B2 |
| Fuel Gauge | H1（未实现安全模式 fail-closed） | PC build | pending | AES/SHA provider pending；plaintext passthrough 已移除 | Host 驱动契约；`NONE` 明文兼容，未接入 provider 的安全模式返回 unsupported 且保持输出 | 受审查 authentication/encryption provider；SMBus B1/B2 |
| Charger | H1（standalone BQ25620 fake-I2C transaction/status contract） | PC build | pending | safety pending | `components/charger/src/xy_bq25620.c` 为 canonical owner、状态 `legacy-maintained`；原文档指向的 `components/drivers/power/charger/` 不存在，禁止迁往空目标 | 真实替代 owner 决策；充电/热故障 B1/B2 |
| Analog Devices | H1 | PC build | pending | calibration pending | active 3-source Host 契约 | MCP3008/HX711 实测与标定 |
| MUX | H1 | PC build | pending | protocol security pending | Host typed ops 可用 | Device adapter/真实跨接口验证 |
| PID | H1 | PC build | pending | performance pending | Host 算法可用 | plant simulation/HIL、抖动和饱和恢复 |
| Trace | H1 | PC build | Pandora CLIB printf sink 已在 IPC consumer task 中完成 17 次有序输出 B1 | throughput pending | Host weak sink/format contract；Pandora 仅证明 bounded UART sink 跨组件正常路径 | RTT/ITM、并发丢日志与吞吐策略 |
| IPC | H1 | Q1 间接/部分 | Pandora Broker producer→consumer dispatch、单调 payload、Device lookup 与 Trace sink 已完成 bounded B1；depth-2 server queue 已取得 fill→`QUEUE_FULL`→clear→后续 pipeline 恢复 B2 | concurrency pending | Host 契约 + Pandora bounded task-context 正常及 queue saturation/recovery；不等于 ISR ingress、吞吐或多 producer/consumer 资格 | ISR ingress、多 producer/consumer 与长时间 stress |
| SYS | H1（timer/SM；默认系统 API fail-closed） | PC/L4/U5 build | Pandora strong backend 已取得稳定 96-bit UID B1 与自动 `NVIC_SystemReset`→`SFTRSTF` reason/recovery B2；Flash write/verify/read-back 与 UART 证据已留存 | n/a | timer/state-machine Host 契约；无 board backend 时 fail-closed；Pandora chip identity 与软件复位原因已实证 | 看门狗与物理复位原因仍 pending；U5 仅补充 compile |
| DM | H1（8 目标；FS lifecycle/path/I/O/error contract，coreJSON parser/search，active `xy_json` parse/mutation/malformed-input contract；NVM newest-complete/restart/torn-append、header/payload partial-write、format partial-erase、metadata/checksum corruption recovery 与 legacy→current layout migration） | PC build | [DM 掉电记录](xinyi-dm-power-loss-validation-record.md)当前为 `HOST_INTERRUPTION_GUARDED`，真实 Flash/board pending | durability pending | Host 数据格式、FS abstraction、active JSON 与 NVM restart/corruption 契约；header/payload exhaustive byte-boundary partial-write 与 256-byte format partial-erase sweep 后可重启并重试；legacy additive checksum/magic 可读，current record 使用 CRC-8 并对 metadata/payload corruption 回退上一完整值；caller-owned storage ops 允许非映射逻辑地址 backend | 目标 Flash program-granule/erase 注入、真实 legacy image dump 与真实 Flash B2 |
| PM | H1 | PC/L4/U5 build | Pandora runtime 已证明 PM platform tick adapter 经 `XY_OSAL_AVAILABLE` 使用 canonical OSAL tick，20 个 bounded 周期无 PM error | power pending | Host framework/fallback；Pandora 仅证明 tick adapter B1，不等于 sleep/wakeup、功耗或 charger/ADC 实板能力 | sleep/wakeup/ADC/charger GPIO 功耗记录 |
| Crypto | H1（契约；SHA-256/HMAC zero-length 与工作状态清理） | C1 部分；本轮仅 PC Crypto target compile | pending | product-classification/owner/origin/license/side-channel/allowed-use 清单已机器守护；provenance 均 review-pending；SM2/ECDSA rejected | contract-guarded；SHA-256/HMAC 已补 API/error/memory-hygiene Host contract，但 production-candidate 仍不等于安全、constant-time、provenance、MCU 或性能批准 | SHA-256/HMAC 外部 provenance/license、独立审计、target compile、fuzz/side-channel；reviewed signature provider |
| GUI | H1（backend 错误传播；SDL fake + real-library headless contract；字体 source-table review；licensed required-UI subset active） | PC build；SDL2 opt-in 缺依赖 fail-closed；canonical CI 用真实 SDL2 headers/library 编译链接 backend，并以 dummy video driver 实跑 window/renderer/texture/fill/flush/event/deinit；fake seam 覆盖错误路径 | pending | legacy 字体视觉 `rejected-needs-regeneration` 且 provenance pending；OFL-1.1 Noto Sans CJK SC 的 15 个 required UI glyph 已按 pinned Host snapshot 接入 active 16x16 table，但尚未视觉/实板批准；performance pending | Host GUI/font/backend contract；SDL headless runtime 仅为 PC runtime evidence，不是人工视觉、性能或屏幕硬件证据；required UI subset 为 distinct/nonblank active table；其余 legacy 16x24/中文 placeholder 不得作为最终产品字体 | 人工视觉审查；替换其余 legacy table；屏幕 B1/P1 |
| Net | H1（core/Modbus/MQTT/AT/CAN/LTE contracts；产品协议 default-off） | PC root；全协议显式 opt-in `xy_net` target | pending | long-run pending | root Kconfig 直接控制 active source/umbrella export；active AT owner 为 lightweight client/server，active MQTT owner 为 `src/xy_mqtt_client.c`；vendor AT trees、legacy MQTT、CAN/LTE 不会因文件存在自动进入产品库 | 明确 modem/CAN 产品选择后补 UART/flow-control/power、attach/PDP/URC/controller 与 B1/B2 |
| FOTA | H1（状态机 + signature-provider/boot-handoff/delta/boot-confirm fail-closed；版本化 candidate header/layout、完整镜像 CRC/vector 校验；validate-before-erase、分块 program/read-back；双记录 generation/CRC/末写 commit-marker 安装 journal；同 generation split-brain 与半范围 generation 歧义拒绝） | PC/FOTA target；L4/U5 compile compatibility；Pandora bootloader `0x08000000` + application `0x08008000` 独立链接 | Pandora 驻留 bootloader 已完成 W25Q128 candidate→internal execution slot 安装、VTOR/MSP/reset handoff与 application marker；软件复位后 journal 命中同 candidate，跳过重复擦写并再次执行 application | Secure FOTA blocked；无 approved provider | Pandora candidate 安装/执行正常路径 B1 与软件复位幂等 B2；Host 覆盖损坏/写失败 recovery 及 ambiguous journal fail-closed。真实掉电、确认/回滚与签名不在当前证据内 | 真实掉电/半写实板注入；安装后确认/失败回滚；reviewed provider/key provisioning |
| CI/Release | canonical Host gate + PC root build 可用（2026-09-01：200/200）；首个 release input `device_driver_template` 已从 `git archive HEAD` clean export 通过独立 configure/build/run；canonical Host committed clean-export gate 从 `git archive HEAD` 独立 configure/build，并在排除 5 个依赖 Git 仓库状态或递归 archive/build 的 policy tests 后通过 195/195；canonical CI 已将常规 Host、committed clean-export 与 PC artifact reproducibility 分步设为必跑；同一 committed source archive 的两次独立 PC Release `xy_device` 构建产出相同 `libxy_device.a` SHA-256/size；[PC release build environment](pc-release-build-environment.json) 固定 Ubuntu 24.04 runner 与 PC/x86_64/Release/config/tool identity contract，[PC release artifact manifest](pc-release-artifact-manifest.json) 将当前选定 artifact set 限定为 `xy_device` / `libxy_device.a` 的 reproducibility gate-only 项；gate 输出每次实际 CMake/CC/AR/Python identity，生成/上传含 source commit/archive hash、artifact-manifest schema/status、双构建 artifact hash/size 与工具身份的机器 JSON evidence，将验证后的 library、SHA-256、CycloneDX JSON 1.6 SBOM、ephemeral Ed25519 signature/public key 及 bounded license/NOTICE review records 作为 14 天 bounded CI artifact 归档，并以独立调用重读校验 checksum/SBOM/signature 与 legal-pending/source-scope 边界；SBOM 绑定 exact artifact/source archive/source commit 与 10 个直接编译源的 SHA-256 和 Apache-2.0 evidence，状态保持 `REVIEW_PENDING`；[bounded technical license review](pc-release-license-review.json) 已记录这 10 个 first-party source、根许可证 hash 与无冲突文件级声明的扫描结果，但 legal/NOTICE/完整 release scope 仍 pending；临时私钥不归档且每次运行丢弃；[release signing policy](release-signing-policy.json) 固定 Ed25519 设计边界，但 release identity 仍为 `UNASSIGNED`、key custody 为 `NOT_ESTABLISHED`、publication 为 `BLOCKED`，ephemeral CI key 不得升级为 release key；[PC release SBOM policy](pc-release-sbom-policy.json) 固定 bounded artifact 的生成/独立验证边界，但不构成法律/license approval、完整 PC/MCU SBOM 或 R1 | PC build；[Kconfig/CMake 配置矩阵](kconfig-cmake-configuration-matrix.md)已建立，all-off 配置不再泄漏关闭组件 target，Device/Crypto/DM/Sensor/Actuator-only 组合的生成值、focused target 与归档对象已验证；Sensor/Actuator 的两个 framework 开关分别映射到各自相同 root target，独立 Device-driver 路径未混入；非法 Display 子功能组合 fail-closed，OLED/SSD1306、LCD SPI/I8080/ST7789 与 LED/serial RGB 合法组合已验证 source selection；Net core 与 MQTT/AT/CAN/LTE 选择均 default-off，显式全协议组合的 `xy_net` target 已验证；无实现源的 standalone RGB 配置已移除；STM32U5 默认组合曾用 `arm-none-eabi-gcc` clean compile，并验证 STM32U5 与 FS/FlashDB 条件默认值 | n/a | [tracked source dependency inventory](source-dependency-inventory.json) 为机器守护的 `REVIEW_PENDING` 前置；bounded artifact SBOM generation 已 guarded，法律/license approval 仍 pending | development CI only；`unit-tests.yml` 为单一可信 ...

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
- [x] `docs/validation/xinyi-dm-power-loss-validation-record.md`（2026-08-28 已建立；当前仅 `HOST_INTERRUPTION_GUARDED`，不构成真实 Flash/板级掉电证据）
- [x] `docs/release/known-limitations.md`
- [x] `docs/release/release-checklist.md`（2026-08-31 已建立；当前 `BLOCKED` / `NO-GO`，有 focused policy guard，不构成 R1）

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
