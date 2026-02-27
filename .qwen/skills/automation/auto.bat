@echo off
REM XinYi Automation Script - Windows Batch Version
REM 功能：自动备份、文件同步、代码审查、文档生成等

setlocal enabledelayedexpansion

REM 脚本目录和项目根目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\..\.."
set "BACKUP_DIR=%PROJECT_ROOT%\.backups"
set "DOCS_DIR=%PROJECT_ROOT%\docs\generated"

REM 获取当前日期时间
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set "dt=%%I"
set "DATE_STAMP=%dt:~0,8%_%dt:~8,6%"

REM 帮助信息
:help
echo XinYi Automation Tools - Windows Version
echo.
echo 用法：%~nx0 ^<command^> [options]
echo.
echo 命令:
echo   backup              自动备份项目
echo   sync ^<src^> ^<dst^>    同步文件
echo   init-project ^<name^> 初始化新项目
echo   install-deps        安装依赖
echo   run-tests           运行测试
echo   gen-docs            生成文档
echo   code-review         代码审查
echo   help                显示帮助
echo.
goto :eof

REM 自动备份
:backup
echo === 自动备份 ===
set "BACKUP_NAME=backup_%DATE_STAMP%"
set "BACKUP_PATH=%BACKUP_DIR%\%BACKUP_NAME%"

echo 备份目标：%BACKUP_PATH%

if not exist "%BACKUP_DIR%" mkdir "%BACKUP_DIR%"
mkdir "%BACKUP_PATH%" 2>nul

REM 备份重要目录
for %%d in (components docs scripts) do (
    if exist "%PROJECT_ROOT%\%%d" (
        echo 备份：%%d
        xcopy /E /I /Y "%PROJECT_ROOT%\%%d" "%BACKUP_PATH%\%%d" 2>nul
    )
)

REM 备份配置文件
for %%f in (CMakeLists.txt Kconfig Makefile) do (
    if exist "%PROJECT_ROOT%\%%f" (
        copy "%PROJECT_ROOT%\%%f" "%BACKUP_PATH%\" 2>nul
    )
)

echo 备份完成：%BACKUP_PATH%

REM 清理旧备份 (保留最近 7 个)
pushd "%BACKUP_DIR%"
for /f "skip=7 delims=" %%f in ('dir /b /o-d backup_* 2^nul') do rmdir /s /q "%%f" 2>nul
popd

goto :eof

REM 文件同步
:sync
set "SRC=%~1"
set "DST=%~2"

if "%SRC%"=="" (
    echo 错误：请提供源目录
    goto :eof
)
if "%DST%"=="" (
    echo 错误：请提供目标目录
    goto :eof
)

echo === 文件同步 ===
echo 源：%SRC%
echo 目标：%DST%

xcopy /E /I /Y "%SRC%" "%DST%"

echo 同步完成
goto :eof

REM 初始化项目
:init_project
set "NAME=%~1"

if "%NAME%"=="" (
    echo 错误：请提供项目名称
    goto :eof
)

echo === 初始化项目：%NAME% ===

set "PROJECT_DIR=%PROJECT_ROOT%\projects\%NAME%"
if not exist "%PROJECT_DIR%" mkdir "%PROJECT_DIR%"
mkdir "%PROJECT_DIR%\src"
mkdir "%PROJECT_DIR%\include"

echo 项目初始化完成：%PROJECT_DIR%
goto :eof

REM 安装依赖
:install_deps
echo === 安装依赖 ===

REM 检查 Chocolatey
where choco >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo 使用 choco 安装依赖...
    choco install -y cmake mingw make
) else (
    echo 未检测到包管理器
    echo 请手动安装:
    echo - CMake: https://cmake.org/download/
    echo - GCC: https://gcc.gnu.org/
)

REM 初始化 Git 子模块
if exist "%PROJECT_ROOT%\.gitmodules" (
    echo 初始化 Git 子模块...
    cd /d "%PROJECT_ROOT%"
    git submodule update --init --recursive
)

