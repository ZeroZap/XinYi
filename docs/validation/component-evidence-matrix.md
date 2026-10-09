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
| HAL / Storage | H1（PC；canonical W25Q128 command/error/page-boundary、FOTA adapter、RTC backup register 与 SYS IRQ control contract） | Q1 部分；通用 QSPI/RTC backup HAL API、STM32L4 QSPI/I2C/UART/internal-Flash/RTC/SYS init/deinit/IRQ backends、canonical W25Q128 Device driver 与 FOTA flash-ops adapter；PC/L4/U5 compile；WCH CH32V307 GPIO/UART/SPI/I2C/DMA/ADC/PWM/watchdog current-contract backends 与 board compile smokes；Pandora board entries 的 system init、UART/GPIO、hardware-I2C3、FOTA internal-Flash、FOTA boot jump SYS deinit、RTOS/FOTA RTC backup 与 RTOS NVIC owners 已通过 canonical HAL policy/clean-build gate；递归 policy 只允许 `pandora_platform_startup.c` 直接持有三个精确 vendor clock calls，其他 board source 必须走 `xy_hal_*` | Pandora U9 W25Q128 的 JEDEC、erase、single/quad program/read、recovery 已迁移为 canonical path；hardware I2C3 transaction owner 已迁移到 STM32L4 backend；SPI1 TX DMA 已取得 completion、re-init 与 active-abort recovery B1/B2。openCH 当前仅 UART1 TX 有 bounded B1，其余新增 WCH HAL 均为 compile baseline。2026-09-06 无 RX source 实板运行按预期停在 `PANDORA_SPI_DMA_RX_ERROR`，未取得 RX/full-duplex 证据 | pending | 允许声明 Pandora canonical storage/I2C3 与 SPI TX DMA B1/B2；WCH watchdog 等新 backend 只允许声明 compile compatibility；`pandora_platform_startup.c` 是唯一 board-local vendor clock owner，精确白名单为 `HAL_PWREx_ControlVoltageScaling`、`HAL_RCC_OscConfig`、`HAL_RCC_ClockConfig`；不得将未接 RX source 的 timeout 升级为 RX/full-duplex、电气或外设响应证据 | openCH 分外设下载/UART/仪器验证；Pandora 使用已确认的物理 loopback/外设补 SPI RX/full-duplex DMA；掉电/NRST恢复 |
| Device | H1 | PC/L4/U5 build | Pandora software-I2C→HAL→Device→AHT10/AP3216C B1；AP3216C continuous `0x03` 有 244-sample bounded IR response B1。hardware I2C3（PC0/PC1）正常路径 B1 后，clean `b1c2429c` 对无人地址 `0x7F` 得到 HAL I/O failure并恢复访问 `0x1E`，取得配置回读、59 samples/27 unique、error 0 与 Flash byte-identical 回读 | n/a | registry/lifecycle、I2C helper、Pandora hardware-I2C3→Device→AP3216C 正常路径 B1及单次无人地址 NACK→后续设备访问 B2；不等于总线物理故障、stuck-bus recovery、定量 ALS/PS 或完整 I2C HAL 资格 | 如需产品阈值则补受控距离/照度夹具；stuck-low/掉线须人工或安全故障夹具 |
| Display drivers | H1（SSD1306 init/refresh error propagation、GUI adapter；LCD/LED transaction contracts；ST7789 checked fill 使用 256-byte bounded buffer并传播 SPI error；RGB/BGR MADCTL 与 runtime rotation atomic-commit contract；RGB Matrix effect cadence 使用 canonical OSAL tick 与真实 elapsed interval） | PC/L4/U5 build；Pandora onboard ST7789 SPI3 board target 已链接 | 旧 `e125038e` visual B1 因 blue→green→red 顺序而撤回。2026-09-08 clean `7b07057d` 修复镜像经 write/verify/read-back byte-identical，且 Eugene 现场确认 red→green→blue→white→black 与最终象限图案，恢复固定面板/方向/图案 visual B1。2026-09-30 clean `8716d264` 13,508-byte image 经 write/verify/read-back byte-identical；WCH-Link UART 417 bytes、exact identity、90→180→270→0 rotation marker 与 final pattern 严格有序、error 0，证明四次 MADCTL checked write 被 panel path 接受并继续运行；无人值守未取得实际可见方向。SSD1306 deferred | performance pending | Pandora onboard ST7789 SPI3 固定初始方向/颜色顺序/象限图案 visual B1；四个 runtime rotation 仅有 machine-observed command-chain B1，不等于可见方向/坐标映射；RGB Matrix cadence 仅为 Host/compile ownership contract；不等于色准、亮度、刷新率、其他面板或长期可靠性 | 人工确认四方向视觉与坐标映射；SPI/DMA/刷新率及 long-run |
| Storage/24xx | H1（page split、尾页容量/16-bit 边界、I2C error propagation、ACK write-cycle polling、re-init recovery 与 failed re-init live-owner atomicity） | PC/L4/U5 build | pending | pending | fake-I2C Device→Driver 契约；page write 同时受 page 与 configured capacity 限制，写后以 bounded device-ready polling 等待内部 write cycle，timeout 时停止后续 page 且不假成功；重复初始化仅在完整 nested transport 成功后原子替换 owner | 写保护、真实 write-cycle 时序、掉电 B2 |
| Storage/SD SPI | H1（SDHC/SDSC init 与 CSD capacity、OCR power-up/2.7–3.6V compatibility、CCS/CSD identity consistency、SDSC CMD16(512) normalization、block/byte addressing、single-block read/write、verified-write、fail-closed teardown 与 transport-error propagation） | PC/L4/U5 build；Pandora SPI1/PC3 独立 probe image 可链接 | clean committed `1aa7e3e2` 10528-byte image 已 ST-Link write/verify/read-back byte-identical（SHA-256 `dc7f7d7b...dc12`）；reset-synchronized WCH-Link capture 203 bytes，exact identity、SDHC、62521344 blocks/32010928128 bytes、block0 `55AA`、strict marker order、error 0，validator 返回 `B1_PANDORA_TF_READ_PASS` | pending | callback-injected Host driver contract；Pandora 当前插入 TF 卡的 SPI1/PC3 初始化与 block0 read-only B1；`55AA` 仅为观测字节，不证明有效分区表；SDSC/verified-write/deinit 仍仅有 Host/compile evidence | 写入前须定义非破坏测试区与恢复边界；真实 SDSC、write-readback、文件系统、热插拔、性能、耐久与掉电 B2 pending |
| Sensor legacy | H1（SHT30/BH1750/MPU6050/BMP280 已降为 canonical Device owners 的 compatibility wrappers；APDS9960 顶层弱重复 owner 已移除并由 top-level/subdir overlap guard 防回归；SGP30/SGP40/ENS160/IM69D/MAX30102/AEAT-8800/MLX90393 false owners、协议不匹配的 IIS2ICLP/QMA6100、无补偿 BMP390/无 calibration INA219、无来源 DMP6100/CMS/HS-ADS1100/GD30DF owners 与浅层 VL53L1X owner 已移除；AS5048 owner 已限定为 I2C AS5048B 并按 `0xFE/0xFF` datasheet bit layout 解码；含 CCS811 factory/public ops 空总线拒绝、SHT40 序列号初始化失败原子清理、LPS22HB deinit/初始化失败传播与 Coulomb Device-helper 初始化失败传播、无寄存器副作用；LSM6DSL range setter NULL/非法枚举 fail-closed） | PC/L4/U5 build；[active-source manifest](sensor-active-source-manifest.md) 记录 40 个 root sources（39 个 `sensors/` owner + top-level ADT7420） | Pandora ICM20608 已取得在线/配置/静态采样诊断 B1；AP3216C 旧 `0x07` one-shot capture 仅诊断，修为 continuous `0x03` 后取得 static B1，并以 244 samples/139 unique/error 0、IR median 2.5→257.0 的 bounded stimulus-response B1；V2.4 原理图明确 U8=AP3216C | pending | AP3216C 芯片/寄存器/freshness 与 bounded IR response 已建立；ALS 10–11、PS 0–15，不能声明定量 ALS/PS 响应、距离阈值、精度或 calibration；ICM20608 两次动态 capture 未观察到预期响应 | AP3216C/AHT10 NACK recovery；需要产品阈值时补固定距离/照度夹具；ICM20608 动态诊断保持隔离 |
| Sensor new `xy_*` | H1（独立测试） | [manifest](sensor-active-source-manifest.md) 记录 5 个 `experimental-test-only` sources，未进入根 Sensor target；SHT30/ADS1115/MPU6050/BH1750/AHT20/SHT40/HDC1080/TSL2561/MLX90614/LPS22HB/BMI088/BNO055/BMI270 duplicate test-local implementations 已移除或迁出；BQ25620 experimental Sensor owner 及对应未引用 prototype 已删除并由 guard 禁止回归 | pending | pending | 仅 test-local Host 实验实现；不得因 focused test 宣称 product-linked；BQ25620 canonical charger owner 与 MAX17043 product-facing owner 分别保留在 standalone Charger/Fuel Gauge 组件，Pandora 无 charger IC；ADXL362/BME280/ICM20608 继续由 root-linked legacy owner 持有 | 继续冻结新增并迁移高价值 owner |
|| Drivers Sensor | H1（51 个 canonical Device sources；QMA6100P 已以 nested Device I2C owner 接入，固定地址 `0x12/0x13`、chip ID `0x90` 与 14-bit XYZ contract；Pandora active-low capture machine gate 要求 data-ready interrupt 配置寄存器 exact readback `ENABLE1/MAP1/MAP3/PIN_CONFIG/INT_CONFIG=10/10/10/00/0C`。2026-09-27 clean `142e515c` image 经烧录/同长度 byte-identical 回读后，WCH-Link capture 5,449 bytes；software-trigger delta 为 2/2，1 秒真实窗口 PC6/PD15 edge 为 262/263，40/40 sample unique且 sensor status 出现 `0x10`，因此固定 Pandora/QMA6100P 配置下 data-ready GPIO edge→EXTI callback 已取得 B1；动态响应、精度、校准、恢复与 endurance 仍 pending；INA228 I2C 与 INA229 SPI 共享 transport-neutral INA22x conversion core，ENERGY/CHARGE 仅接受 continuous bus+shunt ADC mode，诊断溢出/内存状态异常时拒绝整份 sample，首次 init 失败清零 owner、live re-init 失败保持原 transport/lifecycle，支持六类 raw alert threshold 与 staged diagnostic read，并分别保持独立 Device transport owner；LIS2DW12 已以 nested Device I2C owner提升；CCS811 已以 nested Device I2C owner提升；APDS9960 已以 nested Device I2C owner提升；LSM9DS1 已以双 nested Device I2C owner提升；LSM6DSR 已以 nested Device I2C owner提升；LSM6DSL 已以 nested Device I2C owner提升；LSM6DSO 已以 nested Device I2C owner提升；ADXL362 已以 nested Device SPI owner提升；KX023 已以 nested Device I2C owner提升；BMA400 已以 nested Device I2C owner提升；IST8310 已以 nested Device I2C owner提升；AK09918 已以 nested Device I2C owner提升；QMC5883L 已以 nested Device I2C owner提升；VCNL4040 已以 nested Device I2C owner提升；MAX44009 已以 nested Device I2C owner提升；AS5600 已以 nested Device I2C owner提升；AS5048B 已以 nested Device I2C owner提升；BMI088/BMI270/BNO055 已从 experimental 提升为 root-linked typed owner；LTC2945 已按 Analog Devices register/scaling contract 重建；SGP40 已以 nested Device I2C owner提升；LPS22HB 已迁入 root owner并保留既有 focused contract，Device-helper transport hardening pending；VL53L1X 已以 nested Device I2C owner提升；INA226 已按显式 shunt/current-LSB/calibration/current/power contract 重建并提升；HDC1080/TSL2561/MLX90614 已从 experimental 提升为 root-linked typed owners；MLX90614 保留 PEC 温度/发射率读取及 unsupported EEPROM-write 边界，并新增 fixed-address/nested-lifecycle/deinit contract；AHT10/AP3216C/ICM20608 已迁移为单一 typed owner + legacy compatibility wrapper；INA219 已按显式 shunt/current-LSB/calibration contract 重建；BMP390 使用 pinned Bosch BMP3 SensorAPI 完成 NVM calibration 与温度优先压力补偿；INA219/BMP390/HDC1080/TSL2561/MLX90614/LPS22HB/BMI088/BNO055/BMI270/AK09918/LSM9DS1/LIS2DW12 保持 hardware-pending；AHT20 已迁移为单一 typed owner + legacy compatibility wrappers，SHT40 typed owner 已迁出实验树；BME680 使用 pinned Bosch BME68x API） | PC/L4/U5 build；[manifest](sensor-active-source-manifest.md) 记录 51 个 `device-active-root` sources，均进入 `xy_drivers`；Host suite；Pandora QMA6100P `0x12`/ID `0x90`、40 个静态 sample 与 PC6/PD15 data-ready EXTI edge 已取得 bounded B1；动态响应、精度、校准、recovery 与 endurance pending |
| Actuator | H1（framework + GPIO-backed buzzer/RGB/H-bridge motor fail-safe contracts） | PC/L4/U5 build | Pandora PB2 buzzer 与 PE7/PE8/PE9 RGB 已有 control-path evidence；V2.4 原理图确认 PA1=`MOTOR_A`→IA、PA0=`MOTOR_B`→IB。motor `7c779ce4` 7420-byte image 已 write/verify/read-back byte-identical，SHA-256=`c53d349ca30ebc5bb8f5a346185ab87d546f323a77aa6f48741c2a97d97bddfe`；UART 201 bytes 通过 exact identity、forward short-short-long、最终 PA1/PA0=L/L 与 error 0 machine gate；Eugene 现场确认对应振动且最终完全停止 | safety pending | TC214B 型号及 truth table 来自用户提供资料，未独立抓取 datasheet；motor API 固定 L/L standby、方向切换先回 L/L、单步最长 5 秒；仅授予 fixed forward short-short-long 与 standby stop B1，不包含 reverse、H/H brake、PWM、current、性能或 endurance | PWM/current/重复启停耐久；reverse/brake 仅在另行定义安全夹具后验证 |
| Fuel Gauge | H1（未实现安全模式 fail-closed） | PC build | pending | AES/SHA provider pending；plaintext passthrough 已移除 | Host 驱动契约；`NONE` 明文兼容，未接入 provider 的安全模式返回 unsupported 且保持输出 | 受审查 authentication/encryption provider；SMBus B1/B2 |
| Charger | H1（standalone BQ25620 fake-I2C transaction/status contract；公开 full config 与 direct setters 在首个 I2C 写前拒绝越界或无法按寄存器步长精确表示的 input/charge/precharge/termination current、charge voltage 与 recharge threshold；16-bit little-endian setpoint paths 使用 read-modify-write 保留非目标位并传播 read failure；status/fault 依据 ZHCSMZ9C Rev. C `0x1D/0x1E/0x1F` staged decode，safety-timer expiry 映射公开 charge-timeout fault，top-off timer 编码保持 active charging 而不误报 charge done，歧义 `TS_STAT=001` fail-closed 为 unknown，ICHG/VREG/IINDPM decoded setpoints 同时拒绝低于 datasheet 最小值或高于最大值的编码；public register read 仅允许 datasheet `0x02..0x38` address range） | PC/L4/U5 build | pending | safety pending | `components/drivers/power/charger/xy_bq25620.c` 为 canonical owner；公开 API 与 focused contract 迁移后旧 `components/charger/` compatibility component 及 ownerless Device dispatch 已移除；固定 7-bit 地址 `0x6B`，ICHG/VREG/IINDPM/IPRECHG/ITERM/VRECHG 地址、位域、步长与范围已按 TI ZHCSMZ9C Rev. C 重建，fake transport 对每次 TX/RX 校验目标地址；lifecycle 仅由 canonical `xy_device_t.initialized` 与 owner cookie 持有，live-transport fail-closed、完整 status staged commit、所有 init 失败保持 caller storage、live owner re-init 失败保持原 transport/lifecycle；teardown 的 stop-charge read/write 失败传播且保留完整 live owner供重试，成功后才清理 context | 充电/热故障 B1/B2；Pandora 无 charger IC |
| Analog Devices | H1 | PC build | pending | calibration pending | active 3-source Host 契约 | MCP3008/HX711 实测与标定 |
| MUX | H1 | PC build | pending | protocol security pending | Host typed ops 可用 | Device adapter/真实跨接口验证 |
| PID | H1 | PC build | pending | performance pending | Host 算法可用 | plant simulation/HIL、抖动和饱和恢复 |
| Trace | H1 | PC build | Pandora CLIB printf sink 已在 IPC consumer task 中完成 17 次有序输出 B1 | throughput pending | Host weak sink/format contract；Pandora 仅证明 bounded UART sink 跨组件正常路径 | RTT/ITM、并发丢日志与吞吐策略 |
|| IPC | H1（MQ bounded waits 在 elapsed tick 达到 timeout 时精确返回，不额外等待一个 tick；Broker request response 必须匹配 sequence 与 source/destination generation） | Q1 间接/部分 | Pandora Broker producer→consumer dispatch、单调 payload、Device lookup 与 Trace sink 已完成 bounded B1；depth-2 server queue 已取得 fill→`QUEUE_FULL`→clear→后续 pipeline 恢复 B2；handler rejection 现会向调用者传播并计入 dropped，随后消息可恢复处理；TIM6 single-producer ISR ingress 已取得 12 分钟内 12 次 queue-full→192 条严格单调 delivery→recovery、task producer 非饥饿及 1387 个 IPC→PM→Device→Trace→IPC 完整周期 B2 | concurrency pending | Host 契约 + Pandora bounded task-context、queue saturation、handler rejection、generation-stamped response correlation 与 single-TIM6 ISR ingress 12-cycle stress/recovery；request/response hardening 仅为 Host/compile contract；不等于性能吞吐、多 ISR producer、任意 ISR source 或多小时资格 | 多 ISR producer/多小时 endurance stress |
| SYS | H1（timer/SM；默认系统 API fail-closed；RTC backup bounds/read-write） | PC/L4/U5 build；dedicated STM32L4 `xy_hal_sys`/`xy_hal_rtc`/`xy_hal_wdg` backends 与 Pandora board policy 已守护 reset-reason/UID/software-reset/backup-register/IWDG owner 边界 | Pandora strong backend 已取得稳定 96-bit UID B1；自动 software reset、IWDG timeout 与 ST-Link reset command 均取得 reason/recovery B2，且有 Flash write/verify/read-back 与 UART 证据 | n/a | timer/state-machine Host 契约；无 board backend 时 fail-closed；Pandora application/FOTA reset flow 使用 canonical `xy_hal_sys_*`、`xy_hal_rtc_backup_*` 与 `xy_hal_iwdg_*`，vendor RCC/UID/CMSIS reset/RTC backup/IWDG 调用仅位于 STM32L4 backend；既有 chip identity、软件复位、看门狗复位与 ST-Link external-pin reset reason 已实证 | power-loss/brownout 区分与 watchdog refresh policy 仍 pending；U5 仅补充 compile |
| DM | H1（8 目标；FS lifecycle/path/I/O/error contract，coreJSON parser/search，active `xy_json` parse/mutation/malformed-input contract；NVM newest-complete/restart/torn-append、header/payload partial-write、format partial-erase、metadata/checksum corruption recovery 与 legacy→current layout migration） | PC build | [DM 掉电记录](xinyi-dm-power-loss-validation-record.md)当前为 `HOST_INTERRUPTION_GUARDED`，真实 Flash/board pending | durability pending | Host 数据格式、FS abstraction、active JSON 与 NVM restart/corruption 契约；header/payload exhaustive byte-boundary partial-write 与 256-byte format partial-erase sweep 后可重启并重试；legacy additive checksum/magic 可读，current record 使用 CRC-8 并对 metadata/payload corruption 回退上一完整值；caller-owned storage ops 允许非映射逻辑地址 backend | 目标 Flash program-granule/erase 注入、真实 legacy image dump 与真实 Flash B2 |
| PM | H1（umbrella 公开并由 Host 编译消费完整 lifecycle/update、mode dispatch、charger state/control、Fuel Gauge state/SOH、ADC sample/helper API；PM 初始化逐项传播 ADC/charger/Fuel Gauge 错误并仅在全部成功后提交状态；周期更新传播 Fuel Gauge/charger state/自动 start-stop 错误并仅在完整成功后提交 snapshot/timestamp；PM-local Fuel Gauge reset 同步恢复 full-capacity 积分/tick 基线，以 64-bit `mA·ms` 保留亚秒更新积分，并在窄化到公开 `uint8_t` 前 clamp coulomb SOC；charger 与 PM system/Fuel Gauge 统一消费 platform-owned `xy_pm_tick_get()`，不再绕过 OSAL/HAL adapter；charger 无 board backend及 hardware init/enable/disable 失败时不提交软件成功状态，live teardown disable 失败时保留 charger/PM 状态供重试，重复 init 幂等保持 live state且不重复触发 hardware init；PM charging intent 仅在 hardware action 成功后原子提交；sleep lifecycle/state/error contract；deep-sleep/shutdown 无 backend时 fail-closed；dedicated UART evidence validator 要求 exact firmware identity、32 轮 sleep→IRQ wake→state recovery 顺序及零 error marker） | PC/L4/U5 build；STM32L4 shallow SLEEP/WFI 与 PC/U5 compatibility backend 已接入 | Pandora runtime 已证明 PM platform tick adapter 经 `XY_OSAL_AVAILABLE` 使用 canonical OSAL tick；2026-09-08 clean `278cabe8` 38828-byte image 保留 handler/PM symbols，ST-Link write/verify/read-back byte-identical（BIN/read-back SHA-256 `1bb2afff...6293`）；20 秒 WCH-Link capture 8845 bytes，最新完整 boot 严格完成 32 轮 `SLEEP_ENTER→WAKE_IRQ→SLEEP_WAKE_OK→CYCLE n`、repeat marker 一次、零 PM error，validator 返回 `B1_PM_SLEEP_REPEAT_PASS` | power pending | Host framework/fallback；public ACTIVE/SLEEP dispatch 委托受检状态转换；charger API 在无 backend 或硬件动作失败时 fail-closed，PM enabled intent 与 teardown live state 保留上一稳定状态，state/status alias 仍返回 canonical snapshot；public ADC sample API 提供 NULL guard 与毫伏输出；DEEP_SLEEP/SHUTDOWN 无 backend时明确 unsupported；Pandora 已证明 bounded 32-cycle shallow SLEEP/WFI + tick IRQ wake B1，但不等于 STOP/STANDBY/SHUTDOWN、功耗、wake latency、多小时耐久、charger/ADC 实板或 U5 runtime | 补电流、低功耗深度、wake latency 与多小时 endurance；charger/ADC 仅在产品硬件接入后验证 |
| Crypto | H1（契约；SHA-256/HMAC zero-length 与工作状态清理；LWC 仅 focused-test） | PC/L4/U5 Crypto compile | Pandora STM32L475VE 软件路径已完成 SHA-256 `abc`、HMAC-SHA256 quick-brown-fox、AES-128 FIPS-197 单块 encrypt/decrypt KAT 与 1000 次重复运行；clean `35beb5bb` image 10724 bytes，ST-Link write/verify/read-back byte-identical（SHA-256 `3b54b73e...ec7d`），reset-synchronized WCH-Link capture 232 bytes、零 error marker，validator 返回 `B1_CRYPTO_SOFTWARE_KAT_PASS` | product-classification/owner/origin/license/side-channel/allowed-use 清单已机器守护；provenance 均 review-pending；SM2/ECDSA rejected；Ascon/TinyJAMBU/Photon-Beetle 因 provenance/KAT 未闭环而从 root runtime 隔离 | contract-guarded；SHA-256/HMAC/AES 软件实现具有限定 L4 B1 runtime KAT，不等于安全、constant-time、provenance、性能、硬件加速或合规批准；LWC 仍仅允许 focused-test experimentation | SHA-256/HMAC/AES 外部 provenance/license、独立审计、fuzz/side-channel 与性能；LWC authoritative KAT/provenance review；reviewed signature provider |
| GUI | H1（backend 错误传播；SDL fake + real-library headless contract；字体 source-table review；licensed required-UI subset active） | PC build；SDL2 opt-in 缺依赖 fail-closed；canonical CI 用真实 SDL2 headers/library 编译链接 backend，并以 dummy video driver 实跑 window/renderer/texture/fill/flush/event/deinit；fake seam 覆盖错误路径 | pending | legacy 字体视觉 `rejected-needs-regeneration` 且 provenance pending；OFL-1.1 Noto Sans CJK SC 的 15 个 required UI glyph 已按 pinned Host snapshot 接入 active 16x16 table，但尚未视觉/实板批准；performance pending | Host GUI/font/backend contract；SDL headless runtime 仅为 PC runtime evidence，不是人工视觉、性能或屏幕硬件证据；required UI subset 为 distinct/nonblank active table；其余 legacy 16x24/中文 placeholder 不得作为最终产品字体 | 人工视觉审查；替换其余 legacy table；屏幕 B1/P1 |
| Net | H1（core/Modbus/MQTT/AT/CAN/LTE contracts；产品协议 default-off） | PC root；全协议显式 opt-in `xy_net` target | pending | long-run pending | root Kconfig 直接控制 active source/umbrella export；active AT owner 为 lightweight client/server，active MQTT owner 为 `src/xy_mqtt_client.c`；vendor AT trees、legacy MQTT、CAN/LTE 不会因文件存在自动进入产品库 | 明确 modem/CAN 产品选择后补 UART/flow-control/power、attach/PDP/URC/controller 与 B1/B2 |
| Wireless / NRF24L01 | H1（SPI register read/write/restore、floating-bus 与 mismatch fail-closed；PTX ACK/MAX_RT 与 MAX_RT cleanup transport error propagation；identity-bound UART validator 分离 acknowledged PTX、PRX timeout、valid RX payload 与 error/incomplete outcomes） | PC/L4/U5 compile；Pandora SPI2 probe clean link/符号/固件身份 | clean committed `35438285` 11524-byte image 已 ST-Link write/verify/read-back byte-identical；35 秒 WCH-Link capture 337 bytes，exact firmware identity、完整 register snapshot 与 `NRF24_TX_ACK_OK retries=01`；随后 30 秒 PRX 窗口明确 `NRF24_RX_TIMEOUT`，machine validator 返回 `B1_NRF24_ACKNOWLEDGED_TX_PASS`、`rx_outcome=TIMEOUT_NO_PAYLOAD` | RF/performance pending | 允许声明 Pandora onboard NRF24 SPI2 register presence 与一次固定 32-byte payload 的 acknowledged PTX B1；本轮没有接收 payload，ACK 证明兼容接收端响应但 peer identity/payload receipt 未被独立日志绑定，不等于 RX、IRQ transition、range、throughput、recovery 或 endurance | 增加独立已知接收端的 payload/identity 日志，再做 RX、IRQ 与 bounded recovery |
| FOTA | H1（状态机、candidate envelope/tool、source-commit-bound reviewed restage、signature-provider/boot-confirm fail-closed；双记录 journal 与 durable attempt/confirm/rollback） | PC/FOTA target；L4/U5 compile compatibility；Pandora bootloader `0x08000000`、application `0x08008000` 与 opt-in candidate programmer 独立链接 | Pandora 一次性 programmer 已将 confirmation-capable `e73254da` candidate（33916 bytes；SHA-256 `507d1610...`）写入 W25Q128 并逐块回读；resident bootloader 随后完成 `INSTALLED→ATTEMPT_COMMITTED→CONFIRM_REQUESTED→CANDIDATE_CONFIRMED→CONFIRM_ACKNOWLEDGED`，且软件复位后 journal 命中同 candidate、跳过重复擦写。14416-byte bootloader ST-Link write/verify/read-back byte-identical；25 秒独立 UART capture SHA-256 `5f3dab7c...`，多轮有序链路无 FOTA error marker | Secure FOTA blocked；无 approved provider | Pandora candidate 安装/执行 B1、软件复位幂等与 durable confirmation B2；Host 证明 malformed candidate/source commit、无效授权和 Flash failure fail-closed。该证据不等于真实掉电/半写、签名安全或量产 updater | 真实掉电/半写；reviewed provider/key provisioning |
| CI/Release | canonical Host gate + PC root build 可用（2026-09-01：200/200）；首个 release input `device_driver_template` 已从 `git archive HEAD` clean export 通过独立 configure/build/run；canonical Host committed clean-export gate 从 `git archive HEAD` 独立 configure/build，并在排除 5 个依赖 Git 仓库状态或递归 archive/build 的 policy tests 后通过 195/195；canonical CI 已将常规 Host、committed clean-export 与 PC artifact reproducibility 分步设为必跑；同一 committed source archive 的两次独立 PC Release `xy_device` 构建产出相同 `libxy_device.a` SHA-256/size；[PC release build environment](pc-release-build-environment.json) 固定 Ubuntu 24.04 runner 与 PC/x86_64/Release/config/tool identity contract，[PC release artifact manifest](pc-release-artifact-manifest.json) 将当前选定 artifact set 限定为 `xy_device` / `libxy_device.a` 的 reproducibility gate-only 项；gate 输出每次实际 CMake/CC/AR/Python identity，生成/上传含 source commit/archive hash、artifact-manifest schema/status、双构建 artifact hash/size 与工具身份的机器 JSON evidence，将验证后的 library、SHA-256、CycloneDX JSON 1.6 SBOM、ephemeral Ed25519 signature/public key 及 bounded license/NOTICE review records 作为 14 天 bounded CI artifact 归档，并以独立调用重读校验 checksum/SBOM/signature 与 legal-pending/source-scope 边界；SBOM 绑定 exact artifact/source archive/source commit 与 10 个直接编译源的 SHA-256 和 Apache-2.0 evidence，状态保持 `REVIEW_PENDING`；[bounded technical license review](pc-release-license-review.json) 已记录这 10 个 first-party source、根许可证 hash 与无冲突文件级声明的扫描结果，但 legal/NOTICE/完整 release scope 仍 pending；临时私钥不归档且每次运行丢弃；[release signing policy](release-signing-policy.json) 固定 Ed25519 设计边界，但 release identity 仍为 `UNASSIGNED`、key custody 为 `NOT_ESTABLISHED`、publication 为 `BLOCKED`，ephemeral CI key 不得升级为 release key；[PC release SBOM policy](pc-release-sbom-policy.json) 固定 bounded artifact 的生成/独立验证边界，但不构成法律/license approval、完整 PC/MCU SBOM 或 R1 | PC build；[Kconfig/CMake 配置矩阵](kconfig-cmake-configuration-matrix.md)已建立，all-off 配置不再泄漏关闭组件 target，Device/Crypto/DM/Sensor/Actuator-only 组合的生成值、focused target 与归档对象已验证；Sensor/Actuator 的两个 framework 开关分别映射到各自相同 root target，独立 Device-driver 路径未混入；非法 Display 子功能组合 fail-closed，OLED/SSD1306、LCD SPI/I8080/ST7789 与 LED/serial RGB 合法组合已验证 source selection；Net core 与 MQTT/AT/CAN/LTE 选择均 default-off，显式全协议组合的 `xy_net` target 已验证；无实现源的 standalone RGB 配置已移除；STM32U5 默认组合曾用 `arm-none-eabi-gcc` clean compile，并验证 STM32U5 与 FS/FlashDB 条件默认值 | n/a | [tracked source dependency inventory](source-dependency-inventory.json) 为机器守护的 `REVIEW_PENDING` 前置；bounded artifact SBOM generation 已 guarded，法律/license approval 仍 pending | development CI only；`unit-tests.yml` 为单一可信 ...

