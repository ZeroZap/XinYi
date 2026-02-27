# XinYi 任务调度快速参考卡

**打印版本** - 贴在工位随时查看

---

## 一句话指令

```
/skill <角色> <命令> [参数]
```

---

## 角色命令速查

### 📋 项目经理
```
/skill project-manager status     # 看状态
/skill project-manager tasks      # 看任务
/skill project-manager progress   # 看进度
/skill project-manager report     # 生成报告
/skill project-manager add-task "任务描述" 类别 优先级
```

### 🏗️ 架构师
```
/skill architect status           # 架构状态
/skill architect review 组件       # 评审设计
/skill architect specs            # 看规范
```

### 👨‍💻 开发工程师
```
/skill developer tasks            # 开发任务
/skill developer review 组件       # 请求审查
/skill developer specs            # 看规范
```

### 🔍 测试工程师
```
/skill tester status              # 测试状态
/skill tester run 组件            # 运行测试
/skill tester report              # 测试报告
```

### 🔎 代码审计员
```
/skill auditor review 组件         # 审查代码
/skill auditor report             # 审计报告
/skill auditor quality            # 质量报告
```

---

## 常用场景

### 新员工入职
```bash
# 1. 查看项目状态
/skill project-manager status

# 2. 查看任务
/skill project-manager tasks

# 3. 查看规范
/skill developer specs
```

### 每日工作
```bash
# 早会
/skill project-manager report daily

# 晚会
/skill project-manager progress
```

### 新功能开发
```bash
# 创建任务
/skill project-manager add-task "Add I2C driver" hal high

# 开发
/skill developer tasks

# 审查
/skill auditor review hal

# 测试
/skill tester run hal

# 完成
/skill project-manager update hal ok
```

### Bug 修复
```bash
# 记录 Bug
/skill project-manager add-task "Fix I2C timeout" hal high

# 修复
/skill developer tasks

# 验证
/skill tester run hal
```

---

## 脚本命令

### Linux/macOS
```bash
./pm.sh status
./pm.sh tasks
./pm.sh progress
```

### Windows CMD
```cmd
pm.bat status
pm.bat tasks
pm.bat progress
```

### Windows PowerShell
```powershell
.\pm.ps1 status
.\pm.ps1 tasks
.\pm.ps1 progress
```

---

## 优先级说明

| 优先级 | 含义 | 响应时间 |
|--------|------|----------|
| `high` | 紧急 | 立即 |
| `medium` | 普通 | 1-2 天 |
| `low` | 不紧急 | 1 周 |

---

## 状态标记

| 标记 | 含义 |
|------|------|
| ✅ | 完成 |
| ⏳ | 待办 |
| 🔄 | 进行中 |
| 📋 | 基础 |
| ❌ | 缺失 |

---

## 文件位置

```
COMPONENTS_STATUS.md          # 组件状态
.qwen/skills/project-manager/
├── TASKS.md                  # 任务跟踪
├── pm.sh                     # Linux/macOS 脚本
├── pm.bat                    # Windows CMD 脚本
└── pm.ps1                    # Windows PS 脚本
```

---

## 快捷命令

```bash
# 查看帮助
/skill help

# 查看我的任务
/skill developer tasks

# 更新状态
/skill project-manager update 组件 状态

# 生成日报
/skill project-manager report daily

# 生成周报
/skill project-manager report weekly
```

---

## 自然语言示例

```
请安排开发工程师本周完成 HAL 测试

请测试工程师制定 MQTT 组件测试计划

请架构师评审 OSAL 设计

请代码审计员检查 crypto 代码质量
```

---

**提示**: 将此卡片打印贴在工位，随时查看！
