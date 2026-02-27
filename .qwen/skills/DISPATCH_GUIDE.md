# XinYi 任务调度指南

**版本**: 1.0  
**最后更新**: 2026-02-28

---

## 快速开始

### 方式 1: 使用 Skill 命令（推荐）

```bash
# 查看可用角色
/skill help

# 分配任务给特定角色
/skill developer tasks
/skill tester run hal
/skill auditor review crypto
```

### 方式 2: 使用脚本

```bash
# Linux/macOS
./.qwen/skills/project-manager/pm.sh status

# Windows CMD
.qwen\skills\project-manager\pm.bat status

# Windows PowerShell
.qwen\skills\project-manager\pm.ps1 status
```

### 方式 3: 自然语言指令

```
请安排开发工程师本周完成 HAL 组件的单元测试
请架构师评审一下 OSAL 的设计
请测试工程师制定 MQTT 组件的测试计划
```

---

## 角色命令手册

### 项目经理 (project-manager)

| 命令 | 说明 | 示例 |
|------|------|------|
| `status` | 查看组件状态 | `/skill project-manager status` |
| `tasks` | 查看任务列表 | `/skill project-manager tasks` |
| `progress` | 查看项目进度 | `/skill project-manager progress` |
| `report` | 生成报告 | `/skill project-manager report daily` |
| `update` | 更新状态 | `/skill project-manager update hal test` |
| `add-task` | 添加任务 | `/skill project-manager add-task "Add I2C driver" hal high` |

### 架构师 (architect)

| 命令 | 说明 | 示例 |
|------|------|------|
| `status` | 查看架构状态 | `/skill architect status` |
| `decisions` | 查看架构决策 | `/skill architect decisions` |
| `review` | 请求评审 | `/skill architect review hal` |
| `specs` | 查看规范 | `/skill architect specs` |
| `report` | 生成报告 | `/skill architect report` |

### 开发工程师 (developer)

| 命令 | 说明 | 示例 |
|------|------|------|
| `tasks` | 查看开发任务 | `/skill developer tasks` |
| `specs` | 查看编码规范 | `/skill developer specs` |
| `review` | 请求代码审查 | `/skill developer review crypto` |
| `report` | 生成开发报告 | `/skill developer report` |

### 测试工程师 (tester)

| 命令 | 说明 | 示例 |
|------|------|------|
| `status` | 查看测试状态 | `/skill tester status` |
| `cases` | 查看测试用例 | `/skill tester cases` |
| `run` | 运行测试 | `/skill tester run hal` |
| `report` | 生成测试报告 | `/skill tester report` |

### 代码审计员 (auditor)

| 命令 | 说明 | 示例 |
|------|------|------|
| `review` | 代码审查 | `/skill auditor review osal` |
| `report` | 审计报告 | `/skill auditor report` |
| `specs` | 查看规范 | `/skill auditor specs` |
| `quality` | 质量报告 | `/skill auditor quality` |

---

## 典型工作流

### 场景 1: 新功能开发

```bash
# 1. 项目经理创建任务
/skill project-manager add-task "Add I2C driver" hal high

# 2. 架构师设计接口
/skill architect review hal/i2c

# 3. 开发工程师实现
/skill developer tasks
# 开发完成后请求审查
/skill developer review hal/i2c

# 4. 代码审计员审查
/skill auditor review hal/i2c

# 5. 测试工程师测试
/skill tester run hal/i2c

# 6. 项目经理更新状态
/skill project-manager update hal/i2c ok
```

### 场景 2: Bug 修复

```bash
# 1. 项目经理记录 Bug
/skill project-manager add-task "Fix I2C timeout bug" hal high

# 2. 开发工程师修复
/skill developer tasks
# 修复后
/skill developer review hal

# 3. 测试工程师验证
/skill tester run hal

# 4. 项目经理关闭 Bug
/skill project-manager update hal ok
```

### 场景 3: 代码审查

```bash
# 直接请求审计员审查
/skill auditor review crypto

# 查看审查报告
/skill auditor report

# 根据建议修改后重新审查
/skill developer review crypto
/skill auditor review crypto
```

### 场景 4: 项目汇报

```bash
# 项目经理生成周报
/skill project-manager report weekly

# 查看整体进度
/skill project-manager progress

# 查看组件状态
/skill project-manager status
```

---

## 调度模板

### 每日站会

```bash
# 查看昨日完成
/skill project-manager report daily

# 查看今日任务
/skill project-manager tasks

# 查看项目进度
/skill project-manager progress
```

### 每周规划

