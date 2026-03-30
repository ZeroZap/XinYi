# XinYi 文档索引

## 快速导航

### 🚀 快速开始

- [**构建指南**](BUILD_GUIDE.md) - 构建目录说明和常用命令
- [**API 参考**](API_REFERENCE.md) - 完整 API 文档

### 📚 组件文档

- [**HAL (硬件抽象层)**](components/hal/README.md)
  - [GPIO](components/hal/xy_hal_gpio.md)
  - [UART](components/hal/xy_hal_uart.md)
  - [SPI](components/hal/xy_hal_spi.md)
  - [I2C](components/hal/xy_hal_i2c.md)
  - [ADC](components/hal/xy_hal_adc.md)
  - [PWM](components/hal/xy_hal_pwm.md)
  - [Timer](components/hal/xy_hal_timer.md)
  - [RTC](components/hal/xy_hal_rtc.md)
  - [DMA](components/hal/xy_hal_dma.md)
  - [Flash](components/hal/xy_hal_flash.md)
  - [Watchdog](components/hal/xy_hal_wdg.md)
  - [RNG](components/hal/xy_hal_rng.md)

- [**OSAL (操作系统抽象层)**](components/kernel/osal/README.md)
  - [API 参考](components/kernel/osal/API_REFERENCE.md)
  - [后端对比](components/kernel/osal/BACKEND_COMPARISON.md)
  - [使用示例](components/kernel/osal/examples/)

- [**Device (设备框架)**](components/device/README.md)
  - [设备模型](components/device/DEVICE_MODEL.md)
  - [驱动开发](components/device/DRIVER_DEV.md)
  - [总线模型](components/device/BUS_MODEL.md)

- [**CLIB (C 标准库)**](components/clib/xy_clib/README.md)
  - [API 参考](components/clib/xy_clib/xy_clib_apis.md)
  - [使用指南](components/clib/xy_clib/USAGE_GUIDE.md)

- [**Crypto (密码学)**](components/crypto/README.md)
  - [加密算法](components/crypto/ALGORITHMS.md)
  - [API 参考](components/crypto/API_REFERENCE.md)

- [**DM (数据管理)**](components/dm/README.md)
  - [存储管理](components/dm/STORAGE.md)
  - [数据格式](components/dm/FORMAT.md)

- [**Net (网络)**](components/net/README.md)
  - [协议栈](components/net/PROTOCOLS.md)
  - [API 参考](components/net/API_REFERENCE.md)

- [**Trace (跟踪/日志)**](components/trace/README.md)
  - [日志系统](components/trace/xy_log/README.md)
  - [命令系统](components/trace/xy_cmd/README.md)

### 🛠️ 开发工具

- [**智能代理系统**](tools/smart_agent/README.md)
  - [项目经理代理](tools/smart_agent/pm_agent.md)
  - [架构师代理](tools/smart_agent/arch_agent.md)
  - [开发工程师代理](tools/smart_agent/dev_agent.md)
  - [测试工程师代理](tools/smart_agent/test_agent.md)

- [**构建系统**](build-system/README.md)
  - [CMake 配置](build-system/cmake.md)
  - [Kconfig 配置](build-system/kconfig.md)
  - [Makefile 使用](build-system/make.md)

- [**编码规范**](coding-standards/README.md)
  - [C 语言规范](coding-standards/c_coding_standard.md)
  - [命名约定](coding-standards/naming_convention.md)
  - [代码风格](coding-standards/code_style.md)

### 🏗️ 架构文档

- [**整体架构**](architecture/overview.md)
- [**组件模型**](architecture/component_model.md)
- [**设备模型**](architecture/device_model.md)
- [**构建依赖**](architecture/build_dependency.md)
- [**配置系统**](architecture/config_system.md)

### 🧪 测试文档

- [**测试策略**](testing/strategy.md)
- [**单元测试**](testing/unit_test.md)
- [**集成测试**](testing/integration_test.md)
- [**性能测试**](testing/performance_test.md)

### 📖 入门指南

- [**快速开始**](getting-started/quickstart.md)
- [**安装指南**](getting-started/installation.md)
- [**开发教程**](getting-started/tutorial.md)
- [**移植指南**](getting-started/porting_guide.md)

### ❓ 常见问题

- [**FAQ**](faq.md)
- [**故障排除**](troubleshooting.md)
- [**性能调优**](performance_tuning.md)

---

## 文档状态

| 文档 | 完整度 | 更新日期 | 状态 |
|------|--------|----------|------|
| HAL 组件文档 | 90% | 2026-02-28 | ✅ 完善 |
| OSAL 组件文档 | 95% | 2026-02-28 | ✅ 完善 |
| Device 组件文档 | 90% | 2026-02-28 | ✅ 完善 |
| CLIB 组件文档 | 85% | 2026-02-28 | ✅ 完善 |
| Crypto 组件文档 | 70% | 2026-02-28 | ⚠️ 部分完善 |
| DM 组件文档 | 70% | 2026-02-28 | ⚠️ 部分完善 |
| Net 组件文档 | 60% | 2026-02-28 | ⚠️ 需完善 |
| Trace 组件文档 | 70% | 2026-02-28 | ⚠️ 部分完善 |
| Sensor 组件文档 | 30% | 2026-02-28 | ❌ 缺失 |
| IPC 组件文档 | 30% | 2026-02-28 | ❌ 缺失 |
| PM 组件文档 | 30% | 2026-02-28 | ❌ 缺失 |

---

## 贡献文档

### 编写规范

1. 使用标准 Markdown 格式
2. 遵循 [编码规范](coding-standards/code_style.md)
3. 提供实际代码示例
4. 保持文档与代码同步

### 模板

```markdown
# <组件名称> 文档

## 概述

<组件功能概述>

## API 参考

### 函数

`xy_<module>_<function>(<parameters>)`

<函数说明>

**参数**:
- `param1`: 参数说明

**返回值**:
- `XY_OK`: 成功
- `XY_ERROR_*`: 错误码

## 使用示例

```c
// 代码示例
```

## 配置选项

<配置选项说明>
```

---

## 联系与支持

- **问题反馈**: [Issues](https://github.com/zerozap/issues)
- **文档纠错**: [Pull Requests](https://github.com/zerozap/pulls)
- **技术支持**: zerozap2020@gmail.com

---

**最后更新**: 2026-02-28  
**维护者**: XinYi Team  
**许可证**: Apache License 2.0
