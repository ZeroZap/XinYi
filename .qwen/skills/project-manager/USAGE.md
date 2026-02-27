# XinYi 项目管理工具 - 跨平台使用指南

**版本**: 2.0  
**最后更新**: 2026-02-28

---

## 支持平台

| 平台 | Shell 脚本 | Batch 脚本 | PowerShell |
|------|-----------|-----------|------------|
| **Linux** | ✅ `pm.sh` | ❌ | ❌ |
| **macOS** | ✅ `pm.sh` | ❌ | ❌ |
| **Windows (CMD)** | ❌ | ✅ `pm.bat` | ❌ |
| **Windows (PS)** | ❌ | ❌ | ✅ `pm.ps1` |
| **WSL/Git Bash** | ✅ `pm.sh` | ❌ | ❌ |

---

## 使用方法

### Linux / macOS

```bash
# 赋予执行权限
chmod +x pm.sh

# 查看组件状态
./pm.sh status

# 查看任务列表
./pm.sh tasks

# 显示项目进度
./pm.sh progress

# 生成报告
./pm.sh report daily
./pm.sh report weekly
```

### Windows (CMD)

```cmd
REM 查看组件状态
pm.bat status

REM 查看任务列表
pm.bat tasks

REM 显示项目进度
pm.bat progress

REM 生成报告
pm.bat report daily
pm.bat report weekly
```

### Windows (PowerShell)

```powershell
# 可能需要先允许执行
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

# 查看组件状态
.\pm.ps1 status

# 查看任务列表
.\pm.ps1 tasks

# 显示项目进度
.\pm.ps1 progress

# 生成报告
.\pm.ps1 report daily
.\pm.ps1 report weekly
```

---

## 命令参考

### status - 查看组件状态

显示所有组件的当前状态（完成/进行中/待开始）。

```bash
./pm.sh status
```

**输出示例**:
```
=== XinYi 组件状态 ===

| 组件 | 状态 | 测试 | 文档 | 构建 |
|------|------|------|------|------|
| kernel/osal | ✅ | ✅ | ✅ | ✅ |
| hal | ✅ | ❌ | ✅ | ✅ |

详细状态请查看：COMPONENTS_STATUS.md
```

---

### tasks - 查看任务列表

显示高/中/低优先级任务。

```bash
./pm.sh tasks
```

**输出示例**:
```
=== 项目任务列表 ===

高优先级 (1-2 周):
- [ ] T001: 规范 clib 测试到 tests/

中优先级 (1 个月):
- [ ] T005: 添加覆盖率报告 (gcovr)

低优先级 (3 个月):
- [ ] T010: 添加更多 RTOS 后端支持
```

---

### progress - 显示项目进度

显示完成率和进度条。

```bash
./pm.sh progress
```

**输出示例**:
```
=== 项目进度 ===

组件统计:
  总数：14
  完成：7
  进行中：3
  待开始：4

完成率：50%
[##########----------] 50%
```

---

### report - 生成项目报告

生成日报或周报。

```bash
# 日报
./pm.sh report daily

# 周报
./pm.sh report weekly
```

**输出示例**:
```
=== XinYi 项目报告 ===
日期：2026-02-28 15:30:00
类型：daily

今日完成:
- OSAL 组件完善
- HAL STM32U5 实现
- 测试系统优化

进行中:
- 构建系统统一
- 文档完善
```

---

### update - 更新组件状态

```bash
./pm.sh update <component> <status>
```

**示例**:
```bash
./pm.sh update hal test
```

**可用状态**:
- `ok` 或 `✅` - 完善
- `progress` 或 `⚠️` - 进行中
- `base` 或 `📋` - 基础
- `missing` 或 `❌` - 缺失

---

### add-task - 添加新任务

```bash
./pm.sh add-task "<任务描述>" [类别] [优先级]
```

**示例**:
```bash
./pm.sh add-task "Add HAL unit tests" hal high
```

**优先级**:
- `high` - 高优先级
- `medium` - 中优先级
- `low` - 低优先级

---

### help - 显示帮助信息

```bash
./pm.sh help
```

---

## 故障排除

### 问题 1: 脚本无执行权限 (Linux/macOS)

```bash
chmod +x pm.sh
```

### 问题 2: PowerShell 执行策略限制

```powershell
# 临时允许当前会话
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

# 或允许当前用户
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```

### 问题 3: 文件路径问题 (Windows)

确保使用正确的路径分隔符：
- CMD: `\` 
- PowerShell: `\` 或 `/`
- WSL/Git Bash: `/`

### 问题 4: 中文乱码 (Windows)

```cmd
# 设置代码页为 UTF-8
chcp 65001
```

---

## 自动化集成

### GitHub Actions

```yaml
# .github/workflows/status-check.yml
name: Project Status Check

on:
  schedule:
    - cron: '0 9 * * 1'  # 每周一 9:00 UTC
  workflow_dispatch:

jobs:
  status:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Show Status
        run: ./.qwen/skills/project-manager/pm.sh status
      - name: Show Progress
        run: ./.qwen/skills/project-manager/pm.sh progress
```

### Cron (Linux/macOS)

```bash
# 每周一 9:00 执行
crontab -e

# 添加以下行
0 9 * * 1 cd /path/to/XinYi && ./.qwen/skills/project-manager/pm.sh status >> status.log
```

### Task Scheduler (Windows)

1. 打开任务计划程序
2. 创建基本任务
3. 触发器：每周一次
4. 操作：启动程序
   - 程序：`powershell.exe`
   - 参数：`-File "C:\path\to\pm.ps1" status`

---

## 文件结构

```
.qwen/skills/project-manager/
├── pm.sh                 # Linux/macOS/WSL 脚本
├── pm.bat                # Windows CMD 脚本
├── pm.ps1                # Windows PowerShell 脚本
├── SKILL.md              # Skill 配置
├── README.md             # 使用文档
├── QUICK_REFERENCE.md    # 快速参考
└── TASKS.md              # 任务跟踪文件
```

---

## 相关资源

- [组件状态](../../../COMPONENTS_STATUS.md)
- [任务跟踪](TASKS.md)
- [项目总结](../../../PROJECT_OPTIMIZATION_SUMMARY.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
