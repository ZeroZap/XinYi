# XinYi 跨平台部署指南

**版本**: 1.0  
**最后更新**: 2026-02-28

---

## 平台兼容性总览

| 功能 | Windows | Linux | macOS | WSL | 备注 |
|------|---------|-------|-------|-----|------|
| **项目管理** | ✅ | ✅ | ✅ | ✅ | 全平台兼容 |
| pm.bat | ✅ CMD | ❌ | ❌ | ⚠️ | Windows 专用 |
| pm.ps1 | ✅ PS | ⚠️ | ⚠️ | ⚠️ | PowerShell |
| pm.sh | ❌ | ✅ | ✅ | ✅ | Bash |
| **自动化** | ✅ | ✅ | ✅ | ✅ | 全平台兼容 |
| auto.bat | ✅ CMD | ❌ | ❌ | ⚠️ | Windows 专用 |
| auto.ps1 | ✅ PS | ⚠️ | ⚠️ | ⚠️ | PowerShell |
| auto.sh | ❌ | ✅ | ✅ | ✅ | Bash |
| **技能系统** | ✅ | ✅ | ✅ | ✅ | 全平台兼容 |

**图例**: ✅ 完全支持 | ⚠️ 需要额外配置 | ❌ 不支持

---

## 各平台部署说明

### Windows

#### 方式 1: CMD (命令提示符)

```cmd
REM 项目管理
.qwen\skills\project-manager\pm.bat status

REM 自动化
.qwen\skills\automation\auto.bat backup
.qwen\skills\automation\auto.bat install-deps
```

**依赖安装**:
```cmd
REM 管理员 CMD
choco install cmake mingw make llvm doxygen.install
```

#### 方式 2: PowerShell (推荐)

```powershell
# 项目管理
.qwen\skills\project-manager\pm.ps1 status

# 自动化
.qwen\skills\automation\auto.ps1 backup
.qwen\skills\automation\auto.ps1 install-deps

# 允许脚本执行
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
```

**依赖安装**:
```powershell
# 管理员 PowerShell
choco install cmake mingw make llvm doxygen.install graphviz

# 或使用 Winget (Windows 10 1709+)
winget install Kitware.CMake MSYS2.MSYS2
```

#### 方式 3: Git Bash / WSL

```bash
# 和 Linux 相同
./.qwen/skills/project-manager/pm.sh status
./.qwen/skills/automation/auto.sh backup
```

**注意**: 路径使用 Unix 风格 (`/` 而不是 `\`)

---

### Linux

#### Ubuntu/Debian

```bash
# 1. 安装依赖
sudo apt-get update
sudo apt-get install -y cmake gcc make clang-format doxygen

# 2. 赋予执行权限
chmod +x .qwen/skills/project-manager/pm.sh
chmod +x .qwen/skills/automation/auto.sh
chmod +x .qwen/skills/automation/check_deps.sh

# 3. 使用
./.qwen/skills/project-manager/pm.sh status
./.qwen/skills/automation/auto.sh backup
```

#### CentOS/RHEL

```bash
# 1. 安装依赖
sudo yum install -y cmake gcc make clang-tools-extra doxygen

# 2. 赋予执行权限
chmod +x .qwen/skills/project-manager/pm.sh
chmod +x .qwen/skills/automation/auto.sh

# 3. 使用
./.qwen/skills/project-manager/pm.sh status
```

---

### macOS

```bash
# 1. 安装 Homebrew (如果没有)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. 安装依赖
brew install cmake llvm doxygen graphviz

# 3. 赋予执行权限
chmod +x .qwen/skills/project-manager/pm.sh
chmod +x .qwen/skills/automation/auto.sh

# 4. 使用
./.qwen/skills/project-manager/pm.sh status
./.qwen/skills/automation/auto.sh backup
```

---

### WSL (Windows Subsystem for Linux)

```bash
# 1. 安装 WSL (Windows 10/11)
wsl --install

# 2. 在 WSL 中安装依赖
sudo apt-get update
sudo apt-get install -y cmake gcc make clang-format doxygen

# 3. 使用 (和 Linux 相同)
./.qwen/skills/project-manager/pm.sh status
./.qwen/skills/automation/auto.sh backup

# 4. 访问 Windows 文件
cd /mnt/c/Users/YourName/XinYi
```

---

## 跨平台脚本特性

### 自动检测系统

所有脚本都包含系统检测：

```bash
# auto.sh 片段
case "$(uname -s)" in
    Linux*)     OS="linux";;
    Darwin*)    OS="macos";;
    MINGW*|MSYS*|CYGWIN*) OS="windows";;
    *)          OS="unknown";;
