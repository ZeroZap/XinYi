@echo off
REM XinYi Quick Deploy Script - Windows
REM 快速部署所有依赖

setlocal enabledelayedexpansion

echo === XinYi 快速部署 (Windows) ===
echo.

REM 检查管理员权限
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo 请以管理员身份运行此脚本
    echo.
    echo 方法 1: 右键点击 - "以管理员身份运行"
    echo 方法 2: PowerShell 运行 Start-Process cmd -Verb RunAs
    exit /b 1
)

REM 检查 Chocolatey
echo 检查 Chocolatey...
where choco >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Chocolatey 未安装
    echo 正在安装 Chocolatey...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
    if %ERRORLEVEL% NEQ 0 (
        echo Chocolatey 安装失败，请手动安装
        echo 访问：https://chocolatey.org/install
        exit /b 1
    )
)
echo Chocolatey 已安装

REM 安装依赖
echo.
echo 安装核心依赖...
choco install -y cmake mingw make

echo.
echo 安装可选依赖...
choco install -y llvm doxygen.install graphviz rsync cppcheck 2>nul || echo 部分可选依赖安装跳过

echo.
echo 设置脚本权限...

REM 赋予执行权限 (PowerShell)
powershell -Command "Get-ChildItem '.qwen\skills' -Recurse -Filter '*.ps1' | ForEach-Object { Unblock-File $_.FullName }" 2>nul || true

echo.
echo === 验证安装 ===
echo.

REM 检查 CMake
where cmake >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] CMake: 已安装
) else (
    echo [FAIL] CMake: 未安装
)

REM 检查 GCC
where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] GCC: 已安装
) else (
    echo [FAIL] GCC: 未安装
)

REM 检查 Make
where make >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] Make: 已安装
) else (
    echo [FAIL] Make: 未安装
)

echo.
echo === 部署完成 ===
echo.
echo 使用示例:
echo   REM 查看项目状态
echo   .qwen\skills\project-manager\pm.bat status
echo.
echo   REM 自动备份
echo   .qwen\skills\automation\auto.bat backup
echo.
echo   REM PowerShell 版本
echo   .qwen\skills\project-manager\pm.ps1 status
echo.

pause
