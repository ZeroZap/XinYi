# 组件状态总览

**最后更新**: 2026-08-30

---

## 📊 组件完成度

### Host-guarded / 分层证据组件

| 组件 | 代码 | 测试 | 文档 | 构建 | 测试用例 | 状态 |
|------|------|------|------|------|---------|------|
| **OSAL** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | Bare-metal Host-guarded；FreeRTOS `compile-guarded-runtime-pending` |
| **HAL** | ✅ | ✅ | ✅ | ✅ | 以 CTest 为准 | PC Host contract、部分 QEMU；目标实板 pending |
| **Crypto** | ✅ | ✅ | ✅ | ✅ | 28 | 🟢 host-guarded |
| **CLib** | ✅ | ✅ | ✅ | ✅ | 21 | 🟢 完善 |
| **DM** | ✅ | ✅ | ✅ | ✅ | 24 | 🟢 host-guarded |
| **NET** | ✅ | ✅ | ✅ | ✅ | 22 | 🟢 host-guarded；LTE 硬件待证据 |
| **Device** | ✅ | ✅ | ✅ | ✅ | 6 | 🟢 host-guarded |
| **Trace** | ✅ | ✅ | ✅ | ✅ | 10 | 🟢 完善 |
| **Sensor** | ✅ | ✅ | ✅ | ✅ | 18 | 🟢 tail host coverage 已收口 |
| **IPC** | ✅ | ✅ | ✅ | ✅ | 14 | 🟢 host-guarded |
| **PM** | ✅ | ✅ | ✅ | ✅ | 19 | 🟢 host-guarded；功耗待实证 |
| **PID** | ✅ | ✅ | ✅ | ✅ | 20 | 🟢 完善 |
| **ADDC** | ✅ | ✅ | ✅ | ✅ | 24 | 🟢 完善 |

### 🟡 主线可用 / 硬件或人工证据待补 (3 个)

| 组件 | 代码 | 测试 | 文档 | 构建 | 状态 |
|------|------|------|------|------|------|
| **FOTA** | ✅ | ✅ | ✅ | ✅ | 🟡 Host fail-closed contract；board Flash/bootloader/security/hardware pending |
| **Fuel Gauge** | ✅ | ✅ | ✅ | ✅ | 🟡 standalone host-guarded；SMBus/I2C 硬件验证 pending |
| **GUI** | ✅ | ✅ | ✅ | ✅ | 🟡 host-guarded core/widgets/effects/fonts/display-backend；真实屏幕、字体美术/来源审查仍待证据 |

---

## 📈 统计图表

### 测试用例分布

```
OSAL      ████████████████  17
Crypto    ████████████████████████████  28
CLib      █████████████████████     21
DM        ████████████████████████  24
NET       ██████████████████████  22
Device    ██████  6
Sensor    ██████████████████  18
IPC       ██████████████  14
PM        ███████████████████  19
HAL       ███████████  11
PID       ████████████████████  20
ADDC      ████████████████████████  24
Trace     ██████████  10
────────────────────────────────────
总计：234 个测试用例
```

### 组件状态分布

| 状态 | 数量 | 百分比 |
|------|------|--------|
| Host-guarded / 分层证据 | 13 | 81% |
| 硬件、安全或人工证据待补 | 3 | 19% |

这些比例只分类公开组件入口，不是产品完成度。Host/PC/QEMU/compile-only 不构成实板、安全或 production-ready 证据。

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
