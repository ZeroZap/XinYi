# XinYi Automation Script - PowerShell Version
# 功能：自动备份、文件同步、代码审查、文档生成等

param(
    [Parameter(Position=0)]
    [ValidateSet('backup','sync','init-project','install-deps','run-tests','csv-convert','gen-docs','code-review','help')]
    [string]$Command = 'help',

    [Parameter(Position=1)]
    [string]$Param1,

    [Parameter(Position=2)]
    [string]$Param2
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path "$ScriptDir\..\..\.."
$BackupDir = "$ProjectRoot\.backups"
$DocsDir = "$ProjectRoot\docs\generated"
$DateStamp = Get-Date -Format "yyyyMMdd_HHmmss"

$Colors = @{
    Red    = [ConsoleColor]::Red
    Green  = [ConsoleColor]::Green
    Yellow = [ConsoleColor]::Yellow
    Blue   = [ConsoleColor]::Blue
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
    Write-Host "XinYi Automation Tools - PowerShell Version"
    Write-Host ""
    Write-Host "用法：.\auto.ps1 <command> [options]"
    Write-Host ""
    Write-Host "命令:" -ForegroundColor Cyan
    Write-Host "  backup              自动备份项目"
    Write-Host "  sync <src> <dst>    同步文件"
    Write-Host "  init-project <name> 初始化新项目"
    Write-Host "  install-deps        安装依赖"
    Write-Host "  run-tests           运行测试"
    Write-Host "  csv-convert <file>  CSV 格式转换"
    Write-Host "  gen-docs            生成文档"
    Write-Host "  code-review         代码审查"
    Write-Host ""
}

# 1. 自动备份
function Do-Backup {
    Write-Color "=== 自动备份 ===" $Colors.Blue

    $backupName = "backup_$DateStamp"
    $backupPath = "$BackupDir\$backupName"

    if (-not (Test-Path $BackupDir)) {
        New-Item -ItemType Directory -Path $BackupDir | Out-Null
    }

    New-Item -ItemType Directory -Path $backupPath -Force | Out-Null

    # 备份重要目录
    foreach ($dir in @('components', 'docs', 'scripts')) {
        $srcPath = "$ProjectRoot\$dir"
        if (Test-Path $srcPath) {
            Write-Host "备份：$dir"
            Copy-Item -Path $srcPath -Destination $backupPath -Recurse -Force
        }
    }

    # 备份配置文件
    foreach ($file in @('CMakeLists.txt', 'Kconfig', 'Makefile')) {
        $srcPath = "$ProjectRoot\$file"
        if (Test-Path $srcPath) {
            Copy-Item -Path $srcPath -Destination $backupPath -Force
        }
    }

    Write-Color "备份完成：$backupPath" $Colors.Green

    # 清理旧备份
    Get-ChildItem $BackupDir -Filter "backup_*" |
        Sort-Object CreationTime -Descending |
        Select-Object -Skip 7 |
        Remove-Item -Recurse -Force
}

# 2. 文件同步
function Do-Sync {
    param([string]$Src, [string]$Dst)

    if (-not $Src -or -not $Dst) {
        Write-Color "错误：请提供源目录和目标目录" $Colors.Red
        return
    }

    Write-Color "=== 文件同步 ===" $Colors.Blue
    Write-Host "源：$Src"
    Write-Host "目标：$Dst"

    Copy-Item -Path "$Src\*" -Destination $Dst -Recurse -Force

    Write-Color "同步完成" $Colors.Green
}

# 3. 初始化项目
function Do-Init-Project {
    param([string]$Name)

    if (-not $Name) {
        Write-Color "错误：请提供项目名称" $Colors.Red
        return
    }

    Write-Color "=== 初始化项目：$Name ===" $Colors.Blue

    $projectDir = "$ProjectRoot\projects\$Name"

    New-Item -ItemType Directory -Path "$projectDir\src" -Force | Out-Null
    New-Item -ItemType Directory -Path "$projectDir\include" -Force | Out-Null

    # 创建 CMakeLists.txt
    @"
cmake_minimum_required(VERSION 3.12)
project($Name C)

add_subdirectory(../../components/kernel/osal)
add_subdirectory(../../components/hal)

add_executable($Name src/main.c)
target_link_libraries($Name xy_osal)
"@ | Out-File -FilePath "$projectDir\CMakeLists.txt" -Encoding UTF8

    # 创建 main.c
    @"
#include <stdio.h>

int main(void) {
    printf("$Name starting...\n");
    return 0;
}
"@ | Out-File -FilePath "$projectDir\src\main.c" -Encoding UTF8

    # 创建 README.md
    @"
# $Name

## 构建

```bash
mkdir build && cd build
cmake ..
make
```
"@ | Out-File -FilePath "$projectDir\README.md" -Encoding UTF8

    Write-Color "项目初始化完成：$projectDir" $Colors.Green
}

# 4. 安装依赖
function Do-Install-Deps {
    Write-Color "=== 安装依赖 ===" $Colors.Blue

    # 检查包管理器
    if (Get-Command choco -ErrorAction SilentlyContinue) {
        Write-Host "使用 choco 安装依赖..."
        choco install -y cmake mingw make
    } elseif (Get-Command winget -ErrorAction SilentlyContinue) {
        Write-Host "使用 winget 安装依赖..."
        winget install Kitware.CMake
    } else {
        Write-Color "未检测到包管理器" $Colors.Yellow
        Write-Host "请手动安装:"
        Write-Host "- CMake: https://cmake.org/download/"
        Write-Host "- GCC: https://gcc.gnu.org/"
    }

    # 初始化 Git 子模块
    if (Test-Path "$ProjectRoot\.gitmodules") {
        Write-Host "初始化 Git 子模块..."
        Push-Location $ProjectRoot
        git submodule update --init --recursive
        Pop-Location
    }

    Write-Color "依赖安装完成" $Colors.Green
}

# 5. 运行测试
function Do-Run-Tests {
    Write-Color "=== 运行测试 ===" $Colors.Blue

    $buildDir = "$ProjectRoot\build"
    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir

    cmake .. -DBUILD_TESTING=ON
    cmake --build . --config Release

    Write-Color "运行测试..." $Colors.Blue
    ctest --output-on-failure

    Pop-Location

    Write-Color "测试完成" $Colors.Green
}

# 6. CSV 转换
function Do-Csv-Convert {
    param([string]$File, [string]$Format = 'json')

    if (-not $File) {
        Write-Color "错误：请提供 CSV 文件路径" $Colors.Red
        return
    }

    Write-Color "=== CSV 转换 ===" $Colors.Blue
    Write-Host "文件：$File"
    Write-Host "格式：$Format"

    if (-not (Test-Path $File)) {
        Write-Color "文件不存在：$File" $Colors.Red
        return
    }

    $output = [System.IO.Path]::ChangeExtension($File, ".$Format")
    $csvData = Import-Csv $File

    switch ($Format) {
        'json' {
            $csvData | ConvertTo-Json | Out-File -FilePath $output -Encoding UTF8
        }
        'xml' {
            $xml = New-Object System.Xml.XmlDocument
            $root = $xml.CreateElement("data")
            $xml.AppendChild($root) | Out-Null

            foreach ($row in $csvData) {
                $record = $xml.CreateElement("record")
                foreach ($prop in $row.PSObject.Properties) {
                    $elem = $xml.CreateElement($prop.Name)
                    $elem.InnerText = $prop.Value
                    $record.AppendChild($elem) | Out-Null
                }
                $root.AppendChild($record) | Out-Null
            }

            $xml.Save($output)
        }
    }

    Write-Color "转换完成：$output" $Colors.Green
}

# 7. 生成文档
function Do-Gen-Docs {
    Write-Color "=== 生成文档 ===" $Colors.Blue

    if (-not (Test-Path $DocsDir)) {
        New-Item -ItemType Directory -Path $DocsDir | Out-Null
    }

    # 生成组件列表
    $content = "# 组件文档索引`n`n"
    $content += "| 组件 | 状态 | 文档 |`n"
    $content += "|------|------|------|`n"

    Get-ChildItem "$ProjectRoot\components" -Directory | ForEach-Object {
        $name = $_.Name
        $readme = "$($_.FullName)\README.md"
        $status = "📋"
        $link = "无"

        if (Test-Path $readme) {
            $status = "✅"
            $link = "[查看](../$name/README.md)"
        }

        $content += "| $name | $status | $link |`n"
    }

    $content | Out-File -FilePath "$DocsDir\components_index.md" -Encoding UTF8

    # 运行 Doxygen
    $doxyfile = "$ProjectRoot\docs\doxygen\Doxyfile.osal"
    if ((Get-Command doxygen -ErrorAction SilentlyContinue) -and (Test-Path $doxyfile)) {
        Write-Host "生成 API 文档..."
        Push-Location $ProjectRoot
        doxygen $doxyfile
        Pop-Location
        Write-Color "API 文档：$ProjectRoot\docs\doxygen\html\index.html" $Colors.Green
    }

    Write-Color "文档索引：$DocsDir\components_index.md" $Colors.Green
}

# 8. 代码审查
function Do-Code-Review {
    param([string]$Target = 'components')

    Write-Color "=== 代码审查 ===" $Colors.Blue
    Write-Host "审查目标：$Target"

    $report = "$ProjectRoot\code_review_$DateStamp.md"

    $content = "# 代码审查报告`n"
    $content += "日期：$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')`n"
    $content += "目标：$Target`n`n"

    # 代码统计
    $content += "## 代码统计`n"
    $cFiles = Get-ChildItem -Path "$ProjectRoot\$Target" -Filter *.c -Recurse -ErrorAction SilentlyContinue
    $hFiles = Get-ChildItem -Path "$ProjectRoot\$Target" -Filter *.h -Recurse -ErrorAction SilentlyContinue
    $loc = ($cFiles + $hFiles | Get-Content | Measure-Object -Line).Lines
    $content += "- 代码行数：$loc`n"

    # 检查 TODO
    $todoCount = (Select-String -Path "$ProjectRoot\$Target\*.c","$ProjectRoot\$Target\*.h" -Pattern "TODO" -Recurse -ErrorAction SilentlyContinue).Count
    $content += "- TODO 注释：$todoCount 个`n"

    # 检查 FIXME
    $fixmeCount = (Select-String -Path "$ProjectRoot\$Target\*.c","$ProjectRoot\$Target\*.h" -Pattern "FIXME" -Recurse -ErrorAction SilentlyContinue).Count
    $content += "- FIXME 注释：$fixmeCount 个`n"

    $content += "`n## 建议`n"
    $content += "1. 处理 FIXME 标记的问题`n"
    $content += "2. 完成 TODO 标记的功能`n"

    $content | Out-File -FilePath $report -Encoding UTF8

    Write-Color "审查报告：$report" $Colors.Green
}

# 主程序
switch ($Command) {
    'backup' { Do-Backup }
    'sync' { Do-Sync -Src $Param1 -Dst $Param2 }
    'init-project' { Do-Init-Project -Name $Param1 }
    'install-deps' { Do-Install-Deps }
    'run-tests' { Do-Run-Tests }
    'csv-convert' { Do-Csv-Convert -File $Param1 -Format $Param2 }
    'gen-docs' { Do-Gen-Docs }
    'code-review' { Do-Code-Review -Target $Param1 }
    'help' { Show-Help }
    default { Show-Help }
}
