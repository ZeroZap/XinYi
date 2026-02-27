# 快速参考卡

## 常用命令

### 查看状态
```bash
# 组件状态
/skill project-manager status

# 任务列表
/skill project-manager tasks

# 项目进度
/skill project-manager progress
```

### 生成报告
```bash
# 日报
/skill project-manager report daily

# 周报
/skill project-manager report weekly
```

### 更新状态
```bash
# 更新组件
/skill project-manager update hal test

# 添加任务
/skill project-manager add-task "Add tests" crypto high
```

---

## 文件位置

| 文件 | 路径 |
|------|------|
| 组件状态 | `COMPONENTS_STATUS.md` |
| 任务跟踪 | `.qwen/skills/project-manager/TASKS.md` |
| 项目总结 | `PROJECT_OPTIMIZATION_SUMMARY.md` |
| 脚本工具 | `.qwen/skills/project-manager/pm.sh` |

---

## 当前优先级

### 🔴 高 (本周截止)
- T001: 规范 clib 测试
- T002: 规范 crypto 测试
- T003: 规范 dm 测试
- T004: 规范 net 测试

### 🟡 中 (本月截止)
- T005: 覆盖率报告
- T006: CI/CD 集成
- T007: 完善 sensor

---

## 项目统计

```
总任务：13
已完成：7 (54%)
进行中：0
待开始：6
```

---

## GitHub Actions

**自动检查**: 每周一 9:00 AM  
**手动触发**: Actions → Project Status Check → Run workflow

**输出**:
- ✅ 组件状态
- 📋 待办任务
- 📊 完成率

---

## 联系人

- 项目问题：提交 Issue
- 贡献代码：提交 PR
- 咨询讨论：Discussions
