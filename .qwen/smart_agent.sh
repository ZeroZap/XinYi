#!/bin/bash
# XinYi 智能代理 - 利用 Qwen Code 内置 API
# 版本: 1.0
# 日期: 2026-02-28

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="$(dirname "$(dirname "$(dirname "${BASH_SOURCE[0]}")")")"

# 代理主函数
xy_agent() {
    local agent=$1
    local command=$2
    shift 2
    local args=("$@")

    case $agent in
        "pm"|"project-manager")
            xy_pm_agent "$command" "${args[@]}"
            ;;
        "arch"|"architect")
            xy_arch_agent "$command" "${args[@]}"
            ;;
        "dev"|"developer")
            xy_dev_agent "$command" "${args[@]}"
            ;;
        "test"|"tester")
            xy_test_agent "$command" "${args[@]}"
            ;;
        *)
            echo -e "${RED}未知代理: $agent${NC}"
            echo "支持的代理: pm, arch, dev, test"
            return 1
            ;;
    esac
}

# 项目经理代理
xy_pm_agent() {
    local command=$1
    shift
    local args=("$@")

    case $command in
        "status")
            xy_show_component_status
            ;;
        "tasks")
            xy_show_pending_tasks
            ;;
        "files")
            local target=${args[0]:-"components/"}
            xy_list_component_files "$target"
            ;;
        "search")
            local pattern=${args[0]:-"xy_hal"}
            xy_search_code "$pattern"
            ;;
        "stats")
            xy_project_stats
            ;;
        *)
            echo -e "${RED}未知项目管理命令: $command${NC}"
            echo "支持的命令: status, tasks, files [path], search [pattern], stats"
            ;;
    esac
}

# 架构师代理
xy_arch_agent() {
    local command=$1
    shift
    local args=("$@")

    case $command in
        "review")
            local component=${args[0]:-"hal"}
            xy_review_component "$component"
            ;;
        "deps")
            local component=${args[0]:-"hal"}
            xy_analyze_dependencies "$component"
            ;;
        "check")
            xy_code_quality_check
            ;;
        "compat")
            xy_check_compatibility
            ;;
        *)
            echo -e "${RED}未知架构命令: $command${NC}"
            echo "支持的命令: review [component], deps [component], check, compat"
            ;;
    esac
}

# 开发代理
xy_dev_agent() {
    local command=$1
    shift
    local args=("$@")

    case $command in
        "create")
            local component=${args[0]}
            if [ -n "$component" ]; then
                xy_create_component "$component"
            else
                echo -e "${RED}请指定组件名称${NC}"
            fi
            ;;
        "docs")
            local component=${args[0]:-"all"}
            xy_generate_docs "$component"
            ;;
        "fix")
            local issue=${args[0]}
            xy_fix_code_issue "$issue"
            ;;
        "template")
            local type=${args[0]:-"module"}
            xy_generate_template "$type"
            ;;
        *)
            echo -e "${RED}未知开发命令: $command${NC}"
            echo "支持的命令: create [name], docs [component], fix [issue], template [type]"
            ;;
    esac
}

# 测试代理
xy_test_agent() {
    local command=$1
    shift
    local args=("$@")

    case $command in
        "run")
            local target=${args[0]:-"all"}
            xy_run_tests "$target"
            ;;
        "gen")
            local component=${args[0]:-"hal"}
            xy_generate_tests "$component"
            ;;
        "coverage")
            xy_test_coverage
            ;;
        *)
            echo -e "${RED}未知测试命令: $command${NC}"
            echo "支持的命令: run [target], gen [component], coverage"
            ;;
    esac
}

