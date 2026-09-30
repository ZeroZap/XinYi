# 组件状态总览

**最后更新**: 2026-09-30

> 平台事实：Pandora STM32L475VE 是正式 reference board；STM32U5/M33/TrustZone 仅保留
> enhancement compile compatibility。SSD1306 deferred，不选择、不推进，也不阻塞 Sprint 1–4。

---

## 📊 组件完成度

### Host-guarded / 分层证据组件

| 组件 | 代码 | 测试 | 文档 | 构建 | 测试用例 | 状态 |
|------|------|------|------|------|---------|------|
| **OSAL** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | Bare-metal Host；FreeRTOS Pandora bounded runtime；多小时/性能 `runtime-pending` |
| **HAL** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | PC Host、部分 QEMU；Pandora L4 多条 B1/B2；U5 compile-only |
| **Crypto** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；security review pending |
| **CLib** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；MCU size/heap pending |
| **DM** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；真实 Flash durability pending |
| **NET** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；modem/硬件/长稳 pending |
| **Device** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | Host；Pandora I2C/跨组件 bounded B1/B2 |
| **Trace** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | Host；Pandora 跨组件并发 B1；吞吐 pending |
| **Sensor** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；精度/时序/实板 pending |
| **IPC** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | Host；Pandora task/ISR ingress bounded B1/B2；多小时 pending |
| **PM** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | Host；Pandora shallow sleep/tick wake B1；功耗/深度模式 pending |
| **PID** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；plant/HIL pending |
| **ADDC** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；精度/标定 pending |

### Host-guarded，但关键硬件、安全或人工证据待补

| 组件 | 代码 | 测试 | 文档 | 构建 | 状态 |
|------|------|------|------|------|------|
| **FOTA** | ✅ | ✅ | ✅ | ✅ | 🟡 Host fail-closed contract；board Flash/bootloader/security/hardware pending |
| **Fuel Gauge** | ✅ | ✅ | ✅ | ✅ | 🟡 standalone host-guarded；SMBus/I2C 硬件验证 pending |
| **GUI** | ✅ | ✅ | ✅ | ✅ | 🟡 host-guarded core/widgets/effects/fonts/display-backend；真实屏幕、字体美术/来源审查仍待证据 |

---

## 📈 统计口径

- 测试数量以 canonical CTest 实际发现结果为准，不在本页维护易漂移的静态分组件计数。
- 公开入口分类不是产品完成度或 maturity 百分比；代码、文档或 Host 测试存在也不自动提升产品状态。
- Host/PC/QEMU/compile-only 不构成实板、安全或 production-ready 证据。

---

## 🔍 组件详情

### OSAL (OS 抽象层)

**目录**: `components/kernel/osal/`

**证据边界**:
- Bare-metal：Host contract
- FreeRTOS：Sprint 5 reference；Pandora STM32L475VE/CM4F 已有 bounded scheduler、同步、ISR→task、资源恢复、2P/2C、stress 与跨组件实板证据；多小时/性能仍 `runtime-pending`
- STM32U5/M33：enhancement compile compatibility，不阻塞 Pandora-first 基础验收
- SSD1306 deferred，不是 OSAL 验收依赖
- RT-Thread/CMSIS-RTX：source candidate，未建立 target/runtime gate
- 软件定时器与 Tick：Host contract；不能外推为所有 RTOS backend runtime 通过

**文档**:
- [简介](components/osal/introduction.md)
- [快速开始](components/osal/quickstart.md)
- [API 参考](components/osal/api-reference.md)

---

### HAL (硬件抽象层)

**目录**: `components/hal/`

**证据边界**:
- HAL：PC Host contract、部分 QEMU；Pandora L4 已有 UART/I2C/QSPI/DMA/SPI-TX/SYS/RTC/IWDG 等受限 B1/B2；未覆盖项仍 pending
- 外设 API/source 存在不等于每个平台均已实现或运行验证
- 逐平台、逐外设状态以 [HAL 平台证据矩阵](../validation/hal-platform-evidence-matrix.md)为准