---

### Sprint 6 pre-RC artifact evidence (2026-09-30)

- Pandora STM32L475VE is the sole reference board; STM32U5 remains enhancement compile-only.
- The frozen pre-RC MCU build-gate input is `pandora_stm32l475_rtos.bin` at `0x08000000`; no examples/projects are selected for support.
- Two independent clean-export STM32L4 Release builds from current `git archive HEAD` (`212df50192baa787aea9d6e0e0eadd1a8e4e6f57`) plus pinned STM32CubeL4 commit `9203b3843e219d2025a7f868d7656b5a5d208885` produced identical 39260-byte BINs with SHA-256 `052b81842e506457ad2531589cd73fa6910eaacd70c5e13abe635ae6a8b09d9b`.
- The BIN and `.sha256` pair were independently re-read and verified. This is compile-reproducibility and checksum evidence only; it does not establish MCU SBOM, signature, CI publication, flash/install, runtime, RC, or R1 qualification.
- The explicit opt-in `tests/unit/check_pandora_release_sbom.py` generator and verifier produced
  `build/pandora-release-sbom.json` with 39 linked source components from the RTOS link map and
  linked FreeRTOS/CLIB archive hashes. Status remains `REVIEW_PENDING`; this is bounded input
  inventory evidence only and does not establish complete MCU dependency coverage, legal approval,
  security approval, runtime qualification, RC, or R1.