# 显示组件状态
xy_show_component_status() {
    echo -e "${BLUE}=== 组件状态 ===${NC}"

    if [ -f "$PROJECT_ROOT/COMPONENTS_STATUS.md" ]; then
        # 显示状态表的关键部分
        grep -A 20 "| 组件 | 状态 |" "$PROJECT_ROOT/COMPONENTS_STATUS.md" | head -20
    else
        echo -e "${YELLOW}组件状态文件不存在，正在分析...${NC}"

        # 简单分析组件目录
        for dir in "$PROJECT_ROOT"/components/*/; do
            if [ -d "$dir" ]; then
                local name=$(basename "$dir")
                local status="⚠️"

                if [ -f "$dir/README.md" ] && [ -f "$dir/CMakeLists.txt" ]; then
                    if [ -d "$dir/tests" ] || [ -d "$dir/test" ]; then
                        status="✅"
                    else
                        status="⚠️"
                    fi
                else
                    status="❌"
                fi

                echo "  $name: $status"
            fi
        done
    fi
}

# 显示待办任务
xy_show_pending_tasks() {
    echo -e "${BLUE}=== 待办任务 ===${NC}"

    if [ -f "$PROJECT_ROOT/PROJECT_OPTIMIZATION_SUMMARY.md" ]; then
        grep -A 20 "## 待完成任务\|## 任务计划" "$PROJECT_ROOT/PROJECT_OPTIMIZATION_SUMMARY.md" | head -20
    else
        echo "  无任务文件"
    fi
}

# 列出组件文件
xy_list_component_files() {
    local path=$1
    echo -e "${BLUE}=== $path 文件列表 ===${NC}"

    if [ -d "$PROJECT_ROOT/$path" ]; then
        find "$PROJECT_ROOT/$path" -type f -name "*.c" -o -name "*.h" | head -20
    else
        echo -e "${RED}目录不存在: $PROJECT_ROOT/$path${NC}"
    fi
}

# 搜索代码
xy_search_code() {
    local pattern=$1
    echo -e "${BLUE}=== 搜索 '$pattern' ===${NC}"

    # 使用 grep 搜索
    grep -r -n -i "$pattern" "$PROJECT_ROOT/components/" --include="*.c" --include="*.h" | head -10 || echo "未找到匹配项"
}

# 项目统计
xy_project_stats() {
    echo -e "${BLUE}=== 项目统计 ===${NC}"

    local c_files=$(find "$PROJECT_ROOT" -name "*.c" | wc -l)
    local h_files=$(find "$PROJECT_ROOT" -name "*.h" | wc -l)
    local total_lines=$(find "$PROJECT_ROOT" -name "*.c" -o -name "*.h" | xargs -r cat | wc -l)

    echo "  C 文件数: $c_files"
    echo "  H 文件数: $h_files"
    echo "  总行数: $total_lines"
}

# 审查组件
xy_review_component() {
    local component=$1
    echo -e "${BLUE}=== 审查组件: $component ===${NC}"

    local comp_path="$PROJECT_ROOT/components/$component"
    if [ ! -d "$comp_path" ]; then
        echo -e "${RED}组件不存在: $component${NC}"
        return 1
    fi

    # 检查基本文件
    echo "检查文件完整性..."
    local has_inc=$( [ -d "$comp_path/inc" ] && echo "✅" || echo "❌")
    local has_src=$( [ -d "$comp_path/src" ] || [ -f "$comp_path/*.c" ] && echo "✅" || echo "❌")
    local has_test=$( [ -d "$comp_path/tests" ] || [ -d "$comp_path/test" ] && echo "✅" || echo "❌")
    local has_doc=$( [ -f "$comp_path/README.md" ] && echo "✅" || echo "❌")

    echo "  头文件: $has_inc"
    echo "  源文件: $has_src"
    echo "  测试: $has_test"
    echo "  文档: $has_doc"

    # 检查函数注释
    echo "检查代码质量..."
    local c_files=$(find "$comp_path" -name "*.c" 2>/dev/null)
    for file in $c_files; do
        local func_count=$(grep -c "^.*[^;{}]$" "$file" | grep -c "^\w\+\s\+\w\+\s*(" || echo 0)
        local comment_count=$(grep -c "/\*\*" "$file" || echo 0)
        echo "  $(basename $file): $func_count 函数, $comment_count 注释"
    done
}

# 分析依赖
xy_analyze_dependencies() {
    local component=$1
    echo -e "${BLUE}=== 分析 $component 依赖 ===${NC}"

    local comp_path="$PROJECT_ROOT/components/$component"
    if [ ! -d "$comp_path" ]; then
        echo -e "${RED}组件不存在: $component${NC}"
        return 1
    fi

    # 查找包含的头文件
    local includes=$(find "$comp_path" -name "*.c" -o -name "*.h" -exec grep -h "#include" {} \; 2>/dev/null | grep -v '<' | sort | uniq)

    echo "包含的头文件:"
    echo "$includes" | while read line; do
        echo "  $line"
    done
}

# 代码质量检查
xy_code_quality_check() {
    echo -e "${BLUE}=== 代码质量检查 ===${NC}"

    # 检查是否安装了 clang-format
    if command -v clang-format &> /dev/null; then
        echo "clang-format 检查..."
        local bad_files=$(find "$PROJECT_ROOT/components" -name "*.c" -o -name "*.h" 2>/dev/null | head -10 | xargs -r clang-format --dry-run --Werror 2>&1 | grep -v "no problems" | head -5 || echo "")
        if [ -n "$bad_files" ] && [ "$bad_files" != "" ]; then
            echo -e "${YELLOW}格式问题:${NC}"
            echo "$bad_files"
        else
            echo -e "${GREEN}✓ 格式检查通过${NC}"
        fi
    else
        echo -e "${YELLOW}clang-format 未安装，跳过格式检查${NC}"
    fi

    # 检查 TODO/FIXME
    local todos=$(grep -r "TODO\|FIXME" "$PROJECT_ROOT/components/" --include="*.c" --include="*.h" | wc -l)
    echo "待办事项: $todos"
}

# 创建新组件
xy_create_component() {
    local component=$1
    echo -e "${BLUE}=== 创建组件: $component ===${NC}"

    local comp_path="$PROJECT_ROOT/components/$component"
    if [ -d "$comp_path" ]; then
        echo -e "${RED}组件已存在: $component${NC}"
        return 1
    fi

    # 创建目录结构
    mkdir -p "$comp_path/inc"
    mkdir -p "$comp_path/src"
    mkdir -p "$comp_path/tests"

    # 创建基本文件
    cat > "$comp_path/inc/xy_$component.h" << EOF
/**
 * @file xy_$component.h
 * @brief XY $component Module
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_$COMPONENT_UPPER_H
#define XY_$COMPONENT_UPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/* 组件特定定义 */

#ifdef __cplusplus
}
#endif