**支持平台与当前证据**:
- STM32U5（enhancement compile compatibility；Board pending，不阻塞基础验收）
- STM32F4（部分 QEMU；部分外设仍 unsupported）
- STM32L4（Pandora 正式 reference board；dedicated HAL owners 与受限 B1/B2）
- WCH/HC32（部分 source；Board pending）
- PC simulation（Host contract）

**文档**:
- [简介](components/hal/introduction.md)
- [支持平台](components/hal/platforms.md)
- [API 参考](components/hal/api-reference.md)

---

### Crypto (密码学)

**目录**: `components/crypto/`

**算法**:
- ✅ AES (ECB, CBC, CTR)
- ✅ MD5, SHA-256
- ✅ HMAC
- ✅ CRC32
- ✅ Base64, Hex
- ✅ 随机数生成

**文档**:
- [简介](components/crypto/introduction.md)
- [算法列表](components/crypto/algorithms.md)
- [API 参考](components/crypto/api-reference.md)

---

### 其他组件

查看各组件详细文档：

- [CLib - 自定义 C 库](components/clib/index.md)
- [DM - 数据管理](components/dm/index.md)
- [NET - 网络协议](components/net/index.md)
- [Sensor - 传感器框架](components/sensor/index.md)
- [IPC - 进程间通信](components/ipc/index.md)
- [PM - 电源管理](components/pm/index.md)
- [PID - 控制算法](components/pid/index.md)
- [ADDC - ADC/DAC 辅助](components/addc/index.md)
- [FOTA - 固件升级](components/fota/index.md)
- [GUI - 图形界面](components/gui/index.md)
- [Trace - 日志系统](components/trace/index.md)

FOTA：Host fail-closed contract；board Flash、bootloader、secure provider 与实板 pending。其
Host metadata journal、callback 和错误边界不能升级为可烧录镜像、真实掉电恢复或安全批准。

---

## 🎯 下一步计划

### Sprint 5 已收口（2026-09-30）

- Pandora STM32L475VE 作为唯一正式 reference board；STM32U5 仅作为 enhancement compile gate。
- FreeRTOS 已取得 bounded task/ISR/同步/资源恢复/2P2C/120 秒 stress 证据；多小时耐久和功耗不在本次完成声明内。
- ST7789 已取得固定面板 visual B1，并完成 checked pixel/framebuffer/rotation 软件契约；GUI 字体、输入、帧率与 RAM 产品证据仍 pending。
- Sensor/Driver micro-hardening 已冻结；除 P0 回归外不再继续扩张 Sprint 5。
- ICM20608 dynamic、TF destructive/endurance、nRF peer、Fuel Gauge SMBus 与其余人工硬件项转显式 deferred/blocked backlog。

### Sprint 6 前置（当前 `BLOCKED/NO-GO`）

- [x] 冻结 Pandora pre-RC 组件/输入/制品集合：唯一 MCU build-gate artifact 为
  `pandora_stm32l475_rtos.bin`，examples/projects 全部排除；不授权发布。
- [x] 同步 tracker、evidence matrix、Known Limitations 与 release checklist，消除 stale 状态；
  当前 release 仍为 `BLOCKED/NO-GO`。
- [x] 运行 committed clean-export、Host、PC、STM32L4、STM32U5 compile 和 artifact
  reproducibility 最终门；具体结果以 tracker 与 release checklist 为准。
- [x] 建立机器可读 release blocker register：B-HIL、B-SEC、B-SBOM、B-SIGN、B-OWNER 五项
  保持 OPEN；在证据完成前禁止 RC/R1/tag。
- [ ] Crypto/FOTA provenance/security 未批准时保持 fail-closed，不进入 release claim。
- [ ] signing identity/key custody、完整 SBOM/license/legal review 未完成前不得发布 RC/tag。

### 后续产品验证 backlog

- [ ] 恢复硬件测试后处理 ICM20608 dynamic、TF endurance、nRF peer、Fuel Gauge SMBus。
- [ ] GUI→ST7789 字体/输入/帧时间/RAM 的单一产品纵切。
- [ ] FreeRTOS/PM 多小时耐久、真实功耗与恢复测试。

---

## 📞 需要帮助？

- 📚 [组件开发指南](components/index.md)
- ❓ [常见问题](../about/faq.md)
- 💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)

---

*维护者：XinYi Team | 许可证：Apache License 2.0*
