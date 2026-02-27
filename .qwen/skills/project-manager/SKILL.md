# XinYi 项目管理 Skill

**名称**: project-manager

**描述**: 跟进 XinYi 项目的定时任务、计划和组件状态

**功能**:
- 组件状态跟踪
- 任务计划管理
- 进度报告生成
- 待办事项提醒

---

## 使用方法

### 1. 查看组件状态

```
/skill project-manager status
/skill project-manager components
```

### 2. 查看任务计划

```
/skill project-manager tasks
/skill project-manager plan
```

### 3. 更新组件状态

```
/skill project-manager update <component> <status>
```

### 4. 生成报告

```
/skill project-manager report daily
/skill project-manager report weekly
```

---

## 数据文件

- `COMPONENTS_STATUS.md` - 组件状态汇总
- `PROJECT_OPTIMIZATION_SUMMARY.md` - 项目优化总结
- `docs/*.md` - 各类分析文档

---

## 任务优先级

| 优先级 | 时间范围 | 任务数 | 说明 |
|--------|----------|--------|------|
| 🔴 高 | 1-2 周 | 4 | 测试规范、构建统一 |
| 🟡 中 | 1 个月 | 5 | CI/CD、覆盖率 |
| 🟢 低 | 3 个月 | 3 | 文档、示例 |

---

## 相关文件

- [组件状态](../COMPONENTS_STATUS.md)
- [优化总结](../PROJECT_OPTIMIZATION_SUMMARY.md)
- [测试布局](../docs/test_layout_analysis.md)
- [构建系统](../docs/build_system_analysis.md)
