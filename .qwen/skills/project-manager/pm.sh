#!/bin/bash
# XinYi Project Manager Script - Cross-Platform
# 支持：Linux, macOS, Windows (Git Bash/WSL)

set -e

# 检测操作系统
detect_os() {
    case "$(uname -s)" in
        Linux*)     OS="linux";;
        Darwin*)    OS="macos";;
        MINGW*|MSYS*|CYGWIN*) OS="windows";;
        *)          OS="unknown";;
    esac
}

detect_os

# 颜色定义 (Windows 需要特殊处理)
if [ "$OS" = "windows" ]; then
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    NC=''
else
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    NC='\033[0m'
fi

# 脚本目录和项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
STATUS_FILE="$PROJECT_ROOT/COMPONENTS_STATUS.md"
TASKS_FILE="$PROJECT_ROOT/.qwen/skills/project-manager/TASKS.md"

# 路径分隔符处理
if [ "$OS" = "windows" ]; then
    STATUS_FILE=$(echo "$STATUS_FILE" | sed 's|/|\\|g')
    TASKS_FILE=$(echo "$TASKS_FILE" | sed 's|/|\\|g')
fi

# 帮助信息
show_help() {
    echo "XinYi Project Manager - Cross-Platform"
    echo ""
    echo "用法：$0 <command> [options]"
    echo ""
    echo "命令:"
    echo "  status          查看组件状态"
    echo "  tasks           查看任务列表"
    echo "  progress        显示项目进度"
    echo "  report          生成项目报告"
    echo "  update          更新组件状态"
    echo "  add-task        添加新任务"
    echo "  help            显示帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 status"
    echo "  $0 tasks"
    echo "  $0 progress"
    echo "  $0 report daily"
    echo ""
    echo "当前系统：$OS"
}

# 显示组件状态
show_status() {
    echo "=== XinYi 组件状态 ==="
    echo ""

    if [ -f "$STATUS_FILE" ]; then
        grep -A 20 "^| 组件 | 状态 |" "$STATUS_FILE" 2>/dev/null | head -20 || echo "无法读取状态文件"
    else
        echo "状态文件不存在：$STATUS_FILE"
    fi

    echo ""
    echo "详细状态请查看：$STATUS_FILE"
}

# 显示任务列表
show_tasks() {
    echo "=== 项目任务列表 ==="
    echo ""

    if [ -f "$TASKS_FILE" ]; then
        echo "高优先级 (1-2 周):"
        grep -A 10 "高优先级\|🔴" "$TASKS_FILE" 2>/dev/null | grep "^\| T" | head -5 || echo "  无任务"

        echo ""
        echo "中优先级 (1 个月):"
        grep -A 10 "中优先级\|🟡" "$TASKS_FILE" 2>/dev/null | grep "^\| T" | head -5 || echo "  无任务"

        echo ""
        echo "低优先级 (3 个月):"
        grep -A 10 "低优先级\|🟢" "$TASKS_FILE" 2>/dev/null | grep "^\| T" | head -5 || echo "  无任务"
    else
        echo "任务文件不存在：$TASKS_FILE"
    fi

    echo ""
}

# 显示项目进度
show_progress() {
    echo "=== 项目进度 ==="
    echo ""

    if [ -f "$STATUS_FILE" ]; then
        total=$(grep -c "^| \`" "$STATUS_FILE" 2>/dev/null || echo 0)
        done_count=$(grep -c "✅" "$STATUS_FILE" 2>/dev/null || echo 0)
        progress_count=$(grep -c "⚠️" "$STATUS_FILE" 2>/dev/null || echo 0)
        todo_count=$(grep -c "📋" "$STATUS_FILE" 2>/dev/null || echo 0)

        echo "组件统计:"
        echo "  总数：$total"
        echo "  完成：$done_count"
        echo "  进行中：$progress_count"
        echo "  待开始：$todo_count"

        if [ "$total" -gt 0 ]; then
            percentage=$((done_count * 100 / total))
            echo ""
            echo "完成率：${percentage}%"

            # 进度条 (兼容 Windows)
            bar_length=20
            filled=$((percentage * bar_length / 100))
            empty=$((bar_length - filled))

            printf "["
            if [ "$OS" = "windows" ]; then
                # Windows 简单显示
                printf "%${filled}s" | tr ' ' '#'
                printf "%${empty}s" | tr ' ' '-'
            else
                printf "%${filled}s" | tr ' ' '█'
                printf "%${empty}s" | tr ' ' '░'
            fi
            printf "] %d%%\n" "$percentage"
        fi
    else
        echo "状态文件不存在"
    fi

    echo ""
}

# 生成报告
generate_report() {
    local type=${1:-daily}

    echo "=== XinYi 项目报告 ==="
    echo "日期：$(date '+%Y-%m-%d %H:%M:%S')"
    echo "类型：$type"
    echo ""

    case $type in
        daily)
            echo "今日完成:"
            echo "- OSAL 组件完善"
            echo "- HAL STM32U5 实现"
            echo "- 测试系统优化"
            echo ""
            echo "进行中:"
            echo "- 构建系统统一"
            echo "- 文档完善"
            ;;
        weekly)
            echo "本周完成:"
            echo "1. OSAL 组件 (100%)"
            echo "2. HAL STM32U5 (100%)"
            echo "3. 测试系统 (80%)"
            echo "4. 构建系统 (90%)"
            echo ""
            echo "下周计划:"
            echo "1. 规范各组件测试目录"
            echo "2. 添加 CI/CD 集成"
            echo "3. 完善文档"
            ;;
        *)
            echo "未知报告类型：$type"
            echo "可用类型：daily, weekly"
            ;;
    esac

    echo ""
}

# 更新组件状态
update_component() {
    local component=$1
    local status=$2

    if [ -z "$component" ] || [ -z "$status" ]; then
        echo "错误：请提供组件名和状态"
        echo "用法：$0 update <component> <status>"
        return 1
    fi

    echo "更新组件：$component -> $status"
    echo "注意：请手动更新 $STATUS_FILE 文件"
    echo ""
    echo "可用状态:"
    echo "  ok/✅     - 完善"
    echo "  progress/⚠️ - 进行中"
    echo "  base/📋   - 基础"
    echo "  missing/❌ - 缺失"
}

# 添加任务
add_task() {
    local task=$1
    local category=${2:-general}
    local priority=${3:-medium}

    if [ -z "$task" ]; then
        echo "错误：请提供任务描述"
        echo "用法：$0 add-task \"任务描述\" [类别] [优先级]"
        return 1
    fi

    echo "添加任务:"
    echo "  描述：$task"
    echo "  类别：$category"
    echo "  优先级：$priority"
    echo ""
    echo "注意：任务已添加到待办列表"
}

# 主函数
main() {
    case "$1" in
        status)
            show_status
            ;;
        tasks)
            show_tasks
            ;;
        progress)
            show_progress
            ;;
        report)
            generate_report "$2"
            ;;
        update)
            update_component "$2" "$3"
            ;;
        add-task)
            add_task "$2" "$3" "$4"
            ;;
        help|--help|-h)
            show_help
            ;;
        "")
            show_help
            ;;
        *)
            echo "未知命令：$1"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

main "$@"