---

## 3. 证据记录索引

### 2026-10-02 VL53L0X Device re-init atomicity

- 组件：Drivers Sensor / VL53L0X
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_vl53l0x_device.c`; focused `sensor_vl53l0x_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, transport, and identity failures; first initialization failures remain fail-closed.
- 仍不允许宣称：VL53L0X board identity, range accuracy, timing, recovery, or endurance.

### 2026-10-02 VL53L1X Device re-init atomicity

- 组件：Drivers Sensor / VL53L1X
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_vl53l1x.c`; focused `sensor_vl53l1x`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization validates live nested transport and preserves a live owner across identity/configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：VL53L1X board identity, range accuracy, timing, recovery, or endurance.

### 2026-10-02 VCNL4040 Device re-init atomicity

- 组件：Drivers Sensor / VCNL4040
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_vcnl4040_device.c`; focused `sensor_vcnl4040_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper and transport failures; first initialization failures remain fail-closed.
- 仍不允许宣称：VCNL4040 board identity, optical response, distance calibration, recovery, or endurance.

### 2026-10-02 HMC5883L Device re-init atomicity

- 组件：Drivers Sensor / HMC5883L
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_hmc5883l.c`; focused `sensor_hmc5883l`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：HMC5883L board identity, magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 AHT20 Device re-init atomicity

