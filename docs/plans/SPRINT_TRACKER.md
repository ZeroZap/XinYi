# XinYi Sprint 跟踪看板

**建立日期**：2026-08-17
**当前阶段**：Sprint 5 — Pandora STM32L475VE reference-board 闭环；STM32U5 仅保留增强兼容门，Release 门禁按证据保持阻塞
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
| S0-02 | P0 | 将本地 477 个提交直接推送 `origin/main`，以服务器作为单机开发备份 | DONE | Zero | SSH/远端权限 | 2026-09-04 复核 `HEAD`、tracking ref 与远端 `refs/heads/main` 均为 `a000ca66cc141ef8d47836769cde356b03a712ef`，ahead/behind `0/0`；初始 477 个提交已于 2026-08-23 无历史重写推送 | `02a9be45`；复核 `a000ca66` | 2026-09-04 |
| S0-03 | P0 | 收敛 canonical CI workflow | DONE | Zero | S0-02（DONE） | canonical `unit-tests.yml`：Host 178/178 + PC root build；删除 stale `ci.yml`/`ci-cd.yml`，移除过期 `-DPLATFORM`、empty root CTest 与无说明 `|| true` 路径；`git diff --check` | `045a9e56` | 2026-08-23 |
| S0-04 | P0 | 建立 Kconfig/CMake 配置组合矩阵 | DONE | Zero | S0-03（DONE） | [矩阵](../validation/kconfig-cmake-configuration-matrix.md)已覆盖 Display 全组合、all-off、Device/Crypto/DM/Sensor/Actuator-only 及 STM32U5 默认组合；STM32U5 clean root compile、条件默认值检查、Host 178/178、PC root build 与 `git diff --check` 通过 | `cc1b3b75`～`e4c0ff4c` | 2026-08-24 |
| S0-05 | P0 | 统一版本、tag、release note 与 workflow 触发 | DONE | Zero | S0-03（DONE） | `VERSION` 驱动 root CMake；Kconfig/public header 为受检镜像；canonical changelog、Known Limitations 与 fail-closed `vMAJOR.MINOR.PATCH` workflow 已建立；release facts 正/负向 probe、workflow YAML、Host 178/178、PC root build 与 `git diff --check` 通过 | `5b1943e1` | 2026-08-24 |
| S0-06 | P0 | 降级无证据的 production/security/hardware 宣称 | DONE | Zero | 组件证据台账 | 根/组件/HAL/Net/Modbus/OSAL/Crypto 公开 README 已降级到 Host/compile/hardware/security 分层口径并链接证据台账；CI capability-claim guard、release facts、workflow YAML、Host 178/178、PC root build、`git diff --check` 通过 | `73464378` | 2026-08-24 |
| S0-07 | P1 | GUI Sprint 1 任务细化与失败测试清单 | DONE | Zero | GUI 独立变更规则 | 审计计划已明确 backend 错误传播、SDL strict、字体/显示三组任务及验收；首个失败测试已于 Sprint 1 RED→GREEN | `e4faf3c3` | 2026-08-24 |
| S0-08 | P1 | STM32U5 HAL/HIL 夹具与记录模板准备 | DONE | Zero | S0-04（DONE）；实板执行仍依赖板卡/仪器 | 已建立 fail-closed record、夹具/接线清单及 GPIO/UART/I2C/SPI/IRQ/DMA normal/negative/recovery 场景；focused policy guard、Host 181/181、PC root build 与 `git diff --check` 通过；当前环境缺板卡、调试器、SDK checkout 与 ARM toolchain，分类保持 `BLOCKED_NO_HARDWARE` | `3e4c2c54` | 2026-08-25 |

### Sprint 0 退出条件

- [x] 本地 477 个提交与后续已验证提交均已直接推送到 `origin/main`；2026-09-04 复核本地/远端 SHA 均为 `a000ca66cc141ef8d47836769cde356b03a712ef`，ahead/behind `0/0`。
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
| S1-04 | P1 | SSD1306 单一显示纵切记录 | CANCELLED | - | 产品路线已调整 | 2026-09-04 明确 deferred；当前 Sprint 不选择、不推进，也不作为其他工作阻塞项。既有 Host/模板记录保留但不升级为 B1/B2 | deferred | 2026-09-04 |
| S1-05 | P1 | 恢复 SDL2 backend source 与 Host contract | DONE | Zero | S1-02（DONE） | 新增 explicit-context SDL2 backend；fake seam 覆盖错误与 RGB565 contract；real-library `gui_sdl_runtime` 使用 dummy video driver 实跑 window/renderer/texture/fill/flush/event/deinit，并修复无 accelerated renderer 时缺少 software fallback 的 headless 初始化失败；Host 180/180、默认 PC root、SDL-enabled `xy_gui`、real SDL runtime 与 `git diff --check` 通过。不宣称人工视觉、性能或硬件证据 | `2fd4e668`～`a881eb55` | 2026-08-25 |
| S1-06 | P1 | SSD1306 实板验证记录与阻塞探测 | DONE | Zero | S1-03（DONE）；S1-04 硬件阻塞 | 已建立 fail-closed 记录，覆盖板卡/接线/SHA、init/fill/text/flush、NACK/timeout、re-init、帧时间与 RAM；2026-08-25 focused 2/2、Host 180/180、`git diff --check` 通过；`lsusb` 未发现开发板/调试器，且无 `/dev/ttyACM*`/`ttyUSB*`，故记录为 `BLOCKED_NO_HARDWARE`，不填写实板通过 | `f9ebc4a9` | 2026-08-25 |

---

## 4. 后续 Sprint 队列

### Sprint 2 前置看板

> Pandora STM32L475VE 是正式 reference board 与 Sprint 1–4 当前实板验收基线；STM32U5/M33/
> TrustZone 只保留后续 enhancement compile compatibility，不再阻塞基础验收。SSD1306 deferred，
> 当前不选择、不推进、不作为依赖。无人值守运行仅执行构建、自动测试、ST-Link 烧录/复位与
> 独立 UART 采集；不得依赖人工接线、按键、目视确认或物理故障注入。

| ID | 优先级 | 工作项 | 状态 | 负责人 | 依赖 | 验收/证据 | 分支/提交 | 更新时间 |
|---|---:|---|---|---|---|---|---|---|
| S2-01 | P0 | HAL 平台实现与证据矩阵 | DONE | Zero | S0-08（DONE） | STM32U5/F4/L4/WCH/HC32 的 implementation/unsupported/Host/compile/QEMU/HIL 边界已逐项记录；未升级实板声明 | `fef4f1fe` | 2026-08-26 |
| S2-02 | P0 | Pandora GPIO/UART/I2C/SPI/IRQ/DMA 实板基础外设 | IN_PROGRESS | Zero | Pandora STM32L475VE；自动化链路 | Pandora 已有 board-local GPIO/UART/software-I2C/KEY0 B1、FreeRTOS SysTick/TIM6 ISR→task、framework DMA mem2mem/IRQ/recovery，以及 SPI1 TX request→DMA1 Channel3→framework callback→OSAL task B1/B2（含 active abort→re-init→retransmit）。2026-09-06 实板复核确认不接 SPI RX source 时 RX DMA 超时并输出 `PANDORA_SPI_DMA_RX_ERROR`，因此移除会虚报 internal RX/full-duplex 的路径；电气/外设响应、RX/full-duplex DMA 与对应 recovery 仍需真实 loopback/peripheral，不在无人值守下伪造。STM32U5 仅保留 compile compatibility | `683ab5ea`～`480c3f1c`、`f322534a` + 本记录提交 | 2026-09-06 |
| S2-03 | P0 | Pandora I2C→Device helper→现有非 SSD1306 设备纵切 | IN_PROGRESS | Zero | S2-02；Host 前置已完成 | Pandora board-owned software-I2C 已置于 `xy_hal_i2c_master_*` 后并通过 canonical `xy_i2c_device_*` 驱动板载 AHT10 与 AP3216C。AP3216C 旧 `0x07` one-shot 误用已修为 continuous ALS+PS `0x03`；clean `f320087c` image write/verify/read-back byte-identical。随后人工 bounded capture 保留 16,812 bytes/244 samples/139 unique lines/error 0；first-50 IR median 2.5，later 50-sample rolling median 达 257.0，机器 fail-closed validator 判定 bounded IR stimulus-response B1。ALS 仅 10–11、PS 仅 0–15，不升级定量 ALS/PS 响应。仍需真实 NACK→recovery B2 与 hardware-I2C peripheral；SSD1306 排除，U5 仅 compile 补充 | `72391b51`～`f320087c` + 本记录提交 | 2026-09-06 |
| S2-04 | P1 | Pandora SYS reset/bootreason/chip-ID strong backend | DONE | Zero | Pandora STM32L475VE board ownership 已确定 | board strong backend 已接入；稳定 96-bit UID、software reset、IWDG timeout reset 与 ST-Link reset pin 已分别取得 write/verify/read-back 和独立 UART B1/B2。external reset capture 的 CSR=`0x04000600`，后续 AHT10 继续运行；power-loss/brownout 与人工按键仍 pending。U5 不阻塞本项 | `620b06a0`、`28f4b21d`、`12bf990f`、`960f68b0` + 本记录提交 | 2026-09-06 |
| S2-05 | P0 | Pandora ICM20608 静态采样与动态响应诊断 | IN_PROGRESS | Zero | S2-03；Pandora I2C3 `0x68` | `WHO_AM_I=0xAE`、配置回读、静态约 1g 与 raw 14-byte burst 逐次变化已实证；58/58 raw burst 唯一、data-ready active、无 I/O error，排除当前软件路径的冻结读取和 double-init。两次人工移动/约 90° 姿态 capture 未出现应有轴重分配或 gyro 响应，故 dynamic B1 不通过；需先确认实际移动的是含 ICM20608 的板体 | `997f46f8`、`e3c5f87d` + 诊断记录提交 | 2026-09-06 |

### 后续 Sprint 队列

### Sprint 3 前置看板

> Sprint 2 的实板工作继续保持阻塞时，只推进不冒充安全批准的 Crypto/FOTA 边界。
> `production-candidate` 仅表示下一步重建/审查候选；provenance、side-channel、硬件与产品批准仍须独立证据。

| ID | 优先级 | 工作项 | 状态 | 负责人 | 依赖 | 验收/证据 | 分支/提交 | 更新时间 |
|---|---:|---|---|---|---|---|---|---|
| S3-01 | P0 | Crypto 产品算法清单 | DONE | Zero | Sprint 0 证据边界（DONE） | 11 个算法区域已记录 product classification、implementation owner、source origin、license status、side-channel target、allowed usage、runtime/focused sources 与 review record；policy RED 后 focused 5/5、Host 183/183、PC root build、`git diff --check` 通过；SM2/ECDSA 强制 `security-rejected`，无安全批准升级 | `fdce5449` | 2026-08-26 |
| S3-02 | P0 | Signature provider 边界与 Secure FOTA fail-closed | DONE | Zero | S3-01（DONE） | Secure FOTA 不再调用 format-only ECDSA placeholder；缺 provider、provider 拒绝（含错误 key ID）、回滚版本及截断包均 fail-closed；focused 1/1、Host 184/184、PC root/FOTA target build、`git diff --check` 通过；不升级安全批准 | `12cdb5f6` | 2026-08-26 |
| S3-03 | P0 | SHA-256/HMAC 单算法重建试点 | DONE | Zero | S3-01（DONE） | RED 证明 zero-length `NULL` 输入被错误拒绝；实现后 SHA-256/HMAC focused 2/2、Host 184/184、PC root 与 Crypto-enabled `xy_tiny_crypto` target build、`git diff --check` 通过；补充 SHA-256 context 与 HMAC working-key/pad volatile clearing。仍缺 provenance、独立审计、target compile、side-channel 与硬件证据 | `dc47807c` | 2026-08-26 |
| S3-04 | P0 | FOTA 状态机去模拟化与 bootloader contract | DONE | Zero | S3-02（DONE）；Pandora board owner | Pandora 驻留 bootloader 已从 W25Q128 candidate 安装并跳转 `0x08008000` application；双记录 journal、attempt/confirmed/rollback-required 与 exact-candidate reviewed restage 均 fail-closed。新增 source-commit-bound restage authorization、candidate envelope 校验/metadata/C-header 生成器及 opt-in 一次性 W25Q128 programmer，chunk write 后逐块 read-back。当前 confirmation-capable `e73254da` application candidate（33916 bytes，SHA-256 `507d1610...`）经 programmer 实板写入并回读，随后 resident bootloader 完成 `install→attempt→request→durable confirm→ack`；14416-byte bootloader ST-Link write/verify/read-back byte-identical，25 秒 UART capture SHA-256 `5f3dab7c...`，多轮链路无 FOTA error marker。Host 213/213、PC/L4/U5 compile 与 `git diff --check` 通过。该结果关闭 bounded software-reset confirmation B1/B2；真实掉电/半写与 approved signature provider 仍缺 | `54d8b735`～`e73254da` + 本 slice 提交 | 2026-09-06 |
| S4-01 | P0 | Sensor active-source manifest 与三轨 ownership 冻结 | DONE | Zero | S3-04 的 board-owned 集成阻塞不影响治理子项 | manifest 初始校准 55 个 `legacy-active-root`、23 个 `experimental-test-only` 与 4 个 `device-active-root`；SHT30/ADS1115/MPU6050 duplicate test-local owners 移除后当前为 55/20/4；policy CTest 防止 source ownership 漂移并冻结第四套生命周期；不升级硬件声明 | `f691bd23`、`2fde1393` | 2026-08-28 |
| S4-02 | P1 | SHT30 canonical Device owner 迁移 | DONE | Zero | S4-01（DONE） | Device driver 为单一实现 owner；root `sensor_sht30.c` 改为 compatibility-only 委托，保留 0x44 API 并支持 0x45；focused 3/3、Host 186/186、PC root/`sensor_component` 与 `git diff --check` 通过；不升级硬件声明 | `10543486`～`a60ee6de` | 2026-08-28 |
| S4-03 | P1 | ADS1115 canonical Device owner 迁移 | DONE | Zero | S4-01（DONE） | Device driver 吸收 channel/diff/PGA/data-rate/voltage/error/output-preservation contract；删除 duplicate test-local source/header；focused 3/3、Host 186/186、PC root/`sensor_component`/`xy_adc` 与 `git diff --check` 通过；不升级硬件声明 | `f317cb11` | 2026-08-28 |
| S4-04 | P1 | MPU6050 canonical Device owner 迁移 | DONE | Zero | S4-01（DONE） | Device driver 吸收 range/calibration/converted-output/error-preservation contract；删除 duplicate test-local source/header；focused `sensor_mpu6050`、Host 186/186、PC root/`sensor_component` 与 `git diff --check` 通过；不升级硬件声明 | `bb5863d7` | 2026-08-28 |
| S4-05 | P1 | DM FS focused contract | DONE | Zero | S4-04（DONE） | RED：未注册 FS mount 空指针崩溃；新增 lifecycle/path/I/O/seek/close-error focused contract，修复未注册 mount、deinit/close 错误传播与 seek 边界；focused 1/1、Host 187/187、PC root/`xy_dm` 与 `git diff --check` 通过 | `fa0a8044` | 2026-08-28 |
| S4-06 | P1 | DM active `xy_json` focused contract | DONE | Zero | S4-05（DONE） | RED：trailing/incomplete JSON、非法 number token 被接受且重复 key 形成 duplicate member；新增 dedicated parse/mutation/guard contract，修复完整输入消费、字符串终止、严格 number grammar、重复 key replacement 与 realloc fail-closed；focused 1/1、Host 188/188、PC root/`xy_dm` 与 `git diff --check` 通过 | `9b64f4fb` | 2026-08-28 |
| S4-07 | P1 | DM NVM restart/torn-append recovery | DONE | Zero | S4-06（DONE） | RED：partial header 使后续 retry 错误返回 FULL；现对 header/payload 每个 byte boundary 注入 partial-write failure，并对 256-byte Host format 区域逐 byte 注入 partial erase；错误均传播，重启只提升完整记录且后续 write/erase 可重试；caller-owned storage ops 与 fail-closed 掉电记录已建立；focused/full/PC/`xy_dm` gate 通过 | `b8db6135`～`04fe1026` | 2026-08-29 |
| S4-08 | P1 | DM NVM layout migration contract | DONE | Zero | S4-07（DONE） | RED：legacy/current record 无版本区分，新 append 仍写 legacy magic；实现 legacy magic 可读、current magic 只写，并验证同 key 跨格式 append/restart 提升最新值；focused、Host 188/188、PC root/`xy_dm`、`git diff --check` 通过 | `b9742b98` | 2026-08-29 |
| S4-09 | P1 | DM NVM metadata/checksum corruption contract | DONE | Zero | S4-08（DONE） | RED：caller-owned backend 使用非映射逻辑地址时 checksum scan 直接解引用并崩溃；V2 record 改用 CRC-8，legacy additive checksum 保持兼容；逐 byte 破坏 V2 magic/key/enable/length/CRC/payload 后重启均回退上一完整值；focused、Host 188/188、PC root/`xy_dm`、`git diff --check` 通过 | `0966f67d` | 2026-08-29 |
| S4-10 | P0 | Fuel Gauge security passthrough fail-closed | DONE | Zero | D-002 | RED：AES128 encrypt 仍返回成功并原样复制明文；现未接入受审查 provider 的安全模式返回 `XY_FG_ERROR_NOT_SUPPORTED`，encrypt/decrypt 输出与长度保持不变；`NONE` 明文兼容行为保留；focused、Host、PC root/`xy_fuel_gauge` 与 `git diff --check` 通过；不构成安全批准 | `4fb59bf8` | 2026-08-29 |
| S4-11 | P1 | BMP280 canonical Device owner contract | DONE | Zero | S4-04（DONE） | RED：Device owner 缺双地址 API；现补 Bosch 补偿、初始化/反初始化错误传播、缓存输出保持与 root `sensor_component` ownership；focused 3/3、Host 189/189、PC root/`sensor_component` 与 `git diff --check` 通过；不升级硬件声明 | `35b2f7d0` | 2026-08-29 |
| S4-12 | P1 | BMP280 stale lifecycle/example 收口 | DONE | Zero | S4-11（DONE） | RED policy probe 发现未引用第四生命周期仍存在，且 smart-hygrometer 指向不存在的 experimental source/旧三参数 API；移除 stale source、切换 canonical owner，并加入防回归 guard；focused 4/4、Host 189/189、PC root/`sensor_component` 与 `git diff --check` 通过 | `219917db` | 2026-08-29 |
| S4-13 | P1 | Charger ownership 与弃用文档事实收口 | DONE | Zero | S4-12（DONE） | RED probe 证明 `DEPRECATED.md` 指向不存在的 `components/drivers/power/charger/`；已校准 standalone BQ25620 canonical owner、legacy-maintained/Host-only 边界并加入 policy guard；focused 2/2、Host 190/190、PC root、Charger-enabled `charger` target 与 `git diff --check` 通过；不升级硬件/安全声明 | `5c18b117` | 2026-08-29 |

### Sprint 5 前置看板

> 只选择一个 reference RTOS。选择记录不等于 compile/runtime/ISR/并发或实板证据；
> 在 project-owned config、Cortex-M33 port 与 runtime stress 到位前不得标记 `DONE`。

