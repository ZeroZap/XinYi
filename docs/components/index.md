# 组件状态总览

**最后更新**: 2026-08-30

---

## 📊 组件完成度

### Host-guarded / 分层证据组件

| 组件 | 代码 | 测试 | 文档 | 构建 | 测试用例 | 状态 |
|------|------|------|------|------|---------|------|
| **OSAL** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | Bare-metal Host-guarded；FreeRTOS `compile-guarded-runtime-pending` |
| **HAL** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | PC Host contract、部分 QEMU；目标实板 pending |
| **Crypto** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；security review pending |
| **CLib** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；MCU size/heap pending |
| **DM** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；真实 Flash durability pending |
| **NET** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；modem/硬件/长稳 pending |
| **Device** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；RTOS/Driver B1/B2 pending |
| **Trace** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；并发/吞吐 pending |
| **Sensor** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；精度/时序/实板 pending |
| **IPC** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；RTOS 并发/ISR pending |
| **PM** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | host-guarded；功耗/唤醒实证 pending |
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
- FreeRTOS：Sprint 5 reference，STM32U5 source/static-library compile gate；runtime/ISR/并发/实板 pending
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
- HAL：PC Host contract、部分 QEMU；STM32U5/WCH/HC32 实板证据 pending
- 外设 API/source 存在不等于每个平台均已实现或运行验证
- 逐平台、逐外设状态以 [HAL 平台证据矩阵](../validation/hal-platform-evidence-matrix.md)为准

**支持平台与当前证据**:
- STM32U5（source/compile 前置；Board pending）
- STM32F4（部分 QEMU；部分外设仍 unsupported）
- STM32L4（当前复用 STM32F4 wrapper；Board pending）
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

### 短期 (1-2 周)

- [x] 完善 FOTA host-safe 主线实现、README、focused CTest 与 public smoke example
- [x] 将 GUI core/widgets/effects/fonts/display-backend adapter 推进到 host-guarded 状态
- [x] 将 Device 组件状态同步为 host-guarded，并补统一 README/current CTest 事实源
- [ ] 补真实硬件或人工证据：GUI 字体来源/host snapshot review/真实屏幕记录、FOTA bootloader/board NOR、Fuel Gauge SMBus/I2C

### 中期 (1 个月)

- [ ] 硬件在环测试
- [ ] 性能基准测试
- [ ] 覆盖率目标 >80%

### 长期 (3 个月)

- [ ] 更多 RTOS 支持
- [ ] 完整的应用示例
- [ ] 性能优化

---

## 📞 需要帮助？

- 📚 [组件开发指南](components/index.md)
- ❓ [常见问题](../about/faq.md)
- 💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)

---

*维护者：XinYi Team | 许可证：Apache License 2.0*