- 组件：Drivers Sensor / AHT20
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_aht20.c`; focused `sensor_aht20`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, transport, busy-status, and final-status failures; first initialization failures remain fail-closed.
- 仍不允许宣称：AHT20 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-02 MLX90614 Device re-init atomicity

- 组件：Drivers Sensor / MLX90614
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_mlx90614.c`; focused `sensor_mlx90614`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper and identity failures; first initialization failures remain fail-closed.
- 仍不允许宣称：MLX90614 board identity, temperature accuracy, emissivity calibration, recovery, or endurance.

### 2026-10-02 AS5600/AS5048B Device re-init atomicity

- 组件：Drivers Sensor / AS5600 + AS5048B
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_as5600_device.c`, `tests/unit/sensor/test_as5048b_device.c`; focused `sensor_as5600_device`, `sensor_as5048b_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper failures; first initialization failures remain fail-closed.
- 仍不允许宣称：angle encoder board identity, angle accuracy, magnetic installation, recovery, or endurance.

### 2026-10-02 SHT30 Device re-init atomicity

- 组件：Drivers Sensor / SHT30
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_sht30_device.c`; focused `sensor_sht30_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, transport, and reset-command failures; first initialization failures remain fail-closed.
- 仍不允许宣称：SHT30 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-03 LSM9DS1 Device re-init atomicity