| ID | 优先级 | 工作项 | 状态 | 负责人 | 依赖 | 验收/证据 | 分支/提交 | 更新时间 |
|---|---:|---|---|---|---|---|---|---|
| S5-01 | P0 | 单一 reference RTOS 并发验证 | IN_PROGRESS | Zero | reference board/runtime fixture | `REFERENCE_SELECTED`：选择 FreeRTOS；project-owned config、pinned V10.4.6 Cortex-M33 non-secure port 与 Arm GNU `-Werror` 9-object gate 已建立；root STM32U5 Kconfig/CMake opt-in 现构建匹配的 `freertos_kernel` + `xy_osal`，PC 误选 fail-closed；backend/kernel/OSAL 与公开 component index/introduction/priority README 的无证据完成度、性能及多 RTOS 声明已降级并由 policy guard 防回归。MPS2-AN505 runtime spike 在 secure/non-secure boot/alias 边界 HardFault，未提交为 runtime gate；仍缺 board-correct runnable link/runtime、ISR→task、并发 stress 与 B1/B2；static-library compile 不升级运行时/实板声明 | `a7de72e7`～`18c61df6` | 2026-08-30 |
| S5-02 | P1 | 公开 HAL/FOTA 组件状态证据校准 | DONE | Zero | S2/S3 Host 前置与证据矩阵 | RED guard 证明公开 component index 仍将 HAL 标为“完善”、STM32U5 标为“完整实现”并将 FOTA 标为“主线可用”；现统一为 HAL Host/部分 QEMU、FOTA Host fail-closed，Board/security/runtime 均 pending；focused 3/3、Host 193/193、PC root 与 `git diff --check` 通过 | `323fbca3` | 2026-08-30 |
| S5-03 | P1 | 公开组件完成度与静态测试计数校准 | DONE | Zero | S5-02（DONE） | RED guard 证明 component index 仍将 CLib/Trace/PID/ADDC 标为“完善”、Sensor 标为“已收口”，并维护已漂移的 234 项静态计数与 81% maturity 比例；现统一为分层 Host contract/pending 边界，测试数改以 canonical CTest 为准；focused、Host 193/193、PC root 与 `git diff --check` 通过 | `0364bb8c` | 2026-08-30 |
| S5-04 | P1 | Net 产品选择门与 active owner 收口 | DONE | Zero | S5-01 runtime 阻塞不影响配置治理 | RED guard 证明 NETWORK/MQTT 默认开启、AT/MQTT 源无选择门且 CAN/LTE Kconfig 缺失；现 NETWORK 与 MQTT/AT client/AT server/CAN/LTE 均 default-off，root Kconfig 直接驱动 active source、umbrella export 与 compile definitions；active AT/MQTT owner 已冻结。focused 6/6、全协议 `xy_net` root opt-in target、Host 194/194、PC root 与 `git diff --check` 通过；不升级 modem/CAN controller/长稳/实板声明 | `3e8181fc` | 2026-08-30 |
| S5-05 | P1 | Net README 与 active selection 事实同步 | DONE | Zero | S5-04（DONE） | RED guard 证明 README 仍把 CAN 标为 implemented、示例指向未选 AT-Command-V2，并称 MQTT 尚未由 root/umbrella 接入；已切换为 active AT owner 示例、selected MQTT export 与 Host/hardware pending 边界；focused 5/5、Host 194/194、PC root 与 `git diff --check` 通过 | `51f7902e` | 2026-08-30 |
| S5-06 | P1 | 公开 components README 驱动/构建事实校准 | DONE | Zero | S5-03（DONE）；S4 canonical ownership | RED guard 证明公开 README 仍以无条件 `✅` 标记 experimental/legacy 驱动与 PM/FOTA/Crypto 等组件，并建议绕过 root selection 直接加入 driver 子目录；已改为 canonical owner、Host/Board 分层与 root Kconfig/CMake 事实源；focused 1/1、Host 194/194、PC root 与 `git diff --check` 通过 | `f17305ed` | 2026-08-30 |
| S5-07 | P1 | Charger/Fuel Gauge 架构历史事实校准 | DONE | Zero | S5-06（DONE）；S4-13（DONE） | RED guard 证明两份公开架构/重构记录仍把不存在的 `components/drivers/power/charger/` 写成已迁移 owner；已改为 BQ25620 standalone canonical owner、Fuel Gauge independent 与历史提案边界；focused 1/1、Host 194/194、PC root 与 `git diff --check` 通过 | `81b9bc04` | 2026-08-30 |
| S5-08 | P1 | 旧架构重组执行命令收口 | DONE | Zero | S5-07（DONE） | RED guard 证明 `ARCHITECTURE_REFACTORING_PLAN.md` 仍推荐并可复制执行不存在的 power/charger 与 Fuel Gauge 批量迁移；现降级为 superseded 历史提案，固定 BQ25620 standalone canonical owner、Fuel Gauge standalone 与 Device-model 迁移边界；focused 1/1、Host 194/194、PC root 与 `git diff --check` 通过 | `6f9937d7` | 2026-08-30 |
| S5-09 | P1 | 组件差距清单成熟度事实校准 | DONE | Zero | S5-03（DONE）；S5-08（DONE） | RED guard 证明 `COMPONENT_GAP_ANALYSIS.md` 仍以静态百分比宣称 HAL/FOTA/GUI 等完整并按生态凑数排期；现降级为历史候选清单，删除无证据成熟度/数量预测，改以 tracker/evidence/root selection 为事实源；focused 1/1、Host 194/194、PC root 与 `git diff --check` 通过 | `9e2ab686` | 2026-08-30 |
| S5-10 | P1 | 历史构建系统报告事实校准 | DONE | Zero | S0-03/S0-04（DONE）；S5-09（DONE） | RED guard 证明 `docs/build_system_analysis.md` 仍包含无证据“完善”状态、8.5/10 评分及不存在的 build/config 命令；已降级为 superseded 历史报告并改指 root facts 与配置矩阵；focused 1/1、Host 194/194、PC root、`git diff --check` 通过 | `a7319445` | 2026-08-30 |
| S5-11 | P1 | Release fail-closed checklist 与 R1 边界 | DONE | Zero | S0-05/S0-06（DONE）；S5-10（DONE） | RED probe 证明 checklist 缺失；现建立 source/version、Host/target、HIL/recovery、security/SBOM、reproducible artifacts/checksum/signature 的 fail-closed 门禁，当前固定 `BLOCKED`/`NO-GO`；focused 1/1、Host 195/195、PC root 与 `git diff --check` 通过，不升级 R1 | `bb556e5b` | 2026-08-31 |
| S5-12 | P1 | Tracked source dependency inventory 前置 | DONE | Zero | S5-11（DONE） | RED：focused guard 因 inventory 缺失失败；现机器守护 7 个 vendored source inputs 与 10 个 top-level gitlinks/path/SHA，旧候选许可证清单标记 superseded；状态固定 `REVIEW_PENDING`，不冒充 artifact SBOM/license approval；focused 1/1、Host 196/196、PC root 与 `git diff --check` 通过 | `86f6cdde` | 2026-08-31 |
| S5-13 | P1 | Examples/projects release input 清单 | DONE | Zero | S5-11（DONE）；S5-12（DONE） | RED：focused guard 因 release input inventory 缺失失败；现机器守护 16 个 tracked top-level examples 与 16 个 projects，逐项区分 Host/compile-only/candidate/historical，且全部保持 `excluded-pending-review`；focused 5/5、Host 197/197、PC root 与 `git diff --check` 通过，不升级 release/hardware 声明 | `9f7bf41d` | 2026-08-31 |
| S5-14 | P1 | 首个 release input clean-export 可复现 gate | DONE | Zero | S5-13（DONE） | RED：直接配置 exported canonical unit tree 因 ignored Crypto sources 缺失而 fail；现使用 tracked minimal CMake smoke 从 `git archive HEAD` 配置、构建并运行 canonical device-driver template，release scope 仍为 `excluded-pending-review`；focused 3/3、Host 198/198、PC root 与 `git diff --check` 通过 | `b402ba74` | 2026-08-31 |
| S5-15 | P0 | Clean export canonical Host source closure | DONE | Zero | S5-14（DONE） | RED：`git archive HEAD` 的 canonical unit configure 因 LWC runtime sources 被 `.gitignore` 排除而失败；现跟踪 Ascon/TinyJambu/Photon Beetle canonical module sources，staged-tree clean export configure/build 后 195/195 非递归 CTest 通过，focused `crypto_lwc`、Host 198/198、PC root 与 `git diff --check` 通过；不升级 LWC 安全/provenance/release 声明 | `cd48110b` | 2026-08-31 |
| S5-16 | P0 | Canonical Host committed clean-export gate | DONE | Zero | S5-15（DONE） | 将一次性 staged-tree probe 固化为 `git archive HEAD` 独立 configure/build/CTest；排除 4 个依赖 Git 仓库状态或递归 archive 的 policy tests，其余 195/195 通过；Host 199/199、PC root 与 `git diff --check` 通过 | `a383327e` | 2026-08-31 |
| S5-17 | P0 | Canonical CI 接入 committed clean-export gate | DONE | Zero | S5-16（DONE） | canonical workflow 将常规 Host 198 项与 committed clean-export gate 显式分步执行，避免递归/重复且确保该 gate 在 CI 必跑；workflow YAML、focused 1/1（内部 195/195）、Host 199/199、PC root 与 `git diff --check` 通过 | `8025a05b` | 2026-08-31 |
| S5-18 | P0 | PC static-library artifact reproducibility 最小 gate | DONE | Zero | S5-17（DONE） | RED：root 默认配置没有 `xy_device` target，证明 artifact probe 必须显式固定 release config；现从同一 `git archive HEAD` 两次独立配置/构建 PC Release `xy_device`，比较 `libxy_device.a` SHA-256 与 size，并在 canonical CI 独立必跑；focused 1/1、Host 200/200、PC root 与 `git diff --check` 通过。仅为单一 PC static library，不构成 tagged release/target/SBOM/R1 | `8aa76252` | 2026-08-31 |
| S5-19 | P0 | PC release build environment/tool identity 固定 | DONE | Zero | S5-18（DONE） | RED：artifact gate 只记录 hash/size，CI runner 使用漂移的 `ubuntu-latest`；现固定 `ubuntu-24.04`，机器守护 PC/x86_64/Release/target/artifact/config/tool 命令并在每次 gate 输出实际 CMake/CC/AR/Python identity；focused 1/1、Host 200/200、PC root、workflow YAML 与 `git diff --check` 通过。尚无 container digest/完整依赖锁，不构成 R1 | `eaf33985` | 2026-08-31 |
| S5-20 | P0 | PC artifact 可归档机器证据记录 | DONE | Zero | S5-19（DONE） | RED probe：`--record` 被旧脚本静默忽略且未生成 JSON；现记录 source commit/archive SHA-256、双构建 artifact hash/size、实际 tool identity 与 fail-closed scope，canonical CI 上传独立 evidence artifact 且缺失即失败；focused record/schema、Host 200/200、PC root、workflow YAML 与 `git diff --check` 通过。不构成 immutable environment、target/hardware 或 R1 | `0422bcfa` | 2026-08-31 |
| S5-21 | P0 | PC release artifact set 事实源 | DONE | Zero | S5-20（DONE） | RED：release readiness 因 artifact manifest 缺失失败；现机器守护唯一选定项 `xy_device` / `libxy_device.a`、固定 build config 与 gate-only 边界，并由 reproducibility record 引用 manifest schema/status；focused 2/2、Host 200/200、PC root 与 `git diff --check` 通过。不构成完整 PC/MCU artifact set、SBOM、签名或 R1 | `a7691932` | 2026-08-31 |
| S5-22 | P0 | PC reproducible gate artifact 归档 | DONE | Zero | S5-21（DONE） | RED：既有脚本拒绝 `--artifact-dir`，证明 CI 只归档 evidence JSON、未保存已验证 binary；现双构建一致后输出 `libxy_device.a` 与 SHA-256，manifest 固定文件集/14 天 retention，canonical CI 缺文件即失败上传；focused artifact/readiness contract、Host 200/200、PC root、workflow YAML 与 `git diff --check` 通过。checksum 未独立验证/签名，不构成 release publication 或 R1 | `2130ecf2` | 2026-08-31 |
| S5-23 | P0 | PC 归档制品 checksum 独立验证 | DONE | Zero | S5-22（DONE） | RED：脚本拒绝 `--verify-artifact-dir`，证明归档 checksum 只生成未独立读取校验；现 CI 在上传前以独立调用重读 library/checksum，严格校验格式、文件名与 SHA-256，并以篡改 artifact 负向 probe 证明 fail-closed；focused 2/2、Host 200/200、PC root、workflow YAML 与 `git diff --check` 通过。仍无签名/发布/不可变环境，不构成 R1 | `2f4e9ff6` | 2026-09-01 |
| S5-24 | P0 | PC 归档制品签名管道与独立验证 | DONE | Zero | S5-23（DONE） | RED：release guard 要求 signature 文件/public key/签名边界后按预期失败 2 项；现 CI 用每轮临时 Ed25519 key 签名归档 library，丢弃 private key，并由独立调用重读验证 checksum 与 signature；篡改 artifact/signature 负向 probe 均 fail-closed；focused 2/2、Host 200/200、PC root、workflow YAML 与 `git diff --check` 通过。临时 key 无 release identity/publication authority，不构成签名发布或 R1 | `05720b61` | 2026-09-01 |
| S5-25 | P0 | Release signing identity/key-custody 决策门 | DONE | Zero | S5-24（DONE） | RED：release readiness 因 signing policy 缺失失败；现机器记录 Ed25519 设计、release identity `UNASSIGNED`、custody `NOT_ESTABLISHED`、publication `BLOCKED`，并固定 owner/custodian/recovery/revocation/独立验签前置；focused、Host、PC root 与 `git diff --check` 通过。不创建/提交 release private key，不构成签名发布或 R1 | `f6b2cf26` | 2026-09-01 |
| S5-26 | P0 | PC bounded artifact SBOM 生成策略门 | DONE | Zero | S5-25（DONE）；S5-12（DONE） | RED：release readiness 因 SBOM policy 缺失失败；现固定 CycloneDX JSON 1.6、`xy_device` bounded artifact scope、source/artifact binding、dependency/license fail-closed inputs、schema/独立归档要求；focused 1/1、Host 200/200、PC root 与 `git diff --check` 通过。尚未生成 SBOM，license approval/R1 保持阻塞 | `a56dc909` | 2026-09-01 |
| S5-27 | P0 | PC bounded artifact SBOM 生成与独立验证 | DONE | Zero | S5-26（DONE） | RED：artifact gate 拒绝未知 `--sbom-dir`，证明策略尚无生成路径；现从同一 committed source archive 生成 CycloneDX JSON 1.6，绑定 artifact/source archive/source commit 与 10 个直接编译源 SHA-256/license evidence，归档后由独立调用重读校验；篡改 artifact binding 负向 probe fail-closed；focused 2/2、Host 200/200、PC root、官方 CycloneDX 1.6 schema probe 与 `git diff --check` 通过。状态保持 `REVIEW_PENDING`，不构成法律/license approval、完整 PC/MCU SBOM 或 R1 | `83e8358d` | 2026-09-01 |
| S5-28 | P1 | Release checklist 同步 bounded SBOM 事实 | DONE | Zero | S5-27（DONE） | RED：release readiness guard 证明 checklist 仍声称 SBOM generation `BLOCKED`；现同步为已生成并独立验证一个 bounded CycloneDX 1.6 SBOM，同时保留 legal/license/完整 PC/MCU/R1 阻塞；focused 1/1、Host 200/200、PC root 与 `git diff --check` 通过 | `011f43ce` | 2026-09-01 |
| S5-29 | P1 | PC bounded artifact 技术许可证审查记录 | DONE | Zero | S5-27（DONE） | RED：release readiness 因 license review record 缺失失败；现记录 10 个直接 first-party source、根 Apache-2.0 license hash、文件级冲突声明扫描及开放 NOTICE/redistribution/legal 要求；状态固定 `LEGAL_REVIEW_PENDING`，不构成法律批准或 R1；focused、Host 200/200、PC root 与 `git diff --check` 通过 | `d2571881` | 2026-09-01 |
| S5-30 | P1 | PC bounded artifact NOTICE 判定记录 | DONE | Zero | S5-29（DONE） | RED：release readiness 因 NOTICE record 缺失失败；现限定审查 10 个 first-party direct source，未发现第三方 attribution/license marker，故不为 bounded artifact 虚构 NOTICE 文件；完整 release scope、独立 legal review 与 R1 仍阻塞；focused、Host 200/200、PC root 与 `git diff --check` 通过 | `e313e644` | 2026-09-01 |
| S5-31 | P1 | PC bounded license/NOTICE 证据漂移门 | DONE | Zero | S5-30（DONE） | probe 发现 release guard 只检查 license source 数量，未绑定 artifact gate 的 exact source inventory，也未重算 LICENSE hash；现绑定 10 个 direct source、artifact selection、license identity/hash 与 NOTICE scope；focused 1/1、Host 200/200、PC root 与 `git diff --check` 通过，legal/完整 release/R1 仍阻塞 | `4000bed0` | 2026-09-01 |
| S5-32 | P1 | PC bounded legal evidence 随制品归档 | DONE | Zero | S5-31（DONE） | RED：artifact manifest 未归档 license/NOTICE records；现将两份 committed records 与 artifact 同包，并独立校验 `LEGAL_REVIEW_PENDING` 与 exact direct-source scope；focused 2/2、真实 build/sign/verify archive 7-file contract、Host 200/200、PC root 与 `git diff --check` 通过；不构成 legal approval、完整 release 或 R1 | `99dc4e3e` | 2026-09-01 |
| S5-33 | P1 | PC bounded artifact 许可证文本归档与 hash 验证 | DONE | Zero | S5-32（DONE） | RED：release guard 要求 archive 包含 `LICENSE` 后按预期失败；现将 committed Apache-2.0 文本作为第 8 个文件归档，并由独立 verify 同时重算 license/NOTICE records 中的 SHA-256 binding；focused 2/2、真实 build/sign/verify 8-file archive、篡改 LICENSE 负向 probe、Host 200/200、PC root 与 `git diff --check` 通过；不构成 legal approval、完整 release 或 R1 | `8d3fac14` | 2026-09-01 |
| S5-34 | P1 | Known Limitations 同步 bounded artifact 事实 | DONE | Zero | S5-33（DONE） | RED：release readiness guard 证明 Known Limitations 仍声称无 SBOM/可复现制品/签名；现同步一个 bounded PC static-library artifact、CycloneDX 1.6 SBOM、ephemeral CI signature 与 legal-pending 边界，并防止回归；focused 1/1、Host 200/200、PC root 与 `git diff --check` 通过，不构成完整 release、legal approval 或 R1 | `0a23af28` | 2026-09-01 |
| S5-35 | P0 | Tag release publication fail-closed authorization | DONE | Zero | S5-34（DONE）；Sprint 6/R1 仍 BLOCKED | RED：tag workflow 只校验版本即会创建 GitHub Release；现显式 authorization gate 要求 checklist `READY`/`GO`、零未完成项及 exact R1 qualification marker，否则 tag workflow 在发布前失败；focused 2/2、Host 201/201、PC root、workflow YAML 与 `git diff --check` 通过 | `bccdc06b` | 2026-09-01 |
| S5-36 | P0 | Release workflow runner 与 canonical CI 对齐 | DONE | Zero | S5-35（DONE） | RED：release readiness probe 发现 tag workflow 仍使用漂移的 `ubuntu-latest`；现固定 `ubuntu-24.04` 并机器守护 runner 与 authorization wiring；focused、Host、PC root、workflow YAML 与 `git diff --check` 通过。不构成 immutable environment、发布授权或 R1 | `a6945eaf` | 2026-09-01 |
| S5-37 | P0 | Release workflow checkout action 不可变固定 | DONE | Zero | S5-36（DONE） | RED：release readiness probe 证明 tag workflow 仍引用 movable `actions/checkout@v4`；现固定到经查询的 v4 commit `11d5960a326750d5838078e36cf38b85af677262` 并拒绝版本 tag 回归；focused、Host、PC root、workflow YAML 与 `git diff --check` 通过。不构成全部 CI action 固定、不可变构建环境、发布授权或 R1 | `fd24929c` | 2026-09-01 |
| S5-38 | P0 | Canonical CI checkout/upload actions 不可变固定 | DONE | Zero | S5-37（DONE） | RED：release readiness probe 证明 canonical `unit-tests.yml` 仍引用 movable `actions/checkout@v4` 与 4 处 `actions/upload-artifact@v4`；现固定到经远端 tag 查询的 commits `11d5960a326750d5838078e36cf38b85af677262` / `ea165f8d65b6e75b540449e92b4886f43607fa02`，并守护全部 5 处 action 不回归；focused 1/1、Host 201/201、PC root、workflow YAML 与 `git diff --check` 通过。不构成全部 workflow action 固定、不可变构建环境、发布授权或 R1 | `34be410b` | 2026-09-01 |
| S5-39 | P0 | 全部 GitHub workflow action 与 runner 不可变收口 | DONE | Zero | S5-38（DONE） | RED：全 workflow action inventory 发现 docs/deploy/status 仍有 11 处 movable version tag 与 `ubuntu-latest`；现将 checkout/setup-python/Pages/github-script 固定到经远端 tag 查询的 commit，统一 `ubuntu-24.04`，并机器守护 exact 18-use immutable set；focused 1/1、Host 201/201、PC root、workflow YAML 与 `git diff --check` 通过。不构成 container digest、发布授权或 R1 | `65101ce7` | 2026-09-01 |
| S5-40 | P0 | Pandora STM32L475 board smoke 前置 | DONE | Zero | 潘多拉参考板与 CubeL4 checkout | 独立 board target 已完成 LED/USART1/KEY0/AHT10 software-I2C 初始化、测量与 ACK/NACK 恢复 probe；focused contract、Host 202/202、PC root 与 Arm GNU 15.2 clean link 通过，ELF text/data/bss=`7236/12/2700` bytes。仅为 compile-only；未记录烧录、串口、LED、按键或 AHT10 实板结果 | `683ab5ea`～`e354fe99` | 2026-09-01 |
| S5-41 | P0 | Pandora STM32L475 board runtime smoke | DONE | Zero | S5-40（DONE）；独立 UART | 当前 `9b50ec38` image 已完成 write/verify；PE7 500 ms 翻转已观察，独立 WCH-Link UART 留存匹配 SHA 的 AHT10 正常路径及 4 个 KEY0 事件，B1 已关闭；证据 SHA-256 与内容由 focused policy gate 守护；NACK→recovery 仍为 B2 pending | `29cc8524`、`482327be`、`0d753bc5`、`bb10f736` | 2026-09-03 |
| S5-42 | P0 | Pandora bounded UART capture gate | DONE | Zero | S5-41（DONE） | 新增 fail-closed 115200-8-N-1 capture helper；PTY Host contract 覆盖真实字节保存、source SHA/metadata、bounded no-data timeout 与 missing-device refusal；focused、Host 203/203、PC root build 与 `git diff --check` 通过。2026-09-02 建档探测时无 ST-Link、无 `/dev/ttyACM*`/`ttyUSB*`；后续设备恢复与 B1 证据见 S5-41，不追溯升级本项的 Host gate | `564b9600` | 2026-09-03 |
| S5-43 | P0 | Pandora UART capture 固件身份绑定 | DONE | Zero | S5-42（DONE） | RED：capture metadata 自动记录当前 checkout `HEAD`，无法绑定被烧录固件；现要求显式 exact 40-char `--firmware-commit`，并覆盖缺失/非 exact identity；focused 2/2、committed clean-export、Host 203/203、PC root 与 `git diff --check` 通过，不升级 runtime/B1/B2 | `5efff54b` | 2026-09-02 |
| S5-44 | P0 | Pandora UART 掉线 fail-closed 分类 | DONE | Zero | S5-43（DONE） | RED：PTY peer 断开时 `read()` 返回 EOF，helper 曾等待到期并误记 `NO_DATA_TIMEOUT`；现立即记录 `CAPTURE_IO_FAILED`，保留已捕获字节并返回失败。focused 2/2、Host 203/203、PC root 与 `git diff --check` 通过，不升级 runtime/B1/B2 | `79010108` | 2026-09-02 |
| S5-45 | P0 | Pandora UART 内容身份 fail-closed 分类 | DONE | Zero | S5-44（DONE） | RED：任意 bootloader/noise bytes 曾被记为 `CAPTURED`；现仅 exact Pandora firmware banner 可获得成功状态，其他字节保留并记为 `CAPTURE_CONTENT_MISMATCH`；focused 2/2、Host 203/203、PC root 与 `git diff --check` 通过，不升级 runtime/B1/B2 | `3152073a` | 2026-09-02 |
| S5-46 | P0 | Pandora UART B1 候选内容门 | DONE | Zero | S5-45（DONE） | RED：仅 banner 即可成功，未证明 AHT10 测量路径；现要求 banner + 数值测量行才标记 `B1_REVIEW_CANDIDATE`，banner-only/noise 保留但 fail-closed；focused 1/1、Host 203/203、PC root 与 `git diff --check` 通过；PTY 结果不升级 B1 | `03e35fcb` | 2026-09-02 |
| S5-47 | P0 | Pandora AHT10 候选值域门 | DONE | Zero | S5-46（DONE） | RED：格式正确但明显越界的 RH/温度仍获 `B1_REVIEW_CANDIDATE`；现仅接受 AHT10 plausible range（0–100000 milli-percent、-50000–150000 milli-C），越界 capture 保留但 fail-closed；focused 2/2、Host 203/203、PC root 与 `git diff --check` 通过；当前无 ST-Link/串口设备，不升级 runtime/B1/B2 | `b67e02f1` | 2026-09-02 |
| S5-48 | P0 | Pandora AHT10 ACK 候选内容门 | DONE | Zero | S5-47（DONE） | RED：banner + 合法测量但无 `AHT10 0x38 ACK` 曾获候选；现要求 banner、ACK 与 plausible measurement 三者齐备；focused 2/2、Host 203/203、PC root、`git diff --check` 通过；PTY 不升级 runtime/B1/B2 | `882d66c7` | 2026-09-02 |
| S5-49 | P0 | Pandora AHT10 NACK→恢复候选门 | DONE | Zero | S5-48（DONE） | RED：NACK 后恢复的 capture 仍只能标为 B1 candidate 且 metadata 不记录 NACK marker；现要求同一 capture 含 NACK、后续 ACK 与 plausible measurement 才标记 `B2_REVIEW_CANDIDATE`；focused 2/2、Host 203/203、PC root 与 `git diff --check` 通过；PTY 不升级 B2 | `969c262e` | 2026-09-02 |
| S5-50 | P0 | Pandora AHT10 恢复事件顺序门 | DONE | Zero | S5-49（DONE） | RED：ACK/measurement 后出现 trailing NACK 仍被误标 B2 candidate；现要求 NACK→后续 ACK→后续 plausible measurement 的严格顺序；focused 2/2、Host 203/203、PC root 与 `git diff --check` 通过；PTY 不升级 B2 | `37edb207` | 2026-09-02 |
| S5-51 | P0 | Pandora B1 启动事件顺序门 | DONE | Zero | S5-50（DONE） | RED：measurement→ACK 的乱序 capture 曾获 B1 candidate；现要求 banner→ACK→plausible measurement 的严格顺序，乱序字节保留但 fail-closed；focused 1/1、Host 203/203、PC root、Pandora Arm target 与 `git diff --check` 通过；PTY 不升级 B1/B2 | `0772a0f4` | 2026-09-02 |
| S5-52 | P0 | Pandora B2 启动身份顺序门 | DONE | Zero | S5-51（DONE） | RED：firmware banner 前的 stale NACK 曾与后续 ACK/measurement 组合并误获 B2 candidate；现要求 banner→NACK→ACK→plausible measurement；focused 2/2、Host 203/203、PC root、Pandora Arm target 与 `git diff --check` 通过；PTY 不升级 B1/B2 | `f558cbaf` | 2026-09-02 |
| S5-53 | P0 | Pandora B2 恢复窗口唯一性门 | DONE | Zero | S5-52（DONE） | RED：同一 capture 先有成功测量、后有 NACK→ACK→measurement 时仍误获 B2 candidate；现仅当首次 banner 后、NACK 前没有 plausible measurement 才形成 recovery candidate，避免把后续周期故障拼接为启动恢复证据；focused 1/1、Host 203/203、PC root、Pandora Arm target 与 `git diff --check` 通过；PTY 不升级 B1/B2 | `0d286c75` | 2026-09-02 |
| S5-54 | P0 | Pandora capture 与固件内嵌 SHA 一致性门 | DONE | Zero | S5-53（DONE） | RED：CLI 接受的任意 40 字符 SHA 与 UART 内容无绑定，错误固件仍可获得 B1/B2 candidate；固件现输出 configure-time source SHA，capture 要求与 `--firmware-commit` exact match；focused 2/2、Host 203/203、PC root、Pandora Arm target（text/data/bss=`7312/12/2700`）及 ELF identity probe、`git diff --check` 通过；PTY 不升级 B1/B2 | `2bb88e6b` | 2026-09-02 |
| S5-55 | P0 | Pandora 固件身份事件顺序门 | DONE | Zero | S5-54（DONE） | RED：matching commit marker 位于 banner 前仍可获 B1 candidate；现要求 banner→matching firmware commit→ACK 的有序身份链；focused 1/1、Host 203/203、PC root、Pandora Arm target 与 `git diff --check` 通过；PTY 不升级 B1/B2 | `f40f1e4c` | 2026-09-02 |
| S5-56 | P0 | Pandora B2 固件身份恢复顺序门 | DONE | Zero | S5-55（DONE） | RED：banner 后、matching firmware commit 前的 stale NACK 曾与后续 ACK/measurement 组合并误获 B2 candidate；现要求 banner→matching firmware commit→NACK→ACK→plausible measurement；focused 1/1、Host 203/203、PC root、Pandora Arm target 与 `git diff --check` 通过；PTY 不升级 B1/B2 | `72574948` | 2026-09-02 |
| S5-57 | P0 | Pandora UART 单周期身份闭环门 | DONE | Zero | S5-56（DONE） | RED：首周期 ACK/measurement 可与次周期 matching commit/ACK 拼接并误获 B1 candidate；现要求同一 banner 周期内完整出现 matching commit→ACK→plausible measurement；focused 1/1、Host 203/203、PC root 与 `git diff --check` 通过；不升级 B1/B2 | `4546c922` | 2026-09-03 |
| S5-58 | P0 | Pandora B2 单周期恢复闭环门 | DONE | Zero | S5-57（DONE） | RED：matching commit 与后续另一 banner 周期的 NACK→ACK→measurement 可拼接并误获 B2 candidate；现将 B2 恢复链限制在同一 banner 周期；Python 20/20、focused 1/1、Host 203/203、PC root 与 `git diff --check` 通过；不升级 B1/B2 | `de4f85a7` | 2026-09-03 |
| S5-59 | P0 | FreeRTOS OSAL thread flags 选择性清除 | DONE | Zero | S5-01（IN_PROGRESS） | RED：旧 `xy_os_thread_flags_clear()` 以 `~flags` 覆盖通知值，会将未指定 bit 错误置位；现使用 `ulTaskNotifyValueClear(NULL, flags)` 原子返回旧值并仅清指定 bit；focused 3/3、Host 204/204、`xy_device`、PC root、STM32U5 root 与 `git diff --check` 通过；不升级 scheduler/runtime/ISR/实板声明 | `8465b830` | 2026-09-03 |
| S5-60 | P0 | FreeRTOS OSAL thread flags set 返回值与失败传播 | DONE | Zero | S5-59（DONE） | RED：旧 `xy_os_thread_flags_set()` 忽略 `xTaskNotify()` 失败且只返回本次 mask，丢失已存在 flags；现改用 `xTaskNotifyAndQuery(..., eSetBits, ...)`，backend 拒绝时 fail-closed，成功时返回完整 post-set flags；focused 2/2、Host 204/204、PC root、FreeRTOS-enabled STM32U5 root 与 `git diff --check` 通过；不升级 scheduler/runtime/ISR/实板声明 | `381ba52e` | 2026-09-03 |
| S5-61 | P0 | FreeRTOS OSAL thread flags get 失败传播 | DONE | Zero | S5-60（DONE） | RED：旧 `xy_os_thread_flags_get()` 忽略 `xTaskNotifyAndQuery(..., eNoAction, ...)` 失败并返回未初始化值；现 backend 拒绝时返回 OSAL flag error sentinel；focused 3/3、Host 204/204、PC root、FreeRTOS-enabled STM32U5 root 与 `git diff --check` 通过；不升级 scheduler/runtime/ISR/实板声明 | `deac74a0` | 2026-09-03 |
| S5-62 | P0 | STM32L4 FreeRTOS+OSAL 编译纵切 | DONE | Zero | S5-01（IN_PROGRESS）；Pandora STM32L475VE | RED：canonical Host gate 缺 STM32L4 compile probe，root FreeRTOS selection 仅允许 U5/M33；现引入 FreeRTOS V10.4.6 官方 GCC ARM_CM4F port（固定 upstream commit/SHA-256）、L475 80 MHz 配置，并接通 Kconfig/root/third-party/OSAL 选择；focused 3/3、Host 205/205、`xy_device`、PC、U5、L4、L4 FreeRTOS kernel+OSAL 与 `git diff --check` 通过；不升级 scheduler/runtime/ISR/实板声明 | `ad4ff5e1` | 2026-09-03 |
| S5-63 | P0 | Pandora OSAL+FreeRTOS 可链接 runtime 镜像 | DONE | Zero | S5-62（DONE） | RED：Pandora 缺 OSAL runtime main、FreeRTOS exception handlers 与 board link target；现新增仅通过 `xy_os_*` 创建的 500/1000 ms 双任务候选，接通 SVC/PendSV/SysTick（并维持 HAL tick），输出身份及任务 marker；focused、Host 206/206、PC/U5/L4、Pandora RTOS ELF/BIN 与 handler symbol gate 通过，text/data/bss=`11664/16/19728`；尚未烧录，不升级 scheduler/runtime/ISR/实板声明 | `d0f259b9` | 2026-09-03 |
| S5-64 | P0 | Pandora FreeRTOS pre-scheduler SysTick 与双任务实板烟测 | DONE | Zero | S5-63（DONE）；Pandora/WCH-Link UART | RED/实板：首次镜像只输出 11–12 byte banner 前缀，定位为 HAL 初始化阶段 SysTick 无条件进入 FreeRTOS port handler；加入 scheduler-state guard 后，ST-Link write/verify 成功，独立 WCH-Link 6 秒 capture 绑定 `46499b33` 并记录双任务 500/1000 ms 交错 marker；focused、Host 206/206、PC root、Pandora target 与 `git diff --check` 通过。仅升级 Pandora thread-scheduling B1，ISR-to-task/sync/stress/STM32U5 仍 pending | `46499b33` + 本记录提交 | 2026-09-03 |
| S5-65 | P0 | Pandora 当前 HEAD FreeRTOS 调度周期实板复验 | DONE | Zero | S5-64（DONE）；ST-Link/WCH-Link 在线 | 从 clean `ac3f20f4` 重建并确认 ELF 内嵌同一 SHA，RTOS BIN SHA-256=`88855cc1...`；`st-flash --reset write` 报告 write/verify 成功。复位同步 8 秒 UART capture 为 489 bytes：FAST 16 次、间隔 506–508 ms（均值 506.53），SLOW 8 次、间隔 1013–1014 ms（均值 1013.14），证明当前固件上的 OSAL task scheduling/tick/delay B1；本轮未人工复核 LED，ISR-to-task/queue/semaphore stress 仍 pending | 本记录提交 | 2026-09-03 |
| S5-66 | P0 | Pandora OSAL semaphore 任务同步实板纵切 | DONE | Zero | S5-65（DONE）；ST-Link/WCH-Link 在线 | runtime 通过 OSAL 创建二值 semaphore；FAST 每 500 ms release，SLOW 阻塞 acquire。固件 `c9b2d3d1` write/verify 13084 bytes；6 秒 capture 670 bytes，12 轮严格 `FAST→SEM_TAKE→SLOW`，release-to-take 0–2 ms、timeout 0、FAST 周期 506–507 ms；Host 206/206、PC/U5/L4、Pandora link 与 `git diff --check` 通过。证明 task-context semaphore B1；queue、ISR-to-task 与长稳仍 pending | `c9b2d3d1` + 本记录提交 | 2026-09-03 |
| S5-67 | P0 | Pandora OSAL message queue 任务同步实板纵切 | DONE | Zero | S5-66（DONE）；ST-Link/WCH-Link 在线 | RED policy probe 要求 message queue OSAL 调用与错误 marker；runtime 以 depth=2 的 `uint32_t` queue 传递单调序号，接收端逐项校验。固件 `4ebf46da` write/verify 13336 bytes；6 秒 capture 1078 bytes，12 轮均为 `FAST→QUEUE_SEND→SEM_TAKE→QUEUE_RECV→SLOW`，mismatch/timeout 为 0；focused 2/2、Host 206/206、PC/U5/L4、Pandora link 与 `git diff --check` 通过。证明 task-context queue B1；payload 数值由固件 fail-closed 校验，UART 不单独打印数值；ISR-to-task/长稳仍 pending | `4ebf46da` + 本记录提交 | 2026-09-03 |
| S5-68 | P0 | Pandora OSAL event flags 任务同步实板纵切 | DONE | Zero | S5-67（DONE）；ST-Link/WCH-Link 在线 | RED policy probe 要求 event-flags OSAL 调用与错误 marker；固件 `48ca0509` 每轮 queue send 后 set data-ready bit，接收端阻塞 wait-all 并自动清除。write/verify 14144 bytes；6 秒 capture 1284 normalized bytes，12 轮严格 `FAST→QUEUE_SEND→EVENT_SET→SEM_TAKE→EVENT_WAIT→QUEUE_RECV→SLOW`，event/queue mismatch 与 semaphore timeout 均为 0；focused、Host 206/206、PC/U5/L4/Pandora link 与 `git diff --check` 通过。证明 task-context event-flags B1；ISR-to-task、timeout/resource exhaustion 与长稳仍 pending | `48ca0509` + 本记录提交 | 2026-09-03 |
| S5-69 | P0 | Pandora OSAL mutex 共享状态实板纵切 | DONE | Zero | S5-68（DONE）；ST-Link/WCH-Link 在线 | RED policy probe 要求 mutex create/acquire/release 与 timeout/mismatch marker；固件 `33c3a665` 用 mutex 保护 producer/consumer 共享序号。write/verify 14448 bytes；6 秒 capture 1881 bytes，12 轮严格 `MUTEX_FAST→FAST→QUEUE_SEND→EVENT_SET→SEM_TAKE→EVENT_WAIT→QUEUE_RECV→MUTEX_SLOW→SLOW`，mutex/queue/event/semaphore 错误均为 0，mutex roundtrip 9–10 ms；Host 206/206、PC/U5/L4/Pandora link 与 `git diff --check` 通过。证明 task-context mutex B1；ISR-to-task、资源耗尽与长稳仍 pending | `33c3a665` + 本记录提交 | 2026-09-03 |
| S5-70 | P0 | Pandora OSAL ISR→task semaphore 实板纵切 | DONE | Zero | S5-69（DONE）；ST-Link/WCH-Link 在线 | 新增显式 ISR-safe semaphore release；FreeRTOS backend 使用 `xSemaphoreGiveFromISR` + `portYIELD_FROM_ISR`。固件 `8443f907` write/verify 14912 bytes；7 秒独立 UART capture 2025 bytes，`OSAL_ISR_TAKE` 6 次、timeout 0，且原 task pipeline 继续运行；focused 2/2、Host 206/206、PC/U5/L4/Pandora link 与 `git diff --check` 通过。仅证明 SysTick ISR→semaphore→task B1；外设 IRQ、资源耗尽、shutdown/re-init、长稳与 STM32U5 runtime 仍 pending | `13af9bc2`～`8443f907` + 本记录提交 | 2026-09-03 |
| S5-71 | P0 | Pandora OSAL 资源耗尽/恢复与 lifecycle re-init 候选 | DONE | Zero | S5-70（DONE）；实板运行依赖 ST-Link 恢复 | runtime candidate 使用容量 2 的 memory pool 与深度 1 的 queue 验证 no-wait exhaustion、释放后恢复、delete→recreate；focused `pandora_freertos_runtime` 1/1、Host 206/206、PC/U5/L4、Pandora FreeRTOS link 与 `git diff --check` 通过，ELF text/data/bss=`16056/16/19768`、BIN SHA-256=`731fe90a...`。本轮实测 WCH-Link UART 在线但 `st-info --probe` 为 0 个 ST-Link，故未烧录/捕获，不升级 resource/lifecycle B1 | `9c2d4d4e` + 本记录提交 | 2026-09-03 |
| S5-72 | P0 | Pandora OSAL 资源耗尽/恢复与 lifecycle re-init 实板闭环 | DONE | Zero | S5-71（DONE）；ST-Link/WCH-Link 恢复在线 | clean `e6cd0906` image write/verify 16080 bytes 且同长度 read-back 与 BIN byte-identical；8 秒 WCH-Link capture 2651 bytes，匹配固件身份并严格出现 `RESOURCE_EXHAUSTED→RESOURCE_RECOVERED→LIFECYCLE_REINIT` 各 1 次，resource/既有 pipeline error marker 为 0；focused 2/2、Host 206/206、PC/U5/L4、Pandora link 与 `git diff --check` 通过。仅升级 bounded no-wait resource/lifecycle B1，blocking timeout/长稳/STM32U5 runtime 仍 pending | `4e74c8d4` | 2026-09-04 |
| S5-73 | P0 | Pandora OSAL blocking timeout 错误映射与实板闭环 | DONE | Zero | S5-72（DONE）；ST-Link/WCH-Link 在线 | 实板 RED：满队列等待 100 ticks 后 FreeRTOS adapter 返回 generic `XY_OS_ERROR`；现 mutex/semaphore/queue wait 失败映射 `XY_OS_ERROR_TIMEOUT`。clean `34bb39f4` image write/verify 16216 bytes且同长度 read-back byte-identical；8 秒 capture 2677 bytes，严格出现 `RESOURCE_EXHAUSTED→BLOCKING_TIMEOUT_OK→RESOURCE_RECOVERED→LIFECYCLE_REINIT`，并保留 16 轮 task pipeline/7 次 ISR wake、错误 marker 为 0；focused 6/6、Host 206/206、PC/U5/L4/Pandora link、`git diff --check` 通过。100–120 tick 判定为固件内部 bounded contract，不升级性能/长稳 | `1ad861ff`、`34bb39f4` + 本记录提交 | 2026-09-04 |
| S5-74 | P0 | Pandora OSAL 120 秒 bounded stress 实板闭环 | DONE | Zero | S5-73（DONE）；ST-Link/WCH-Link 在线 | RED：validator 单测先因模块缺失失败；新增 identity/顺序/阈值/error fail-closed gate 与固件 stress-ready marker。`50fad12a` image write/verify 16252 bytes且 read-back byte-identical；reset 同步 120 秒 capture 36677 bytes，234 个完整 ordered pipeline cycles、116 次 ISR wake、resource/timeout/recovery/re-init 各 1 次且错误 marker 为 0；focused 2/2、Host 207/207、Pandora link 与 `git diff --check` 通过。仅为 bounded B1，不升级性能、多小时耐久、跨组件并发或 STM32U5 runtime | `50fad12a` + 本记录提交 | 2026-09-04 |
| S5-75 | P0 | Pandora OSAL→IPC→Device→Trace 跨组件并发纵切 | DONE | Zero | S5-74（DONE）；ST-Link/WCH-Link 在线 | runtime producer 经 Broker 队列发送单调序号，consumer dispatch 校验 payload、Device registry lookup，并经 CLIB Trace sink 输出；`af503aa3` image write/verify 18192 bytes且 read-back byte-identical。8 秒 UART 捕获 17 个严格 `IPC_SEND→DEVICE_LOOKUP→TRACE_DELIVER→IPC_DELIVER` 周期，IPC/Trace error marker 为 0；focused 3/3、Host 207/207、PC/L4/U5 compile 与 `git diff --check` 通过。捕获起点晚于 reset banner，故固件身份由 clean committed ELF、烧录与回读链绑定，不将该 capture 单独作为 identity 证据；PM 与多小时耐久仍 pending | `af503aa3` + 本记录提交 | 2026-09-04 |
| S5-76 | P0 | Pandora OSAL→PM tick 跨组件纵切 | DONE | Zero | S5-75（DONE）；ST-Link/WCH-Link 在线 | board runtime 纳入 PM platform adapter 并以 `XY_OSAL_AVAILABLE` 绑定 canonical OSAL tick；clean committed `b1107d1b` image 18288 bytes write/verify 且同长度 read-back byte-identical。UART 捕获 20 次 `OSAL_IPC_SEND→OSAL_PM_TICK`，`OSAL_PM_ERROR`/IPC/Trace error 均为 0；focused 3/3、Host 207/207、PC/L4/U5 compile、handler/PM symbol 与 `git diff --check` 通过。仅证明 PM tick adapter 的 bounded B1，不代表 sleep/wakeup、功耗、ADC/charger GPIO 或 PM 全组件资格 | `b1107d1b` + 本记录提交 | 2026-09-04 |
| S5-77 | P0 | Pandora FOTA bounded automatic rollback 实板闭环 | DONE | Zero | S3-04（IN_PROGRESS）；ST-Link/WCH-Link 在线 | clean committed `d37ecf1c` image 11992 bytes write/verify/read-back byte-identical；5-boot UART chain 绑定 exact firmware SHA，有序完成 v2 confirm、v1 anti-rollback rejection、v3 pending attempt 与 automatic rollback。raw metadata latest generation=7、active/min version=2、pending clear、CRC valid；focused validator、Host、PC/L4/U5 compile 与 `git diff --check` 通过。仅为 board-owned metadata/reset recovery B2，不是候选 image 执行、真实 bootloader vector handoff、掉电或 secure FOTA | `82823116`、`d37ecf1c` + 本记录提交 | 2026-09-04 |
| S5-78 | P0 | Pandora TIM6 peripheral IRQ→task 实板纵切 | DONE | Zero | S5-70（DONE）；ST-Link/WCH-Link 在线 | clean committed `c9509e81` image 19372 bytes write/verify；ELF 保留 SVC/PendSV/SysTick 与 TIM6 handler/callback。8 秒 UART capture 4337 bytes，匹配 exact firmware SHA，16 个完整 task pipeline cycles、6 次 SysTick ISR wake、10 次 TIM6 peripheral IRQ wake，错误 marker 为 0；focused 2/2、Host 208/208、PC/L4/U5 compile 与 `git diff --check` 通过。仅证明 TIM6 update IRQ→ISR-safe semaphore→task B1，不构成任意外设 IRQ、IRQ 错误恢复、时序性能或完整 HAL 资格 | `c9509e81` + 本记录提交 | 2026-09-04 |
| S5-79 | P0 | Pandora TIM6 peripheral IRQ timeout/restart recovery | DONE | Zero | S5-78（DONE）；ST-Link/WCH-Link 在线 | clean committed `f78f4417` image 19652 bytes write/verify/read-back byte-identical；runtime 受控停止 TIM6 IRQ，确认 900-tick OSAL timeout，再启动并取得 fresh IRQ wake。10 秒 capture 5415 bytes，匹配 exact firmware SHA，20 个 task pipeline cycles、9 次 SysTick ISR wake、11 次 TIM6 wake、错误 marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile 与 `git diff --check` 通过。仅升级 TIM6 disable/timeout/re-enable recovery B2，不是物理故障注入、任意 IRQ 或性能资格 | `f78f4417` + 本记录提交 | 2026-09-04 |
| S5-80 | P0 | Pandora IPC Broker queue saturation/recovery | DONE | Zero | S5-75（DONE）；ST-Link/WCH-Link 在线 | runtime 将 depth-2 Broker server queue 填满，第三次 send 必须返回 `XY_BROKER_QUEUE_FULL`，随后 clear queue 并继续原跨组件 pipeline。clean `1e3acc34` image 19868 bytes write/verify/read-back byte-identical；12 秒 capture 6257 bytes，匹配 exact firmware SHA，`IPC_SATURATED→IPC_RECOVERED` 各 1 次、23 个完整 pipeline cycles、12 次 SysTick ISR 与 13 次 TIM6 wake，错误 marker 0；focused 3/3、Host 208/208、PC/L4/U5 compile 与 `git diff --check` 通过。仅为 bounded task-context saturation/recovery B2，不是多 producer、ISR ingress、吞吐或长时间压力资格 | `1e3acc34` + 本记录提交 | 2026-09-05 |
| S5-81 | P0 | Pandora 跨组件完整周期 fail-closed 验证 | DONE | Zero | S5-80（DONE）；ST-Link/WCH-Link 在线 | RED：stress validator 只计算 OSAL primitive 周期，缺 Device/Trace/IPC/PM marker 仍可通过；现每个完整周期必须严格包含 `IPC_SEND→PM_TICK→...→DEVICE_LOOKUP→TRACE_DELIVER→IPC_DELIVER`。clean `6b552c31` image 19868 bytes write/verify/read-back byte-identical；12 秒 capture 6501 bytes，匹配 exact firmware SHA，24 个完整跨组件周期、11 次 SysTick ISR、14 次 TIM6 wake、IPC saturation/recovery 与 TIM6 timeout/restart 各一次，错误 marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile 与 `git diff --check` 通过。仅加强 bounded evidence 的完整性，不构成性能、多 producer/consumer、多小时耐久或完整 RTOS 资格 | `6b552c31` + 本记录提交 | 2026-09-05 |
| S5-82 | P0 | Pandora OSAL 多 producer/consumer queue 并发 | DONE | Zero | S5-81（DONE）；ST-Link/WCH-Link 在线 | 两个 producer 各发送 8 个带 producer/sequence 的消息，两个 consumer 共享队列并在 mutex 下以 bitset 拒绝重复/遗漏；最后一个 consumer 仅在 16/16 唯一消息齐备时输出完成 marker。clean committed `4a97b462` image 20540 bytes write/verify/read-back byte-identical；20 秒 reset-synchronized capture 10700 bytes，匹配 exact firmware SHA，`OSAL_MULTI_PRODUCER_OK` 一次、40 个完整跨组件周期、19 次 SysTick ISR、25 次 TIM6 wake、错误 marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile 与 `git diff --check` 通过。仅证明 bounded 2P/2C queue 正常路径，不构成 throughput、公平性、多小时耐久或完整 RTOS 资格 | `4a97b462` + 本记录提交 | 2026-09-05 |
| S5-83 | P0 | Pandora 2P/2C 实际 consumer 分发闭环 | DONE | Zero | S5-82（DONE）；ST-Link/WCH-Link 在线 | RED：原 2P/2C 完成条件只证明两个 consumer 退出，未证明两者都消费过 payload；现按 consumer ID 计数并要求双方非零，消费后主动让出 1 tick，validator 要求独立 distribution marker。clean committed `f8037aa7` image 20636 bytes write/verify/read-back byte-identical；25 秒 reset-synchronized capture 9711 bytes，匹配 exact firmware SHA，`OSAL_MULTI_CONSUMER_DISTRIBUTED` 与 `OSAL_MULTI_PRODUCER_OK` 各一次、36 个完整跨组件周期、18 次 SysTick ISR、23 次 TIM6 wake、错误 marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile、handler symbol 与 `git diff --check` 通过。仅证明 bounded 双 consumer 均参与，不构成公平性、吞吐、优先级反转或多小时耐久资格 | `f8037aa7` + 本记录提交 | 2026-09-05 |
| S5-84 | P0 | Pandora 2P/2C 每条 payload consumer 归属证据 | DONE | Zero | S5-83（DONE）；ST-Link/WCH-Link 在线 | RED：仅 completion/distribution marker 无法证明 16 条 payload 的逐条归属；新增 consumer-ID take marker、exact 16 条计数与双方非零 fail-closed validator。首版 mutex UART 序列化实板只运行一周期，修正为保存 PRIMASK 的短临界区后，clean `cea47157` image 20728 bytes write/verify/read-back byte-identical；25 秒 capture 13337 bytes，consumer 0/1 分别消费 9/7 条、48 个完整跨组件周期、24 次 SysTick ISR、34 次 TIM6 wake、错误 marker 0。focused 2/2、Host 208/208、PC/L4/U5 compile 与 handler symbol gate 通过。只证明 bounded 16-message distribution，不构成公平性或性能资格 | `fe5dbebf`、`cea47157` + 本记录提交 | 2026-09-05 |
| S5-85 | P0 | Pandora framework STM32L4 DMA memory-to-memory 实板纵切 | DONE | Zero | S2-02（IN_PROGRESS）；ST-Link/WCH-Link 在线 | RED policy gate 要求 dedicated L4 DMA wrapper 与 fail-closed board marker；首个实板 capture 暴露 memory alignment 错映射为 `DMA_PDATAALIGN_*`，导致 word copy compare failure。修复为独立 `DMA_MDATAALIGN_*` 映射后，clean `6b2ff630` image 22152 bytes write/verify/read-back byte-identical；20 秒 reset-synchronized UART capture 10975 bytes、SHA-256=`ed4ed44d...`，`PANDORA_DMA_MEM2MEM_OK` 恰好一次、39 个完整跨组件周期、19 次 SysTick ISR、26 次 TIM6 wake、2P/2C 为 9/7，错误 marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile、Pandora link、handler/DMA symbol 与 `git diff --check` 通过。仅证明 framework DMA1 Channel1、8-word SRAM→SRAM polling B1，不构成 SPI/DMA、peripheral request、IRQ/callback、吞吐或恢复资格 | `f824a075`、`8da41938`、`6b2ff630` + 本记录提交 | 2026-09-05 |
| S5-86 | P0 | Pandora framework DMA IRQ/callback 实板纵切 | DONE | Zero | S5-85（DONE）；ST-Link/WCH-Link 在线 | STM32L4 wrapper 实现 per-instance callback context，并在已注册 callback 时以 `HAL_DMA_Start_IT` 启动；DMA1 Channel1 IRQ 经 HAL dispatch、framework callback 与 ISR-safe OSAL semaphore 唤醒 task。clean committed `308faceb` image 22896 bytes write/verify/read-back byte-identical；20 秒 reset-synchronized capture 10765 bytes、SHA-256=`3fa49bac...`，DMA mem2mem 与 IRQ callback marker 各一次、38 个完整跨组件周期、19 次 SysTick ISR、26 次 TIM6 wake、2P/2C 为 9/7、错误 marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile、Pandora link/handler symbols 与 `git diff --check` 通过。仅证明 DMA1 Channel1 mem2mem completion IRQ/callback B1，不构成 peripheral-request DMA、error IRQ、abort/restart B2、性能或完整 HAL 资格 | `308faceb` + 本记录提交 | 2026-09-05 |
| S5-87 | P0 | Pandora framework DMA abort→deinit→re-init→copy recovery | DONE | Zero | S5-86（DONE）；ST-Link/WCH-Link 在线 | RED validator/policy gate 要求 recovery marker；首轮实板确认 idle handle 上 `HAL_DMA_Abort` fail-closed，修正为 start active transfer 后 abort，再 deinit/re-init 并 polling 完成新 8-word SRAM copy。clean `f95dfed6` image 23684 bytes write/verify/read-back byte-identical；20 秒 reset-synchronized capture 11034 bytes、SHA-256=`fcfe3674...`，DMA mem2mem、IRQ callback、stop recovery marker 各一次，39 个完整跨组件周期、19 次 SysTick ISR，错误 marker 0；focused 15/15、Host 208/208、PC/L4/U5 compile 与 Pandora link 通过。仅证明受控 software abort/re-init B2，不构成物理故障、SPI/peripheral DMA、超时或性能资格 | `e169af41`、`604e6e7c`、`f95dfed6` + 本记录提交 | 2026-09-05 |
| S5-88 | P0 | Pandora framework SPI1 TX DMA IRQ/callback 纵切 | DONE | Zero | S5-87（DONE）；ST-Link/WCH-Link 在线 | 新增 STM32L4 framework SPI wrapper；SPI1 transmit request 驱动 DMA1 Channel3 completion IRQ，经 framework SPI callback 与 ISR-safe semaphore 唤醒 OSAL task。clean committed `d904cbf2` image 25636 bytes write/verify/read-back byte-identical；20 秒 capture 11057 bytes、SHA-256=`11fa5cd8...`，SPI DMA marker 恰好一次、39 个完整跨组件周期、19 次 SysTick ISR、26 次 TIM6 wake、错误 marker 0；focused、Host 208/208、PC/L4/U5 compile、Pandora link/handler symbols 与 `git diff --check` 通过。未配置 SPI GPIO 或外设，故仅证明 MCU 内部 TX request/DMA/IRQ/callback B1，不构成电气信号、外设响应、RX/full-duplex、性能或 recovery B2 | `d904cbf2` + 本记录提交 | 2026-09-05 |
| S5-89 | P0 | Pandora framework SPI1 TX DMA deinit→re-init→retransmit recovery | DONE | Zero | S5-88（DONE）；ST-Link/WCH-Link 在线 | RED validator/policy gate 要求独立 recovery marker；首轮 TX DMA 完成后 deinit SPI/DMA，再重新初始化相同 SPI1/DMA1 Channel3、重新注册 callback 并完成第二次 TX request→DMA IRQ→OSAL task。clean committed `3c3e5f60` image 25828 bytes write/verify/read-back byte-identical；20 秒 capture 11086 bytes、SHA-256=`9e05bbc3...`，normal/recovery marker 各一次、39 个完整跨组件周期、19 次 SysTick ISR，错误 marker 0；focused 17/17、Host 208/208、PC/L4/U5 compile 与 Pandora link 通过。仅证明受控 peripheral re-init/retransmit B2；未配置 GPIO/外设，不构成电气响应、RX/full-duplex、物理故障或性能资格 | `3c3e5f60` + 本记录提交 | 2026-09-05 |
| S5-90 | P0 | Pandora SPI1 TX DMA active abort→re-init→retransmit recovery | DONE | Zero | S5-89（DONE）；ST-Link/WCH-Link 在线 | RED：validator 与 runtime policy 均缺 active-transfer abort recovery marker；现第三轮 SPI TX DMA 启动后调用 `HAL_SPI_DMAStop`，deinit/re-init SPI/DMA、重新注册 callback 并完成 fresh TX DMA IRQ→OSAL task。clean committed `480c3f1c` image 26160 bytes write/verify/read-back byte-identical，BIN SHA-256=`5562ace4...`；20 秒 reset-synchronized capture 11141 bytes、SHA-256=`d9d9ae29...`，normal/re-init/abort-recovery marker 各一次、39 个完整跨组件周期、19 次 SysTick ISR、27 次 TIM6 IRQ、2P/2C 为 9/7、validator error marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile、Pandora link/handler symbol 与 `git diff --check` 通过。仅证明受控 software abort recovery B2；未配置 SPI GPIO/外设，不构成电气响应、RX/full-duplex、物理故障或性能资格 | `480c3f1c` + 本记录提交 | 2026-09-05 |
| S5-91 | P0 | Pandora 板载 W25Q128 QSPI JEDEC ID 实板纵切 | DONE | Zero | S5-90（DONE）；Pandora V2.4 schematic | 按 V2.4 原理图确认 U9 W25Q128：PE10 CLK、PE11 NCS、PE12–PE15 IO0–IO3；启用 CubeL4 QSPI，以单线 `0x9F` 读取并 fail-closed 校验 `EF 40 18`。clean committed `37fa7b95` image 27600 bytes write/verify/read-back byte-identical，BIN SHA-256=`4b1bbf9c...`；20 秒 reset-synchronized capture 11150 bytes、SHA-256=`36258be5...`，`PANDORA_W25Q128_JEDEC_ID_OK` 恰好一次、39 个完整跨组件周期、19 次 SysTick ISR，错误 marker 0；focused 19/19、Host 208/208 与 Pandora link 通过。证明板载 16 MiB SPI Flash 识别 B1；尚未证明擦写、四线模式、掉电恢复、耐久或 FOTA candidate storage | `37fa7b95` + 本记录提交 | 2026-09-05 |
| S5-92 | P0 | Pandora W25Q128 专用测试扇区擦除→页写→读回 | DONE | Zero | S5-91（DONE）；ST-Link/WCH-Link 在线 | 固定使用末尾独立 4 KiB 测试扇区 `0x00FFF000`，执行 write-enable/WEL 校验、sector erase、busy polling、256-byte pattern page program 与 byte-for-byte read-back。首次实板停在 QSPI 阶段，定位为 task stack 仅 512 bytes；增至 1536 bytes 后 clean committed `5375f02f` image 28332 bytes write/verify/read-back byte-identical，BIN SHA-256=`ce548238...`。20 秒 reset-synchronized capture 11187 bytes、SHA-256=`79186b49...`，erase/write/read marker 一次、39 个完整跨组件周期、19 次 SysTick ISR、26 次 TIM6 IRQ、2P/2C 为 9/7、错误 marker 0；focused 2/2、Host 208/208、PC/L4/U5 compile 与 Pandora link 通过。仅证明受控测试区擦写 B1，不构成掉电恢复、耐久、四线模式或 FOTA candidate storage | `4a39b2a8`、`5375f02f` + 本记录提交 | 2026-09-05 |
| S5-93 | P0 | Pandora W25Q128 QSPI controller re-init persistence | DONE | Zero | S5-92（DONE）；ST-Link/WCH-Link 在线 | 在末尾测试扇区完成 erase/program/read 后，显式 QSPI deinit→re-init，再从同地址读取并逐字节匹配原 256-byte pattern；validator 要求 staged/recovered marker 各一次且 error marker 为 0。clean committed `11002838` image 28560 bytes write/verify/read-back byte-identical，BIN SHA-256=`6e1eb361...`；20 秒 capture 11355 bytes、SHA-256=`0c30e2b2...`，39 个完整跨组件周期、19 次 SysTick ISR、26 次 TIM6 IRQ、2P/2C 为 9/7；focused 21/21、Host 208/208、PC/L4/U5 compile 与 Pandora link 通过。仅证明控制器重初始化后的数据持久化，不是 MCU/板级复位、掉电恢复、耐久、四线模式或 FOTA storage | `11002838` + 本记录提交 | 2026-09-05 |
| S5-94 | P0 | Pandora W25Q128 MCU software-reset persistence | DONE | Zero | S5-93（DONE）；ST-Link/WCH-Link 在线 | 固件在测试扇区写入并校验 256-byte pattern 后，以 RTC backup register 标记一次性 recovery，调用 `NVIC_SystemReset()`；第二次 boot 重新初始化 QSPI 并在任何重写前读回相同 pattern。clean `3b6fa217` image 28904 bytes write/verify/read-back byte-identical，BIN SHA-256=`e021f395...`；50 秒 capture 26170 bytes、SHA-256=`fc37f6a3...`，两次匹配 firmware identity、严格 staged→second boot→recovered，94 个完整跨组件周期、46 次 SysTick ISR，错误 marker 0。仅证明 MCU software-reset B2，不是掉电、NRST、耐久、四线模式或 FOTA candidate storage | `3b6fa217` + 本记录提交 | 2026-09-05 |
| S5-95 | P0 | Pandora W25Q128 quad-output fast read | DONE | Zero | S5-94（DONE）；ST-Link/WCH-Link 在线 | 使用 `0x6B`、1-line instruction/address、8 dummy cycles 与 4-line data 从专用测试扇区读取既有 256-byte pattern，并逐字节比较。clean committed `8f2981b2` image 29116 bytes write/verify/read-back byte-identical，BIN SHA-256=`323b4730...`；复位后 30 秒 capture 16312 bytes，匹配 exact firmware SHA，`PANDORA_W25Q128_QUAD_READ_OK` 一次、58 个完整跨组件周期、29 次 SysTick ISR、错误 marker 0。仅证明 quad-output read B1，不是 quad page-program、掉电/NRST、耐久、性能或 FOTA candidate storage | `8f2981b2` + 本记录提交 | 2026-09-05 |
| S5-97 | P0 | W25Q128 canonical QSPI HAL→Device driver→Pandora 迁移 | DONE | Zero | S5-96（DONE）；组件 ownership 决策 | 新增通用 `xy_hal_qspi` command/config API、STM32L4 backend，以及 `components/drivers/storage/flash/w25q128` canonical driver；driver 通过 `xy_device_register` 注册 `XY_DEV_TYPE_FLASH`，JEDEC、erase、page/quad program、single/quad read 均走 HAL 抽象。Pandora `rtos_main.c` 不再直接调用 `HAL_QSPI_Command/Transmit/Receive`。Host 新增 `storage_w25q128` 4/4 契约，canonical Host 209/209、PC/L4/U5 compile 通过；clean `2ba546ad` image 30100 bytes write/verify/read-back byte-identical，BIN SHA-256=`f9d41296...`；两段 reset capture 合并 17436 bytes，60 个跨组件周期、29 次 SysTick ISR，JEDEC/erase-write-read/quad-read/quad-program/MCU-reset recovery marker 完整且错误 marker 0。仅证明 Pandora canonical storage path B1/B2；FOTA/DM 尚未成为 consumer | `6052ef88`～`2ba546ad` + 本记录提交 | 2026-09-05 |
| S5-98 | P0 | Pandora FOTA candidate storage→canonical W25Q128 | DONE | Zero | S5-97（DONE）；W25Q128 canonical driver | 新增 `xy_fota_w25q128` flash-ops adapter，绑定显式 base/size，按 4 KiB 扇区覆盖 erase range、跨 256-byte 边界拆分 page program，并 fail-closed 拒绝越界/未绑定访问及映射 driver 错误。Pandora 独立使用 `0x00FFE000` 4 KiB FOTA 测试区，完成 300-byte candidate erase→跨页 write→read-back。Host adapter 3/3、canonical Host 210/210，FOTA-enabled PC/L4/U5 compile 通过；clean `2b5d04a2` image 30820 bytes write/verify/read-back byte-identical，BIN SHA-256=`a7a41e0e...`；双 boot UART chain 17714 bytes、SHA-256=`865c20a9...`，candidate marker 完整、60 个跨组件周期、29 次 SysTick ISR、错误 marker 0。仅证明 bounded candidate storage B1，不是完整 image download/CRC/signature、bootloader handoff、掉电或 secure FOTA | `2b5d04a2` + 本记录提交 | 2026-09-05 |
| S5-99 | P0 | Pandora FOTA core download→W25Q128→全镜像 CRC | DONE | Zero | S5-98（DONE）；canonical FOTA core | FOTA core 新增显式 expected CRC setter；`finish_download()` 对完整 candidate 执行跨 chunk 连续 CRC32，失败进入 ERROR。clean `89cd80d4` image 32792 bytes write/verify/read-back byte-identical，BIN SHA-256=`3588302a...`；20 秒 UART capture 11401 bytes、SHA-256=`7c341c8e...`，匹配 exact firmware identity 且 `PANDORA_W25Q128_FOTA_DOWNLOAD_CRC_OK` 一次、error marker 0。focused 5/5、canonical Host 除 committed-HEAD clean-export gate 外 209/209、PC/L4/U5 compile 与 handler symbol gate 通过；clean-export 仅因本提交尚未包含最终 docs 状态而失败，提交后复验。仅证明 bounded 300-byte candidate download/read-back/CRC B1，不是签名、bootloader handoff、真实 candidate execution、掉电或 secure FOTA | `89cd80d4` + 本记录提交 | 2026-09-05 |
| S5-100 | P0 | Pandora 当前完整固件镜像→W25Q128 candidate storage | DONE | Zero | S5-99（DONE）；canonical W25Q128/FOTA adapter | linker 导出实际 Flash image load end；runtime 从 `0x08000000` 读取 clean committed image 的 33108-byte load span，经 FOTA core 256-byte chunks 写入 W25Q128 `0x00F00000` 的独立 512 KiB candidate region，并由 `finish_download()` 从外部 Flash 全量重读 CRC。`fb12dab9` BIN SHA-256=`bb0f777d...`，ST-Link write/verify 与 33108-byte read-back byte-identical；reset-synchronized UART 4969 bytes、SHA-256=`15d947d2...`，exact firmware identity/full-image OK 各 1、full-image error 0。focused 3/3、Host 210/210、PC/L4/U5 compile、Pandora handler/image-end symbols 与 `git diff --check` 通过。仅证明完整镜像 candidate storage B1，不是独立 bootloader、candidate execution/vector handoff、签名、掉电或 secure FOTA | `fb12dab9` + 本记录提交 | 2026-09-05 |
| S5-101 | P0 | Pandora candidate→internal execution slot 安装 contract 与独立 application layout | DONE | Zero | S5-100（DONE）；安全驻留 bootloader 尚未实现 | TDD RED 首先因 `xy_fota_boot_install_ops_t`/install API 不存在而编译失败；新增 fail-closed validate-before-erase、按 program granule 分块写入及逐块 read-back 校验。独立 Pandora application linker 从 `0x08008000` 起始，保留前 32 KiB 给未来 bootloader，ELF vector 位于 `0x08008000`，initial SP=`0x20018000`、Thumb reset=`0x0800ad15`，BIN 33736 bytes/SHA-256=`5adf3ee7...`；focused、Host 211/211、PC/L4/U5 compile 与 `git diff --check` 通过。由于尚无驻留 bootloader、board internal-Flash copy ops 与安全 jump，本轮未烧录偏移 application，不宣称 candidate execution | 本记录提交 | 2026-09-05 |
| S5-96 | P0 | Pandora W25Q128 quad-input page program | DONE | Zero | S5-95（DONE）；ST-Link/WCH-Link 在线 | canonical W25Q128 driver 新增 `0x32` quad-input page-program，并以 Host transaction/page-boundary contract 与 Pandora fail-closed marker 守护。clean committed `15aa75e4` image 30100 bytes write/verify；UART 捕获匹配 exact firmware SHA，并在 erase→quad program→quad read byte-compare 后输出 `PANDORA_W25Q128_QUAD_PROGRAM_OK`。随后同一 BIN 再次 write/verify/read-back byte-identical，SHA-256=`682b588e...`。WCH-Link VCP 在 marker 后 EOF，故不将本轮短 capture 提升为完整 stress 记录；仅证明板载 Flash quad-input program B1，不是性能、耐久、掉电/NRST 或 FOTA candidate storage | `15aa75e4` + 本记录提交 | 2026-09-05 |
| S5-102 | P0 | Pandora SPI RX/full-duplex DMA 无外部 source 边界收口 | DONE | Zero | S5-90（DONE）；无人值守且未确认 SPI loopback/peripheral | clean committed candidate `f322534a` 先保留已验证 TX active-abort recovery，再尝试 RX DMA；35536-byte image ST-Link write/verify/read-back byte-identical，BIN SHA-256=`f7f15db3...`。第二次 boot 捕获 1891 bytes，TX normal/re-init/active-abort markers 均成功，随后明确输出 `PANDORA_SPI_DMA_RX_ERROR`，未出现 RX/full-duplex success marker。按证据边界移除会要求无 source RX 完成的路径与 validator 假成功契约；保留 FOTA root/application 地址修复。focused 3/3、Host 213/213、PC/L4/U5 compile 已通过，最终 clean rebuild/烧录复核见本记录提交。RX/full-duplex 继续 pending，必须有已确认的物理 loopback/外设后才能升级 | `33eb8c8d` | 2026-09-06 |
| S5-103 | P0 | Pandora IPC handler 拒绝传播与后续恢复 | DONE | Zero | S5-81（DONE）；ST-Link/WCH-Link 在线 | RED：Broker handler 返回错误时 `xy_broker_process_msgs()` 仍返回已处理 1 并增加 delivered；现传播 handler 错误、记 dropped，且后续消息仍可处理。clean committed `a015abac` 34072-byte image write/verify/read-back byte-identical，BIN/read-back SHA-256=`5c565f4f...`；UART 捕获匹配 exact firmware identity，`OSAL_IPC_HANDLER_REJECTED→RECOVERED` 各一次、recovery error 0，后续跨组件 pipeline 持续。focused 3/3、Host 213/213、PC/L4/U5 compile 与 handler symbols 通过。完整 stress validator 因历史 RTC reset marker 捕获窗口顺序不满足而 fail-closed，故仅升级 bounded handler rejection/recovery B2，不声明新的全链 stress 记录 | `a015abac`、`7657d80f` | 2026-09-06 |
| S5-104 | P0 | Pandora IPC ISR ingress queue-full/recovery | DONE | Zero | S5-103（DONE）；TIM6/OSAL ISR-safe wake 已验证 | 新增 single-producer ISR ingress ring；TIM6 callback 先发布 payload 再以 `xy_os_semaphore_release_from_isr` 唤醒 task，Broker enqueue/handler 仅在 task context 执行。Host 覆盖 ring full、drain 与 fresh-message recovery；clean committed `39861483` 34820-byte image write/verify/read-back byte-identical，BIN SHA-256=`37f8dded...`；20 秒 UART capture 1980 bytes、SHA-256=`2de9646b...`，最后 boot exact identity 下 `ISR_QUEUE_FULL→ISR_DELIVER→ISR_RECOVERED` 各一次且 error 0。focused 3/3、Host 213/213、PC/L4/U5 compile 与 handler symbols 通过。独立 bounded validator排除历史 RTC marker，不声明新的 full-chain stress、吞吐或任意 ISR producer 资格 | `39861483` + 本记录提交 | 2026-09-06 |
| S5-105 | P0 | Pandora single-ISR ingress sustained backpressure/non-starvation | DONE | Zero | S5-104（DONE）；ST-Link/WCH-Link 在线 | TIM6 single producer 在 task consumer 延迟期间触发四槽 ring backpressure，随后 task-context drain 精确交付 16 条单调 payload；同一 bounded interval 内 normal task producer 继续完成 Broker delivery。clean committed `e33f8d28` 35052-byte image write/verify，30 秒 UART capture 14201 bytes、SHA-256=`e2ae3a3b...`，machine validator 要求 backpressure/16 deliveries/task progress/recovery/sustained markers 且 error 0。focused、Host 213/213、PC/L4/U5 compile 通过。仅为 bounded B2，不构成性能吞吐、多 ISR producer 或长稳资格 | `2b0c7fba`～`e33f8d28` + 本记录提交 | 2026-09-06 |
| S5-106 | P0 | Pandora single-ISR ingress 12-cycle stress/recovery continuity | DONE | Zero | S5-105（DONE）；stress-only committed image `be7cf950` | ST-Link 对 35020-byte image 回读与 BIN byte-identical，SHA-256=`6aef27b0...`，ELF exact firmware identity 与 SVC/PendSV/SysTick handlers 匹配。首次 reset-synchronized 660 秒 capture 仅完成 11/12 cycles，validator 按 exact-count fail-closed；不改固件，重置后执行 720 秒 capture，保留 371021 bytes、SHA-256=`76bb3c83...`。dedicated validator 通过：12 次 backpressure/recovery、192 条严格单调 ISR payload、12 次 task producer progress、1387 个完整 IPC→PM→Device→Trace→IPC 周期、sustained marker 1、ISR error 0，exact firmware identity 匹配。仅授予 single-TIM6-producer 12-cycle/12-minute bounded stress B2；不构成性能吞吐、多 ISR producer 或多小时 endurance | `be7cf950` + 本记录提交 | 2026-09-06 |
| S5-107 | P0 | Pandora SYS IWDG reset reason/recovery | DONE | Zero | S2-04（DONE）；ST-Link/WCH-Link 在线 | board smoke 在 software-reset 后自动启动 IWDG 且不刷新；clean `12bf990f` 12248-byte image write/verify/read-back byte-identical，BIN SHA-256=`a22ed235...`。reset-synchronized UART 1602 bytes、SHA-256=`01841e3f...`，CSR=`0x24000600`、稳定 UID、`WATCHDOG`/recovery marker 与后续 8 个 AHT10 ACK 周期完整。focused、Host 213/213、PC/L4/U5 compile 与 `git diff --check` 通过。仅证明 IWDG timeout reset detection/recovery B2，不构成 external NRST、掉电、refresh policy 或耐久资格 | `12bf990f` + 本记录提交 | 2026-09-06 |
| S5-108 | P0 | Pandora SYS ST-Link reset-pin reason/recovery | DONE | Zero | S5-107（DONE）；ST-Link/WCH-Link 在线 | reset classification 将 `PINRSTF` 且无 `BORRSTF` 明确识别为 external pin reset；clean committed `960f68b0` 12348-byte image write/verify/read-back byte-identical，BIN SHA-256=`6d6d4cdd...`。独立 UART capture 2394 bytes、SHA-256=`bf244d55...`，CSR=`0x04000600`、稳定 UID、external-pin marker 各一次，并在 capture 中保留 15 个 AHT10 ACK 周期。focused、Host 213/213、PC/L4/U5 compile 与 `git diff --check` 通过。仅证明 ST-Link reset command 驱动的 NRST B2，不构成掉电/brownout、人工 reset button 或耐久资格 | `960f68b0` + 本记录提交 | 2026-09-06 |
| S5-109 | P0 | Pandora Actuator buzzer PB2 short-short-long | DONE | Zero | Pandora PB2 active buzzer；ST-Link/WCH-Link 自动链路 | reusable Actuator buzzer API 与 GPIO-backed HAL adapter 已接入。clean committed `e771b994` 7144-byte image 经 ST-Link write/verify，回读与 BIN SHA-256 同为 `1b11f781...`；8 秒独立 UART 186 bytes，exact identity、short-short-long start/done 与 final-off marker 有序且 error 0。focused 2/2、Host 216/216、PC/L4/U5 compile 与 `git diff --check` 通过；Eugene 在现场人工确认听到短—短—长且最终停止，授予该固定模式 audible B1，不构成声压、频率或耐久资格 | `e771b994` + 本记录提交 | 2026-09-06 |

