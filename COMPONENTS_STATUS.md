# XinYi 组件状态汇总

**最后更新**: 2026-02-28

**文档**: [测试布局分析](docs/test_layout_analysis.md) | [RTOS 选择指南](docs/rtos_selection_guide.md)

---

## 快速导航

- [组件总览](#组件总览)
- [测试状态](#测试状态)
- [详细状态](#详细状态)
- [优先级建议](#优先级建议)

---

## 组件总览

| 组件 | 状态 | 测试 | 文档 | 构建 | 备注 |
|------|------|------|------|------|------|
| **[kernel/osal](#kernelosal)** | ✅ | ✅ | ✅ | ✅ | 支持 4 种后端 |
| **[hal](#hal)** | ✅ | ❌ | ✅ | ✅ | STM32U5 完整实现 |
| **[clib/xy_clib](#clibxy_clib)** | ✅ | ⚠️ | ✅ | ✅ | 测试已去重 |
| **[crypto](#crypto)** | ✅ | ⚠️ | ✅ | ✅ | 测试需规范 |
| **[dm](#dm)** | ⚠️ | ⚠️ | ⚠️ | ✅ | 测试需规范 |
| **[net](#net)** | ⚠️ | ⚠️ | ⚠️ | ✅ | 测试需规范 |
| **[trace](#trace)** | ✅ | ❌ | ✅ | ✅ | 缺少测试 |
| **sensor** | ⚠️ | ⚠️ | ⚠️ | ⚠️ | 需完善 |
| **ipc** | 📋 | ❌ | ⚠️ | ⚠️ | 需完善 |
| **pm** | 📋 | ❌ | ⚠️ | ⚠️ | 需完善 |
| **fota** | 📋 | ❌ | ⚠️ | ⚠️ | 需完善 |
| **gui** | 📋 | ❌ | ⚠️ | ⚠️ | 需完善 |
| **pid** | ✅ | ❌ | ✅ | ✅ | 控制算法 |
| **addc** | ✅ | ❌ | ✅ | ✅ | 数学库 |

**图例**: ✅ 完善 | ⚠️ 进行中 | 📋 基础 | ❌ 缺失

---

## 测试状态

### 测试布局

```
XinYi/
├── tests/                     # ✅ 统一测试入口
│   └── CMakeLists.txt         # 统一构建配置
│
├── third_party/unity/         # ✅ Unity 测试框架
│   ├── unity.c
│   ├── unity.h
│   └── README.md
│
└── components/
    ├── kernel/osal/tests/     ✅ 规范测试目录
    ├── clib/xy_clib/tests/    ⚠️ 需规范
    ├── crypto/tests/          ⚠️ 需规范
    └── ...
```

### 测试统计

| 组件 | 测试目录 | 测试框架 | 用例数 | 状态 |
|------|---------|----------|--------|------|
| **osal** | `tests/` | Unity | 17 | ✅ |
| **hal** | - | - | 0 | ❌ |
| **clib** | `tests/` | Unity | 20+ | ⚠️ |
| **crypto** | `test/` | 自定义 | 5+ | ⚠️ |
| **dm** | 分散 | 自定义 | 3+ | ⚠️ |
| **net** | 分散 | 自定义 | 2+ | ⚠️ |
| **trace** | - | - | 0 | ❌ |

---

## 详细状态

### kernel/osal

**状态**: ✅ 完善

**功能**:
- [x] Bare-metal 后端
- [x] FreeRTOS 后端
- [x] RT-Thread 后端
- [x] CMSIS-RTX 配置
- [x] 软件定时器
- [x] Tick 模块

**测试**:
- [x] Unity 框架
- [x] 17 个测试用例
- [ ] 覆盖率报告

**文档**:
- [x] README.md
- [x] RTOS 选择指南
- [x] 布局优化方案
- [x] Doxygen 配置

**目录**: `components/kernel/osal/`

---

### hal

**状态**: ✅ 完善 (STM32U5)

**功能**:
- [x] GPIO, UART, SPI, I2C
- [x] Timer, PWM, RTC, DMA
- [x] ADC, DAC, Flash
- [x] Watchdog, RNG, EXTI
- [x] I2S, CAN, LP Timer

**测试**:
- [ ] 单元测试缺失
- [ ] 硬件在环测试

**文档**:
- [x] README.md
- [x] 实现指南
- [x] 错误码文档

**目录**: `components/hal/`

---

### clib/xy_clib

**状态**: ✅ 完成

**功能**:
- [x] 字符串操作
- [x] 数学工具
- [x] 数据结构
- [x] 编码转换

**测试**:
- [x] 已去重
- [ ] 需移到 `tests/`

**目录**: `components/clib/xy_clib/`

---

### crypto

**状态**: ✅ 完成

**功能**:
- [x] AES, HMAC, CRC
- [x] Base64, MD5, SHA
- [x] Curve25519, RNG

**测试**:
- [x] 基础测试
- [ ] 需规范到 `tests/`

**目录**: `components/crypto/`

---

### dm

**状态**: ⚠️ 进行中

**功能**:
- [x] EEPROM, Flash, TLV
- [x] NVM, JSON, YAML
- [ ] 磨损均衡

**测试**:
- [ ] 需规范

**目录**: `components/dm/`

---

### net

**状态**: ⚠️ 进行中

**功能**:
- [x] MQTT, Modbus
- [x] AT 命令，ISO7816
- [ ] HTTP, TCP/IP

**测试**:
- [ ] 需规范

**目录**: `components/net/`

---

### trace

**状态**: ✅ 完成

**功能**:
- [x] xy_log 日志
- [x] xy_cmd 命令
- [ ] 性能追踪

**测试**:
- [ ] 缺失

**目录**: `components/trace/`

---

## 优先级建议

### 短期 (1-2 周) ✅ 已完成

- [x] 删除重复测试 (UniTest/component/xy_clib/)
- [x] 移动 Unity 到 third_party/
- [x] 创建 tests/CMakeLists.txt
- [x] 创建测试布局分析文档

### 中期 (1 个月)

- [ ] 规范 clib 测试到 `tests/`
- [ ] 规范 crypto 测试到 `tests/`
- [ ] 规范 dm 测试到 `tests/`
- [ ] 规范 net 测试到 `tests/`

### 长期 (3 个月)

- [ ] 添加 hal 测试
- [ ] 添加 trace 测试
- [ ] 完善 sensor, ipc, pm 等组件

---

## 更新记录

| 日期 | 更新内容 |
|------|----------|
| 2026-02-28 | 初始版本，完成 OSAL 和 HAL 分析 |
| 2026-02-28 | 删除重复测试，创建统一测试框架 |
| 2026-02-28 | 创建 tests/CMakeLists.txt 统一入口 |

---

**维护者**: XinYi Team
**许可证**: Apache License 2.0