```bash
# 项目经理生成周报
/skill project-manager report weekly

# 查看高优先级任务
/skill project-manager tasks | grep "高优先级"

# 分配本周任务
/skill project-manager add-task "Complete HAL tests" hal high
/skill project-manager add-task "Fix crypto bugs" crypto medium
```

### 月度审查

```bash
# 架构师生成架构报告
/skill architect report

# 审计员生成质量报告
/skill auditor quality

# 测试工程师生成测试报告
/skill tester report

# 项目经理汇总
/skill project-manager progress
```

---

## 自动化调度

### GitHub Actions

```yaml
# .github/workflows/schedule.yml
name: Daily Status Check

on:
  schedule:
    - cron: '0 9 * * 1-5'  # 工作日 9:00
  workflow_dispatch:

jobs:
  status:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Show Status
        run: ./.qwen/skills/project-manager/pm.sh status
      - name: Show Tasks
        run: ./.qwen/skills/project-manager/pm.sh tasks
```

### Cron (Linux/macOS)

```bash
# 每日 9 点检查状态
crontab -e

# 添加以下行
0 9 * * 1-5 cd /path/to/XinYi && ./.qwen/skills/project-manager/pm.sh status >> daily.log
```

### Task Scheduler (Windows)

1. 打开任务计划程序
2. 创建基本任务
3. 名称：XinYi 每日检查
4. 触发器：工作日 9:00
5. 操作：启动程序
   - 程序：`powershell.exe`
   - 参数：`-File ".qwen\skills\project-manager\pm.ps1" status`

---

## 自然语言指令

### 直接分配任务

```
请安排开发工程师在本周内完成 HAL 组件的单元测试，优先级高

请测试工程师制定 MQTT 组件的测试计划，包括单元测试和集成测试

请架构师评审一下 OSAL 组件的接口设计是否合理

请代码审计员检查 crypto 组件的代码质量，重点关注安全性
```

### 查询状态

```
查看当前项目进度

显示所有待完成的任务

HAL 组件的测试覆盖率是多少

crypto 组件的代码审查通过了吗
```

### 生成报告

```
生成今日项目日报

生成本周项目周报

生成 HAL 组件的质量报告

生成测试覆盖率报告
```

---

## 任务优先级

| 优先级 | 响应时间 | 示例 |
|--------|----------|------|
| 🔴 高 | 立即 | Bug 修复、阻塞问题 |
| 🟡 中 | 1-2 天 | 新功能开发 |
| 🟢 低 | 1 周 | 文档完善、代码优化 |

### 设置优先级

```bash
# 高优先级
/skill project-manager add-task "Fix critical bug" crypto high

# 中优先级
/skill project-manager add-task "Add unit tests" hal medium

# 低优先级
/skill project-manager add-task "Update docs" docs low
```

---

## 任务跟踪

### 查看任务状态

```bash
# 所有任务
/skill project-manager tasks

# 高优先级任务
/skill project-manager tasks | grep "高优先级"

# 我的任务
/skill developer tasks
```

### 更新任务状态

```bash
# 开始任务
/skill project-manager update hal progress

# 完成任务
/skill project-manager update hal ok

# 添加备注
/skill project-manager add-task "Need more time for HAL tests" hal medium
```

---

## 协作技巧

### 1. 明确角色职责

```
✅ 好：请开发工程师实现 I2C 驱动
❌ 差：请测试工程师实现 I2C 驱动
```

### 2. 设定明确期限

```
✅ 好：请在本周五前完成 HAL 测试
❌ 差：请尽快完成 HAL 测试
```

### 3. 提供足够信息

```
✅ 好：请修复 I2C 组件在 STM32U5 上的超时问题，复现步骤...
❌ 差：请修复 I2C Bug
```

### 4. 及时反馈

```bash
# 任务完成后更新状态
/skill project-manager update hal ok

# 审查通过后通知
/skill auditor review hal  # 审查通过，可合并
```

---

## 常见问题

### Q: 如何查看谁在做什么任务？

```bash
/skill project-manager tasks
```

### Q: 如何紧急插队任务？

```bash
/skill project-manager add-task "紧急：修复生产 Bug" crypto high
```

### Q: 如何查看历史任务？

```bash
cat .qwen/skills/project-manager/TASKS.md
```

### Q: 如何批量分配任务？

```bash
# 创建任务列表文件
cat >> tasks_batch.txt << EOF
Add UART driver hal high
Add SPI driver hal high
Add I2C driver hal medium
EOF

# 批量导入（需要脚本支持）
```

---

## 相关文件

- [项目管理](project-manager/README.md)
- [角色说明](ROLES.md)
- [组件状态](../../COMPONENTS_STATUS.md)
- [任务跟踪](.qwen/skills/project-manager/TASKS.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