| Sprint | 周期 | 目标 | 进入条件 | 当前状态 |
|---|---:|---|---|---|
| Sprint 1 | 2 周 | GUI backend 错误传播、strict backend、字体与单一显示纵切 | Sprint 0 门禁可信 | IN_PROGRESS |
| Sprint 2 | 2 周 | Pandora HAL→Device→Driver 最小实板证据链；U5 compile compatibility | Pandora reference board、ST-Link/WCH-Link 自动链路与 Host 前置 | IN_PROGRESS（Pandora-first；SSD1306 deferred） |
| Sprint 3 | 2 周 | Crypto 产品级重建 Phase 1；Pandora Secure FOTA fail-closed | 产品算法清单与 signature provider 边界已完成 | IN_PROGRESS（Pandora FOTA board contract；非安全批准前置） |
| Sprint 4 | 2 周 | Sensor 三轨收敛、DM 掉电测试、Fuel Gauge 实板 | active-source manifest 与 SHT30/ADS1115/MPU6050 single-owner migrations 已完成 | IN_PROGRESS（Sensor ownership 前置）/BLOCKED（实板） |
| Sprint 5 | 2 周 | 单一 RTOS 并发验证；Net/PM 按产品需求推进 | FreeRTOS 已选为 reference；Pandora 已有 bounded task/ISR/resource-lifecycle/120 秒 stress 与 IPC/Device/Trace 跨组件 B1 | IN_PROGRESS（PM/多小时耐久 pending）/BLOCKED（剩余实板矩阵） |
| Sprint 6 | 1–2 周 | Release Candidate | 目标平台 HIL、安全边界和发布门禁达标 | BLOCKED |

