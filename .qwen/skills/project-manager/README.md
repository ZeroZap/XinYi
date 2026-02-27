# XinYi 项目管理 Agent

**版本**: 1.0.0  
**最后更新**: 2026-02-28

---

## 功能概述

此 Agent/Skill 用于跟进 XinYi 项目的定时任务和计划，提供：

1. **组件状态跟踪** - 实时查看各组件完成状态
2. **任务计划管理** - 短期/中期/长期任务跟踪
3. **进度报告生成** - 日报/周报自动生成
4. **待办事项提醒** - 截止日期提醒

---

## 使用方式

### 方式 1: 使用 Skill 命令

```bash
# 查看组件状态
/skill project-manager status

# 查看任务列表
/skill project-manager tasks

# 查看项目进度
/skill project-manager progress

# 生成报告
/skill project-manager report daily
/skill project-manager report weekly
```

### 方式 2: 使用 Shell 脚本

```bash
cd .qwen/skills/project-manager/

# 查看状态
./pm.sh status

# 查看任务
./pm.sh tasks

# 查看进度
./pm.sh progress

# 生成报告
./pm.sh report daily
```

### 方式 3: GitHub Actions 自动检查

**触发方式**:
- 每周一上午 9 点自动运行
- 手动触发 Workflow

**输出**:
- 组件状态摘要
- 待完成任务列表
- 完成率统计

---

## 文件结构

```
.qwen/skills/project-manager/
├── SKILL.md              # Skill 说明文档
├── pm.sh                 # 项目管理脚本
├── TASKS.md              # 任务跟踪文件
└── README.md             # 本文档

.github/workflows/
└── status-check.yml      # 自动状态检查

项目根目录/
├── COMPONENTS_STATUS.md  # 组件状态汇总
├── PROJECT_OPTIMIZATION_SUMMARY.md  # 项目优化总结
└── docs/                 # 各类分析文档
```

---

## 任务优先级定义

| 优先级 | 时间范围 | 颜色标记 | 说明 |
|--------|----------|----------|------|
| 🔴 高 | 1-2 周 | 红色 | 必须完成，影响项目进度 |
| 🟡 中 | 1 个月 | 黄色 | 重要，可适度延后 |
| 🟢 低 | 3 个月 | 绿色 | 改进性质，长期规划 |

---

## 状态标记说明

| 标记 | 含义 | 说明 |
|------|------|------|
| ✅ | 完成 | 任务已完成 |
| ⏳ | 待办 | 任务等待开始 |
| 🔄 | 进行中 | 任务正在执行 |
| 📋 | 基础 | 基础完成，需完善 |
| ❌ | 缺失 | 功能缺失 |

---

## 自动化规则

### 状态检查

- **频率**: 每周一上午 9 点
- **内容**: 
  - 组件状态统计
  - 待完成任务列表
  - 完成率计算

### 报告生成

- **日报**: 每日完成和进行中任务
- **周报**: 周进度和下周计划

### 提醒规则

- **截止日期前 3 天**: 黄色提醒
- **截止日期前 1 天**: 红色提醒
- **已过期**: 创建 GitHub Issue

---

## 示例输出

### 组件状态

```
=== XinYi 组件状态 ===

| 组件 | 状态 | 测试 | 文档 | 构建 |
|------|------|------|------|------|
| kernel/osal | ✅ | ✅ | ✅ | ✅ |
| hal | ✅ | ❌ | ✅ | ✅ |
| clib/xy_clib | ✅ | ⚠️ | ✅ | ✅ |

完成率：54%
```

### 任务列表

```
=== 项目任务列表 ===

🔴 高优先级 (1-2 周):
- [ ] T001: 规范 clib 测试到 tests/
- [ ] T002: 规范 crypto 测试到 tests/

🟡 中优先级 (1 个月):
- [ ] T005: 添加覆盖率报告 (gcovr)
- [ ] T006: 集成 CI/CD

🟢 低优先级 (3 个月):
- [ ] T010: 添加更多 RTOS 后端支持
```

---

## 扩展功能

### 添加新组件

编辑 `COMPONENTS_STATUS.md`:

```markdown
| 新组件 | 📋 | ❌ | ⚠️ | ⚠️ | 说明 |
```

### 添加新任务

编辑 `.qwen/skills/project-manager/TASKS.md`:

```markdown
| T014 | 任务描述 | ⏳ 待办 | - | 2026-06-01 |
```

### 自定义报告

修改 `pm.sh` 中的 `generate_report()` 函数。

---

## 故障排除

### 问题 1: 脚本无执行权限

```bash
chmod +x .qwen/skills/project-manager/pm.sh
```

### 问题 2: 状态文件不存在

```bash
# 检查文件路径
ls -la COMPONENTS_STATUS.md

# 如果不存在，创建模板
touch COMPONENTS_STATUS.md
```

### 问题 3: GitHub Actions 不运行

检查 `.github/workflows/status-check.yml` 配置：
- Workflow 是否启用
- Cron 表达式是否正确
- 权限设置

---

## 相关资源

- [组件状态汇总](../../COMPONENTS_STATUS.md)
- [项目优化总结](../../PROJECT_OPTIMIZATION_SUMMARY.md)
- [测试布局分析](../../docs/test_layout_analysis.md)
- [构建系统分析](../../docs/build_system_analysis.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
