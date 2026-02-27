# XinYi Quick Deploy Script - PowerShell
# 快速部署所有依赖

param(
    [switch]$NoChocolatey,
    [switch]$SkipVerify
)

$ErrorActionPreference = "Stop"

Write-Host "=== XinYi 快速部署 (PowerShell) ===" -ForegroundColor Cyan
Write-Host ""

# 检查管理员权限
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin -and -not $NoChocolatey) {
    Write-Host "警告：非管理员模式，部分功能可能受限" -ForegroundColor Yellow
    Write-Host "建议：右键点击 - "以管理员身份运行"" -ForegroundColor Yellow
    Write-Host ""
}

# 检查/安装 Chocolatey
if (-not $NoChocolatey) {
    Write-Host "检查 Chocolatey..." -ForegroundColor Cyan

    $chocoPath = Get-Command choco -ErrorAction SilentlyContinue

    if (-not $chocoPath) {
        Write-Host "Chocolatey 未安装" -ForegroundColor Yellow
        Write-Host "正在安装 Chocolatey..." -ForegroundColor Cyan

        try {
            Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
            $env:Path += ";%ALLUSERSPROFILE%\chocolatey\bin"
            Write-Host "Chocolatey 安装完成" -ForegroundColor Green
        } catch {
            Write-Host "Chocolatey 安装失败，请手动安装" -ForegroundColor Red
            Write-Host "访问：https://chocolatey.org/install" -ForegroundColor Yellow
            return
        }
    } else {
        Write-Host "Chocolatey 已安装：$($chocoPath.Source)" -ForegroundColor Green
    }
}

# 安装依赖
Write-Host ""
Write-Host "安装核心依赖..." -ForegroundColor Cyan

if ($NoChocolatey) {
    Write-Host "跳过 Chocolatey 安装，使用 Winget..." -ForegroundColor Yellow

    # 使用 Winget
    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if ($winget) {
        winget install --id Kitware.CMake --silent
        winget install --id MSYS2.MSYS2 --silent
    } else {
        Write-Host "Winget 不可用，请手动安装依赖" -ForegroundColor Red
    }
} else {
    choco install -y cmake mingw make --no-progress
}

Write-Host ""
Write-Host "安装可选依赖..." -ForegroundColor Cyan

if (-not $NoChocolatey) {
    choco install -y llvm doxygen.install graphviz rsync --no-progress 2>$null | Out-String
}

# 设置执行策略
Write-Host ""
Write-Host "设置 PowerShell 执行策略..." -ForegroundColor Cyan

try {
    Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned -Force
    Write-Host "执行策略已设置" -ForegroundColor Green
} catch {
    Write-Host "执行策略设置失败，请手动运行：" -ForegroundColor Yellow
    Write-Host "Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned" -ForegroundColor Yellow
}

# 解除脚本锁定
Write-Host ""
Write-Host "解除脚本锁定..." -ForegroundColor Cyan

Get-ChildItem -Path ".qwen\skills" -Recurse -Filter "*.ps1" | ForEach-Object {
    Unblock-File -Path $_.FullName -ErrorAction SilentlyContinue
}

Write-Host "脚本已解锁" -ForegroundColor Green

# 验证安装
if (-not $SkipVerify) {
    Write-Host ""
    Write-Host "=== 验证安装 ===" -ForegroundColor Cyan
    Write-Host ""

    $tools = @('cmake', 'gcc', 'make', 'clang', 'doxygen')

    foreach ($tool in $tools) {
        $cmd = Get-Command $tool -ErrorAction SilentlyContinue
        if ($cmd) {
            Write-Host "[OK] $tool": $($cmd.Source) -ForegroundColor Green
        } else {
            Write-Host "[FAIL] $tool": 未安装 -ForegroundColor Red
        }
    }
}

Write-Host ""
Write-Host "=== 部署完成 ===" -ForegroundColor Green
Write-Host ""
Write-Host "使用示例:" -ForegroundColor Cyan
Write-Host "  # 查看项目状态"
Write-Host "  .qwen\skills\project-manager\pm.ps1 status"
Write-Host ""
Write-Host "  # 自动备份"
Write-Host "  .qwen\skills\automation\auto.ps1 backup"
Write-Host ""
Write-Host "  # 依赖检查"
Write-Host "  .qwen\skills\automation\check_deps.ps1"
Write-Host ""
Write-Host "提示：也可以使用 CMD 版本 (.bat) 或 Git Bash (.sh)" -ForegroundColor Yellow
Write-Host ""
