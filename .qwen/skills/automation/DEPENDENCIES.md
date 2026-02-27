# XinYi 自动化脚本依赖说明

**版本**: 1.0  
**最后更新**: 2026-02-28

---

## 功能总览

| 功能 | 脚本命令 | 依赖工具 | 必需程度 |
|------|---------|----------|----------|
| 自动备份 | `backup` | tar (可选) | 🟢 可选 |
| 文件同步 | `sync` | rsync (可选) | 🟢 可选 |
| 项目初始化 | `init-project` | 无 | ✅ 无依赖 |
| 安装依赖 | `install-deps` | 包管理器 | 🟡 推荐 |
| 运行测试 | `run-tests` | CMake, GCC | 🔴 必需 |
| CSV 转换 | `csv-convert` | 无 | ✅ 无依赖 |
| 文档生成 | `gen-docs` | Doxygen (可选) | 🟢 可选 |
| 代码审查 | `code-review` | clang-format (可选) | 🟢 可选 |

**图例**: 🔴 必需 | 🟡 推荐 | 🟢 可选

---

## 核心依赖 (必需)

### 1. CMake

**用途**: 构建系统，运行测试必需

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install cmake

# CentOS/RHEL
sudo yum install cmake

# macOS
brew install cmake

# Windows (Chocolatey)
choco install cmake

# Windows (Winget)
winget install Kitware.CMake

# 手动下载
https://cmake.org/download/
```

**验证**:
```bash
cmake --version
```

---

### 2. GCC/编译器

**用途**: 编译代码，运行测试

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# CentOS/RHEL
sudo yum groupinstall "Development Tools"

# macOS
xcode-select --install

# Windows (MinGW)
choco install mingw

# Windows (MSVC)
# 安装 Visual Studio Build Tools
https://visualstudio.microsoft.com/downloads/
```

**验证**:
```bash
gcc --version
```

---

## 推荐依赖 (可选但推荐)

### 3. clang-format

**用途**: 代码格式化，代码审查

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install clang-format

# macOS
brew install clang-format

# Windows (Chocolatey)
choco install llvm

# 手动下载
https://releases.llvm.org/
```

**验证**:
```bash
clang-format --version
```

**配置**:
项目根目录已有 `.clang-format` 文件，自动应用代码风格。

---

### 4. clang-tidy

**用途**: 静态分析，代码审查

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install clang-tidy

# macOS
brew install llvm

# Windows (Chocolatey)
choco install llvm
```

**验证**:
```bash
clang-tidy --version
```

---

### 5. Doxygen

**用途**: 生成 API 文档

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows (Chocolatey)
choco install doxygen.install graphviz

# 手动下载
https://www.doxygen.nl/download.html
```

**验证**:
```bash
doxygen --version
```

---

## 可选依赖 (按需安装)

### 6. rsync

**用途**: 高效文件同步

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install rsync

# macOS
# 系统自带

# Windows (Git Bash)
# 随 Git 一起安装

# Windows (Cygwin/WSL)
# 包管理器安装
```

**替代方案**: 如无 rsync，脚本会自动使用 `cp` 或 `xcopy`

---

### 7. cppcheck

**用途**: 代码静态分析

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install cppcheck

# macOS
brew install cppcheck

# Windows (Chocolatey)
choco install cppcheck
```

**验证**:
```bash
cppcheck --version
```

---

### 8. gcov/lcov

**用途**: 测试覆盖率报告

**安装**:

```bash
# Ubuntu/Debian
sudo apt-get install gcov lcov

# macOS
brew install lcov

# Windows (MinGW)
# 随 MinGW 一起安装
```

**验证**:
```bash
gcov --version
```

---

## 包管理器 (推荐安装)

### Windows

**Chocolatey**:
```powershell
# 管理员 PowerShell
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# 安装工具
choco install -y cmake mingw make clang-format doxygen graphviz
```

**Winget**:
```powershell
# Windows 10 1709+ 自带
winget install Kitware.CMake
winget install MSYS2.MSYS2
```

### Linux

**apt (Debian/Ubuntu)**:
```bash
sudo apt-get update
sudo apt-get install -y cmake gcc make clang-format doxygen graphviz
```

**yum (CentOS/RHEL)**:
```bash
sudo yum install -y cmake gcc make clang doxygen graphviz
```

### macOS

**Homebrew**:
```bash
# 安装 Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装工具
brew install cmake clang-format doxygen graphviz
```

---

## 一键安装脚本

### Ubuntu/Debian

```bash
#!/bin/bash
# 一键安装所有依赖

