# XinYi 文档中心

**最后更新**: 2026-08-17
**文档版本**: 1.2

---

## 🚀 快速开始

- [**构建指南**](BUILD_GUIDE.md) - **重要** - 构建目录说明和常用命令
- [**快速入门**](getting-started/quickstart.md) - 5 分钟上手

---

## 📚 文档导航

### 🎯 当前计划与跟踪

- [全组件状态审计与 Sprint 计划](plans/2026-08-17-component-audit-sprint-plan.md) - 当前组件状态、风险与 Sprint 0–6 范围
- [Sprint 跟踪看板](plans/SPRINT_TRACKER.md) - 当前执行状态、负责人、阻塞和周报入口
- [组件证据台账](validation/component-evidence-matrix.md) - Host、编译、QEMU、实板、安全与发布证据等级
- [组件设计与质量闭环](design/xinyi-component-quality-loop.md) - 每个 slice 的开发、验证、提交和跟进流程
- [历史 30 天路线图](component-roadmap.md) - 仅作演进记录，不作为当前执行清单

### 🏗️ 设计文档 (design/)

- [项目概述](design/XinYi_Project_Overview.md) - 项目愿景、架构、核心组件
- [代码风格指南](design/Code_Style_Design_Guide.md) - 编码规范、命名约定
- [HAL 设计概览](design/HAL_Component_Design_Overview.md) - 硬件抽象层设计
- [OSAL 布局优化](design/osal_layout_optimization.md) - OS 抽象层重构方案
- [测试布局分析](design/test_layout_analysis.md) - 测试目录规范
- [网页文档计划](design/web_documentation_plan.md) - 网页文档规划

### 📖 API 参考 (api/)

- [API 索引](api/index.md) - API 文档入口

### 🔧 工具链 (toolchain/)

- [构建系统分析](toolchain/build_system_analysis.md) - CMake/Kconfig/Makefile 详解

### 🧩 组件文档 (components/)

- [组件总览](components/index.md) - 15 个组件状态
- [OSAL](components/osal/introduction.md) - OS 抽象层
- [HAL](components/hal/introduction.md) - 硬件抽象层
- [Crypto](components/crypto/introduction.md) - 密码学库
- [更多组件...](components/index.md)

### 🚀 快速开始 (getting-started/)

- [快速入门](getting-started/quickstart.md) - 5 分钟上手

### 💻 硬件支持 (hardware/)

- [硬件概览](hardware/index.md) - 支持的 MCU 和外设
- [RTOS 选择指南](hardware/rtos_selection_guide.md) - FreeRTOS/RT-Thread/RTX 对比

### 🛠️ 示例代码 (samples/)

- [示例索引](samples/index.md) - 示例代码集合

### 🤝 贡献指南 (contribute/)

- [贡献指南](contribute/index.md) - 如何贡献代码
- [代码风格](contribute/code-style.md) - 编码规范检查清单

### 📋 编码规范 (rules/)

- [RULEBOOK](rules/RULEBOOK.md) - 开发规则
- [代码风格](rules/100-code_style/xy_code_style.md) - 详细编码规范

### 🗂️ 杂项文档 (misc/)

- [6A 工作流](misc/6A 工作流详解.md) - 开发工作流说明
- [嵌入式产品日志设计](misc/嵌入式产品日志设计.md) - 日志系统设计
- [文档结构](misc/doc_struture.md) - 文档目录说明
- [AI 提示词](misc/ai-prompts/) - AI 辅助开发提示
- [AI 技能](misc/ai-skills/) - AI 技能配置

### 📄 Doxygen 配置 (doxygen/)

- [OSAL Doxygen 配置](doxygen/Doxyfile.osal) - API 文档生成配置

---

## 📊 文档统计

| 类别 | 文档数 | 说明 |
|------|--------|------|
| 设计文档 | 6 | 架构设计、规范 |
| API 参考 | 1 | API 文档索引 |
| 工具链 | 1 | 构建系统 |
| 组件文档 | 15+ | 各组件说明 |
| 快速开始 | 1 | 入门指南 |
| 硬件支持 | 2 | MCU/RTOS |
| 示例代码 | 1 | 示例集合 |
| 贡献指南 | 2 | 贡献说明 |
| 编码规范 | 5+ | 规范文档 |
| 杂项 | 6 | 其他文档 |

---

## 🔍 快速查找

### 新手入门
1. [快速入门](getting-started/quickstart.md)
2. [项目概述](design/XinYi_Project_Overview.md)
3. [示例代码](samples/index.md)

### 开发者
1. [代码风格指南](design/Code_Style_Design_Guide.md)
2. [组件文档](components/index.md)
3. [API 参考](api/index.md)
4. [贡献指南](contribute/index.md)

### 硬件工程师
1. [HAL 设计概览](design/HAL_Component_Design_Overview.md)
2. [硬件支持](hardware/index.md)
3. [RTOS 选择指南](hardware/rtos_selection_guide.md)

### 维护者
1. [编码规范](rules/RULEBOOK.md)
2. [测试布局分析](design/test_layout_analysis.md)
3. [OSAL 布局优化](design/osal_layout_optimization.md)

---

## 📝 文档维护

### 文档位置约定

| 文档类型 | 位置 |
|----------|------|
| 架构设计 | `docs/design/` |
| API 文档 | `docs/api/` 或组件目录 |
| 组件说明 | `docs/components/<组件>/` |
| 快速开始 | `docs/getting-started/` |
| 硬件相关 | `docs/hardware/` |
| 工具链 | `docs/toolchain/` |
| 编码规范 | `docs/rules/` |
| 杂项 | `docs/misc/` |

### 文档更新原则

1. **代码与文档同步** - 代码变更时更新相关文档
2. **使用中文** - 主要文档使用简体中文
3. **版本标注** - 重要文档标注版本和更新日期
4. **示例代码** - 提供可运行的示例代码

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