---

## 5. 决策与阻塞日志

| 日期 | ID | 类型 | 内容 | 所需决策/解除条件 | 状态 |
|---|---|---|---|---|---|
| 2026-08-17 | D-001 | 决策 | Sensor 实际有 legacy/new/drivers 三条实现路径 | 2026-08-29 canonical API 已确定为 Device model；SHT30/ADS1115/MPU6050/BMP280 四个 owner 已进入 root target，legacy 仅保留明确 compatibility boundary，experimental 保持 test-only | CLOSED |
| 2026-08-17 | D-002 | 安全阻塞 | Fuel Gauge security AES 曾存在明文透传风险 | 2026-08-29 已改为缺 provider 时 fail-closed；真实认证/加密仍须受审查 provider | CLOSED |
| 2026-08-17 | D-003 | 安全阻塞 | Secure FOTA 依赖 security-rejected ECDSA placeholder | production signature provider 未落地前保持 feature-off | OPEN |
| 2026-08-17 | D-004 | 仓库策略 | XinYi 当前仅此 PC 开发，无其他设备并行同步；`origin` 用作服务器备份 | 本地 path-limited commit 后直接推送 `origin/main`；不需要为多设备同步保留审查缓冲 | CLOSED |
| 2026-08-17 | D-005 | 路线决策 | 原 STM32U5-first HIL 路线缺统一硬件证据 | 2026-09-04 确认 Pandora STM32L475VE 为正式 reference board；U5/M33/TrustZone 转为 enhancement-only compile gate，SSD1306 deferred；无人值守边界固定 | CLOSED |
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

