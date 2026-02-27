# 关于 XinYi

XinYi 嵌入式框架的项目信息和常见问题。

---

## 📖 项目简介

**XinYi** 是一个模块化、生产级的嵌入式 C 框架，专为资源受限的微控制器系统设计。

### 项目目标

- 🧩 **模块化架构** - 独立、可复用的组件
- 🌍 **跨平台支持** - STM32、RT-Thread、FreeRTOS、裸机
- 🏭 **生产级代码** - 完善的错误处理、日志和文档
- 📚 **开发者友好** - 统一 API、丰富示例、清晰规范

### 核心组件

| 组件 | 说明 | 状态 |
|------|------|------|
| OSAL | OS 抽象层 | ✅ 完善 |
| HAL | 硬件抽象层 | ✅ 完善 |
| Crypto | 密码学库 | ✅ 完善 |
| CLib | 自定义 C 库 | ✅ 完善 |
| DM | 数据管理 | ✅ 完善 |
| NET | 网络协议 | ✅ 完善 |
| Sensor | 传感器框架 | ✅ 完善 |
| IPC | 进程间通信 | ✅ 完善 |
| PM | 电源管理 | ✅ 完善 |
| PID | 控制算法 | ✅ 完善 |
| ADDC | ADC/DAC 辅助 | ✅ 完善 |
| FOTA | 固件升级 | 📋 基础 |
| GUI | 图形界面 | 📋 基础 |

---

## 📊 项目统计

| 指标 | 数量 | 更新日期 |
|------|------|---------|
| 测试用例 | 228 个 | 2026-02-28 |
| 代码文件 | 14 个 | 2026-02-28 |
| 测试文件 | 12 个 | 2026-02-28 |
| 工具脚本 | 7 个 | 2026-02-28 |
| 文档文件 | 15+ 个 | 2026-02-28 |
| Git 提交 | 17+ 个 | 2026-02-28 |

---

## 🏗️ 架构层次

```
┌─────────────────────────────────────────┐
│           应用层 (Projects)              │
│  电源银行 | 电烙铁 | USB 桥接器 | ...      │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│           组件层 (Components)            │
│  Crypto | Network | Sensor | Data Mgmt  │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│        OS 抽象层 (OSAL)                  │
│  FreeRTOS | RT-Thread | 裸机 | RTX      │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│       硬件抽象层 (HAL)                   │
│  UART | SPI | I2C | GPIO | Timer | ...  │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│          平台层 (MCU SDK)                │
│  STM32 | HC32 | WCH | PC Sim            │
└─────────────────────────────────────────┘
```

---

## 📜 许可证

本项目采用 **Apache License 2.0** 许可证。

### 许可证要点

- ✅ 可商用
- ✅ 可修改
- ✅ 可分发
- ✅ 可专利使用
- ⚠️ 需保留许可证和版权声明
- ⚠️ 需说明修改内容

---

## 👥 团队信息

**维护者**: XinYi Team  
**官网**: [xinyi.zerovoid.com](https://xinyi.zerovoid.com/)  
**GitHub**: [ZeroZap/XinYi](https://github.com/ZeroZap/XinYi)

---

## ❓ 常见问题 (FAQ)

### 一般问题

**Q: XinYi 适合什么场景？**

A: XinYi 适合资源受限的嵌入式系统，特别是需要跨平台移植的项目。典型应用包括：
- 物联网设备
- 工业控制
- 消费电子
- 传感器节点

**Q: 支持哪些 MCU？**

A: 目前主要支持：
- STM32 系列（完整支持）
- HC32 系列（占位符）
- WCH 系列（占位符）
- PC 仿真（开发测试）

**Q: 支持哪些 RTOS？**

A: 支持 4 种后端：
- Bare-metal（裸机）
- FreeRTOS
- RT-Thread
- CMSIS-RTX

**Q: 如何开始使用？**

A: 参考 [快速开始](../getting-started/quickstart.md) 指南。

---

### 技术问题

**Q: 如何配置项目？**

A: 使用 CMake 或 Kconfig：
```bash
# CMake
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON

# Kconfig（需要 kconfig-frontends）
make menuconfig
```

**Q: 如何运行测试？**

A: 
```bash
cd build
make test
# 或
ctest --output-on-failure
```

**Q: 如何生成文档？**

A:
```bash
# 安装 MkDocs
pip install mkdocs mkdocs-material

# 本地预览
cd docs
mkdocs serve

# 访问 http://localhost:8000
```

**Q: 代码风格要求？**

A: 参考 [代码风格规范](../contribute/code-style.md)。主要要求：
- C99 标准
- 4 空格缩进
- 小写命名 + 下划线
- Doxygen 注释

**Q: 如何贡献代码？**

A: 参考 [贡献指南](../contribute/index.md)。

---

### 故障排除

**Q: 编译失败怎么办？**

A: 检查以下几点：
1. 确认 CMake 版本 >= 3.10
2. 确认子模块已初始化：`git submodule update --init`
3. 清理构建目录重新构建

**Q: 测试失败怎么办？**

A: 检查以下几点：
1. 确认 Unity 框架已正确引入
2. 查看测试输出日志
3. 在 GitHub 报告问题

**Q: 如何调试？**

A: 
```bash
# 启用调试构建
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 使用 GDB
gdb ./build/your_target

# 启用详细日志
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_VERBOSE
```

---

## 📞 联系方式

| 渠道 | 链接 |
|------|------|
| GitHub | https://github.com/ZeroZap/XinYi |
| Issues | https://github.com/ZeroZap/XinYi/issues |
| Discussions | https://github.com/ZeroZap/XinYi/discussions |

---

## 📚 相关资源

- [快速开始](../getting-started/quickstart.md)
- [组件文档](../components/index.md)
- [API 参考](../api/index.md)
- [贡献指南](../contribute/index.md)
- [代码风格](../contribute/code-style.md)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
