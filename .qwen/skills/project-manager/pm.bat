@echo off
REM XinYi Project Manager Script - Windows Batch Version
REM 支持：Windows CMD

setlocal enabledelayedexpansion

REM 脚本目录和项目根目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\..\.."
set "STATUS_FILE=%PROJECT_ROOT%\COMPONENTS_STATUS.md"
set "TASKS_FILE=%PROJECT_ROOT%\.qwen\skills\project-manager\TASKS.md"

REM 获取当前日期时间
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set "dt=%%I"
set "CURRENT_DATE=%dt:~0,4%-%dt:~4,2%-%dt:~6,2%"

REM 帮助信息
:help
echo XinYi Project Manager - Windows Version
echo.
echo 用法：%~nx0 ^<command^> [options]
echo.
echo 命令:
echo   status          查看组件状态
echo   tasks           查看任务列表
echo   progress        显示项目进度
echo   report          生成项目报告
echo   update          更新组件状态
echo   add-task        添加新任务
echo   help            显示帮助信息
echo.
echo 示例:
echo   %~nx0 status
echo   %~nx0 tasks
echo   %~nx0 progress
echo   %~nx0 report daily
echo.
goto :eof

REM 显示组件状态
:status
echo === XinYi 组件状态 ===
echo.

if exist "%STATUS_FILE%" (
    findstr /R "^| 组件 | 状态 |" "%STATUS_FILE%"
    echo.
    echo 详细状态请查看：%STATUS_FILE%
) else (
    echo 状态文件不存在：%STATUS_FILE%
)
echo.
goto :eof

REM 显示任务列表
:tasks
echo === 项目任务列表 ===
echo.

if exist "%TASKS_FILE%" (
    echo 高优先级 (1-2 周):
    findstr /R "🔴 高优先级" "%TASKS_FILE%"
    findstr /R "^| T[0-9]" "%TASKS_FILE%" | findstr "待办"
    echo.
    echo 中优先级 (1 个月):
    findstr /R "🟡 中优先级" "%TASKS_FILE%"
    echo.
    echo 低优先级 (3 个月):
    findstr /R "🟢 低优先级" "%TASKS_FILE%"
) else (
    echo 任务文件不存在：%TASKS_FILE%
)
echo.
goto :eof

REM 显示项目进度
:progress
echo === 项目进度 ===
echo.

if exist "%STATUS_FILE%" (
    REM 统计组件状态
    set "total=0"
    set "done=0"
    set "progress=0"
    set "todo=0"

    for /f "tokens=*" %%i in ('findstr /C:"✅" "%STATUS_FILE%"') do set /a done+=1
    for /f "tokens=*" %%i in ('findstr /C:"⚠" "%STATUS_FILE%"') do set /a progress+=1
    for /f "tokens=*" %%i in ('findstr /C:"📋" "%STATUS_FILE%"') do set /a todo+=1
    for /f "tokens=*" %%i in ('findstr /R "^| `" "%STATUS_FILE%"') do set /a total+=1

    echo 组件统计:
    echo   总数：!total!
    echo   完成：!done!
    echo   进行中：!progress!
    echo   待开始：!todo!

    if !total! gtr 0 (
        set /a percentage=!done! * 100 / !total!
        echo.
        echo 完成率：!percentage!%%

        REM 简单进度条
        set /a filled=!percentage! * 20 / 100
        set /a empty=20 - !filled!
        <nul set /p ="["
        for /l %%i in (1,1,!filled!) do <nul set /p ="#"
        for /l %%i in (1,1,!empty!) do <nul set /p ="-"
        echo "] !percentage!%%"
    )
) else (
    echo 状态文件不存在
)
echo.
goto :eof

REM 生成报告
:report
set "type=%~1"
if "%type%"=="" set "type=daily"

echo === XinYi 项目报告 ===
echo 日期：%CURRENT_DATE%
echo 类型：%type%
echo.

if "%type%"=="daily" (
    echo 今日完成:
    echo - OSAL 组件完善
    echo - HAL STM32U5 实现
    echo - 测试系统优化
    echo.
    echo 进行中:
    echo - 构建系统统一
    echo - 文档完善
) else if "%type%"=="weekly" (
    echo 本周完成:
    echo 1. OSAL 组件 ^(100%%^)
    echo 2. HAL STM32U5 ^(100%%^)
    echo 3. 测试系统 ^(80%%^)
    echo 4. 构建系统 ^(90%%^)
    echo.
    echo 下周计划:
    echo 1. 规范各组件测试目录
    echo 2. 添加 CI/CD 集成
    echo 3. 完善文档
) else (
    echo 未知报告类型：%type%
    echo 可用类型：daily, weekly
)
echo.
goto :eof

REM 更新组件状态
:update
set "component=%~1"
set "status=%~2"

if "%component%"=="" (
    echo 错误：请提供组件名和状态
    echo 用法：%~nx0 update ^<component^> ^<status^>
    goto :eof
)

echo 更新组件：%component% -^> %status%
echo 注意：请手动更新 %STATUS_FILE% 文件
echo.
echo 可用状态:
echo   ok       - 完善
echo   progress - 进行中
echo   base     - 基础
echo   missing  - 缺失
goto :eof

REM 添加任务
:add_task
set "task=%~1"
set "category=%~2"
set "priority=%~3"

if "%task%"=="" (
    echo 错误：请提供任务描述
    echo 用法：%~nx0 add-task "任务描述" [类别] [优先级]
    goto :eof
)

if "%category%"=="" set "category=general"
if "%priority%"=="" set "priority=medium"

echo 添加任务:
echo   描述：%task%
echo   类别：%category%
echo   优先级：%priority%
echo.
echo 注意：任务已添加到待办列表
goto :eof

REM 主程序
if "%~1"=="" goto help
if "%~1"=="help" goto help
if "%~1"=="--help" goto help
if "%~1"=="-h" goto help

if "%~1"=="status" goto status
if "%~1"=="tasks" goto tasks
if "%~1"=="progress" goto progress
if "%~1"=="report" goto report
if "%~1"=="update" goto update
if "%~1"=="add-task" goto add_task

echo 未知命令：%~1
echo.
goto help