- 组件：Drivers Sensor / LSM9DS1
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lsm9ds1_device.c`; focused `sensor_lsm9ds1_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged dual-transport initialization preserves a live owner across IMU or magnetometer helper failures; first initialization failures remain fail-closed; magnetometer identity uses its canonical WHO_AM_I register.
- 仍不允许宣称：LSM9DS1 board identity, motion/magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 KX023 Device re-init atomicity

- 组件：Drivers Sensor / KX023
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_kx023_device.c`; focused `sensor_kx023_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across transport, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：KX023 board identity, motion accuracy, dynamic response, recovery, or endurance.

### 2026-10-03 QMC5883L Device re-init atomicity

- 组件：Drivers Sensor / QMC5883L
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_qmc5883l_device.c`; focused `sensor_qmc5883l_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across helper, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：QMC5883L board identity, magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 IST8310 Device re-init atomicity

- 组件：Drivers Sensor / IST8310
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_ist8310_device.c`; focused `sensor_ist8310_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across transport, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：IST8310 board identity, magnetic accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 BME680 Device re-init atomicity

- 组件：Drivers Sensor / BME680
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_i2c2_sensor_devices.c`; focused `sensor_i2c2_devices`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner when the nested I2C helper fails during re-init; first initialization failures remain fail-closed.
- 仍不允许宣称：BME680 board identity, environmental accuracy, gas-response quality, recovery, or endurance.

### 2026-10-03 CCS811 Device re-init atomicity

- 组件：Drivers Sensor / CCS811
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_ccs811_device.c`; focused `sensor_ccs811_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner when the nested I2C helper fails during re-init; first initialization failures remain fail-closed and transport readiness rejects non-binary initialized state.
- 仍不允许宣称：CCS811 board identity, eCO2/TVOC accuracy, warm-up quality, recovery, or endurance.

### 2026-10-03 AHT10 Device re-init atomicity

- 组件：Drivers Sensor / AHT10
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_env_i2c_sensors.c`; focused `sensor_env_i2c_sensors`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across nested I2C helper and init-command failures; first initialization failures remain fail-closed.
- 仍不允许宣称：AHT10 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-03 APDS9960 Device re-init atomicity

- 组件：Drivers Sensor / APDS9960
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_apds9960_device.c`; focused `sensor_apds9960_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across nested transport, identity, and enable-write failures; first initialization failures remain fail-closed and require an exact live nested transport.
- 仍不允许宣称：APDS9960 board identity, optical response, gesture classification, timing, recovery, or endurance.

### 2026-10-03 LSM6DSO Device re-init atomicity

- 组件：Drivers Sensor / LSM6DSO
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lsm6dso_device.c`; focused `sensor_lsm6dso_device`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged initialization preserves a live owner when nested transport or identity/configuration fails; first initialization failures remain fail-closed.
- 仍不允许宣称：LSM6DSO board identity, motion accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 SC7A22H Device re-init atomicity

- 组件：Drivers Sensor / SC7A22H
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_sc7a22h.c`, `tests/unit/sensor/test_i2c2_sensor_devices.c`; focused `sensor_sc7a22h`, `sensor_i2c2_devices`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner when the nested I2C helper fails; first initialization failures remain fail-closed and preserve the existing public contract.
- 仍不允许宣称：SC7A22H board identity, motion accuracy, FIFO, interrupt, recovery, or endurance.

### 2026-10-03 HDC1080 Device re-init atomicity