echo 依赖安装完成
goto :eof

REM 运行测试
:run_tests
echo === 运行测试 ===

set "BUILD_DIR=%PROJECT_ROOT%\build"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"
cmake .. -DBUILD_TESTING=ON
cmake --build . --config Release

echo 运行测试...
ctest --output-on-failure

echo 测试完成
goto :eof

REM 生成文档
:gen_docs
echo === 生成文档 ===

if not exist "%DOCS_DIR%" mkdir "%DOCS_DIR%"

REM 生成组件列表
echo # 组件文档索引 > "%DOCS_DIR%\components_index.md"
echo. >> "%DOCS_DIR%\components_index.md"
echo | 组件 | 状态 | 文档 | >> "%DOCS_DIR%\components_index.md"
echo |------|------|------| >> "%DOCS_DIR%\components_index.md"

for /d %%d in ("%PROJECT_ROOT%\components\*") do (
    set "name=%%~nxd"
    set "status=📋"
    set "link=无"
    if exist "%%d\README.md" (
        set "status=✅"
        set "link=[查看](../!name!/README.md)"
    )
    echo | !name! | !status! | !link! | >> "%DOCS_DIR%\components_index.md"
)

echo 文档索引：%DOCS_DIR%\components_index.md
goto :eof

REM 代码审查
:code_review
set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=components"

echo === 代码审查 ===
echo 审查目标：%TARGET%

set "REPORT=%PROJECT_ROOT%\code_review_%DATE_STAMP%.md"

echo # 代码审查报告 > "%REPORT%"
echo 日期：%DATE% >> "%REPORT%"
echo 目标：%TARGET% >> "%REPORT%"
echo. >> "%REPORT%"

echo ## 代码统计 >> "%REPORT%"

REM 统计代码行数
set "LOC=0"
for /f "delims=" %%f in ('dir /s /b "%PROJECT_ROOT%\%TARGET%\*.c" "%PROJECT_ROOT%\%TARGET%\*.h" 2^>nul') do (
    for /f "tokens=3" %%c in ('find /c /v "" "%%f"') do set /a LOC+=%%c
)
echo - 代码行数：%LOC% >> "%REPORT%"

REM 检查 TODO
set "TODO_COUNT=0"
for /f "delims=" %%f in ('findstr /s /i "TODO" "%PROJECT_ROOT%\%TARGET%\*.c" "%PROJECT_ROOT%\%TARGET%\*.h" 2^>nul ^| find /c /v ""') do set "TODO_COUNT=%%f"
echo - TODO 注释：%TODO_COUNT% 个 >> "%REPORT%"

REM 检查 FIXME
set "FIXME_COUNT=0"
for /f "delims=" %%f in ('findstr /s /i "FIXME" "%PROJECT_ROOT%\%TARGET%\*.c" "%PROJECT_ROOT%\%TARGET%\*.h" 2^>nul ^| find /c /v ""') do set "FIXME_COUNT=%%f"
echo - FIXME 注释：%FIXME_COUNT% 个 >> "%REPORT%"

echo. >> "%REPORT%"
echo ## 建议 >> "%REPORT%"
echo 1. 处理 FIXME 标记的问题 >> "%REPORT%"
echo 2. 完成 TODO 标记的功能 >> "%REPORT%"

echo 审查报告：%REPORT%
goto :eof

REM 主程序
if "%~1"=="" goto help
if "%~1"=="help" goto help
if "%~1"=="--help" goto help

if "%~1"=="backup" goto backup
if "%~1"=="sync" shift & goto sync
if "%~1"=="init-project" shift & goto init_project
if "%~1"=="install-deps" goto install_deps
if "%~1"=="run-tests" goto run_tests
if "%~1"=="gen-docs" goto gen_docs
if "%~1"=="code-review" shift & goto code_review

echo 未知命令：%~1
goto help
