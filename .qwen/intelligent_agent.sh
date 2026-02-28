#!/bin/bash
# XinYi 智能代理系统 - 当前环境可执行版本
# 版本: 1.0
# 日期: 2026-02-28

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 工作目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

# 配置文件
CONFIG_FILE="$PROJECT_ROOT/.qwen/config.json"

# 钩子系统
HOOKS_DIR="$PROJECT_ROOT/.hooks"

# 日志系统
LOG_FILE="$PROJECT_ROOT/logs/agent.log"

# 初始化
init_agent() {
    echo -e "${BLUE}=== 初始化 XinYi 智能代理 ===${NC}"

    # 创建必要目录
    mkdir -p "$HOOKS_DIR"
    mkdir -p "$(dirname "$LOG_FILE")"
    mkdir -p "$PROJECT_ROOT/build"

    # 创建配置文件
    if [ ! -f "$CONFIG_FILE" ]; then
        cat > "$CONFIG_FILE" << EOF
{
    "project_name": "XinYi",
    "version": "2.0",
    "components": [
        "hal",
        "osal",
        "clib",
        "crypto",
        "dm",
        "net",
        "trace"
    ],
    "build_system": "cmake",
    "default_backend": "baremetal"
}
EOF
        echo -e "${GREEN}配置文件创建: $CONFIG_FILE${NC}"
    fi

    echo -e "${GREEN}智能代理初始化完成${NC}"
}

# 项目经理智能体
project_manager_agent() {
    local action=$1
    local target=$2

    case $action in
        "status")
            show_component_status
            ;;
        "tasks")
            show_pending_tasks
            ;;
        "build")
            build_component $target
            ;;
        *)
            echo -e "${RED}未知项目管理命令: $action${NC}"
            echo "用法: $0 pm status|tasks|build [component]"
            ;;
    esac
}

# 架构师智能体
architect_agent() {
    local action=$1
    local target=$2

    case $action in
        "review")
            review_component $target
            ;;
        "design")
            design_component $target
            ;;
        "analyze")
            analyze_architecture
            ;;
        *)
            echo -e "${RED}未知架构师命令: $action${NC}"
            echo "用法: $0 arch review|design|analyze [component]"
            ;;
    esac
}

# 开发工程师智能体
developer_agent() {
    local action=$1
    local target=$2

    case $action in
        "impl")
            implement_component $target
            ;;
        "test")
            test_component $target
            ;;
        "docs")
            generate_docs $target
            ;;
        *)
            echo -e "${RED}未知开发命令: $action${NC}"
            echo "用法: $0 dev impl|test|docs [component]"
            ;;
    esac
}

# 显示组件状态
show_component_status() {
    echo -e "${BLUE}=== 组件状态 ===${NC}"

    for comp in hal osal clib crypto dm net trace; do
        local comp_path="$PROJECT_ROOT/components/$comp"
        if [ -d "$comp_path" ]; then
            local status="✅"
            if [ ! -f "$comp_path/README.md" ]; then
                status="⚠️"
            fi
            if [ ! -d "$comp_path/tests" ]; then
                status="❌"
            fi
            echo -e "  $comp: $status"
        else
            echo -e "  $comp: ❌ (不存在)"
        fi
    done
}

# 显示待办任务
show_pending_tasks() {
    echo -e "${BLUE}=== 待办任务 ===${NC}"

    # 从任务文件读取任务
    local task_file="$PROJECT_ROOT/.qwen/tasks.todo"
    if [ -f "$task_file" ]; then
        cat "$task_file"
    else
        echo "  1. 完善 sensor 组件文档"
        echo "  2. 添加 ipc 组件实现"
        echo "  3. 优化 pm 组件功能"
        echo "  4. 完善 net 组件协议支持"
    fi
}

# 构建组件
build_component() {
    local comp=$1
    if [ -z "$comp" ]; then
        comp="all"
    fi

    echo -e "${BLUE}构建组件: $comp${NC}"

    cd "$PROJECT_ROOT/build"
    if [ "$comp" = "all" ]; then
        cmake .. && make -j$(nproc 2>/dev/null || echo 4)
    else
        cmake .. && make xy_$comp -j$(nproc 2>/dev/null || echo 4)
    fi
}

# 代码审查
review_component() {
    local comp=$1
    if [ -z "$comp" ]; then
        echo -e "${RED}请指定组件名称${NC}"
        return 1
    fi

    echo -e "${BLUE}审查组件: $comp${NC}"

    local comp_path="$PROJECT_ROOT/components/$comp"
    if [ ! -d "$comp_path" ]; then
        echo -e "${RED}组件不存在: $comp${NC}"
        return 1
    fi

    # 检查代码风格
    echo "检查代码风格..."
    find "$comp_path" -name "*.c" -o -name "*.h" | head -10 | xargs -r clang-format --dry-run --Werror 2>/dev/null || echo "  ⚠️ 代码风格检查失败或未安装 clang-format"

    # 检查函数注释
    echo "检查函数注释..."
    local func_count=$(find "$comp_path" -name "*.c" | xargs -r grep -c "^/\*\*" 2>/dev/null || echo 0)
    local total_funcs=$(find "$comp_path" -name "*.c" | xargs -r grep -c "^[a-zA-Z_].*(.*);" 2>/dev/null || echo 0)
    echo "  函数总数: $total_funcs, 已注释: $func_count"
}

# 生成文档
generate_docs() {
    local comp=$1
    if [ -z "$comp" ]; then
        comp="all"
    fi

    echo -e "${BLUE}生成文档: $comp${NC}"

    if command -v doxygen &> /dev/null; then
        echo "使用 Doxygen 生成 API 文档..."
        doxygen "$PROJECT_ROOT/docs/doxygen/Doxyfile.osal" 2>/dev/null || echo "  Doxygen 配置可能需要调整"
    else
        echo -e "${YELLOW}Doxygen 未安装，跳过 API 文档生成${NC}"
    fi
}

# 智能代理主函数
main() {
    if [ $# -lt 2 ]; then
        echo "用法: $0 <agent> <command> [args...]"
        echo ""
        echo "智能体 (agent):"
        echo "  pm     - 项目经理"
        echo "  arch   - 架构师"
        echo "  dev    - 开发工程师"
        echo ""
        echo "示例:"
        echo "  $0 pm status"
        echo "  $0 arch review hal"
        echo "  $0 dev test osal"
        exit 1
    fi

    local agent=$1
    local command=$2
    shift 2

    # 记录日志
    echo "$(date): $agent $command $*" >> "$LOG_FILE"

    case $agent in
        "pm")
            project_manager_agent $command $@
            ;;
        "arch")
            architect_agent $command $@
            ;;
        "dev")
            developer_agent $command $@
            ;;
        *)
            echo -e "${RED}未知智能体: $agent${NC}"
            echo "支持的智能体: pm, arch, dev"
            exit 1
            ;;
    esac
}

# 初始化系统
if [ "$1" = "init" ]; then
    init_agent
    exit 0
fi

# 执行主函数
main $@
