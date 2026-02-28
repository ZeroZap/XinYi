# Rules 规则文档索引

**最后更新**: 2026-02-28

---

## 📋 概述

本目录包含 XinYi 项目的所有开发规则和编码规范。

---

## 📚 规则文档结构

```
docs/rules/
├── RULEBOOK.md                    # 开发规则总览（中英文）
├── RULEBOOK.en.md                 # 英文版规则
├── RULEBOOK.zh-CN.md              # 中文版规则
├── repo_guide.md                  # 仓库阅读分析与代码指导
├── review_checklist.md            # AI 审查检查点
│
├── 000-project-context/           # 项目上下文（待完善）
├── 002-naming-conventoon/         # 命名约定（待完善）
├── 100-code_style/                # 代码风格 ✅
│   ├── xy_code_style.md           # 详细编码规范
│   ├── .clang-format              # 格式化配置
│   ├── doxygen.txt                # Doxygen 配置
│   ├── function_style.md          # 函数风格
│   └── ccg.md                     # 条件编译指南
│
├── 200-memory-safety/             # 内存安全（待完善）
├── 300-security-rules/            # 安全规则
│   ├── safety_overview.md         # 安全概览
│   └── Zephyr RTOS：Security 与 Safety 的区别与实现.md
│
├── 400-unit_test/                 # 单元测试（待完善）
└── 500-code_pr/                   # 代码 PR
    ├── code_pr.md                 # PR 指南
    └── code-commit.md             # 提交规范
```

---

## 🔑 核心规则文档

### 1. RULEBOOK.md - 开发规则总览

**内容**:
- 编码标准（引用 xy_code_style.md）
- 日志和调试规范
- 错误处理约定
- 文档要求
- 提交流程

**适用范围**: 所有代码贡献（人类和 AI）

**关键要求**:
- ✅ 使用 C99 标准
- ✅ 4 空格缩进
- ✅ 使用 `xy_log_*()` 代替 `printf()`
- ✅ 所有公共函数有 Doxygen 注释
- ✅ 提交前使用 clang-format 格式化

### 2. repo_guide.md - 仓库指导

**内容**:
- 仓库定位与目标
- 仓库结构说明
- 构建系统与配置体系
- 组件设计与依赖管理
- 错误码和返回值约定
- 文档规范
- 提交流程与 Review 清单

**适用场景**:
- 新贡献者入门
- 代码 Review 参考
- 重构与新增模块指导

### 3. review_checklist.md - AI 审查清单

**自动检查项**:
- 🔧 代码质量（函数长度、复杂度、嵌套深度）
- 📝 文档完整性
- 🔒 安全性（禁用函数、指针检查、内存安全）
- ⚡ 性能（内存分配、循环优化）

**输出格式**: JSON 格式审查报告

---

## 📖 代码风格 (100-code_style)

### xy_code_style.md - 详细编码规范

**章节**:
1. 一般规则（语言标准、空格、命名）
2. 变量（声明、使用、指针、布尔）
3. 函数（声明、实现、参数）
4. 结构体/枚举/类型定义
5. 宏和预处理器
6. 注释
7. 复合语句
8. Switch 语句
9. 文档 (Doxygen)
10. 头文件和源文件
11. 快速参考检查清单

### 辅助文档

| 文档 | 说明 |
|------|------|
| `function_style.md` | 函数风格规范 |
| `ccg.md` | 条件编译指南 |
| `ccg-条件编译.md` | 条件编译中文版 |
| `doxygen.txt` | Doxygen 配置文件 |
| `.clang-format` | Clang 格式化配置 |

---

## 🔒 安全规则 (300-security-rules)

| 文档 | 说明 |
|------|------|
| `safety_overview.md` | 安全概览 |
| `Zephyr RTOS：Security 与 Safety 的区别与实现.md` | Safety 与 Security 区别 |

---

## 🤝 代码 PR (500-code_pr)

| 文档 | 说明 |
|------|------|
| `code_pr.md` | Pull Request 指南 |
| `code-commit.md` | 提交信息规范 |

---

## ⚠️ 待完善目录

| 目录 | 状态 | 建议内容 |
|------|------|---------|
| `000-project-context/` | ❌ 空白 | 项目背景、架构图、术语表 |
| `002-naming-conventoon/` | ❌ 空白 | 命名约定详细说明 |
| `200-memory-safety/` | ❌ 空白 | 内存安全指南、最佳实践 |
| `400-unit_test/` | ❌ 空白 | 单元测试规范、示例 |

---

## 📊 文档完整性

| 类别 | 完成 | 缺失 | 完成率 |
|------|------|------|--------|
| 核心规则 | 3 | 0 | 100% |
| 代码风格 | 7 | 0 | 100% |
| 安全规则 | 2 | 0 | 100% |
| 代码 PR | 2 | 0 | 100% |
| 项目上下文 | 0 | 1 | 0% |
| 命名约定 | 0 | 1 | 0% |
| 内存安全 | 0 | 1 | 0% |
| 单元测试 | 0 | 1 | 0% |
| **总计** | **14** | **4** | **78%** |

---

## 🔗 相关文档

- [贡献指南](../contribute/index.md)
- [提交 PR 指南](../contribute/submit-pr.md)
- [代码风格指南](../design/Code_Style_Design_Guide.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