- 组件：Drivers Sensor / HDC1080
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_hdc1080.c`; focused `sensor_hdc1080`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged candidate initialization preserves a live owner across nested helper and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：HDC1080 board identity, temperature/humidity accuracy, recovery, or endurance.

### 2026-10-04 SHT40 Device re-init atomicity

- 组件：Drivers Sensor / SHT40
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_sht40.c`; focused `sensor_sht40`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`
- 适用范围：staged live-owner preservation covers helper, incomplete transport, serial-command/read, and CRC failures; first initialization failures remain fail-closed.
- 仍不允许宣称：SHT40 board identity, temperature/humidity accuracy, CRC robustness, recovery, or endurance.

### 2026-10-04 BMI270 Device re-init atomicity

- 组件：Drivers Sensor / BMI270
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_bmi270.c`; focused `sensor_bmi270`; full Host/PC/STM32L4/STM32U5 gate pending
- 适用范围：staged candidate initialization preserves a live owner when reset, identity, or configuration fails; first initialization failures remain fail-closed.
- 仍不允许宣称：BMI270 board identity, motion accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-03 LSM6DSL/LSM6DSR Device re-init atomicity

- 组件：Drivers Sensor / LSM6DSL + LSM6DSR
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lsm6dsl_device.c`, `tests/unit/sensor/test_lsm6dsr_device.c`; focused `sensor_lsm6dsl_device`, `sensor_lsm6dsr_device`; full Host/PC/STM32L4/STM32U5 gate pending
- 适用范围：staged candidate initialization preserves a live owner across nested transport, identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：LSM6DSL/LSM6DSR board identity, motion accuracy, calibration, dynamic response, recovery, or endurance.

### 2026-10-04 BMA400/KX023/LSM6 Device re-init atomicity

- 组件：Drivers Sensor / BMA400 + KX023 + LSM6 family
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_bma400_device.c`, `test_kx023_device.c`,
  `test_lsm6dso_device.c`, `test_lsm6dsl_device.c`, `test_lsm6dsr_device.c`;
  focused CTests `sensor_bma400_device`, `sensor_kx023_device`,
  `sensor_lsm6dso_device`, `sensor_lsm6dsl_device`, `sensor_lsm6dsr_device`;
  `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged candidate initialization preserves a live owner across nested transport,
  identity, and configuration failures; first initialization failures remain fail-closed.
- 仍不允许宣称：board identity, motion accuracy, calibration, dynamic response, recovery,
  or endurance for these sensors.

### 2026-10-04 ADXL362 Device re-init atomicity

- TDD RED：live owner 重复 `xy_adxl362_init()` 在 SPI transport、identity 或 configuration 失败时会先清空原 transport、lifecycle 与 sample cache。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 identity/filter/power 配置成功后才原子替换。
- 验证：focused `sensor_adxl362_device` 1/1；Host `make test-unit` 264/264；PC、STM32L4、STM32U5 build 与 `git diff --check` 通过。
- 边界：仅为 Host/compile lifecycle contract，不升级 ADXL362 实板身份、运动精度、动态响应、恢复或 endurance 证据。

### 2026-10-04 ADS1115 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ads1115_init()` 在 nested I2C helper 或 configuration read failure 时会先清空原 transport、lifecycle 与 cached conversion state。
- 收口：初始化改用 staged candidate；首次失败清零未初始化 owner，live owner 失败保持原 owner，完整 configuration read 成功后才原子替换。
- 验证：focused `sensor_ads1115`; Host 265/265; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 ADS1115 实板身份、转换精度、校准、恢复或 endurance 证据。

### 2026-10-04 AK09918 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ak09918_init()` 在 transport、身份或配置失败时会先清空原 transport、lifecycle 与 sample cache。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 identity/reset/configuration 成功后才原子替换。
- 验证：focused `sensor_ak09918_device` 1/1；Host `make test-unit` 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check` 通过。
- 边界：仅为 Host/compile lifecycle contract，不升级 AK09918 实板身份、磁场精度、校准、动态响应、恢复或 endurance 证据。

### 2026-10-04 INA219 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ina219_init()` 在 helper/configuration 写失败时会先清空原 transport、lifecycle 与 sample。
- 收口：初始化改用 staged candidate；首次失败清零未初始化 owner，live owner 失败保持原 owner，完整配置成功后才原子替换。
- 验证：focused `sensor_power_monitors`；Host 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 INA219 实板身份、计量精度、校准、恢复或 endurance 证据。

### 2026-10-04 LTC2945 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ltc2945_init()` 在 helper、STATUS 读取或配置写失败时会先清空原 transport、lifecycle、配置与 sample cache。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 STATUS/configuration 成功后才原子替换。
- 验证：focused `sensor_adc_power_monitors`；Host 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 LTC2945 实板身份、计量精度、校准、告警、电气、恢复或 endurance 证据。

### 2026-10-04 BNO055 Device re-init atomicity

- TDD RED：live owner 重复 `xy_bno055_init()` 在 reset、身份或配置失败时会先清空原 transport、lifecycle 与模式/单位状态。
- 收口：初始化改用 staged candidate；首次失败清零未初始化 owner，live owner 失败保持原 owner，完整 identity、firmware、mode 与 unit 配置成功后才原子替换。
- 验证：focused `sensor_bno055`；Host/PC/STM32L4/STM32U5 gate 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 BNO055 实板身份、融合精度、动态响应、恢复或 endurance 证据。

### 2026-10-06 Device owner re-init atomicity continuation

- 组件：Drivers Sensor / LPS22HB、MPU6050、BMI088、INA219、QMA6100P
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_lps22hb.c`, `test_mpu6050.c`,
  `test_bmi088.c`, `test_adc_power_monitors.c`, `test_qma6100p.c`; focused CTests
  `sensor_lps22hb`, `sensor_mpu6050`, `sensor_bmi088`, `sensor_adc_power_monitors`,
  `sensor_qma6100p`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`.
- 适用范围：live owners preserve transport/lifecycle/cache across nested-helper,
  identity, status/configuration, and re-init failures; successful candidates commit
  atomically and first initialization failures remain fail-closed.
- 验证结果：Host 265/265、PC/STM32L4/STM32U5 builds and `git diff --check` passed.
- 仍不允许宣称：board identity, measurement/motion accuracy, calibration,
  interrupt/electrical behavior, recovery, endurance, or security approval.

### 2026-10-04 INA228/INA229 Device re-init atomicity

- TDD RED：live owner 重复 `xy_ina228_init()` / `xy_ina229_init()` 在身份失败时会先清空原 transport、lifecycle、配置与 sample 状态。
- 收口：初始化改用 staged candidate；首次失败仍清零未初始化 owner，live owner 失败保持原 owner，完整 identity/configuration 成功后才原子替换。
- 验证：focused `sensor_ina228_device`、`sensor_ina229_device`；Host 265/265；PC、STM32L4、STM32U5 build 与 `git diff --check`。
- 边界：仅为 Host/compile lifecycle contract，不升级 INA228/INA229 实板身份、计量精度、校准、告警、电气、恢复或 endurance 证据。

### 2026-10-07 24xx EEPROM Device re-init atomicity

- 组件：Storage/24xx EEPROM
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/storage/eeprom/24xx/xy_eeprom_24xx.c`,
  `tests/unit/storage/test_storage_eeprom_24xx.c`; focused `storage_eeprom_24xx`;
  `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged nested-I2C initialization preserves the existing live owner when a
  replacement helper initialization fails; first initialization failures remain fail-closed,
  incomplete helper success is rejected, and successful re-init commits the complete candidate.
- 仍不允许宣称：24xx board presence, write-protect behavior, real write-cycle timing,
  power-loss recovery, endurance, or EEPROM hardware qualification.

### 2026-10-07 W25Q128 Device re-init atomicity

- 组件：HAL / Storage / W25Q128
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/storage/flash/w25q128/xy_w25q128.c`,
  `tests/unit/storage/test_w25q128.c`; focused `storage_w25q128`; `make test-unit`;
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged JEDEC probing preserves a live QSPI/Device owner on failed re-init;
  successful re-init atomically replaces transport and name without attempting duplicate
  registration of the same embedded Device object; initial registration failure restores caller state.
- 仍不允许宣称：W25Q128 board identity, erase/program recovery, power-loss safety,
  performance, endurance, or Flash hardware qualification.

### 2026-10-07 nRF24L01 failed re-probe owner atomicity

- 组件：Wireless / NRF24L01
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/wireless/nrf24l01/xy_nrf24l01.c`,
  `tests/unit/net/test_nrf24l01.c`; focused `wireless_nrf24l01`; `make test-unit`;
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：a failed replacement probe preserves the complete live radio owner, including
  transport callbacks, probed register snapshot, PRX payload width, and lifecycle state; the
  staged candidate is published only after the complete probe succeeds.
