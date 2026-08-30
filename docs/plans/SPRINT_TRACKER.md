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
| S2-04 | P1 | SYS reset/bootreason/chip-ID strong backend | BLOCKED | Zero | 参考板/board ownership 决策 | Host fail-closed 子项完成：默认 reset/reboot-reason/chip-ID/MAC/version 不再返回伪成功，focused `sys_core`、Host 183/183、PC root build 与 `git diff --check` 通过；strong backend 与上电/软件/看门狗复位 B1/B2 仍缺参考板决策和实板 | `648b1a31` | 2026-08-26 |

### 后续 Sprint 队列

### Sprint 3 前置看板

> Sprint 2 的实板工作继续保持阻塞时，只推进不冒充安全批准的 Crypto/FOTA 边界。
> `production-candidate` 仅表示下一步重建/审查候选；provenance、side-channel、硬件与产品批准仍须独立证据。

| ID | 优先级 | 工作项 | 状态 | 负责人 | 依赖 | 验收/证据 | 分支/提交 | 更新时间 |
|---|---:|---|---|---|---|---|---|---|
| S3-01 | P0 | Crypto 产品算法清单 | DONE | Zero | Sprint 0 证据边界（DONE） | 11 个算法区域已记录 product classification、implementation owner、source origin、license status、side-channel target、allowed usage、runtime/focused sources 与 review record；policy RED 后 focused 5/5、Host 183/183、PC root build、`git diff --check` 通过；SM2/ECDSA 强制 `security-rejected`，无安全批准升级 | `fdce5449` | 2026-08-26 |
| S3-02 | P0 | Signature provider 边界与 Secure FOTA fail-closed | DONE | Zero | S3-01（DONE） | Secure FOTA 不再调用 format-only ECDSA placeholder；缺 provider、provider 拒绝（含错误 key ID）、回滚版本及截断包均 fail-closed；focused 1/1、Host 184/184、PC root/FOTA target build、`git diff --check` 通过；不升级安全批准 | `12cdb5f6` | 2026-08-26 |
| S3-03 | P0 | SHA-256/HMAC 单算法重建试点 | DONE | Zero | S3-01（DONE） | RED 证明 zero-length `NULL` 输入被错误拒绝；实现后 SHA-256/HMAC focused 2/2、Host 184/184、PC root 与 Crypto-enabled `xy_tiny_crypto` target build、`git diff --check` 通过；补充 SHA-256 context 与 HMAC working-key/pad volatile clearing。仍缺 provenance、独立审计、target compile、side-channel 与硬件证据 | `dc47807c` | 2026-08-26 |
| S3-04 | P0 | FOTA 状态机去模拟化与 bootloader contract | IN_PROGRESS | Zero | S3-02（DONE） | boot handoff/delta/mark-valid 均 fail-closed；双副本 metadata journal 持久化 generation+CRC+commit marker、pending version/slot 与 boot-attempt count；handoff/confirm/boot-attempt callback 均执行 load→状态转换→commit，boot-attempt 只有持久化成功才向 bootloader 返回 rollback 决策，提交失败不推进 durable count；已有 pending candidate 时重复 handoff/stage 返回 `XY_FOTA_IN_PROGRESS` 且不重置 attempt count，避免重复请求绕过 bounded rollback；饱和/损坏的 `uint8_t` attempt count 不再 wrap 后错误继续启动；journal 扫描遇到任一 Flash read 错误即 fail-closed，不把 unreadable copy 当作 empty/corrupt 后继续覆盖；两份有效副本 generation 半范围歧义或相同 generation 却 payload 冲突时均拒绝选择，不依赖 slot 顺序静默提升；提交后的 read-back 失败会擦除目标副本；commit/load 及公开 stage/attempt/confirm 状态转换均校验 slot/flags/pending、`active_version >= min_version`，非法内存状态不被推进；backend 地址范围也在任何 Flash I/O 前校验，避免双槽地址计算发生 `uint32_t` wrap；STM32U5 skeleton 在 callback 注册前验证 backend，默认未绑定 Flash ops 时 fail-closed。FOTA focused 4/4、Host 185/185、PC root、FOTA-enabled `xy_fota` target、Arm M33 syntax probe、`git diff --check` 通过。尚缺 board-owned Flash ops/保留区、可链接 STM32U5 image、bootloader 实际调用与实板 | `54d8b735`～`b832a7ae` | 2026-08-28 |
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

| Sprint | 周期 | 目标 | 进入条件 | 当前状态 |
|---|---:|---|---|---|
| Sprint 1 | 2 周 | GUI backend 错误传播、strict backend、字体与单一显示纵切 | Sprint 0 门禁可信 | IN_PROGRESS |
| Sprint 2 | 2 周 | STM32U5 HAL→Device→Driver 最小实板证据链 | [HAL 平台实现与证据矩阵](../validation/hal-platform-evidence-matrix.md)已建立；Host 前置推进中，HIL 夹具仍缺 | IN_PROGRESS（Host 前置）/BLOCKED（实板） |
| Sprint 3 | 2 周 | Crypto 产品级重建 Phase 1；Secure FOTA fail-closed | 产品算法清单与 signature provider 边界已完成 | IN_PROGRESS（FOTA 去模拟化；非安全批准前置） |
| Sprint 4 | 2 周 | Sensor 三轨收敛、DM 掉电测试、Fuel Gauge 实板 | active-source manifest 与 SHT30/ADS1115/MPU6050 single-owner migrations 已完成 | IN_PROGRESS（Sensor ownership 前置）/BLOCKED（实板） |
| Sprint 5 | 2 周 | 单一 RTOS 并发验证；Net/PM 按产品需求推进 | FreeRTOS 已选为 reference；board/config/port/runtime 仍待闭环 | IN_PROGRESS（reference selected）/BLOCKED（runtime/实板） |
| Sprint 6 | 1–2 周 | Release Candidate | 目标平台 HIL、安全边界和发布门禁达标 | BLOCKED |

---

## 5. 决策与阻塞日志

| 日期 | ID | 类型 | 内容 | 所需决策/解除条件 | 状态 |
|---|---|---|---|---|---|
| 2026-08-17 | D-001 | 决策 | Sensor 实际有 legacy/new/drivers 三条实现路径 | 2026-08-29 canonical API 已确定为 Device model；SHT30/ADS1115/MPU6050/BMP280 四个 owner 已进入 root target，legacy 仅保留明确 compatibility boundary，experimental 保持 test-only | CLOSED |
| 2026-08-17 | D-002 | 安全阻塞 | Fuel Gauge security AES 曾存在明文透传风险 | 2026-08-29 已改为缺 provider 时 fail-closed；真实认证/加密仍须受审查 provider | CLOSED |
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
