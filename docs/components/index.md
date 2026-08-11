# 组件状态总览

**最后更新**: 2026-08-11

---

## 📊 组件完成度

### ✅ 完善 / host-guarded 组件 (13 个)

| 组件 | 代码 | 测试 | 文档 | 构建 | 测试用例 | 状态 |
|------|------|------|------|------|---------|------|
| **OSAL** | ✅ | ✅ | ✅ | ✅ | 17 | 🟢 完善 |
| **HAL** | ✅ | ✅ | ✅ | ✅ | 11 | 🟢 完善 |
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
| **FOTA** | ✅ | ✅ | ✅ | ✅ | 🟢 主线可用；bootloader/board NOR 仍待真实硬件记录 |
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
| 🟢 完善 / host-guarded | 13 | 81% |
| 🟡 主线可用 / 硬件或人工证据待补 | 3 | 19% |
| 🟡 进行中 | 0 | 0% |
| 🔴 缺失 | 0 | 0% |

---

## 🔍 组件详情

### OSAL (OS 抽象层)

**目录**: `components/kernel/osal/`

**功能**:
- ✅ Bare-metal 后端
- ✅ FreeRTOS 后端
- ✅ RT-Thread 后端
- ✅ CMSIS-RTX 配置
- ✅ 软件定时器
- ✅ Tick 模块

**文档**:
- [简介](components/osal/introduction.md)
- [快速开始](components/osal/quickstart.md)
- [API 参考](components/osal/api-reference.md)

---

### HAL (硬件抽象层)

**目录**: `components/hal/`

**功能**:
- ✅ GPIO, UART, SPI, I2C
- ✅ Timer, PWM, RTC, DMA
- ✅ ADC, DAC, Flash
- ✅ Watchdog, RNG, EXTI
- ✅ I2S, CAN, LP Timer

**支持平台**:
- STM32U5 (完整实现)
- STM32F1/F4/L4
- HC32, WCH (占位符)
- PC 仿真层

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