- 仍不允许宣称：real peer RX, IRQ edge behavior, RF recovery, range, throughput, endurance,
  or radio hardware qualification.

### 2026-10-07 RGB Matrix effect tick ownership

- 组件：Display / RGB Matrix
- 旧等级 -> 新等级：H1/Host timing contract strengthened; no Board claim change
- 证据路径/命令：`components/drivers/display/led_drivers/serial_rgb/xy_rgb_matrix.c`,
  `tests/unit/display/test_display_rgb_matrix.c`; focused `display_rgb_matrix`; `make test-unit`;
  `make`; `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：effect cadence consumes canonical `xy_os_tick_get()` and updates only after the
  configured elapsed interval, including deterministic pre-boundary and boundary checks.
- 仍不允许宣称：WS2812 electrical timing, animation frame-rate accuracy, visual quality,
  performance, endurance, or display hardware qualification.

### 2026-10-07 Device PM idle-time ownership

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host timing contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `components/device/inc/xy_device_pm.h`, `tests/unit/device/test_device_pm.c`; focused
  `device_pm`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`.
- 适用范围：idle timeout 与 activity timestamps 消费 canonical `xy_device_get_tick()`；
  timeout 边界前保持 ACTIVE，达到边界时经 driver PM callback 进入 SLEEP。
- 仍不允许宣称：Pandora sleep/wakeup, power consumption, wake latency, deep-power modes,
  recovery, endurance, or Device PM hardware qualification.

### 2026-10-07 IPC MQ exact timeout boundary

- 组件：IPC / Message Queue
- 旧等级 -> 新等级：H1/Host timeout contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/src/xy_mq.c`, `tests/unit/ipc/test_ipc_mq.c`;
  focused `ipc_mq`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：bounded send/receive waits now return `XY_MQ_TIMEOUT` when elapsed canonical
  OSAL ticks reach the requested timeout, without the previous extra one-tick delay; unsigned
  subtraction keeps tick-wrap behavior bounded.
- 仍不允许宣称：Pandora IPC runtime timing, wake latency, throughput, multi-producer safety,
  ISR safety, recovery, endurance, or product qualification.

### 2026-10-07 IPC Broker request response correlation

- 组件：IPC / Broker
- 旧等级 -> 新等级：H1/Host request-response contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_request()` 仅提取 source queue 中匹配目标 server/message ID 的
  response；不相关消息保持原 FIFO 顺序等待正常 dispatch，不再被误当 response 消耗。
- 仍不允许宣称：Pandora request/response runtime、并发 correlation token、throughput、
  multi-request safety、recovery、endurance 或 product qualification。

### 2026-10-07 Trace runtime dynamic-level filtering

- 组件：Trace
- 旧等级 -> 新等级：H1/Host runtime-filter contract strengthened; no Board claim change
- 证据路径/命令：`components/trace/xy_log/inc/xy_log.h`,
  `components/trace/xy_log/src/xy_log.c`, `tests/unit/trace/test_trace_core.c`;
  focused `trace_component`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：linked `xy_trace` consumers enable runtime filtering so public ERROR/WARN/INFO/DEBUG
  macros emit only when the current dynamic threshold allows them; invalid threshold updates preserve
  the previous level. Source-only focused targets that include `xy_log.h` without linking `xy_trace`
  retain compile-time-only behavior and do not gain an unresolved runtime symbol.
- 仍不允许宣称：Pandora Trace runtime filtering、throughput、concurrent loss behavior、ISR safety、
  performance、recovery、endurance 或 product qualification。

### 2026-10-08 openCH CH32V307 SYS HAL compile baseline

- 组件：HAL / SYS / WCH compile compatibility
- 旧等级 -> 新等级：source-only -> C1 compile baseline; no Board claim
- 证据路径/命令：`components/hal/wch/ch32x/src/xy_hal_sys.c`,
  `boards/opench_ch32v307/sys_identity_main.c`; focused targets
  `opench_ch32v307_sys_identity_smoke` and `xy_hal`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 制品：ELF SHA-256 `af7cdada7959ce64dfaf1bd19484636e0f263c958726c4b97f3477c91c576625`；
  BIN SHA-256 `d2d2b5bb52e354e88aeee493f1a76abdceb650ebab49f9822f2a4619e108776f`。
- 适用范围：CH32V307 system identity、clock/memory query、reset/IRQ/critical-section 与
  bounded unsupported API compile/link reachability；Pandora 仍是唯一 reference-board runtime 基线。
- 仍不允许宣称：openCH flash/read-back、UART runtime、真实 clock/memory identity、reset reason、
  low-power behavior、IRQ runtime、performance、recovery、endurance 或 hardware qualification。

### 2026-10-08 openCH CH32V307 internal Flash HAL compile baseline

- 组件：HAL / Storage / WCH compile compatibility
- 旧等级 -> 新等级：source-only -> C1 compile baseline; no Board claim
- 证据路径/命令：`components/hal/wch/ch32x/src/xy_hal_flash.c`,
  `boards/opench_ch32v307/flash_info_main.c`; focused targets
  `opench_ch32v307_flash_info_smoke` and `xy_hal`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 制品：ELF SHA-256 `a875228b0d89771f1016a297567d5790c356a1a961c28de859152d2001e5af6f`；
  BIN SHA-256 `36808560f087daa744f738e1e6f8942d7a2f84de997d4e0c601a7b796f495cdb`。
- 适用范围：288 KiB address range、72 个 4 KiB page、4-byte program alignment、
  lifecycle/lock/range validation 与 SDK erase/program link reachability；board smoke 只读取
  immutable geometry/boundary metadata，不执行 Flash erase/program。
- 仍不允许宣称：openCH Flash erase/program/read-back runtime、掉电恢复、保护位、bootloader/FOTA、
  performance、endurance 或 hardware qualification。

### 2026-10-08 Device PM owner isolation

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：PM side-table 以 Device identity 独立索引，不再覆盖 canonical
  `xy_device_t.data` driver-private storage；两个 Device 的 PM state 保持隔离。