### 2026-08-26 Sprint 3 Crypto 清单前置

- 完成：S3-01 将 11 个算法区域按 `production-candidate`、`legacy-compatibility`、`test-only`、`security-rejected` 分类，并补齐 owner、source origin、license、side-channel target 与 allowed usage；policy guard 会拒绝缺字段或将 rejected 实现伪装成候选。
- 边界：候选仍全部 provenance `review-pending`；Host/vector/policy 结果不构成安全、侧信道、硬件或产品批准。
- 下一步：S3-02 已以可替换 signature provider 移除 Secure FOTA 对 format-only ECDSA placeholder 的认证依赖，并覆盖无 provider/provider 拒绝/错误 key ID/回滚版本/截断包 fail-closed；下一 slice 进入 S3-03，只选择一个明确候选做来源审查与重建，不把 Host seam 当作安全批准。

### 2026-08-26 Sprint 3 Signature provider 边界

- 完成：S3-02 新增显式 `xy_fota_signature_provider_t`，由 caller 提供 key ID、context 与真实 verify 回调；Secure FOTA 不再内建回退到 `xy_ecdsa_verify_simple()`。
- RED/GREEN：focused target 初次暴露 secure source 未纳入现有 FOTA CTest 以及 bank/core link 依赖；闭环后 `fota_secure_provider` 1/1 通过，覆盖缺 provider、provider 拒绝（含错误 key ID）、版本回滚和截断包。
- 验证：Host 184/184、PC root build、FOTA-enabled `xy_fota` target build、`git diff --check` 通过；clang-format 当前环境不可用。以上仅为 Host/PC fail-closed 边界，不构成 cryptographic/security/key-provisioning/bootloader/hardware 批准。
- 下一步：S3-03 单算法重建试点，优先 SHA-256/HMAC provenance 与 API/error/memory-clearing contract；不先做 benchmark 扩张。

### 2026-08-26 Sprint 3 SHA-256/HMAC 重建试点

- RED：`crypto_hash` 与 `crypto_cipher_hmac` 证明 zero-length `NULL` 输入此前返回 `XY_CRYPTO_INVALID_PARAM`，与空消息契约不一致。
- 实现：zero-length update/HMAC 允许 `NULL` data，非零长度仍 fail-closed；SHA-256 final 清除 context，HMAC 清除 pads、长 key digest 与 working context。
- 验证：focused 2/2、Host 184/184、PC root build、Crypto-enabled `xy_tiny_crypto` target build 与 `git diff --check` 通过；target compile 仅为 PC，不构成 MCU、安全、constant-time、provenance 或硬件批准。
- 剩余：外部来源/许可证证据、独立实现审计、target compile、fuzz、side-channel 与真实 MCU 记录仍 pending。