#endif /* XY_$COMPONENT_UPPER_H */
EOF

    cat > "$comp_path/src/xy_$component.c" << EOF
/**
 * @file xy_$component.c
 * @brief XY $component Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../inc/xy_$component.h"

// 实现组件功能

EOF

    cat > "$comp_path/README.md" << EOF
# XY $component

## 概述

$component 模块提供...

## 功能

- [ ] 功能 1
- [ ] 功能 2

## 使用

\`\`\`c
#include "xy_$component.h"
\`\`\`

## API

### 函数

EOF

    echo -e "${GREEN}组件 $component 创建完成${NC}"
}

# 生成文档
xy_generate_docs() {
    local component=$1
    echo -e "${BLUE}=== 生成 $component 文档 ===${NC}"

    if [ "$component" = "all" ]; then
        for dir in "$PROJECT_ROOT"/components/*/; do
            if [ -d "$dir" ]; then
                local name=$(basename "$dir")
                if [ -f "$dir/inc/xy_$name.h" ]; then
                    echo "生成 $name 文档..."
                fi
            fi
        done
    else
        echo "生成 $component 文档..."
        # 实际文档生成逻辑
    fi
}

# 运行测试
xy_run_tests() {
    local target=$1
    echo -e "${BLUE}=== 运行测试: $target ===${NC}"

    if [ "$target" = "all" ]; then
        echo "运行所有测试..."
        # 这里需要实际的测试执行逻辑
    else
        echo "运行 $target 测试..."
        # 运行特定组件测试
    fi
}

# 生成测试
xy_generate_tests() {
    local component=$1
    echo -e "${BLUE}=== 生成 $component 测试 ===${NC}"

    local test_file="$PROJECT_ROOT/components/$component/tests/test_$component.c"
    cat > "$test_file" << EOF
/**
 * @file test_$component.c
 * @brief $component Unit Tests
 */

#include "unity.h"
#include "xy_$component.h"

void setUp(void) { /* 设置测试环境 */ }
void tearDown(void) { /* 清理测试环境 */ }

void test_$component_init(void) {
    // 测试初始化
    TEST_ASSERT_TRUE(1); // 占位
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_$component_init);
    return UNITY_END();
}
EOF

    echo -e "${GREEN}测试文件创建: $test_file${NC}"
}

# 主入口点
if [ $# -lt 2 ]; then
    echo "用法: $0 <agent> <command> [args...]"
    echo ""
    echo "代理 (agent):"
    echo "  pm/pm     - 项目经理 (status, tasks, files, search, stats)"
    echo "  arch/arch - 架构师 (review, deps, check, compat)"
    echo "  dev/dev   - 开发者 (create, docs, fix, template)"
    echo "  test/test - 测试员 (run, gen, coverage)"
    echo ""
    echo "示例:"
    echo "  $0 pm status"
    echo "  $0 arch review hal"
    echo "  $0 dev create new_component"
    echo "  $0 test gen hal"
    exit 1
fi

# 执行代理
xy_agent "$@"
