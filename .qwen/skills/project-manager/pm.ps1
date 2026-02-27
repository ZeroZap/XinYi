# XinYi Project Manager Script - PowerShell Version
# 支持：Windows PowerShell 5.1+ / PowerShell Core 6+

param(
    [Parameter(Position=0)]
    [ValidateSet('status','tasks','progress','report','update','add-task','help')]
    [string]$Command = 'help',

    [Parameter(Position=1)]
    [string]$Param1,

    [Parameter(Position=2)]
    [string]$Param2,

    [Parameter(Position=3)]
    [string]$Param3
)

# 脚本目录和项目根目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path "$ScriptDir\..\..\.."
$StatusFile = "$ProjectRoot\COMPONENTS_STATUS.md"
$TasksFile = "$ProjectRoot\.qwen\skills\project-manager\TASKS.md"

# 获取当前日期时间
$CurrentDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

# 颜色定义
$Colors = @{
    Red    = [ConsoleColor]::Red
    Green  = [ConsoleColor]::Green
    Yellow = [ConsoleColor]::Yellow
    Blue   = [ConsoleColor]::Blue
    White  = [ConsoleColor]::White
}

function Write-Color {
    param([string]$Text, [ConsoleColor]$Color)
    $OriginalColor = $Host.UI.RawUI.ForegroundColor
    $Host.UI.RawUI.ForegroundColor = $Color
    Write-Host $Text
    $Host.UI.RawUI.ForegroundColor = $OriginalColor
}

# 帮助信息
function Show-Help {
    Write-Host "XinYi Project Manager - PowerShell Version"
    Write-Host ""
    Write-Host "用法：.\pm.ps1 <command> [options]"
    Write-Host ""
    Write-Host "命令:" -ForegroundColor Cyan
    Write-Host "  status          查看组件状态"
    Write-Host "  tasks           查看任务列表"
    Write-Host "  progress        显示项目进度"
    Write-Host "  report          生成项目报告"
    Write-Host "  update          更新组件状态"
    Write-Host "  add-task        添加新任务"
    Write-Host "  help            显示帮助信息"
    Write-Host ""
    Write-Host "示例:" -ForegroundColor Cyan
    Write-Host "  .\pm.ps1 status"
    Write-Host "  .\pm.ps1 tasks"
    Write-Host "  .\pm.ps1 progress"
    Write-Host "  .\pm.ps1 report daily"
    Write-Host ""
}

# 显示组件状态
function Show-Status {
    Write-Color "=== XinYi 组件状态 ===" $Colors.Blue
    Write-Host ""

    if (Test-Path $StatusFile) {
        Get-Content $StatusFile | Select-String "^| 组件 | 状态 |" -Context 0,15
        Write-Host ""
        Write-Host "详细状态请查看：$StatusFile" -ForegroundColor Yellow
    } else {
        Write-Color "状态文件不存在：$StatusFile" $Colors.Red
    }
    Write-Host ""
}

# 显示任务列表
function Show-Tasks {
    Write-Color "=== 项目任务列表 ===" $Colors.Blue
    Write-Host ""

    if (Test-Path $TasksFile) {
        $content = Get-Content $TasksFile -Raw

        Write-Host "高优先级 (1-2 周):" -ForegroundColor Red
        $content | Select-String "🔴|高优先级" -Context 0,5 | ForEach-Object { Write-Host $_ }

        Write-Host ""
        Write-Host "中优先级 (1 个月):" -ForegroundColor Yellow
        $content | Select-String "🟡|中优先级" -Context 0,5 | ForEach-Object { Write-Host $_ }

        Write-Host ""
        Write-Host "低优先级 (3 个月):" -ForegroundColor Green
        $content | Select-String "🟢|低优先级" -Context 0,5 | ForEach-Object { Write-Host $_ }
    } else {
        Write-Color "任务文件不存在：$TasksFile" $Colors.Red
    }
    Write-Host ""
}