### 2026-08-26 Sprint 3 FOTA boot-handoff 去模拟化

- RED：`fota_core` 首先因缺少 `XY_FOTA_NOT_SUPPORTED` 与 boot-handoff API 编译失败，证明 public contract 尚不存在；旧 `xy_fota_start_update()` 会在没有 bootloader 的情况下本地翻转 slot 并返回成功。
- 实现：新增 caller-owned boot-handoff callback；缺 callback 明确返回 unsupported，callback 拒绝时不提交目标 slot 并进入 ERROR，只有 callback 接受后才更新 core slot 状态。
- 验证：focused `fota_core` + `fota_smoke_example` 2/2、Host 184/184、PC Release root build、FOTA-enabled `xy_fota` target build、`git diff --check` 通过。FOTA probe 仍有既存 CLIB/FOTA Flash warning；不构成 STM32U5、bootloader、掉电、硬件或安全批准。
- 剩余：durable boot metadata、mark-valid/rollback/anti-rollback、掉电恢复、`projects/stm32u5_fota` stale API/compile gate 与实板 B1/B2。

### 2026-08-26 Sprint 3 FOTA delta callback 去占位

- RED：`fota_core` 新增 delta contract 后失败（缺 callback 的 `xy_fota_finish_download()` 仍返回成功），证明旧实现错误地用 progress callback 作为 patch capability，并未调用 public patch callback。
- 实现：FOTA handle 持有 caller-owned patch callback/context；delta finish 从暂存区按 256-byte bounded chunks 读取并派发，缺 callback、读取失败或 callback 拒绝均 fail-closed 为 `XY_FOTA_DELTA_ERROR`/ERROR state。
- 验证：focused `fota_core` 1/1、Host 184/184、PC Release root build、FOTA-enabled `xy_fota` target 与 `git diff --check` 通过；既存 CLIB/FOTA Flash warning 未由本 slice 引入。以上仅证明 Host callback/error contract，不构成 patch 算法、掉电、STM32U5、bootloader、安全或实板批准。
- 剩余：durable metadata、mark-valid/rollback/anti-rollback、掉电恢复、STM32U5 项目 compile gate与 B1/B2。

### 2026-08-26 Sprint 3 FOTA mark-valid/anti-rollback 边界

- RED：`fota_core` 因缺 `xy_fota_mark_valid()` / boot-confirm API 编译链接失败，证明 core 没有可验证的 durable confirmation 边界。
- 实现：新增 caller-owned boot-confirm callback；缺 callback 返回 `XY_FOTA_NOT_SUPPORTED`，callback 拒绝时 active slot 与版本下限保持不变，只有 durable confirmation 成功后才提交 active slot 并推进 anti-rollback floor。
- 验证：focused `fota_core` 1/1、Host 184/184、PC Release root build、FOTA-enabled `xy_fota` target 与 `git diff --check` 通过；clang-format 当前环境不可用。以上不构成 durable metadata backend、掉电、STM32U5、bootloader、安全或实板批准。
- 剩余：durable metadata 具体 backend、rollback metadata/掉电恢复、`projects/stm32u5_fota` compile gate 与 B1/B2。

### 2026-08-26 Sprint 3 STM32U5 FOTA 项目 compile skeleton

- RED：Cortex-M33 `-fsyntax-only -Werror` 首先因不存在的 `xy_flashdb.h` 失败；检查还发现旧项目目录被 `.gitignore` 整体排除、`xy_nor_init()` 调用签名过期、`..work_buffer` 语法错误、内部 Flash ops 全为 `NULL` 却打印“全部初始化成功”。
- 实现：将项目入口纳入版本控制并缩为公开 FOTA API 的 fail-closed integration skeleton；显式注册 handoff/confirm board callbacks，默认均返回 `XY_FOTA_NOT_SUPPORTED`；移除 stale FlashDB/NOR 伪集成，并把旧 cache preset 标为非 canonical。
- 验证：Arm GNU 15.2 Cortex-M33 syntax compile（`-Wall -Wextra -Werror`）通过；FOTA focused 3/3、Host 184/184、PC Release root、FOTA-enabled `xy_fota` target、`git diff --check` 通过。FOTA target 仍有既存 CLIB/flash warning；此 gate 不链接 startup/linker/HAL/bootloader，不构成可烧录镜像或硬件证据。
- 剩余：durable metadata backend、rollback/掉电恢复、board-owned internal Flash 与 bootloader、完整 STM32U5 link/image gate、B1/B2。

### 2026-08-26 Sprint 3 FOTA metadata journal

- RED：新增 `fota_metadata` 后 focused build 首先因新 public header 尚未加入 include root 失败；补齐 wiring 后，以 empty flash、partial write、newest-copy corruption 明确 fail-closed/recovery contract。
- 实现：新增 board-neutral 双副本 flash journal，record 使用 generation、CRC 与 commit marker；commit 写入 inactive slot 并 read-back 验证，partial/corrupt newest copy 不覆盖上一有效状态。
- 验证：FOTA focused 4/4、Host 185/185、PC Release root build、FOTA-enabled `xy_fota` target（确认 metadata object 纳入 archive）与 `git diff --check` 通过；clang-format 当前环境不可用。以上仅为 Host/PC 持久化算法契约，不构成 STM32U5 Flash、真实掉电、耐久性、bootloader 或安全批准。
- 剩余：将 journal 接入项目的 handoff/confirm 与 board-owned Flash；定义 boot-attempt/rollback policy 和掉电矩阵；完整 STM32U5 link/image 与 B1/B2。

### 2026-08-27 Sprint 3 FOTA boot-attempt/rollback policy

- RED：`fota_metadata` 因缺少 pending version、boot-attempt 字段及 stage/attempt/confirm API 编译失败，明确原 journal 无法跨复位执行有限次数启动策略。
- 实现：metadata format 升级为 v2；候选 stage 后持久化 version/slot/attempt count，每次未确认启动递增计数，达到 caller-defined 上限即清除 pending 并返回 rollback required；confirm 才切换 active slot/version 并推进 anti-rollback floor。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、通过 `KCONFIG_OVERRIDES=FOTA_ENABLED=ON` 启用的 `xy_fota` target 与 `git diff --check` 通过。FOTA target 保留既存 CLIB/flash warning。以上仅为 Host policy contract，不构成 bootloader、真实掉电、STM32U5 link/image、安全或实板批准。
- 剩余：board-owned Flash 与 bootloader 在每次状态转换后调用 journal commit；补 callback wiring、掉电矩阵、完整 image link 与 B1/B2。

### 2026-08-27 Sprint 3 FOTA metadata callback wiring

- RED：`fota_metadata` 新增 callback contract 后因缺 `xy_fota_metadata_boot_handoff()` / `boot_confirm()` 编译链接失败；随后 commit-marker 写失败测试暴露目标槽可能残留看似已提交的有效 record。
- 实现：新增 load→stage/confirm→双副本 commit 的 core callback adapter；slot/version 不匹配 fail-closed；STM32U5 project skeleton 已注册 metadata backend context。journal 改为先写 record body、最后写 commit marker，marker 写失败时擦除目标副本。
- 验证：FOTA focused 4/4、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过。以上仍不是可链接/可烧录 STM32U5 image，也不构成真实 Flash、bootloader、掉电、安全或实板批准。
- 剩余：board owner 必须提供 internal-Flash ops 与保留区；bootloader 在启动计数路径持久化 journal；完整 STM32U5 link/image 与 B1/B2。

### 2026-08-27 Sprint 3 FOTA durable boot-attempt callback

- RED：`fota_metadata` 因缺 `xy_fota_metadata_boot_attempt()` 编译链接失败，证明 boot-attempt policy 虽存在，但 bootloader 没有可直接调用的 durable adapter。
- 实现：新增 load→record attempt→commit callback；只有 journal commit 成功才输出 rollback 决策，Flash 提交失败时保留上一份 durable attempt count。STM32U5 skeleton 暴露 board wrapper，并明确 bootloader 必须在候选启动前调用。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过。以上仍不是可链接/可烧录镜像，也不构成真实 Flash、bootloader runtime、掉电、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA metadata read-error fail-closed