sudo apt-get update
sudo apt-get install -y \
    cmake \
    gcc \
    make \
    clang-format \
    clang-tidy \
    doxygen \
    graphviz \
    gcov \
    lcov \
    rsync

echo "依赖安装完成"
```

### Windows (Chocolatey)

```powershell
# 管理员 PowerShell
choco install -y \
    cmake \
    mingw \
    make \
    llvm \
    doxygen.install \
    graphviz

Write-Host "依赖安装完成"
```

### macOS (Homebrew)

```bash
#!/bin/bash
brew install cmake llvm doxygen graphviz lcov
echo "依赖安装完成"
```

---

## 依赖检查脚本

### check_deps.sh (Linux/macOS)

```bash
#!/bin/bash
# 依赖检查脚本

echo "=== 检查依赖 ==="

check_cmd() {
    if command -v $1 &> /dev/null; then
        echo "✅ $1: $($1 --version 2>&1 | head -1)"
    else
        echo "❌ $1: 未安装"
    fi
}

check_cmd cmake
check_cmd gcc
check_cmd make
check_cmd clang-format
check_cmd clang-tidy
check_cmd doxygen
check_cmd rsync
check_cmd gcov
```

### check_deps.ps1 (Windows PowerShell)

```powershell
# 依赖检查脚本

Write-Host "=== 检查依赖 ==="

$tools = @(
    'cmake',
    'gcc',
    'make',
    'clang',
    'doxygen'
)

foreach ($tool in $tools) {
    $cmd = Get-Command $tool -ErrorAction SilentlyContinue
    if ($cmd) {
        Write-Host "✅ $tool: $(& $tool --version 2>&1 | Select-Object -First 1)" -ForegroundColor Green
    } else {
        Write-Host "❌ $tool: 未安装" -ForegroundColor Red
    }
}
```

---

## 故障排除

### 问题 1: cmake 找不到

**症状**: `cmake: command not found`

**解决**:
```bash
# 添加到 PATH
export PATH=/usr/local/cmake/bin:$PATH

# 或重新安装
sudo apt-get install --reinstall cmake
```

### 问题 2: gcc 版本过低

**症状**: 编译 C++11/14 代码失败

**解决**:
```bash
# Ubuntu
sudo apt-get install gcc-9 g++-9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90

# CentOS
sudo yum install devtoolset-9-gcc
scl enable devtoolset-9 bash
```

### 问题 3: Doxygen 中文乱码

**症状**: 生成的文档中文乱码

**解决**:
修改 `Doxyfile`:
```
DOXYFILE_ENCODING      = UTF-8
OUTPUT_LANGUAGE        = Chinese
```

### 问题 4: Windows 路径问题

**症状**: 脚本找不到文件

**解决**:
使用 Git Bash 或 WSL，避免 CMD 的路径问题。
或使用 PowerShell 版本脚本。

---

## 最小安装 (仅核心功能)

如果只想运行核心功能，安装以下即可：

```bash
# Ubuntu/Debian
sudo apt-get install cmake gcc make

# Windows (Chocolatey)
choco install cmake mingw

# macOS
brew install cmake
```

---

## 完整安装 (所有功能)

```bash
# Ubuntu/Debian
sudo apt-get install cmake gcc make clang-format clang-tidy doxygen graphviz gcov lcov rsync cppcheck

# Windows (Chocolatey)
choco install cmake mingw make llvm doxygen.install graphviz cppcheck

# macOS
brew install cmake llvm doxygen graphviz lcov
```

---

## 环境验证

安装完成后运行：

```bash
# Linux/macOS
./.qwen/skills/automation/check_deps.sh

# Windows PowerShell
.qwen/skills/automation/check_deps.ps1

# 或直接运行
./.qwen/skills/automation/auto.sh install-deps
```

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
