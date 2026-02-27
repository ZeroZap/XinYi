# 🎉 欢迎来到 XinYi 嵌入式框架

**模块化、生产级的嵌入式 C 框架**

[快速开始](getting-started/introduction.md){ .md-button .md-button--primary }
[查看组件](components/index.md){ .md-button }
[API 参考](api/index.md){ .md-button }
[示例代码](samples/index.md){ .md-button }

---

## 📖 什么是 XinYi？

XinYi 是一个模块化、生产级的嵌入式 C 框架，专为资源受限的微控制器系统设计。它提供硬件、通信协议、密码学和 RTOS 管理的统一抽象层，使开发人员能够在多个平台上构建可移植、可维护的嵌入式应用。

### 核心特性

| 特性 | 说明 |
|------|------|
| 🧩 **模块化架构** | 独立、可复用的组件，最小化耦合 |
| 🌍 **跨平台支持** | STM32、RT-Thread、FreeRTOS、裸机环境 |
| 🏭 **生产级代码** | 完善的错误处理、日志系统和文档 |
| 🔧 **硬件抽象层** | UART、SPI、I2C、PWM、RTC、定时器、DMA、GPIO |
| 💤 **多 RTOS 支持** | 统一 OSAL 层支持 FreeRTOS、RT-Thread 和裸机 |
| 📡 **通信协议** | MQTT、Modbus、AT 命令、ISO7816 (SIM 卡) |
| 🔐 **密码学** | AES、HMAC、RNG、CRC、Base64、MD5 |
| 💾 **数据管理** | EEPROM、NOR Flash、TLV 编码、NVM 存储 |

---

## 📊 项目统计

<div class="grid cards" markdown>

-   🧪 **228** 个测试用例
-   📁 **14** 个代码文件
-   📝 **12** 个测试文件
-   🛠️ **7** 个工具脚本
-   📚 **8** 个文档文件
-   ⭐ **100%** 开源

</div>

---

## 🚀 选择你的起点

### 👶 新手入门

第一次使用 XinYi？从这里开始！

-   [项目简介](getting-started/introduction.md)
-   [5 分钟快速上手](getting-started/quickstart.md)
-   [开发环境搭建](getting-started/toolchain.md)

### 🔧 组件开发

想了解具体组件的使用？

-   [组件总览](components/index.md)
-   [OSAL 使用指南](components/osal/introduction.md)
-   [HAL 外设驱动](components/hal/drivers.md)
-   [Crypto 加密算法](components/crypto/algorithms.md)

### 📟 硬件移植

需要移植到新平台？

-   [支持的开发板](hardware/boards.md)
-   [移植指南](hardware/porting.md)
-   [设备树配置](hardware/device-tree.md)

### 🤝 贡献代码

想为 XinYi 做贡献？

-   [贡献指南](contribute/index.md)
-   [代码风格](contribute/code-style.md)
-   [提交 PR](contribute/submit-pr.md)

---

## 🧩 核心组件

<div class="grid cards" markdown>

-   **OSAL** - OS 抽象层，支持 4 种后端
    [:octicons-arrow-right-24: 了解更多](components/osal/introduction.md)

-   **HAL** - 硬件抽象层，STM32 完整实现
    [:octicons-arrow-right-24: 了解更多](components/hal/introduction.md)

-   **Crypto** - 密码学库，AES/SHA/CRC 等
    [:octicons-arrow-right-24: 了解更多](components/crypto/introduction.md)

-   **DM** - 数据管理，EEPROM/Flash/TLV
    [:octicons-arrow-right-24: 了解更多](components/dm/introduction.md)

-   **NET** - 网络协议，MQTT/Modbus/AT
    [:octicons-arrow-right-24: 了解更多](components/net/introduction.md)

-   **Sensor** - 传感器框架
    [:octicons-arrow-right-24: 了解更多](components/sensor/introduction.md)

</div>

[查看所有组件](components/index.md){ .md-button }

---

## 📝 最近更新

| 日期 | 更新内容 |
|------|----------|
| 2026-02-28 | 添加 HAL 测试和 CI/CD 配置 |
| 2026-02-28 | 完善 pid/addc 测试和 fota/gui 基础代码 |
| 2026-02-28 | 完善 sensor/ipc/pm 组件测试和代码 |
| 2026-02-28 | 完善 dm/net 组件单元测试 |
| 2026-02-28 | 完善组件单元测试 (crypto/clib/trace) |

[查看完整更新日志](about/changelog.md){ .md-button }

---

## 🛠️ 快速命令

```bash
# 构建项目
./utils/script/build.sh

# 运行测试
./utils/script/run_tests.sh

# 生成覆盖率报告
./utils/script/coverage.sh

# 格式化代码
./utils/script/format_code.sh
```

---

## 📞 获取帮助

-   📚 [文档索引](index.md)
-   ❓ [常见问题](about/faq.md)
-   💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)
-   🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

## 📄 许可证

本项目采用 **Apache License 2.0** 许可证。

---

*最后更新：2026-02-28 | 文档版本：1.0*