- RED：`fota_metadata` 注入 journal slot read failure 后，load 错误返回 `XY_FOTA_NO_IMAGE`，commit 仍擦除/写入目标槽，证明 I/O failure 被错误降级为 empty/corrupt record。
- 实现：双槽扫描单独区分 invalid record 与 read failure；任一槽不可读即返回 `XY_FOTA_FLASH_ERROR`，load 保留调用方输出，commit 不 erase/write，上一 durable record 保持不变。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check`。以上仍不构成真实 Flash 掉电、bootloader runtime、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、erase/write/read 各掉电边界矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA metadata read-back cleanup

- RED：提交 body 与 marker 后注入 read-back I/O failure，focused `fota_metadata` 观察到目标副本未被擦除；本次 API 虽返回 Flash error，但重启后仍可能把未经 read-back 验证的新 generation 识别为有效。
- 实现：最终 read-back 失败、CRC/marker 无效或内容不一致时 best-effort 擦除目标 slot；上一 committed copy 继续作为 newest valid record。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 syntax probe 与 `git diff --check` 通过。以上仍不构成真实掉电、Flash 耐久性、bootloader runtime、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、erase/write/read 各掉电边界矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA boot-attempt counter saturation

- RED：将持久化 `boot_attempts` 构造成 `UINT8_MAX` 后，focused `fota_metadata` 观察到递增 wrap 为 0 且错误返回“不需 rollback”，可能无限延长损坏候选的启动窗口。
- 实现：attempt count 达到或超过 caller 上限时不再递增，直接进入既有 rollback 清理路径；覆盖 `UINT8_MAX` 持久化状态与 `UINT8_MAX` 上限。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 syntax probe 与 `git diff --check` 通过。以上仍不构成真实 bootloader、Flash 掉电、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA metadata commit state validation

- RED：构造非法 active slot、pending flag/slot/version 不一致、pending 与 active 同槽、候选版本低于 rollback floor 及未知 flags 后，focused `fota_metadata` 观察到 journal commit 仍返回成功并写入 Flash。
- 实现：commit 前统一校验 metadata 状态；非 pending 状态必须清空 pending slot/version/attempt count，pending 状态必须使用另一有效 slot、非零且不低于 min version 的候选版本，并拒绝未知 flags；非法输入不触发 erase/write。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 syntax probe 与 `git diff --check` 通过；clang-format 当前环境不可用。以上仍不构成真实 Flash、bootloader runtime、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA metadata load semantic validation

- RED：将 newest journal copy 的 `active_slot` 改为非法值并重算有效 CRC 后，focused `fota_metadata` 仍将 generation 2 提升为启动状态，证明 load 只验证 envelope/CRC 而未验证 payload 语义。
- 实现：提取统一 metadata 字段校验并同时用于 commit 与 journal load；CRC 正确但 slot、flags 或 pending 字段组合非法的副本现在被视为 invalid，回退到上一有效 generation。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过。以上仍不构成真实 Flash、bootloader runtime、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA metadata active-version floor validation

- RED：构造 `min_version > active_version` 的待提交状态，以及 CRC/marker 均有效但 rollback floor 高于 active image 的 newest journal copy；focused `fota_metadata` 分别观察到 commit 成功与 load 提升非法 generation。
- 实现：统一 metadata 字段校验新增 `active_version >= min_version` 不变量；非法状态不写 Flash，CRC 正确但 anti-rollback floor 与 active image 矛盾的 newest copy 回退到上一有效 generation。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过；clang-format 当前环境不可用。以上仍不构成真实 Flash、bootloader runtime、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA metadata backend address-range guard

- RED：将 metadata base 放到 `UINT32_MAX` 附近后，focused `fota_metadata` 进入 fake Flash，并因双槽地址计算 wrap 到低地址失败，证明 backend 只校验 erase size、未校验完整 journal 地址范围。
- 实现：backend validation 在任何 read/erase/write 前确认最后一个 slot 及其 record 范围均可由 `uint32_t` 地址表示；非法范围返回 `XY_FOTA_INVALID_PARAM`，且无 Flash 副作用。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过。以上仍不构成真实 Flash、bootloader runtime、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA public transition state validation

- RED：向公开 `stage_candidate`、`record_boot_attempt` 与 `confirm_candidate` 直接传入非法 active slot、`active_version < min_version` 或未知 flags 后，focused `fota_metadata` 观察到状态仍被修改并返回成功；此前只有 journal commit/load 边界验证 payload。
- 实现：三个公开内存状态转换入口在任何修改前复用统一 metadata 状态校验；非法状态返回 `XY_FOTA_INVALID_PARAM` 并逐字节保持调用方 metadata 不变。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过。以上仍不构成真实 Flash、bootloader runtime、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA duplicate handoff guard

- RED：候选已持久化且累计一次 boot attempt 后，再次 stage 同一候选仍返回成功并把 `boot_attempts` 清零，允许重复 handoff 绕过 bounded rollback。
- 实现：已有 pending candidate 时，`xy_fota_metadata_stage_candidate()` 在修改状态前统一返回 `XY_FOTA_IN_PROGRESS`；同版本与不同版本的重复 stage 都保持 pending metadata 与 attempt count 不变。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过。以上仍仅为 Host/源级 contract，不构成真实 bootloader、Flash 掉电、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、掉电矩阵与 B1/B2。

### 2026-08-27 Sprint 3 FOTA ambiguous generation ordering guard

- RED：构造两份 CRC/commit marker 与 payload 均有效、generation 恰好相差 `2^31` 的 journal records 后，focused `fota_metadata` 仍按 slot 扫描顺序选择其中一份并返回成功；RFC1982 风格的串号比较在该距离没有唯一新旧顺序。
- 实现：journal scan 显式识别两份有效 record 的 generation 半范围歧义并返回 `XY_FOTA_FLASH_ERROR`；load 不修改调用方输出，commit 也不会在无法可靠确定 inactive slot 时擦写 Flash。
- 验证：focused `fota_metadata` 1/1（内部 19/19）、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU 15.2 Cortex-M33 对 metadata 与 STM32U5 FOTA main 的 `-Wall -Wextra -Werror -fsyntax-only`、`git diff --check` 均通过。PC root 仍有既存 warning，本 slice 未新增。以上仍仅为 Host/源级 contract，不构成真实 bootloader、Flash 掉电、安全或实板批准。

### 2026-08-27 Sprint 3 FOTA equal-generation split-brain guard

- RED：复制有效 journal record 到另一槽并保持相同 generation，但修改 active version/slot 后重算 CRC；focused `fota_metadata` 观察到 load 按 slot 顺序静默选择首份，commit 也继续覆盖另一槽。
- 实现：两份有效 record generation 相等但受 CRC 保护的状态字段不一致时，journal scan 返回 `XY_FOTA_FLASH_ERROR`；load 保持调用方输出，commit 不擦写 Flash。完全相同的冗余副本仍可读取。
- 验证：focused `fota_metadata` 1/1（内部 20/20）、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU Cortex-M33 metadata `-Wall -Wextra -Werror -fsyntax-only` 与 `git diff --check` 通过；clang-format 当前环境不可用。以上仅为 Host/源级 split-brain contract，不构成真实 bootloader、Flash 掉电、安全或实板批准。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、真实掉电矩阵与 B1/B2。

### 2026-08-28 Sprint 3 STM32U5 metadata backend startup gate

- RED：focused `fota_metadata` 在调用预期的 `xy_fota_metadata_flash_validate()` 时编译警告并链接失败，证明项目骨架无法在注册 callback/进入事件循环前显式验证 board backend；默认 `.ops = NULL` 的 skeleton 仍打印 ready。
- 实现：公开复用 journal backend validation；STM32U5 skeleton 在注册 handoff/confirm callback 前校验 read/write/erase、erase size 与地址范围，默认未绑定 backend 时返回错误而不是宣称 ready。README 同步说明 fail-closed 启动边界。
- 验证：focused `fota_metadata` 1/1、Host 185/185、PC Release root、FOTA-enabled `xy_fota` target、Arm GNU 15.2 Cortex-M33 对 metadata 与 STM32U5 FOTA main 的 `-Wall -Wextra -Werror -fsyntax-only`、`git diff --check` 均通过。以上仅证明 Host contract 与源级项目 gate，不构成完整 STM32U5 link/image、真实 Flash、bootloader runtime 或实板证据。
- 剩余：board-owned internal-Flash ops/保留区、真实 bootloader 调用、完整 STM32U5 link/image、真实掉电矩阵与 B1/B2。

### 2026-08-28 Sprint 3 远端与看板事实校准

- 校准：S3-04 提交范围已从临时“本轮提交”更新为真实末端 `b832a7ae`；项目 README 同步为 metadata callback 已接线、但默认 backend 未绑定的真实状态。
- 推送：累计 3 个已验证提交已 fast-forward 推送；本地 `HEAD`、tracking ref 与远端 `main` 均为 `b832a7ae71ddbebe4ed5e86b8bac817900ee6789`，ahead/behind `0/0`。
- 边界：该同步与文档校准不增加 STM32U5 link/image、真实 Flash、bootloader runtime、掉电、安全或实板证据。
- 下一步：在参考板 owner 提供 Flash 保留区与 ops 前，S3-04 保持 `IN_PROGRESS`；不以继续扩张 Host metadata policy 代替 board-owned 集成。

### 2026-08-28 Sprint 4 Sensor ownership 前置

- RED：新增 policy probe 首次执行因缺 `sensor-active-source-manifest.md` 失败，证明三轨 ownership 只有审计描述、没有受检事实源。
- 完成：建立 active-source manifest，固定 55 个 legacy root sources、23 个 test-local experimental sources 与 4 个 Device-model root sources；明确 Host 测试不等于根产品链接，并禁止第四套生命周期。
- 验证：focused `sensor_active_source_manifest`、`sht30_integration`、`sensors_multi` 3/3；Host 全量与 PC root gate；`git diff --check`。以上仅为 source ownership/Host 证据，不构成 Sensor 精度、时序、校准或实板批准。
- 下一步：以 SHT30 为首个迁移候选，先判定 legacy/new/Device 三份实现的唯一 canonical owner 与 compatibility wrapper，再做单一实现链接闭环；不新增传感器型号。

### 2026-08-28 Sprint 4 SHT30 single-owner 迁移收口

- RED：legacy focused target 改为期待 Device 命令/CRC/双地址委托后，旧 `sensor_sht30.c` 仍引用独立 `hal_i2c_master_send/recv`，链接失败，证明 root legacy lifecycle 仍拥有第二套实现。
- 实现：`components/drivers/sensor/temperature/sht30/xy_sht30.c` 成为唯一实现 owner；新增显式双地址初始化入口并保留原 0x44 API；legacy `sht30_create()` 生命周期只做错误映射、humidity 数据适配与委托，root `sensor_component` 显式链接 canonical source。
- 验证：focused `sht30_integration`、`sensor_sht30_device`、`sensor_sht30_legacy` 3/3；Host 186/186；PC Release root 与 `sensor_component` target；`git diff --check`。clang-format 当前环境不可用；既存 Sensor strict-aliasing/unused warning 未由本 slice 引入。
- 边界：只构成 Host/PC source-ownership 证据，不构成 SHT30 精度、总线时序、恢复或实板批准。
- 下一步：按同一准入契约选择 MPU6050 或 ADS1115 做第二个 canonical-owner migration；SHT30 B1/B2 继续受硬件环境阻塞。
- 仓库：实现提交 `a60ee6dec2c87a767e7cd148703b98f05be8951d` 与累计 8 个本地提交已在默认 SSH 路径持续超时后，改用 GitHub 官方 SSH-over-443 endpoint 成功 fast-forward 推送；本地 `HEAD`、tracking ref 与远端 `main` 均为 `5a7bc2a8e5f5625fa609ce7352c21befe396dcf8`，ahead/behind `0/0`。

### 2026-08-28 Sprint 4 MPU6050 single-owner 迁移

- RED：focused `sensor_mpu6050` 切到 Device owner 后因 public API/struct contract 不完整而编译失败；随后 full Host 暴露 heterogeneous fixture 缺日志 stub，root target 暴露 Sensor include ownership缺口。
- 实现：将 range、校准、物理量转换与 I/O 输出保持 contract 合并到唯一 Device owner；默认 0x68 init 保持兼容，显式地址入口支持 0x69；init/deinit/range/calibration I/O 失败 fail-closed；删除 test-local duplicate，并由 root `sensor_component` 显式链接 canonical source。
- 验证：focused `sensor_mpu6050` 1/1；Host 186/186；PC Release root 与 `sensor_component`；`git diff --check`。既存 Sensor strict-aliasing/unused warning 未由本 slice 引入；不构成 IMU 精度、校准质量、总线恢复或实板证据。
- 下一步：评估 BMP280 是否仍有 duplicate lifecycle，或转入 S4-3 DM FS/JSON focused test；不新增 Sensor 型号。

### 2026-08-28 Sprint 4 DM FS focused contract

- RED：新 `dm_fs` focused test 对未注册、`ops == NULL` 的 FS 调用 mount 时稳定触发空指针崩溃；随后覆盖 deinit/close error、非法 seek、整文件 I/O 与跨 drive rename。
- 实现：mount/unmount/read/write/seek/tell 增加结构完整性 guard；unmount 不再吞 deinit 错误；整文件 helper 传播 read/write/close 错误并拒绝短写，失败时不提交 `actual`。
- 验证：focused `dm_fs` 1/1、Host 187/187、PC Release root、`xy_dm` target 与 `git diff --check` 通过。以上仅为 Host/PC FS abstraction contract，不构成 Flash/NOR 掉电、磨损、布局恢复或实板证据。
- 下一步：为 active `xy_json` 补 dedicated focused test；之后建立 DM power-loss fail-closed record 与可注入 interruption 的 NVM/Flash recovery slice。

### 2026-08-28 Sprint 4 DM active JSON focused contract

- RED：新 `dm_json` focused test 证明 active `xy_json` 接受 trailing/incomplete document、非法 number token，且同名 object set 追加 duplicate member。
- 实现：parser 现在要求完整消费输入、拒绝未终止字符串与非法 JSON number grammar；同名 object set 原子替换旧 member；array/object mutation 的 realloc 失败返回 `XY_JSON_ERROR_NO_MEMORY` 而不覆盖 owner pointer。
- 验证：focused `dm_json` 1/1、Host 188/188、PC Release root、`xy_dm` target 与 `git diff --check` 通过。以上仅为 Host/PC parser contract，不构成掉电、持久化、资源上限或实板证据。
- 下一步：建立 DM power-loss fail-closed record，并为 NVM/Flash recovery 增加可注入 interruption 的最小 slice。

### 2026-08-28 Sprint 4 远端备份校准

- 校准：本轮开始本地 `main` 领先 `origin/main` 4 个已验证提交（`fa0a8044`、`6b06e06e`、`9b64f4fb`、`0439c861`），工作树无未提交改动。
- 推送：默认 GitHub SSH 端点超时；改用官方 SSH-over-443 URL 后完成 fast-forward push。推送后本地与远端 `main` 均为 `0439c8619fad0a4fc6f9d99ff87d5f9727973075`，ahead/behind `0/0`。
- 边界：本轮只校准远端备份和 Sprint 事实，不新增 Host、target、持久性或实板证据。
- 下一步：按 S4-3 建立 DM power-loss fail-closed record，并以可注入 interruption 的 NVM/Flash recovery 最小 slice 启动 RED→GREEN。

### 2026-08-28 Sprint 4 DM NVM restart recovery

- RED：同一 key 顺序写入两个完整值并重新初始化后，`xy_nvm_get()` 返回首个旧值，证明 append-only 更新没有 newest-record recovery 语义。
- 实现：查找过程继续扫描并选择最后一个 checksum-valid/enabled record；Host fixture 在第三个 append 位置注入 header 可识别但 checksum 不完整的 torn record，重启后仍读取最后一个完整值。
- 记录：建立 `xinyi-dm-power-loss-validation-record.md`，当前严格分类为 `HOST_INTERRUPTION_GUARDED`；erase/write 全边界、layout migration、真实 Flash 掉电与耐久性仍 pending。
- 验证：focused `dm_nvm` 1/1、Host 188/188、PC Release root、`xy_dm` target 与 `git diff --check` 通过；clang-format 当前环境不可用。以上不构成真实 Flash、板级掉电、性能或耐久性证据。
- 下一步：为 NVM 引入可注入 Flash backend，逐步覆盖 header/data 写入失败与 erase interruption；之后再做 layout migration，避免把 Host byte-array seam 当作 B2。

### 2026-08-28 Sprint 4 DM NVM write interruption seam

- RED：新增 caller-owned storage backend fixture 后，header write 注入失败仍返回 `XY_NVM_OK`，证明 active NVM 的固定内存模拟无法传播真实后端错误。
- 实现：`xy_nvm_config_t` 增加可选 storage ops/context，保留 NULL ops 的内存映射兼容路径；set/delete/format 传播 backend write/erase 错误。Host fixture 在新记录 header 与 payload write 分别失败，重启后均要求保留上一完整值。
- 验证：focused `dm_nvm` 1/1；全量 Host、PC root、`xy_dm` 与 `git diff --check` 见本轮提交 gate。以上仅为 Host fault-injection contract，不构成真实 Flash 掉电、写粒度、耐久性或板级证据。
- 下一步：覆盖 erase interruption 与 exhaustive supported program-granule sweep，再进入 layout migration；硬件可用后按记录补真实 Flash B2。

### 2026-08-29 Sprint 4 DM NVM partial-program recovery

- RED：对 append header/payload 的每个 byte boundary 模拟部分写入后报错；partial header 遗留后，重启虽保留上一完整值，但后续 retry 错误返回 `XY_NVM_ERROR_FULL`。
- 实现：append 扫描以 4-byte 步进跳过无法解释或越界的 torn header，不再将其误判为永久满载；完整 record 仍按对齐长度跳过。
- 验证：focused `dm_nvm`、Host 全量、PC root、`xy_dm` 与 `git diff --check` 见本轮提交。以上仅为 Host byte-boundary fault injection，不构成目标 Flash 写粒度、真实掉电、耐久性或板级证据。
- 下一步：覆盖 erase interruption，再进入 layout migration；真实 Flash 可用后按记录补 program-granule 与 B2。

### 2026-08-29 Sprint 4 DM NVM partial-erase recovery

- Probe：Host backend 在 format 的 256-byte erase 区域每个 byte boundary 执行部分擦除后返回错误，验证 active NVM 的 erase 错误传播和重启边界。
- 契约：`xy_nvm_format()` 必须传播 backend error；重启扫描只允许返回仍完整的最后值或 `NOT_FOUND`，不得提升 torn record；移除故障后完整 format 必须成功并恢复为空。
- 验证：focused `dm_nvm`、Host 全量、PC root、`xy_dm` 与 `git diff --check` 见本轮提交。以上仅为 Host byte-array erase fault injection，不构成目标 Flash erase 粒度、真实掉电、耐久性或板级证据。
- 下一步：进入 NVM layout migration contract；真实 Flash 可用后按记录补 program/erase B2。

### 2026-08-29 Sprint 4 DM NVM layout migration

- RED：legacy/current record 共用 `0xAA55AA55` magic，新 append 无法表明当前 layout；focused contract 要求升级后保留 legacy 数据并只写 current record，初次运行在 magic 断言失败。
- 实现：读取路径兼容 legacy `0xAA55AA55` 与 current `0xAA55AA56`；新记录只写 current magic。同 key 从 legacy 追加 current 值后，重启选择最新 current 值，无需升级时先擦除。
- 验证：focused `dm_nvm` 1/1、Host 188/188、PC Release root、`xy_dm` target 与 `git diff --check` 通过；clang-format 当前环境不可用。以上仅为 Host layout contract，不构成真实旧版本 Flash image、目标 Flash 掉电、耐久性或板级证据。
- 下一步：在真实 Flash 可用前，补 checksum/metadata corruption 的 exhaustive Host contract；真实板到位后用保存的 legacy image dump 执行迁移与 B2。

### 2026-08-29 Sprint 4 DM NVM metadata/checksum corruption

- RED：使用 caller-owned storage backend 与非映射逻辑 `flash_base` 后，focused `dm_nvm` 在 checksum 扫描中直接解引用逻辑地址并崩溃，证明 backend 抽象未贯穿读取路径。
- 实现：checksum 数据统一通过 storage ops 读取；V2 current record 使用 CRC-8（poly `0x07`）保护 key、enable、length 与 payload，legacy V1 additive checksum 保持可读兼容。
- 验证：逐 byte 破坏 newest V2 record 的 magic、key、enable、length、CRC 与 payload 后，重启均回退上一完整值；focused `dm_nvm` 1/1、Host 188/188、PC Release root、`xy_dm` target 与 `git diff --check` 通过。以上仅为 Host corruption contract，不构成目标 Flash ECC、真实掉电、耐久性或板级证据。
- 下一步：DM Host 前置已覆盖计划中的 restart/interruption/corruption/layout；真实硬件未到位时转入下一个无硬件依赖的 Sprint slice，不继续扩张 NVM policy。

### 2026-08-29 Sprint 4 Fuel Gauge security fail-closed

- RED：配置 `XY_FG_SECURITY_AES128` 后，`xy_fuel_gauge_encrypt_data()` 仍返回成功并把明文原样写入“密文”输出。
- 实现：未接入受审查 provider 的 AES/SHA 安全模式统一返回 `XY_FG_ERROR_NOT_SUPPORTED`，encrypt/decrypt 输出及长度保持不变；显式 `XY_FG_SECURITY_NONE` 的兼容复制契约不变。
- 验证：focused `fuel_gauge_core`、Host 全量、PC Release root、`xy_fuel_gauge` target 与 `git diff --check` 见本轮 gate。以上仅关闭 plaintext passthrough 风险，不构成 authentication、cryptographic provider、安全审查或实板批准。
- 下一步：硬件仍不可用时，校准 Sprint 4 剩余无硬件依赖 ownership/治理项；真实 Fuel Gauge SMBus 与安全 provider 分别等待板卡和 security review。

### 2026-08-29 Sprint 4 BMP280 Device owner contract

- RED：heterogeneous Sensor test 改用 `xy_bmp280_init_addr()` 后因 API/地址常量不存在而编译失败，证明第四个 Device owner 仍只有简化占位契约。
- 实现：canonical BMP280 Device source 新增 0x76/0x77 初始化、完整 Bosch 整数补偿、所有 init/deinit I/O 错误传播、缓存输出保持与 status-returning getters，并显式进入 root `sensor_component`；legacy lifecycle 保持冻结兼容，experimental source 仍不进入产品 root。
- 验证：focused `sensor_bmp280_device`/`sensors_multi`/`sensor_bmp280` 3/3、Host 189/189、PC Release root、`sensor_component` target 与 `git diff --check` 通过；clang-format 当前环境不可用。以上仅为 Host/PC source/contract 证据，不构成精度、时序、总线恢复或实板批准。
- 下一步：不继续扩张 Sensor 型号；校准 BMP280 experimental API 的 compatibility/deprecation 决策，或转入下一个 Sprint 无硬件依赖治理项。

### 2026-08-29 Sprint 4 BMP280 stale lifecycle/example 收口

- RED：active-source policy probe 发现未被 CMake/consumer 引用的 `xy_sensor_bmp280.c` 第四生命周期仍存在；tracked smart-hygrometer example 同时指向不存在的 `components/sensor/src/xy_bmp280.c` 并调用旧三参数 API。
- 实现：删除无引用第四生命周期；smart-hygrometer 改为编译 canonical Device owner，并使用 `xy_bmp280_init_addr()`；policy guard 防止 stale source、缺失 experimental source 和旧 API 回归。
- 验证：focused/full/PC/`sensor_component` 与 `git diff --check` 见本轮 gate；以上只构成 source ownership/Host/PC 证据，不构成 BMP280 实板、精度或时序批准。
- 下一步：不继续扩张 Sensor；转入下一项无硬件依赖治理，或等待四个 canonical owner 的 B1/B2 环境。

### 2026-08-29 Sprint 4 Charger ownership 事实收口

- RED：事实 probe 发现 `components/charger/DEPRECATED.md` 推荐迁往不存在的
  `components/drivers/power/charger/`，与实际 root Kconfig/CMake、BQ25620 focused test owner 冲突。
- 收口：明确 `components/charger/src/xy_bq25620.c` 为当前 canonical owner，状态为
  `legacy-maintained`；移除不存在的 include/迁移时间表，新增 policy CTest 防止空目标再次成为推荐路径。
- 验证：focused `charger_ownership` + `charger_bq25620` 2/2、Host 190/190、默认 PC root、
  `KCONFIG_OVERRIDES=COMPONENT_CHARGER=ON` 的 `charger` target 与 `git diff --check` 通过。
  以上仅为 Host/PC ownership 证据，不构成充电、热保护、电池安全或实板批准。
- 下一步：Charger 硬件仍不可用时，不扩张新芯片；转入 Sprint 5 reference RTOS/board 决策，
  或等待 BQ25620 充电/热故障 B1/B2 环境。

### 2026-08-29 Sprint 4 Sensor canonical 决策收口

- 校准：D-001 从 `OPEN` 更新为 `CLOSED`；四个迁移试点已证明 Device model 为 canonical
  owner 路径，legacy 只保留 compatibility boundary，20 个 `xy_*` source 保持
  `experimental-test-only`，不再沿用审计基线中的 23 个旧计数或“尚未决定”措辞。
- Guard：`sensor_active_source_manifest` 同时检查 tracker 决策、审计计划当前计数和
  canonical owner 方向，避免完成事实再次漂回 pending。
- 边界：该收口仅为 Host/source-ownership 治理，不构成 Sensor 精度、时序、总线恢复、
  校准或 B1/B2 实板证据。
- 下一步：不新增 Sensor 型号；无硬件环境时进入 Sprint 5 reference RTOS/board 决策，
  硬件到位后按四个 canonical owner 补 B1/B2。

### 2026-08-29 Sprint 5 reference RTOS 选择

- RED：`reference_rtos_decision` probe 首次因决策记录缺失而失败，证明 Sprint 5 仍只有
  “FreeRTOS 或 RT-Thread”二选一的未决计划。
- 决策：选择 FreeRTOS 作为唯一 reference backend；RT-Thread 本 Sprint 不选。记录已核对
  adapter、kernel tree、CMake 路径，并明确当前缺 project-owned `FreeRTOSConfig.h`、
  STM32U5 Cortex-M33 port 与 runtime fixture。
- Guard：focused policy CTest 固定选择和 fail-closed 证据边界，同时要求默认 backend 在
  integration 通过前保持 bare-metal。
- 边界：`REFERENCE_SELECTED` 不构成 C1、RTOS runtime、ISR、并发或 B1/B2；S5-01 保持
  `IN_PROGRESS`。
- 下一步：最小 integration slice 为 config + Cortex-M33 port + adapter/kernel clean compile；
  再进入 thread/sync/queue/timeout/ISR-to-task runtime stress。

### 2026-08-29 Sprint 5 FreeRTOS STM32U5 compile 前置

- RED：`reference_rtos_decision` guard 在要求 project-owned `FreeRTOSConfig.h` 与 compile probe 后
  失败，确认选择记录尚无可执行 target integration gate。
- 实现：新增 bounded STM32U5 config；从 FreeRTOS-Kernel V10.4.6 commit
  `a4b28e35103d699edf074dfff4835921b481b301` 固定 Cortex-M33 non-secure GCC port，记录
  provenance/SHA-256；新增 compile-only CTest，使用 Arm GNU 15.2 对 adapter、kernel、heap、
  event/timer 与 port 共 9 个对象执行 `-Werror` 编译。
- 边界：该 gate 不链接 startup/vector/HAL，不运行 scheduler，故不构成 RTOS runtime、ISR、
  并发或 B1/B2 实板证据；S5-01 保持 `IN_PROGRESS`。
- 下一步：让 root Kconfig/CMake FreeRTOS selection 只消费受检 config/port，并在缺输入时
  fail closed；之后再建立 runtime fixture。

### 2026-08-29 Sprint 5 FreeRTOS root selection

- RED：显式 `KCONFIG_OVERRIDES=OSAL_BACKEND_FREERTOS=ON` 首先被 root parser 以 unknown symbol
  拒绝，证明 compile probe 尚未成为产品构建选择；policy guard 同时指出 root/third-party CMake
  没有映射受检 kernel/config/port。
- 实现：root Kconfig 新增默认关闭、仅 STM32U5 可选的 reference backend；root CMake 同步选择
  OSAL adapter 与 kernel，third-party target 使用 pinned V10.4.6 kernel layout、project-owned config
  和 Cortex-M33 non-secure port；PC 显式误选 fail-closed，默认 PC/bare-metal 不变。
- 验证：focused policy/compile 2/2；STM32U5 root configure + `xy_osal` target 构建成功（同时构建
  `freertos_kernel`）；Host 190/190、PC root build、`git diff --check` 通过。以上只构成 source/static-
  library compile 前置，不构成 runnable image、scheduler、ISR、并发或 B1/B2。
- 下一步：建立可链接/可运行的 FreeRTOS runtime fixture，再覆盖 thread/sync/queue/timeout 与
  ISR→task；S5-01 继续保持 `IN_PROGRESS`。

### 2026-08-29 Sprint 5 OSAL backend 声明收口

- RED：`reference_rtos_decision` guard 新增证据边界后失败，证明 stale
  `BACKEND_COMPARISON.md` 仍发布无 XinYi record 的 context-switch/IRQ latency、内存占用、
  “seamless migration”与 safety/use-case 排名。
- 收口：backend 文档改为 source/compile inventory，只记录 adapter mapping、当前 root selection、
  已知 unsupported 路径与 runtime gate；移除全部无来源性能数字和产品选择结论。
- 验证：focused `reference_rtos_decision` + `freertos_stm32u5_compile` 2/2、Host 全量、PC root
  与 `git diff --check` 见本轮 gate。以上不新增 runtime、performance、ISR、并发或实板证据。
- 下一步：继续 S5-01 的 runnable link/runtime fixture；若 QEMU Cortex-M33 启动模型不能安全闭环，
  保持 BLOCKED 并等待 reference board，不用 Host fake 冒充 scheduler runtime。

### 2026-08-30 Sprint 5 FreeRTOS README 声明收口

- Probe：继续尝试 MPS2-AN505 Cortex-M33 runtime fixture，但 secure/non-secure 启动与地址 alias
  边界触发 HardFault，未观察到 scheduler marker；该 spike 已移除，未注册成假绿 CTest。
- 收口：FreeRTOS backend README 不再宣称 “Complete/full multitasking”；kernel backend 表不再用
  `✅` 将 FreeRTOS/RT-Thread/CMSIS-RTX5 源码存在误写为 runtime 完成，统一记录 compile/source
  evidence 与 pending 边界，并由 `reference_rtos_decision` guard 防回归。
- 边界：现有证据仍只有 Arm STM32U5 source/static-library compile；无 runnable scheduler、ISR、
  并发、性能或实板升级，S5-01 保持 `IN_PROGRESS`。
- 下一步：选择与 reference board 安全域/启动布局一致的 linker/startup owner 后再恢复 runtime gate；
  在此之前继续清理同域 stale completion 声明，不用 QEMU boot failure 冒充 RTOS 缺陷。

### 2026-08-30 Sprint 5 OSAL 使用/状态文档声明收口

- RED：扩展 `reference_rtos_decision` policy probe 后，`OSAL README`、`QUICK_START` 与
  `IMPLEMENTATION_STATUS` 因缺 `runtime-pending` 边界并保留“只切 backend 源文件即可迁移”、
  “Complete implementation / 无需应用改动”等无运行时证据声明而失败。
- 收口：三份入口文档统一区分 bare-metal Host contract、FreeRTOS source/static-library compile
  与 RT-Thread/CMSIS source candidate；迁移步骤明确还需 Kconfig/CMake、kernel/config/port、
  startup/link 与 runtime 验证。
- 边界：该文档/policy slice 不新增 scheduler、ISR、并发、性能或实板证据；S5-01 保持
  `IN_PROGRESS`。
- 下一步：等待 reference board 的安全域/startup/link owner；到位前不再扩张无硬件 RTOS
  runtime fake，转向下一个可独立闭环的无硬件 Sprint 治理项。

### 2026-08-30 Sprint 5 公开组件入口声明收口

- RED：扩展 `reference_rtos_decision` policy probe 后，公开 `components/README.md`、
  `DEVELOPMENT_PRIORITY.md`、`docs/components/index.md` 与 OSAL introduction 仍把多 RTOS、
  FreeRTOS/RT-Thread/CMSIS-RTX 标为完整或统一通过，focused probe 按预期失败 12 项。
- 收口：公开入口统一为 Bare-metal Host contract、FreeRTOS
  `compile-guarded-runtime-pending`、RT-Thread/CMSIS-RTX source candidate；同时移除 FOTA
  “已完成”旧状态及 OSAL backend README 的模糊 `Complete` 标签。
- 验证：focused `reference_rtos_decision` + `freertos_stm32u5_compile` 2/2；full Host、PC root
  与 `git diff --check` 见本轮提交 gate。以上不新增 scheduler、ISR、并发、性能或实板证据。
- 下一步：S5-01 runtime 继续等待 reference board startup/link owner；下一无硬件 slice 优先校准
  同一公开 component index 中 HAL/FOTA 等仍高于证据台账的状态，不扩张 RTOS fake。

### 2026-08-30 Sprint 5 HAL/FOTA 公开状态校准

- RED：新增 `public_component_evidence` guard 后，公开 component index 因 HAL “完善”、
  STM32U5 “完整实现”与 FOTA “主线可用”等高于证据台账的措辞按预期失败。
- 收口：HAL 统一为 PC Host contract、部分 QEMU、目标实板 pending；FOTA 统一为 Host
  fail-closed contract，board Flash、bootloader、secure provider 与实板 pending；完成度比例明确
  不代表产品成熟度。
- 验证：focused `public_component_evidence` + `hal_platform_evidence_matrix` + `fota_metadata`
  3/3；Host 193/193；PC root build；`git diff --check`。以上不新增 HAL/FOTA runtime、实板、
  掉电恢复或安全批准。
- 下一步：S5-01 继续等待 board-correct startup/link owner；下一无硬件治理 slice 校准公开
  component index 中其余“完善”与静态测试计数；Net 产品选择门后续使用独立 ID，不与本项混用。

### 2026-08-30 Sprint 5 公开组件完成度/测试计数校准

- RED：扩展 `public_component_evidence` guard 后，公开 component index 中 CLib/Trace/PID/ADDC
  “完善”、Sensor “tail host coverage 已收口”、234 项静态测试总数和 81% maturity 比例按预期失败。
- 收口：全部公开组件行改为 Host contract + 明确 pending 证据；删除易漂移的静态分组件测试图和
  maturity 百分比，测试数量统一以 canonical CTest 实际发现结果为准，并由 policy guard 防回归。
- 验证：focused `public_component_evidence` 1/1、Host 193/193、PC root build 与
  `git diff --check` 通过。以上不新增硬件、安全、性能、并发或 production-ready 证据。
- 下一步：S5-01 继续等待 board-correct startup/link owner；下一无硬件 slice 进入独立的 Net
  产品选择门，先校准 AT ownership 与 MQTT/CAN/LTE Kconfig，不推进 modem runtime 假证据。

### 2026-08-30 Sprint 5 Net 产品选择门

- RED：新增 `net_product_selection` guard 后，root `NETWORK`/`PROTO_MQTT` 默认开启，AT/MQTT
  sources 仅按文件存在即进入 `xy_net`，CAN/LTE 无 root 选择项，且 MQTT 未由 umbrella 导出。
- 收口：Net core 与 MQTT、AT client/server、CAN、LTE 全部 default-off；root Kconfig 选择直接控制
  active source、公开 feature definitions 与 umbrella export。active owner 固定为 lightweight
  `at_client.c`/`xy_ats.c` 和 `src/xy_mqtt_client.c`，vendor AT trees 与 legacy MQTT 不进入 root target。
- 验证：focused policy/core/umbrella/MQTT/AT 6/6；全协议显式 opt-in 的 `xy_net` root target；
  Host 194/194；默认 PC root build；`git diff --check`。以上不构成 modem、CAN controller、
  flow-control、attach/PDP/URC、长稳或实板证据。
- 下一步：S5-01 runtime 继续等待 board-correct startup/link owner；Net 无明确 modem/board 产品输入时
  保持 default-off，不扩张 LTE Host fake。

### 2026-08-30 Sprint 5 Net README selection 事实同步

- RED：扩展 `net_product_selection` 后，README 因 CAN `Implemented`、AT-Command-V2 quick start、
  MQTT “尚未自动导出/待对齐 Kconfig”等与 S5-04 active selection 冲突的说明按预期失败。
- 收口：CAN 明确为 Host-guarded/hardware pending；quick start 改用 active `xy_at_client` owner；
  MQTT 配置与剩余工作同步为 root-selected umbrella export，并保留真实 transport/broker/长稳 pending。
- 验证：focused Net 5/5、Host 194/194、PC root 与 `git diff --check` 通过；不升级 modem、CAN controller、
  broker 长稳、性能或实板声明。

### 2026-08-30 Sprint 5 components README 事实校准

- RED：扩展 `public_component_evidence` 后，公开 `components/README.md` 因 SHT40/DHT11 等
  experimental/legacy source、SSD1306/BQ25620 及 PM/FOTA/Crypto 等组件仍使用无条件 `✅`，并建议
  应用绕过 root selection 直接 `add_subdirectory(components/drivers/sensor)` 而按预期失败。
- 收口：驱动清单改为 canonical owner、Host contract 与 Board pending 分层；功能组件状态与证据台账
  对齐；Kconfig/CMake 指南改为 root facts，禁止把目录/source 存在等同 product-linked 或硬件通过。
- 验证：focused `public_component_evidence`、Host 全量、PC root 与 `git diff --check` 见本轮 gate。
  以上不新增硬件、安全、性能或 production-ready 证据。
- 下一步：S5-01 runtime 继续等待 board-correct startup/link owner；下一无硬件 slice 校准仍指向
  不存在 `components/power/`、`drivers/power/charger` 的公开架构图，避免与已冻结 ownership 冲突。

### 2026-08-30 Sprint 5 Charger/Fuel Gauge 架构历史事实校准

- RED：扩展 `public_component_evidence` 后，`ARCHITECTURE_ANALYSIS.md` 与
  `REFACTORING_COMPLETED.md` 仍把不存在的 `components/drivers/power/charger/` 记录为已迁移 owner，
  focused probe 按预期失败 6 项。
- 收口：两份历史记录明确 BQ25620 canonical owner 为 `components/charger/src/xy_bq25620.c`，
  Fuel Gauge 继续 standalone；power-driver 迁移方案降级为未执行历史提案，不覆盖 root/证据台账。
- 验证：focused `public_component_evidence` 1/1、Host 194/194、PC root build 与
  `git diff --check` 通过。以上不新增 charger/Fuel Gauge 硬件、安全、热保护或 SMBus 证据。
- 下一步：S5-01 runtime 继续等待 board-correct startup/link owner；下一无硬件 slice 清理
  `ARCHITECTURE_REFACTORING_PLAN.md` 中同一未执行迁移命令，保持历史文档与 canonical owner 一致。

### 2026-08-30 Sprint 5 旧架构重组执行命令收口

- RED：扩展 `public_component_evidence` 后，`ARCHITECTURE_REFACTORING_PLAN.md` 因仍推荐“完全重构”、
  创建不存在的 power owner 并批量迁移 Charger/Fuel Gauge 而按预期失败 7 项。
- 收口：旧计划降级为 superseded 历史提案，删除可复制执行的批量 `mkdir`/`mv`/`rm` 脚本；固定
  BQ25620 canonical owner 为 `components/charger/src/xy_bq25620.c`，Fuel Gauge 保持 standalone，
  Sensor 后续迁移保持 Device model 与 compatibility boundary。
- 验证：focused `public_component_evidence` 1/1、Host 194/194、PC root build 与
  `git diff --check` 通过。以上不新增硬件、安全、性能或 production-ready 证据。
- 下一步：S5-01 runtime 继续等待 reference board startup/link owner；无硬件时下一 slice 校准
  `COMPONENT_GAP_ANALYSIS.md` 中已漂移的组件能力/成熟度声明，不扩张 legacy driver 或 RTOS fake。

### 2026-08-30 Sprint 5 组件差距清单事实校准

- RED：扩展 `public_component_evidence` 后，`COMPONENT_GAP_ANALYSIS.md` 因继续将 HAL/Device、
  DM/FOTA 与 GUI/Display 标为 100%，并保留“现有 96% 完成”等静态成熟度声明而按预期失败 12 项。
- 收口：该报告降级为历史候选能力清单；删除无证据完成度、组件数量预测和按生态凑齐功能的排期，
  明确 tracker、evidence matrix、root Kconfig/CMake 才是当前事实源，并增加候选进入 Sprint 的准入条件。
- 验证：focused `public_component_evidence` 1/1、Host 194/194、PC root build 与
  `git diff --check` 通过。以上不新增硬件、安全、性能、runtime 或 production-ready 证据。
- 下一步：S5-01 runtime 继续等待 reference board startup/link owner；下一无硬件治理 slice 检查
  `docs/build_system_analysis.md` 等历史报告中的无条件“完善”构建声明，不扩张组件功能。

### 2026-08-30 Sprint 5 历史构建系统报告事实校准

- RED：`public_component_evidence` 加入 build-report contract 后，旧报告因无条件 `✅ 完善`、
  “所有 CMakeLists 同结构/所有主要组件有构建配置”、8.5/10 静态评分及不存在的
  `make test-all`/`XY_COMPONENT_FEATURE_*` 命令按预期失败 11 项。
- 收口：`docs/build_system_analysis.md` 降级为 superseded 历史报告；canonical 命令改指 root
  Makefile/CMake/Kconfig 与 `AGENTS.md`，配置事实改指受检矩阵，并删除虚构模板、评分和另建 CI 的建议。
- 验证：focused `public_component_evidence` 1/1、Host 194/194、PC root build 与
  `git diff --check` 通过。以上仅为文档/构建事实治理，不新增 target runtime、实板、安全或 release 证据。
- 下一步：S5-01 继续等待 reference board startup/link owner；无硬件时优先建立
  `docs/release/release-checklist.md`，保持 Release Sprint 6 为 BLOCKED，不能把 checklist 当作 R1。

### 2026-08-31 Sprint 5 Release fail-closed checklist

- RED：新增 `release_readiness` policy probe 后因 `docs/release/release-checklist.md` 不存在而按预期失败。
- 收口：建立 release checklist，覆盖 tagged clean checkout、canonical Host/PC、supported target matrix、
  reference-board HIL/B2、Secure FOTA/security review、SBOM/license、可复现制品、checksum/signature 与最终批准记录；
  当前显式为 `BLOCKED` / `NO-GO`，unchecked item 均为 blocker。
- 验证：focused `release_readiness` 1/1、Host 195/195、PC root 与 `git diff --check` 通过。以上仅为
  release policy guard，不构成 R1、实板、安全、供应链或可复现制品证据。
- 阻塞：reference-board HIL、approved signature provider/bootloader 集成、SBOM、可复现制品与签名发布仍缺。
- 下一步：S5-01 等待 board-correct startup/link owner；无硬件时继续 supported examples/projects 的
  active/archive 清单与 clean-checkout release 输入边界，不以文档门禁替代执行证据。

### 2026-08-31 Sprint 5 Examples/projects release input 清单

- RED：新增 `release_input_inventory` focused guard 后因清单缺失按预期失败，证明 tracked
  examples/projects 尚未形成受检 release 输入边界。
- 收口：建立机器可验的 32 项 top-level 清单，区分 `host-guarded`、`compile-only`、
  `candidate-unverified` 与 `historical-unverified`；所有条目统一保持
  `excluded-pending-review`，避免 CTest、历史 build artifact 或 source 存在自动升级为 supported。
- 验证：focused policy + 四个既有 example CTest 5/5、Host 197/197、PC Release root build 与
  `git diff --check` 通过。以上不构成 clean-checkout target link、实板、release scope 或 R1。
- 阻塞：supported platform/project owner、clean target link、HIL、artifact SBOM 与 release approval
  仍缺；Sprint 6 保持 `BLOCKED`。
- 下一步：无硬件时对首个候选 entry 做 clean-checkout build 可复现性 probe；优先 canonical
  Host example 或 `projects/stm32u5_fota`，不得从 compile-only 直接升级 supported。

### 2026-08-31 Sprint 5 首个 release input clean-export gate

- RED：从 `git archive HEAD` 导出的 tracked source 直接配置 canonical `tests/unit` 时，因本地 ignored
  Crypto lightweight sources 不在 Git 快照中而失败，证明全量开发树不是首个 release input 的最小可复现边界。
- 实现：为 `examples/device_driver_template.c` 建立 tracked minimal CMake smoke；clean-export probe 在临时目录
  配置、构建并运行 `device_driver_template`，同时由 inventory guard 固定 source/target/CTest metadata。
- 边界：该 entry 继续保持 `excluded-pending-review`；clean export Host 通过不构成 supported release、target、
  hardware、security、artifact reproducibility 或 R1。
- 验证：focused `release_input_inventory`、`release_input_clean_checkout`、`device_driver_template` 3/3；
  full Host/PC root 与 `git diff --check` 见本轮提交 gate。
- 下一步：保持 S5-01 runtime/实板阻塞；下一无硬件 slice 审查第二个可独立导出的 Host example，或先处理
  ignored-but-required source 对 clean checkout canonical Host suite 的可复现性风险，不将单 entry gate 泛化为全仓可复现。

### 2026-08-31 Sprint 5 Clean export canonical Host source 收口

- RED：从 `git archive HEAD` 导出的 canonical `tests/unit` 配置因
  `components/crypto/xy_ascon`、`xy_tinyjambu`、`xy_photon_beetle` 被 `.gitignore` 排除而失败；
  本地 Host 绿灯依赖未跟踪源码，无法由仓库快照重建。
- 收口：移除三个 canonical LWC module 目录的 ignore 规则并纳入版本控制；不修改算法行为，
  其 product classification 继续保持 `test-only`、provenance/license `review-pending`。
- 验证：focused `crypto_lwc` 1/1；staged-tree clean export 独立 configure/build 后，排除三个会递归
  调用 `git archive HEAD` 的仓库状态 policy tests，其余 195/195 CTest 通过；常规 Host 198/198、
  PC Release root build 与 `git diff --check` 通过。
- 边界：该结果只关闭 tracked-source reproducibility blocker，不构成 LWC KAT 完整性、安全、
  side-channel、硬件、artifact reproducibility 或 R1。
- 下一步：提交看板校准并推送；随后审查剩余 ignored-but-required source，或选择第二个独立 Host
  release input，保持 S5-01 runtime/实板阻塞不被 Host gate 替代。

### 2026-08-31 Sprint 5 Canonical Host committed clean-export gate

- Probe：将上一 slice 的 staged-tree 手工验证固化为 canonical CTest；脚本从 committed `HEAD` 的
  `git archive` 在临时目录独立配置并构建完整 Host suite。
- 边界：排除 `supply_chain_inventory`、`release_input_inventory`、`release_input_clean_checkout` 与
  gate 自身四个依赖 Git 仓库状态或递归 archive 的 policy tests；其余 195/195 通过。
- 验证：focused clean export 195/195；full Host 199/199；PC Release root build 与
  `git diff --check` 通过。
- 状态：只构成 committed-source Host reproducibility gate，不构成 tagged clean checkout、PC/target
  artifact reproducibility、SBOM/license、硬件、安全或 R1 证据；Sprint 6 保持 `BLOCKED`。

### 2026-08-31 Sprint 5 Canonical CI clean-export 接入

- RED：workflow 文本 probe 证明 committed clean-export CTest 只隐式包含在全量 Host 命令中，缺少独立的
  canonical CI 步骤，无法从 CI 日志直接确认该高成本可复现性 gate 的执行边界。
- 实现：常规 Host 步骤显式排除 `canonical_host_clean_checkout`，新增独立必跑步骤按精确 CTest 名称执行；
  避免重复运行，同时保留失败即阻断的 canonical workflow 语义。
- 验证：workflow wiring probe 与 YAML parse 通过；focused 1/1（clean export 内部 195/195）；Host
  199/199；PC Release root build；`git diff --check`。
- 边界：该 CI wiring 只提升 committed Host source 可复现性可见性，不构成 tagged checkout、target
  artifact、SBOM/license、硬件、安全或 R1 证据；Sprint 6 保持 `BLOCKED`。
- 下一步：建立 tagged/clean-checkout artifact reproducibility 最小 probe；S5-01 runtime/实板继续等待
  board-correct startup/link owner。

### 2026-08-31 Sprint 5 PC artifact reproducibility 最小 gate

- RED：首个 probe 使用 root 默认配置时不存在 `xy_device` target，明确 artifact gate 必须固定
  `COMPONENT_DEVICE=ON` 与 PC Release 配置，不能依赖开发树缓存或默认组件集合。
- 实现：脚本从同一 committed `git archive HEAD` 提取两份独立 source tree，分别 configure/build
  `xy_device`，并要求 `libxy_device.a` SHA-256 与 byte size 完全一致；canonical CI 将该高成本 gate
  与常规 Host、committed clean-export 分步执行。
- 验证：focused `pc_artifact_reproducibility` 1/1；Host 200/200；PC Release root build；workflow YAML；
  `git diff --check`。
- 边界：只证明当前环境中单一 PC static library 的 committed-source byte reproducibility；不构成 tag、
  完整 PC/MCU artifact set、toolchain container、SBOM/license、签名、硬件、安全或 R1 证据。
- 下一步：固定 release build environment/toolchain identity，再扩展到选定 PC artifact manifest；实板与
  FreeRTOS runtime 继续保持阻塞。

### 2026-08-31 Sprint 5 PC release build environment identity

- RED：既有 artifact gate 仅输出 SHA-256/size，未记录构建工具 identity；canonical CI 仍使用会漂移的
  `ubuntu-latest`，无法把同一字节结果关联到明确 runner/tool 环境。
- 实现：新增机器可读 PC release build environment manifest，固定 PC/x86_64、Release、`xy_device`、
  artifact 路径、CMake options、并行度和 required tool commands；CI runner 固定为 `ubuntu-24.04`；
  reproducibility gate 每次输出实际 CMake、CC、AR、Python identity 并拒绝非 Linux/x86_64 host。
- 验证：focused `pc_artifact_reproducibility` 1/1；Host 200/200；PC Release root build；workflow YAML；
  `git diff --check`。
- 边界：这是 runner/tool identity 与单一 PC static-library gate，不是 OCI/container digest、完整系统包/
  transitive dependency lock、tagged checkout、target/hardware/security 或 R1 证据。
- 下一步：建立选定 PC artifact manifest 并明确 artifact set；完整 release environment 仍需 immutable
  container/toolchain digest 与 SBOM/license 审查。

### 2026-09-01 Sprint 5 PC artifact ephemeral signature gate

- RED：`release_readiness` 将归档文件集扩展为 signature/public key，并要求 Ed25519 ephemeral-key
  边界后按预期失败 2 项，证明此前只有 checksum、没有签名管道。
- 实现：reproducibility helper 新增独立 sign mode；CI 生成每轮临时 Ed25519 key，归档 signature 与
  public key、丢弃 private key，再以独立 verify mode 同时重读 checksum 与 signature。
- 验证：focused `release_readiness` + `pc_artifact_reproducibility` 2/2；真实双构建 artifact 后签名/
  验签成功；篡改 artifact 与 signature 均 fail-closed；Host 200/200、PC Release root、workflow YAML 与
  `git diff --check` 通过。
- 边界：ephemeral key 只证明 CI 签名/验证管道和篡改检测；它没有稳定 release identity、key custody
  或 publication authority，不满足 release checklist 的 signed-publication 门禁，Sprint 6/R1 保持阻塞。
- 下一步：无硬件时建立 release-owned signing identity/key-custody 决策记录，或推进 artifact SBOM/license
  review；不得把临时 CI key 升级为 release signing key。

### 2026-09-01 Sprint 5 Release signing identity/key-custody 决策门

- RED：`release_readiness` 在要求机器可读 signing policy 后因文件缺失按预期失败，证明 ephemeral
  CI key 之外没有 release identity/custody 事实源。
- 收口：新增 fail-closed signing policy，固定 Ed25519 设计边界；release identity 仍为
  `UNASSIGNED`，key custody 为 `NOT_ESTABLISHED`，signed publication 为 `BLOCKED`。在命名 release
  owner/key custodian、受保护外部 signer、recovery/revocation 与独立验签记录到位前，不创建或使用
  release-owned private key。
- 验证：focused `release_readiness`、Host 全量、PC Release root build 与 `git diff --check` 见本轮
  gate。该决策门不构成 key provisioning、签名发布、安全审查或 R1。
- 下一步：推进 artifact SBOM/license review；release key 实施继续等待 owner/custodian 决策。

### 2026-09-04 Sprint 0 远端备份退出条件复核

- 校准：S0-02 与对应退出条件继续保持 `DONE`；初始同步提交为 `02a9be45`，不是建档基线 `9cea83f0`。
- 验证：`git fetch origin main` 后 `git rev-list --left-right --count HEAD...origin/main` 返回 `0 0`；`git rev-parse HEAD origin/main` 与 `git ls-remote origin refs/heads/main` 均返回 `a000ca66cc141ef8d47836769cde356b03a712ef`。
- 仓库：复核开始时 `main` 工作树干净；本次仅校准 Sprint 看板，不新增 Host、target、硬件、安全或 Release 证据。
- 下一步：继续 S5-01 的 IPC/Trace/Device/PM 跨组件并发最小纵切；多小时耐久与 STM32U5 runtime 仍保持 pending。
