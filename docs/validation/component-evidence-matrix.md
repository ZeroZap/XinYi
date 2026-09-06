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
| HAL / Storage | H1（PC；canonical W25Q128 command/error/page-boundary contract；FOTA adapter range/page-split/error contract） | Q1 部分；通用 QSPI HAL API、STM32L4 backend、canonical W25Q128 Device driver 与 FOTA flash-ops adapter；PC/L4/U5 compile | Pandora U9 W25Q128 的 JEDEC、erase、single/quad program/read、recovery 已迁移为 canonical path；FOTA adapter 已在独立 4 KiB candidate 区完成 300-byte erase/write/read B1；SPI1 TX DMA 已取得 completion、re-init 与 active-abort recovery B1/B2。2026-09-06 无 RX source 实板运行按预期停在 `PANDORA_SPI_DMA_RX_ERROR`，未取得 RX/full-duplex 证据 | pending | 允许声明 Pandora canonical storage、bounded FOTA candidate storage 与 SPI TX DMA B1/B2；不得将未接 RX source 的 timeout 升级为 RX/full-duplex、电气或外设响应证据 | 使用已确认的物理 loopback/外设补 SPI RX/full-duplex DMA；掉电/NRST恢复；冻结 legacy Sensor W25Q 与 DM NOR placeholder |
| Device | H1 | PC/L4/U5 build | Pandora software-I2C→HAL→Device→AHT10/AP3216C B1；AP3216C continuous `0x03` 有 244-sample bounded IR response B1。hardware I2C3（PC0/PC1）正常路径 B1 后，clean `b1c2429c` 对无人地址 `0x7F` 得到 HAL I/O failure并恢复访问 `0x1E`，取得配置回读、59 samples/27 unique、error 0 与 Flash byte-identical 回读 | n/a | registry/lifecycle、I2C helper、Pandora hardware-I2C3→Device→AP3216C 正常路径 B1及单次无人地址 NACK→后续设备访问 B2；不等于总线物理故障、stuck-bus recovery、定量 ALS/PS 或完整 I2C HAL 资格 | 如需产品阈值则补受控距离/照度夹具；stuck-low/掉线须人工或安全故障夹具 |
| Display drivers | H1（SSD1306 init/refresh error propagation、GUI adapter；LCD/LED transaction contracts；ST7789 checked fill 使用 256-byte bounded buffer并传播 SPI error；RGB/BGR MADCTL transaction contract） | PC/L4/U5 build；Pandora onboard ST7789 SPI3 board target 已链接 | 旧 `e125038e` visual B1 因 Eugene 更正实际顺序为 blue→green→red→white→black 而撤回；修复后的 clean `c2e34a4a` 12,820-byte image 已 write/verify/read-back byte-identical，独立 UART 取得 exact identity、9 个有序 marker、error 0，因此仅有修复镜像 control-path B1。SSD1306 deferred | pending | ST7789 canonical control-path board execution；当前不得声明颜色正确或 visual B1 | 用户复核修复镜像 red→green→blue→white→black 及最终象限颜色后，方可重新升级 visual B1 |
| Storage/24xx | H1（page split、I2C error propagation、re-init recovery） | PC build | pending | pending | fake-I2C Device→Driver 契约；失败事务不假成功且不进入 write delay | 写保护、write-cycle polling、掉电 B2 |
| Sensor legacy | H1 | PC/L4/U5 build；[active-source manifest](sensor-active-source-manifest.md) 记录 55 个 root sources | Pandora ICM20608 已取得在线/配置/静态采样诊断 B1；AP3216C 旧 `0x07` one-shot capture 仅诊断，修为 continuous `0x03` 后取得 static B1，并以 244 samples/139 unique/error 0、IR median 2.5→257.0 的 bounded capture 取得 qualitative stimulus-response B1；V2.4 原理图明确 U8=AP3216C | pending | AP3216C 芯片/寄存器/freshness 与 bounded IR response 已建立；ALS 10–11、PS 0–15，不能声明定量 ALS/PS 响应、距离阈值、精度或 calibration；ICM20608 两次动态 capture 未观察到预期响应 | AP3216C/AHT10 NACK recovery；需要产品阈值时补固定距离/照度夹具；ICM20608 动态诊断保持隔离 |
| Sensor new `xy_*` | H1（独立测试） | [manifest](sensor-active-source-manifest.md) 记录 20 个 `experimental-test-only` sources，未进入根 Sensor target；SHT30/ADS1115/MPU6050 duplicate test-local implementations 已移除 | pending | pending | 仅 test-local Host 实验实现；不得因 focused test 宣称 product-linked | 继续冻结新增并迁移高价值 owner |
| Drivers Sensor | H1（SHT30、ADS1115、MPU6050 与 BMP280 Device owners 已覆盖 focused transaction/error/output-preservation contracts） | PC build；[manifest](sensor-active-source-manifest.md) 记录四个 `device-active-root` sources，且四个 canonical owners 均进入 `xy_drivers` 与 root `sensor_component`；BMP280 legacy lifecycle 仍冻结兼容，未引用的第四生命周期已移除，tracked smart-hygrometer example 使用 canonical owner | pending | pending | Device-model canonical migration destination；四个 owner 的 root/source 边界已明确 | 四个 owner 的 B1/B2；逐步收敛 legacy compatibility wrappers |
| Actuator | H1（framework + GPIO-backed buzzer/RGB/H-bridge motor fail-safe contracts） | PC/L4/U5 build | Pandora PB2 buzzer 与 PE7/PE8/PE9 RGB 已有 control-path evidence；V2.4 原理图确认 PA1=`MOTOR_A`→IA、PA0=`MOTOR_B`→IB。motor `7c779ce4` 7420-byte image 已 write/verify/read-back byte-identical，SHA-256=`c53d349ca30ebc5bb8f5a346185ab87d546f323a77aa6f48741c2a97d97bddfe`；UART 201 bytes 通过 exact identity、forward short-short-long、最终 PA1/PA0=L/L 与 error 0 machine gate；Eugene 现场确认对应振动且最终完全停止 | safety pending | TC214B 型号及 truth table 来自用户提供资料，未独立抓取 datasheet；motor API 固定 L/L standby、方向切换先回 L/L、单步最长 5 秒；仅授予 fixed forward short-short-long 与 standby stop B1，不包含 reverse、H/H brake、PWM、current、性能或 endurance | PWM/current/重复启停耐久；reverse/brake 仅在另行定义安全夹具后验证 |
| Fuel Gauge | H1（未实现安全模式 fail-closed） | PC build | pending | AES/SHA provider pending；plaintext passthrough 已移除 | Host 驱动契约；`NONE` 明文兼容，未接入 provider 的安全模式返回 unsupported 且保持输出 | 受审查 authentication/encryption provider；SMBus B1/B2 |
| Charger | H1（standalone BQ25620 fake-I2C transaction/status contract） | PC build | pending | safety pending | `components/charger/src/xy_bq25620.c` 为 canonical owner、状态 `legacy-maintained`；原文档指向的 `components/drivers/power/charger/` 不存在，禁止迁往空目标 | 真实替代 owner 决策；充电/热故障 B1/B2 |
| Analog Devices | H1 | PC build | pending | calibration pending | active 3-source Host 契约 | MCP3008/HX711 实测与标定 |
| MUX | H1 | PC build | pending | protocol security pending | Host typed ops 可用 | Device adapter/真实跨接口验证 |
| PID | H1 | PC build | pending | performance pending | Host 算法可用 | plant simulation/HIL、抖动和饱和恢复 |
| Trace | H1 | PC build | Pandora CLIB printf sink 已在 IPC consumer task 中完成 17 次有序输出 B1 | throughput pending | Host weak sink/format contract；Pandora 仅证明 bounded UART sink 跨组件正常路径 | RTT/ITM、并发丢日志与吞吐策略 |
| IPC | H1 | Q1 间接/部分 | Pandora Broker producer→consumer dispatch、单调 payload、Device lookup 与 Trace sink 已完成 bounded B1；depth-2 server queue 已取得 fill→`QUEUE_FULL`→clear→后续 pipeline 恢复 B2；handler rejection 现会向调用者传播并计入 dropped，随后消息可恢复处理；TIM6 single-producer ISR ingress 已取得 12 分钟内 12 次 queue-full→192 条严格单调 delivery→recovery、task producer 非饥饿及 1387 个 IPC→PM→Device→Trace→IPC 完整周期 B2 | concurrency pending | Host 契约 + Pandora bounded task-context、queue saturation、handler rejection 与 single-TIM6 ISR ingress 12-cycle stress/recovery；不等于性能吞吐、多 ISR producer、任意 ISR source 或多小时资格 | 多 ISR producer/多小时 endurance stress |
| SYS | H1（timer/SM；默认系统 API fail-closed） | PC/L4/U5 build | Pandora strong backend 已取得稳定 96-bit UID B1；自动 `NVIC_SystemReset`→`SFTRSTF`、IWDG timeout→`IWDGRSTF` 与 ST-Link reset command→`PINRSTF`（无 `BORRSTF`）均取得 reason/recovery B2，且有 Flash write/verify/read-back 与 UART 证据 | n/a | timer/state-machine Host 契约；无 board backend 时 fail-closed；Pandora chip identity、软件复位、看门狗复位与 ST-Link 驱动的 external-pin reset reason 已实证 | power-loss/brownout 区分与 watchdog refresh policy 仍 pending；U5 仅补充 compile |
| DM | H1（8 目标；FS lifecycle/path/I/O/error contract，coreJSON parser/search，active `xy_json` parse/mutation/malformed-input contract；NVM newest-complete/restart/torn-append、header/payload partial-write、format partial-erase、metadata/checksum corruption recovery 与 legacy→current layout migration） | PC build | [DM 掉电记录](xinyi-dm-power-loss-validation-record.md)当前为 `HOST_INTERRUPTION_GUARDED`，真实 Flash/board pending | durability pending | Host 数据格式、FS abstraction、active JSON 与 NVM restart/corruption 契约；header/payload exhaustive byte-boundary partial-write 与 256-byte format partial-erase sweep 后可重启并重试；legacy additive checksum/magic 可读，current record 使用 CRC-8 并对 metadata/payload corruption 回退上一完整值；caller-owned storage ops 允许非映射逻辑地址 backend | 目标 Flash program-granule/erase 注入、真实 legacy image dump 与真实 Flash B2 |
| PM | H1 | PC/L4/U5 build | Pandora runtime 已证明 PM platform tick adapter 经 `XY_OSAL_AVAILABLE` 使用 canonical OSAL tick，20 个 bounded 周期无 PM error | power pending | Host framework/fallback；Pandora 仅证明 tick adapter B1，不等于 sleep/wakeup、功耗或 charger/ADC 实板能力 | sleep/wakeup/ADC/charger GPIO 功耗记录 |
| Crypto | H1（契约；SHA-256/HMAC zero-length 与工作状态清理） | C1 部分；本轮仅 PC Crypto target compile | pending | product-classification/owner/origin/license/side-channel/allowed-use 清单已机器守护；provenance 均 review-pending；SM2/ECDSA rejected | contract-guarded；SHA-256/HMAC 已补 API/error/memory-hygiene Host contract，但 production-candidate 仍不等于安全、constant-time、provenance、MCU 或性能批准 | SHA-256/HMAC 外部 provenance/license、独立审计、target compile、fuzz/side-channel；reviewed signature provider |
| GUI | H1（backend 错误传播；SDL fake + real-library headless contract；字体 source-table review；licensed required-UI subset active） | PC build；SDL2 opt-in 缺依赖 fail-closed；canonical CI 用真实 SDL2 headers/library 编译链接 backend，并以 dummy video driver 实跑 window/renderer/texture/fill/flush/event/deinit；fake seam 覆盖错误路径 | pending | legacy 字体视觉 `rejected-needs-regeneration` 且 provenance pending；OFL-1.1 Noto Sans CJK SC 的 15 个 required UI glyph 已按 pinned Host snapshot 接入 active 16x16 table，但尚未视觉/实板批准；performance pending | Host GUI/font/backend contract；SDL headless runtime 仅为 PC runtime evidence，不是人工视觉、性能或屏幕硬件证据；required UI subset 为 distinct/nonblank active table；其余 legacy 16x24/中文 placeholder 不得作为最终产品字体 | 人工视觉审查；替换其余 legacy table；屏幕 B1/P1 |
| Net | H1（core/Modbus/MQTT/AT/CAN/LTE contracts；产品协议 default-off） | PC root；全协议显式 opt-in `xy_net` target | pending | long-run pending | root Kconfig 直接控制 active source/umbrella export；active AT owner 为 lightweight client/server，active MQTT owner 为 `src/xy_mqtt_client.c`；vendor AT trees、legacy MQTT、CAN/LTE 不会因文件存在自动进入产品库 | 明确 modem/CAN 产品选择后补 UART/flow-control/power、attach/PDP/URC/controller 与 B1/B2 |
| FOTA | H1（状态机、candidate envelope/tool、source-commit-bound reviewed restage、signature-provider/boot-confirm fail-closed；双记录 journal 与 durable attempt/confirm/rollback） | PC/FOTA target；L4/U5 compile compatibility；Pandora bootloader `0x08000000`、application `0x08008000` 与 opt-in candidate programmer 独立链接 | Pandora 一次性 programmer 已将 confirmation-capable `e73254da` candidate（33916 bytes；SHA-256 `507d1610...`）写入 W25Q128 并逐块回读；resident bootloader 随后完成 `INSTALLED→ATTEMPT_COMMITTED→CONFIRM_REQUESTED→CANDIDATE_CONFIRMED→CONFIRM_ACKNOWLEDGED`，且软件复位后 journal 命中同 candidate、跳过重复擦写。14416-byte bootloader ST-Link write/verify/read-back byte-identical；25 秒独立 UART capture SHA-256 `5f3dab7c...`，多轮有序链路无 FOTA error marker | Secure FOTA blocked；无 approved provider | Pandora candidate 安装/执行 B1、软件复位幂等与 durable confirmation B2；Host 证明 malformed candidate/source commit、无效授权和 Flash failure fail-closed。该证据不等于真实掉电/半写、签名安全或量产 updater | 真实掉电/半写；reviewed provider/key provisioning |
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