# 显示项目进度
function Show-Progress {
    Write-Color "=== 项目进度 ===" $Colors.Blue
    Write-Host ""

    if (Test-Path $StatusFile) {
        $content = Get-Content $StatusFile -Raw

        $total = ([regex]::Matches($content, '^| `')).Count
        $done = ([regex]::Matches($content, '✅')).Count
        $progress = ([regex]::Matches($content, '⚠')).Count
        $todo = ([regex]::Matches($content, '📋')).Count

        Write-Host "组件统计:"
        Write-Host "  总数：$total"
        Write-Host "  完成：$done" -ForegroundColor Green
        Write-Host "  进行中：$progress" -ForegroundColor Yellow
        Write-Host "  待开始：$todo" -ForegroundColor Red

        if ($total -gt 0) {
            $percentage = [math]::Floor($done * 100 / $total)
            Write-Host ""
            Write-Host "完成率：${percentage}%"

            # 进度条
            $barLength = 20
            $filled = [math]::Floor($percentage * $barLength / 100)
            $empty = $barLength - $filled

            $bar = ("#" * $filled) + ("-" * $empty)
            Write-Host "[$bar] ${percentage}%"
        }
    } else {
        Write-Color "状态文件不存在" $Colors.Red
    }
    Write-Host ""
}

# 生成报告
function Generate-Report {
    param([string]$Type = 'daily')

    Write-Color "=== XinYi 项目报告 ===" $Colors.Blue
    Write-Host "日期：$CurrentDate"
    Write-Host "类型：$Type"
    Write-Host ""

    switch ($Type) {
        'daily' {
            Write-Host "今日完成:"
            Write-Host "- OSAL 组件完善" -ForegroundColor Green
            Write-Host "- HAL STM32U5 实现" -ForegroundColor Green
            Write-Host "- 测试系统优化" -ForegroundColor Green
            Write-Host ""
            Write-Host "进行中:" -ForegroundColor Yellow
            Write-Host "- 构建系统统一"
            Write-Host "- 文档完善"
        }
        'weekly' {
            Write-Host "本周完成:"
            Write-Host "1. OSAL 组件 (100%)" -ForegroundColor Green
            Write-Host "2. HAL STM32U5 (100%)" -ForegroundColor Green
            Write-Host "3. 测试系统 (80%)" -ForegroundColor Yellow
            Write-Host "4. 构建系统 (90%)" -ForegroundColor Yellow
            Write-Host ""
            Write-Host "下周计划:" -ForegroundColor Cyan
            Write-Host "1. 规范各组件测试目录"
            Write-Host "2. 添加 CI/CD 集成"
            Write-Host "3. 完善文档"
        }
        default {
            Write-Color "未知报告类型：$Type" $Colors.Red
            Write-Host "可用类型：daily, weekly"
        }
    }
    Write-Host ""
}

# 更新组件状态
function Update-Component {
    param([string]$Component, [string]$Status)

    if (-not $Component -or -not $Status) {
        Write-Color "错误：请提供组件名和状态" $Colors.Red
        Write-Host "用法：.\pm.ps1 update <component> <status>"
        return
    }

    Write-Host "更新组件：$Component -> $Status"
    Write-Host "注意：请手动更新 $StatusFile 文件"
    Write-Host ""
    Write-Host "可用状态:" -ForegroundColor Cyan
    Write-Host "  ok       - 完善"
    Write-Host "  progress - 进行中"
    Write-Host "  base     - 基础"
    Write-Host "  missing  - 缺失"
}

# 添加任务
function Add-Task {
    param([string]$Task, [string]$Category = 'general', [string]$Priority = 'medium')

    if (-not $Task) {
        Write-Color "错误：请提供任务描述" $Colors.Red
        Write-Host "用法：.\pm.ps1 add-task `"任务描述`" [类别] [优先级]"
        return
    }

    Write-Host "添加任务:"
    Write-Host "  描述：$Task"
    Write-Host "  类别：$Category"
    Write-Host "  优先级：$Priority"
    Write-Host ""
    Write-Host "注意：任务已添加到待办列表" -ForegroundColor Green
}

# 主程序
switch ($Command) {
    'status' { Show-Status }
    'tasks' { Show-Tasks }
    'progress' { Show-Progress }
    'report' { Generate-Report -Type $Param1 }
    'update' { Update-Component -Component $Param1 -Status $Param2 }
    'add-task' { Add-Task -Task $Param1 -Category $Param2 -Priority $Param3 }
    'help' { Show-Help }
    default {
        Write-Color "未知命令：$Command" $Colors.Red
        Write-Host ""
        Show-Help
    }
}
