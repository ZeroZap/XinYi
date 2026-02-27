# XinYi 自动化功能总览

**版本**: 1.0  
**最后更新**: 2026-02-28

---

## 功能矩阵

| 功能 | 命令 | 依赖 | 状态 |
|------|------|------|------|
| 🔄 自动备份 | `backup` | 无 (tar 可选) | ✅ 就绪 |
| 📂 文件同步 | `sync` | 无 (rsync 可选) | ✅ 就绪 |
| 💻 项目初始化 | `init-project` | 无 | ✅ 就绪 |
| 📦 安装依赖 | `install-deps` | 包管理器 | ✅ 就绪 |
| 🧪 运行测试 | `run-tests` | CMake, GCC | 🔴 需安装 |
| 📊 CSV 转换 | `csv-convert` | 无 | ✅ 就绪 |
| 📝 文档生成 | `gen-docs` | 无 (Doxygen 可选) | ✅ 就绪 |
| 🔍 代码审查 | `code-review` | 无 (clang-format 可选) | ✅ 就绪 |

---

## 使用示例

### 1. 自动备份 🔄

```bash
# 备份项目
./.qwen/skills/automation/auto.sh backup

# Windows CMD
.qwen\skills\automation\auto.bat backup

# Windows PowerShell
.qwen\skills\automation\auto.ps1 backup
```

**输出**:
```
=== 自动备份 ===
备份目标：/path/to/.backups/backup_20260228_120000
备份：components
备份：docs
备份完成
```

**依赖**: 无 (tar 用于压缩，可选)

---

### 2. 文件同步 📂

```bash
# 同步文件
./.qwen/skills/automation/auto.sh sync /src /dst

# Windows
.qwen\skills\automation\auto.bat sync C:\src C:\dst
```

**依赖**: 无 (rsync 用于高效同步，可选)

---

### 3. 项目初始化 💻

```bash
# 创建新项目
./.qwen/skills/automation/auto.sh init-project MyProject

# 生成的结构
projects/MyProject/
├── CMakeLists.txt
├── README.md
├── src/
│   └── main.c
└── include/
```

**依赖**: 无

---

### 4. 安装依赖 📦

```bash
# 自动安装
./.qwen/skills/automation/auto.sh install-deps
```

**依赖**: 需要包管理器 (apt/yum/brew/choco)

**额外操作**:
- Windows: 需要管理员权限运行 Chocolatey
- Linux: 需要 sudo 权限

---

### 5. 运行测试 🧪

```bash
# 运行所有测试
./.qwen/skills/automation/auto.sh run-tests
```

**依赖**: 
- 🔴 CMake (必需)
- 🔴 GCC/编译器 (必需)

**额外操作**:
```bash
# 如果 cmake 失败，手动安装
sudo apt-get install cmake
# 或
choco install cmake
```

---

### 6. CSV 转换 📊

```bash
# 转换为 JSON
./.qwen/skills/automation/auto.sh csv-convert data.csv json

# 转换为 XML
./.qwen/skills/automation/auto.sh csv-convert data.csv xml
```

**依赖**: 无

**输出**:
```
=== CSV 转换 ===
文件：data.csv
格式：json
转换完成：data.json
```

---

### 7. 文档生成 📝

```bash
# 生成组件文档
./.qwen/skills/automation/auto.sh gen-docs

# 生成 API 文档 (需要 Doxygen)
doxygen docs/doxygen/Doxyfile.osal
```

**依赖**: 无 (Doxygen 用于 API 文档，可选)

**输出**:
```
=== 生成文档 ===
文档索引：docs/generated/components_index.md
```

---

### 8. 代码审查 🔍

```bash
# 审查组件
./.qwen/skills/automation/auto.sh code-review components

# 审查特定目录
./.qwen/skills/automation/auto.sh code-review components/hal
```

**依赖**: 无 (clang-format 用于格式检查，可选)

**输出**:
```
=== 代码审查 ===
审查目标：components
审查报告：/path/to/code_review_20260228.md
```

---

## 依赖安装指南

### 快速检查

```bash
# 运行依赖检查
./.qwen/skills/automation/check_deps.sh
```

### 一键安装

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y cmake gcc make clang-format doxygen

# Windows (Chocolatey)
choco install -y cmake mingw make llvm doxygen.install

# macOS (Homebrew)
brew install cmake llvm doxygen
```

### 详细依赖说明

查看 [DEPENDENCIES.md](DEPENDENCIES.md) 获取完整依赖列表和安装指南。

---

## 自动化工作流

### 每日开发流程

```bash
# 早上：查看状态
./.qwen/skills/project-manager/pm.sh status

# 开发中：运行测试
./.qwen/skills/automation/auto.sh run-tests

# 提交前：代码审查
./.qwen/skills/automation/auto.sh code-review

# 下班前：备份
./.qwen/skills/automation/auto.sh backup
```

### 新项目启动

```bash
# 1. 安装依赖
./.qwen/skills/automation/auto.sh install-deps

# 2. 初始化项目
./.qwen/skills/automation/auto.sh init-project MyProject

# 3. 生成文档
./.qwen/skills/automation/auto.sh gen-docs

# 4. 运行测试
./.qwen/skills/automation/auto.sh run-tests
```

---

## 定时任务

### Linux/macOS (Cron)

```bash
# 每天 9 点备份
crontab -e

# 添加以下行
0 9 * * * /path/to/XinYi/.qwen/skills/automation/auto.sh backup
```

### Windows (Task Scheduler)

1. 打开任务计划程序
2. 创建基本任务
3. 名称：XinYi 每日备份
4. 触发器：每天 9:00
5. 操作：启动程序
   - 程序：`powershell.exe`
   - 参数：`-File ".qwen\skills\automation\auto.ps1" backup`

---

## 故障排除

### 问题 1: 权限不足

**症状**: `Permission denied`

**解决**:
```bash
# 赋予执行权限
chmod +x .qwen/skills/automation/auto.sh
chmod +x .qwen/skills/automation/check_deps.sh
```

### 问题 2: 命令找不到

**症状**: `cmake: command not found`

**解决**:
```bash
# 安装依赖
./.qwen/skills/automation/auto.sh install-deps

# 或手动安装
sudo apt-get install cmake
```

### 问题 3: Windows 路径问题

**症状**: 文件路径错误

**解决**:
- 使用 Git Bash 或 WSL 运行 `.sh` 脚本
- 或使用 `.bat`/`.ps1` 脚本

---

## 相关文件

- [依赖说明](DEPENDENCIES.md) - 详细依赖列表
- [项目依赖检查](check_deps.sh) - 依赖检查脚本
- [调度指南](../DISPATCH_GUIDE.md) - 任务调度指南

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