- 仍不允许宣称：Pandora Device PM runtime、功耗、wake latency、deep-power modes、
  concurrency safety、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM slot lifecycle

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/device/inc/xy_device_pm.h`,
  `components/device/src/xy_device_pm.c`, `tests/unit/device/test_device_pm.c`; focused
  `device_pm`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：公开 deinit 释放固定 PM side-table slot；释放后 owner 返回 not-initialized，
  已满 16-slot table 可复用释放槽位，且不触碰 Device driver-private `data`。
- 仍不允许宣称：Pandora Device PM runtime、并发访问安全、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM public setter fail-closed contract

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host input and callback-failure contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：state/policy setters reject out-of-range enums before callbacks or state mutation;
  wakeup intent is committed only after the optional driver callback succeeds, so failure is retryable.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM consumption output atomicity

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host callback-failure contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：driver power-consumption callbacks write into staged storage; public caller output is
  committed only after callback success and remains unchanged on callback failure.
- 仍不允许宣称：Pandora power measurement、Device PM runtime、功耗精度、wake latency、
  recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM state callback atomicity

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host callback contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：public state reads dispatch the optional driver `get_state` callback; callback failure
  and invalid reported enums preserve caller output and cached state, while a valid result commits
  atomically to both.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM live re-init and driver-state history

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host lifecycle and state-history contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：same-ops re-init preserves a live owner's current/previous state and activity history;
  replacement ops are rejected, and valid driver-reported transitions update wake history atomically.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM idle-timeout fresh window

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host timeout contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：runtime 设置或更新 idle timeout 时以 canonical `xy_device_get_tick()` 重启
  activity window，避免沿用 init 时的旧 timestamp 造成即时休眠；达到新 timeout 边界才进入 SLEEP。
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM policy transition contract

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host policy-transition contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：切换 `ALWAYS_ON` 时先经 driver state callback 将 sleeping owner 原子恢复为
  ACTIVE，callback 失败保持原 policy/state；切换 `AUTO` 时以 canonical Device tick 重启 idle window。
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM ALWAYS_ON state invariant

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host policy-state invariant strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`ALWAYS_ON` owners reject SLEEP、DEEP_SLEEP 与 OFF requests before the driver
  callback, preserving ACTIVE state and avoiding backend side effects.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM driver-report policy invariant

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host policy-state invariant strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`ALWAYS_ON` owners reject valid non-ACTIVE states reported by an optional driver
  callback, preserve caller output and cached ACTIVE state, and accept a later ACTIVE report.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM nested low-power wake invariant

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host wake-state contract strengthened; no Board claim change
- 证据路径/命令：`components/device/src/xy_device_pm.c`,
  `tests/unit/device/test_device_pm.c`; focused `device_pm`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：public wakeup returns owners from SLEEP、DEEP_SLEEP or OFF to ACTIVE through the
  optional driver callback; nested low-power transitions cannot restore a stale low-power state.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 Device PM activity wake failure propagation

- 组件：Device / per-device PM
- 旧等级 -> 新等级：H1/Host activity-wake contract strengthened; no Board claim change
- 证据路径/命令：`components/device/inc/xy_device_pm.h`,
  `components/device/src/xy_device_pm.c`, `tests/unit/device/test_device_pm.c`; focused
  `device_pm`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：public activity recording reports invalid/uninitialized owners and propagates an
  automatic wake callback failure without committing a fresh activity window; a successful retry
  returns the owner to ACTIVE before committing the new activity timestamp.
- 仍不允许宣称：Pandora Device PM runtime、concurrency safety、实际功耗、wake latency、
  deep-power modes、recovery、endurance 或 hardware qualification。

### 2026-10-08 IPC Broker pub/sub callback failure propagation

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host callback-failure contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：publish continues notifying active subscribers but returns the first callback error;
  delivered/dropped counters reflect actual callback outcomes and the publish attempt is counted once.
- 仍不允许宣称：Pandora pub/sub runtime、concurrency safety、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-08 IPC Broker empty-topic ownership

- 组件：IPC / Broker pub/sub
- 旧等级 -> 新等级：H1/Host bounded-resource contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：created topics retain distinct ownership before their first subscriber and after their
  last unsubscribe; duplicate IDs are rejected and the fixed topic table fails closed when full.
- 仍不允许宣称：Pandora pub/sub runtime、concurrency safety、throughput、ISR safety、recovery、
  endurance 或 IPC product qualification。

### 2026-10-08 IPC Broker ISR ingress wake-failure rollback

- 组件：IPC / Broker ISR ingress
- 旧等级 -> 新等级：H1/Host ISR handoff failure contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker_isr_ingress.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：ISR ingress publishes its ring tail before requesting the wake, then rolls the tail
  back to the exact previous slot when the ISR-safe wake callback fails; rejected payloads cannot
  be drained later, the same slot remains reusable by a subsequent successful publish, and the
  callback's exact failure code is preserved for caller recovery policy.
- 仍不允许宣称：Pandora multi-ISR runtime、concurrency safety、throughput、arbitrary interrupt
  sources、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request bounded-wait contract

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host bounded-wait contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：zero-timeout requests perform one nonblocking response poll; bounded waits stop and
  return an error when the canonical OSAL delay backend fails instead of spinning forever on a
  non-advancing tick; successful waits poll again after each completed delay.
- 仍不允许宣称：Pandora request/response runtime、concurrent correlation、多请求安全、throughput、
  recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request source-owner guard

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host request ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_request()` 在发送前要求 source server 已注册；无 response queue owner
  时返回 `XY_BROKER_NOT_FOUND`，不再向 destination 留下必然超时且无法关联的 orphan request。
- 仍不允许宣称：Pandora request/response runtime、concurrent correlation、多请求安全、throughput、
  recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request sequence correlation

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response-correlation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：responses preserve the originating request sequence number, and
  `xy_broker_request()` requires source server, destination server, message ID, and sequence to
  match; stale same-route/same-message replies remain queued instead of satisfying a new request.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker response owner validation

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_respond()` 要求原 request 指定的 responder 仍为 registered server；
  responder 注销后拒绝入队，避免伪造或 stale request metadata 向 requester 注入 response。
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request wait owner lifecycle

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host request lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：bounded request wait 同时绑定 requester 与 destination server 的注册代际，并在每次
  OSAL delay 后重新验证；任一 owner 注销或注销后同 ID 重新注册时立即返回
  `XY_BROKER_NOT_FOUND`，不继续使用 stale server slot，也不修改 caller response。
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker request destination lifecycle

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host request lifecycle contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：request wait 绑定 destination server 的注册代际；destination 注销或被同 ID 新 owner
  替换后立即 fail-closed，防止等待中的 request 接受 replacement responder 的同 route/sequence reply。
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker stale request response rejection

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response ownership contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.h`,
  `components/ipc/xy_broker/xy_broker.c`, `tests/unit/ipc/test_ipc_broker.c`; focused
  `ipc_broker`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：Broker-generated requests carry requester/responder registration generations;
  `xy_broker_respond()` rejects a captured request after either owner is unregistered and replaced,
  preventing stale request metadata from injecting a response into a replacement requester's queue.
  Manually constructed unstamped request records retain the existing compatibility behavior.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker sequence-wrap disambiguation

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response correlation contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.h`,
  `components/ipc/xy_broker/xy_broker.c`, `tests/unit/ipc/test_ipc_broker.c`; focused
  `ipc_broker`; `make test-unit`; `make`; `make HAL_PLATFORM=STM32L4`;
  `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：Broker request/response sequence identity is widened from 16 to 32 bits; a queued stale
  response cannot alias a fresh request after 65,536 intervening messages with the same route/message
  identity. Existing owner-generation validation remains in force.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  32-bit sequence-wrap disambiguation、throughput、recovery、endurance 或 IPC product qualification。

### 2026-10-09 IPC Broker stamped response boundary

- 组件：IPC / Broker request-response
- 旧等级 -> 新等级：H1/Host response provenance contract strengthened; no Board claim change
- 证据路径/命令：`components/ipc/xy_broker/xy_broker.c`,
  `tests/unit/ipc/test_ipc_broker.c`; focused `ipc_broker`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`; `git diff --check`.
- 适用范围：`xy_broker_respond()` now rejects manually fabricated request records without both
  requester and responder generation stamps. Successful immediate/nonblocking response fixtures use
  Broker-generated request identity, and response dequeue also requires live generation matches.
- 仍不允许宣称：Pandora request/response runtime、multi-thread concurrent request safety、
  32-bit sequence-wrap disambiguation、trusted callers、throughput、recovery、endurance 或 IPC product
  qualification。

### 已存在模板/记录

### 2026-10-04 Device owner re-init atomicity continuation

- 组件：Drivers Sensor / INA219、LTC2945、BNO055、INA228、INA229
- 旧等级 -> 新等级：H1/Host contract strengthened; no Board claim change
- 证据路径/命令：`tests/unit/sensor/test_power_monitors.c`, `test_adc_power_monitors.c`,
  `test_bno055.c`, `test_ina228_device.c`, `test_ina229_device.c`; focused CTests
  `sensor_power_monitors`, `sensor_adc_power_monitors`, `sensor_bno055`,
  `sensor_ina228_device`, `sensor_ina229_device`; `make test-unit`; `make`;
  `make HAL_PLATFORM=STM32L4`; `make HAL_PLATFORM=STM32U5`.
- 适用范围：staged candidate initialization preserves a live owner across helper,
  identity, status-read, and configuration failures; first initialization failures remain
  fail-closed and successful re-initialization commits the complete candidate atomically.
- 仍不允许宣称：board identity, measurement/fusion accuracy, calibration, alert/electrical
  behavior, recovery, endurance, or security approval for these sensors.

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