esac
```

### 路径处理

```bash
# 正确处理路径分隔符
if [ "$OS" = "windows" ]; then
    STATUS_FILE=$(echo "$STATUS_FILE" | sed 's|/|\\|g')
else
    # Unix 路径不变
fi
```

### 颜色输出

```bash
# Windows 禁用 ANSI 颜色
if [ "$(uname)" != "Windows_NT" ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    NC=''
fi
```

---

## 平台特定问题

### Windows

#### 问题 1: 脚本执行策略

**症状**: `无法加载脚本，因为在此系统上禁用脚本执行`

**解决**:
```powershell
# 临时允许
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

# 或仅允许当前用户
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```

#### 问题 2: 路径过长

**症状**: `路径超过最大长度限制`

**解决**:
```powershell
# 启用长路径 (Windows 10 1607+)
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" `
    -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```

#### 问题 3: 中文乱码

**症状**: 输出中文乱码

**解决**:
```cmd
chcp 65001
```

---

### Linux

#### 问题 1: 权限不足

**症状**: `Permission denied`

**解决**:
```bash
chmod +x .qwen/skills/project-manager/pm.sh
chmod +x .qwen/skills/automation/auto.sh
```

#### 问题 2: 依赖缺失

**症状**: `cmake: command not found`

**解决**:
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# CentOS/RHEL
sudo yum install cmake
```

---

### macOS

#### 问题 1: Gatekeeper 阻止

**症状**: 无法打开未签名应用

**解决**:
```bash
# 允许脚本执行
xattr -d com.apple.quarantine .qwen/skills/automation/auto.sh
```

#### 问题 2: 缺少命令行工具

**症状**: `gcc: command not found`

**解决**:
```bash
xcode-select --install
```

---

## 跨平台 CI/CD

### GitHub Actions

```yaml
name: Cross-Platform Test

on: [push, pull_request]

jobs:
  test:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    
    runs-on: ${{ matrix.os }}
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup (Windows)
        if: runner.os == 'Windows'
        shell: powershell
        run: |
          .qwen/skills/automation/auto.ps1 install-deps
          .qwen/skills/automation/auto.ps1 run-tests
      
      - name: Setup (Linux/macOS)
        if: runner.os != 'Windows'
        shell: bash
        run: |
          .qwen/skills/automation/auto.sh install-deps
          .qwen/skills/automation/auto.sh run-tests
```

---

## 快速部署脚本

### Windows (deploy.bat)

```batch
@echo off
echo === XinYi 快速部署 (Windows) ===

REM 检查管理员权限
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo 请以管理员身份运行
    exit /b 1
)

REM 安装 Chocolatey
where choco >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo 安装 Chocolatey...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
)

REM 安装依赖
echo 安装依赖...
choco install -y cmake mingw make llvm doxygen.install

REM 赋予权限
echo 配置完成
echo 使用:.qwen\skills\project-manager\pm.bat status
```

### Linux/macOS (deploy.sh)

```bash
#!/bin/bash
echo "=== XinYi 快速部署 ==="

# 检测系统
if command -v apt-get &> /dev/null; then
    echo "检测到 Debian/Ubuntu"
    sudo apt-get update
    sudo apt-get install -y cmake gcc make clang-format doxygen
elif command -v yum &> /dev/null; then
    echo "检测到 CentOS/RHEL"
    sudo yum install -y cmake gcc make clang-tools-extra doxygen
elif command -v brew &> /dev/null; then
    echo "检测到 macOS"
    brew install cmake llvm doxygen
else
    echo "请手动安装依赖"
fi

# 赋予权限
chmod +x .qwen/skills/project-manager/pm.sh
chmod +x .qwen/skills/automation/auto.sh

echo "部署完成"
echo "使用：./.qwen/skills/project-manager/pm.sh status"
```

---

## 平台测试矩阵

运行以下命令测试跨平台兼容性：

```bash
# 所有平台
./check_deps.sh           # 依赖检查

# Linux/macOS/WSL
./.qwen/skills/project-manager/pm.sh status
./.qwen/skills/automation/auto.sh backup

# Windows CMD
.qwen\skills\project-manager\pm.bat status
.qwen\skills\automation\auto.bat backup

# Windows PowerShell
.qwen\skills\project-manager\pm.ps1 status
.qwen\skills\automation\auto.ps1 backup
```

---

## 相关文件

- [依赖说明](automation/DEPENDENCIES.md)
- [自动化功能](automation/README.md)
- [项目管理](project-manager/README.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
