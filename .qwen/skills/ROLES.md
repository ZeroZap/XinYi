# XinYi 角色技能矩阵

**版本**: 1.0  
**最后更新**: 2026-02-28

---

## 角色总览

| 角色 | Skill 名称 | 职责 | 使用场景 |
|------|-----------|------|----------|
| **项目经理** | project-manager | 任务跟踪、进度管理 | 项目规划、进度跟踪 |
| **架构师** | architect | 架构设计、技术决策 | 系统设计、技术评审 |
| **开发工程师** | developer | 模块开发、单元测试 | 功能开发、Bug 修复 |
| **测试工程师** | tester | 测试计划、质量保障 | 测试执行、缺陷跟踪 |
| **代码审计员** | auditor | 代码审查、质量审计 | 代码评审、规范检查 |

---

## 使用方式

### 命令行方式

```bash
# 项目管理
/skill project-manager status
/skill project-manager tasks

# 架构设计
/skill architect status
/skill architect review hal

# 开发
/skill developer tasks
/skill developer review crypto

# 测试
/skill tester status
/skill tester run hal

# 审计
/skill auditor review osal
/skill auditor report
```

### 脚本方式

```bash
# Linux/macOS
./.qwen/skills/project-manager/pm.sh status

# Windows CMD
.qwen\skills\project-manager\pm.bat status

# Windows PowerShell
.qwen\skills\project-manager\pm.ps1 status
```

---

## 角色协作流程

```
┌─────────────┐
│  项目经理   │ 分配任务
│  (PM)       │──────────┐
└─────────────┘          │
                         ▼
┌─────────────┐    ┌─────────────┐
│  架构师     │    │ 开发工程师  │ 提交审查
│  (Arch)     │    │  (Dev)      │──────────┐
└─────────────┘    └─────────────┘          │
      │                                     ▼
      │                              ┌─────────────┐
      │                              │  代码审计员  │ 审查通过
      │                              │  (Auditor)  │──────────┐
      │                              └─────────────┘          │
      │                                     │                 ▼
      │                              ┌─────────────┐    ┌─────────────┐
      └─────────────────────────────▶│  测试工程师  │    │  合并代码   │
                                     │  (Tester)   │────▶│  (Merge)    │
                                     └─────────────┘    └─────────────┘
```

---

## 工作流程示例

### 新功能开发

1. **项目经理** 创建任务
   ```bash
   /skill project-manager add-task "Add I2C driver" hal high
   ```

2. **架构师** 设计接口
   ```bash
   /skill architect review hal/i2c
   ```

3. **开发工程师** 实现功能
   ```bash
   /skill developer tasks
   # 开发完成后
   /skill developer review hal/i2c
   ```

4. **代码审计员** 审查代码
   ```bash
   /skill auditor review hal/i2c
   ```

5. **测试工程师** 执行测试
   ```bash
   /skill tester run hal/i2c
   ```

6. **项目经理** 更新状态
   ```bash
   /skill project-manager update hal/i2c ok
   ```

---

## 技能矩阵

### 项目经理

| 技能 | 工具 | 熟练度要求 |
|------|------|------------|
| 任务管理 | pm.sh/pm.bat/pm.ps1 | 精通 |
| 进度跟踪 | COMPONENTS_STATUS.md | 精通 |
| 报告生成 | report 命令 | 熟练 |
| 风险管理 | TASKS.md | 熟练 |

### 架构师

| 技能 | 工具 | 熟练度要求 |
|------|------|------------|
| 系统设计 | 架构文档 | 精通 |
| 技术选型 | ADR | 精通 |
| 规范制定 | xy_code_style.md | 熟练 |
| 代码评审 | auditor 协作 | 熟练 |

### 开发工程师

| 技能 | 工具 | 熟练度要求 |
|------|------|------------|
| C 语言 | GCC/Clang | 精通 |
| 代码规范 | clang-format | 熟练 |
| 单元测试 | Unity | 熟练 |
| 调试 | GDB/J-Link | 熟练 |

### 测试工程师

| 技能 | 工具 | 熟练度要求 |
|------|------|------------|
| 测试设计 | 测试模板 | 精通 |
| 自动化 | Unity/CTest | 熟练 |
| 覆盖率 | gcov | 熟练 |
| 缺陷跟踪 | Issue | 熟练 |

### 代码审计员

| 技能 | 工具 | 熟练度要求 |
|------|------|------------|
| 代码审查 | 审查清单 | 精通 |
| 静态分析 | clang-tidy | 熟练 |
| 规范检查 | clang-format | 熟练 |
| 安全审计 | cppcheck | 熟练 |

---

## 快速参考

### 常用命令

```bash
# 查看我的任务
/skill project-manager tasks

# 查看组件状态
/skill project-manager status

# 查看项目进度
/skill project-manager progress

# 生成报告
/skill project-manager report daily
```

### 文件位置

| 文件 | 路径 |
|------|------|
| 组件状态 | `COMPONENTS_STATUS.md` |
| 任务跟踪 | `.qwen/skills/project-manager/TASKS.md` |
| 项目总结 | `PROJECT_OPTIMIZATION_SUMMARY.md` |
| 脚本工具 | `.qwen/skills/project-manager/pm.*` |

---

## 角色切换

在对话中指定角色：

```
作为架构师，请评审 HAL 组件的接口设计

作为测试工程师，请制定 I2C 驱动测试计划

作为代码审计员，请审查 OSAL 组件的代码质量
```

---

## 相关文件

- [项目管理](project-manager/README.md)
- [架构师](architect/SKILL.md)
- [开发工程师](developer/SKILL.md)
- [测试工程师](tester/SKILL.md)
- [代码审计员](auditor/SKILL.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
