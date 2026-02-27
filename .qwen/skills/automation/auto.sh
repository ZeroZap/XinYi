#!/bin/bash
# XinYi Automation Script - Cross-Platform
# 功能：自动备份、文件同步、代码审查、文档生成等

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BACKUP_DIR="$PROJECT_ROOT/.backups"
DOCS_DIR="$PROJECT_ROOT/docs/generated"

# 颜色定义
if [ "$(uname)" != "Windows_NT" ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    NC=''
fi

# 帮助信息
show_help() {
    echo "XinYi Automation Tools"
    echo ""
    echo "用法：$0 <command> [options]"
    echo ""
    echo "命令:"
    echo "  backup              自动备份项目"
    echo "  sync <src> <dst>    同步文件"
    echo "  init-project <name> 初始化新项目"
    echo "  install-deps        安装依赖"
    echo "  run-tests           运行测试"
    echo "  csv-convert <file>  CSV 格式转换"
    echo "  gen-docs            生成文档"
    echo "  code-review         代码审查"
    echo "  help                显示帮助"
}

# 1. 自动备份
do_backup() {
    local backup_name="backup_$(date +%Y%m%d_%H%M%S)"
    local backup_path="$BACKUP_DIR/$backup_name"

    echo -e "${BLUE}=== 自动备份 ===${NC}"
    echo "备份目标：$backup_path"

    mkdir -p "$backup_path"

    # 备份重要目录
    for dir in components docs scripts; do
        if [ -d "$PROJECT_ROOT/$dir" ]; then
            echo "备份：$dir"
            cp -r "$PROJECT_ROOT/$dir" "$backup_path/" 2>/dev/null || true
        fi
    done

    # 备份配置文件
    for file in CMakeLists.txt Kconfig Makefile; do
        if [ -f "$PROJECT_ROOT/$file" ]; then
            cp "$PROJECT_ROOT/$file" "$backup_path/" 2>/dev/null || true
        fi
    done

    # 压缩备份
    if command -v tar &> /dev/null; then
        cd "$BACKUP_DIR"
        tar -czf "$backup_name.tar.gz" "$backup_name"
        rm -rf "$backup_name"
        echo -e "${GREEN}备份完成：$BACKUP_DIR/$backup_name.tar.gz${NC}"
    else
        echo -e "${GREEN}备份完成：$backup_path${NC}"
    fi

    # 清理旧备份 (保留最近 7 个)
    cd "$BACKUP_DIR"
    ls -t backup_*.tar.gz 2>/dev/null | tail -n +8 | xargs -r rm
}

# 2. 文件同步
do_sync() {
    local src="$1"
    local dst="$2"

    if [ -z "$src" ] || [ -z "$dst" ]; then
        echo -e "${RED}错误：请提供源目录和目标目录${NC}"
        echo "用法：$0 sync <src> <dst>"
        return 1
    fi

    echo -e "${BLUE}=== 文件同步 ===${NC}"
    echo "源：$src"
    echo "目标：$dst"

    if command -v rsync &> /dev/null; then
        rsync -av --delete "$src/" "$dst/"
    else
        cp -r "$src/"* "$dst/" 2>/dev/null || true
    fi

    echo -e "${GREEN}同步完成${NC}"
}

# 3. 初始化项目
do_init_project() {
    local name="$1"

    if [ -z "$name" ]; then
        echo -e "${RED}错误：请提供项目名称${NC}"
        echo "用法：$0 init-project <name>"
        return 1
    fi

    echo -e "${BLUE}=== 初始化项目：$name ===${NC}"

    local project_dir="$PROJECT_ROOT/projects/$name"
    mkdir -p "$project_dir"

    # 创建项目结构
    cat > "$project_dir/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.12)
project($name C)

set(CMAKE_C_STANDARD 99)

# 添加 XinYi 组件
add_subdirectory(../../components/kernel/osal)
add_subdirectory(../../components/hal)

# 项目源码
add_executable($name
    src/main.c
)

target_link_libraries($name
    xy_osal
)
EOF

    mkdir -p "$project_dir/src"
    mkdir -p "$project_dir/include"

    cat > "$project_dir/src/main.c" << EOF
/**
 * @file main.c
 * @brief $name Main Entry
 */

#include <stdio.h>

int main(void)
{
    printf("$name starting...\\n");

    // TODO: Add your code here

    return 0;
}
EOF

    cat > "$project_dir/README.md" << EOF
# $name

## 概述

TODO: 项目描述

## 构建

\`\`\`bash
mkdir build && cd build
cmake ..
make
\`\`\`

## 使用

\`\`\`bash
./$name
\`\`\`
EOF

    echo -e "${GREEN}项目初始化完成：$project_dir${NC}"
}

# 4. 安装依赖
do_install_deps() {
    echo -e "${BLUE}=== 安装依赖 ===${NC}"

    # 检测包管理器
    if command -v apt-get &> /dev/null; then
        echo "使用 apt-get 安装依赖..."
        sudo apt-get update
        sudo apt-get install -y cmake gcc make
    elif command -v yum &> /dev/null; then
        echo "使用 yum 安装依赖..."
        sudo yum install -y cmake gcc make
    elif command -v brew &> /dev/null; then
        echo "使用 brew 安装依赖..."
        brew install cmake
    elif command -v choco &> /dev/null; then
        echo "使用 choco 安装依赖..."
        choco install -y cmake mingw make
    else
        echo -e "${YELLOW}未检测到包管理器，请手动安装:${NC}"
        echo "- CMake: https://cmake.org/download/"
        echo "- GCC: https://gcc.gnu.org/"
    fi

    # 安装 Git 子模块
    if [ -f "$PROJECT_ROOT/.gitmodules" ]; then
        echo "初始化 Git 子模块..."
        cd "$PROJECT_ROOT"
        git submodule update --init --recursive
    fi

    echo -e "${GREEN}依赖安装完成${NC}"
}

# 5. 运行测试
do_run_tests() {
    echo -e "${BLUE}=== 运行测试 ===${NC}"

    local build_dir="$PROJECT_ROOT/build"
    mkdir -p "$build_dir"
    cd "$build_dir"

    cmake .. -DBUILD_TESTING=ON
    make -j$(nproc 2>/dev/null || echo 4)

    echo -e "${BLUE}运行测试...${NC}"
    ctest --output-on-failure

    echo -e "${GREEN}测试完成${NC}"
}

# 6. CSV 转换
do_csv_convert() {
    local file="$1"
    local format="${2:-json}"

    if [ -z "$file" ]; then
        echo -e "${RED}错误：请提供 CSV 文件路径${NC}"
        return 1
    fi

    echo -e "${BLUE}=== CSV 转换 ===${NC}"
    echo "文件：$file"
    echo "格式：$format"

    if [ ! -f "$file" ]; then
        echo -e "${RED}文件不存在：$file${NC}"
        return 1
    fi

    local output="${file%.*}.$format"

    case $format in
        json)
            # CSV to JSON
            echo "[" > "$output"
            local first=true
            local headers=""
            while IFS=, read -r line; do
                if [ -z "$headers" ]; then
                    headers="$line"
                    continue
                fi
                if [ "$first" = true ]; then
                    first=false
                else
                    echo "," >> "$output"
                fi
                echo "  {$line}" >> "$output"
            done < "$file"
            echo "]" >> "$output"
            ;;
        xml)
            # CSV to XML
            echo '<?xml version="1.0" encoding="UTF-8"?>' > "$output"
            echo '<data>' >> "$output"
            local headers=""
            while IFS=, read -r line; do
                if [ -z "$headers" ]; then
                    headers="$line"
                    continue
                fi
                echo "  <record>$line</record>" >> "$output"
            done < "$file"
            echo '</data>' >> "$output"
            ;;
        *)
            echo -e "${RED}不支持的格式：$format${NC}"
            return 1
            ;;
    esac

    echo -e "${GREEN}转换完成：$output${NC}"
}

# 7. 生成文档
do_gen_docs() {
    echo -e "${BLUE}=== 生成文档 ===${NC}"

    mkdir -p "$DOCS_DIR"

    # 生成组件列表
    echo "# 组件文档索引" > "$DOCS_DIR/components_index.md"
    echo "" >> "$DOCS_DIR/components_index.md"
    echo "| 组件 | 状态 | 文档 |" >> "$DOCS_DIR/components_index.md"
    echo "|------|------|------|" >> "$DOCS_DIR/components_index.md"

    for dir in "$PROJECT_ROOT"/components/*/; do
        if [ -d "$dir" ]; then
            local name=$(basename "$dir")
            local readme="$dir/README.md"
            local status="📋"
            local link="无"

            if [ -f "$readme" ]; then
                link="[查看](../$name/README.md)"
                status="✅"
            fi

            echo "| $name | $status | $link |" >> "$DOCS_DIR/components_index.md"
        fi
    done

    # 运行 Doxygen (如果可用)
    if command -v doxygen &> /dev/null && [ -f "$PROJECT_ROOT/docs/doxygen/Doxyfile.osal" ]; then
        echo "生成 API 文档..."
        cd "$PROJECT_ROOT"
        doxygen docs/doxygen/Doxyfile.osal
        echo -e "${GREEN}API 文档：$PROJECT_ROOT/docs/doxygen/html/index.html${NC}"
    fi

    echo -e "${GREEN}文档索引：$DOCS_DIR/components_index.md${NC}"
}

# 8. 代码审查
do_code_review() {
    local target="${1:-components}"

    echo -e "${BLUE}=== 代码审查 ===${NC}"
    echo "审查目标：$target"

    local report="$PROJECT_ROOT/code_review_$(date +%Y%m%d).md"

    echo "# 代码审查报告" > "$report"
    echo "日期：$(date '+%Y-%m-%d %H:%M:%S')" >> "$report"
    echo "目标：$target" >> "$report"
    echo "" >> "$report"

    # 检查代码规范
    echo "## 代码规范检查" >> "$report"

    if command -v clang-format &> /dev/null; then
        echo "运行 clang-format 检查..."
        local format_issues=0
        for file in $(find "$PROJECT_ROOT/$target" -name "*.c" -o -name "*.h" 2>/dev/null | head -20); do
            if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
                format_issues=$((format_issues + 1))
                echo "- 格式问题：$file" >> "$report"
            fi
        done
        echo "格式问题数：$format_issues" >> "$report"
    else
        echo "clang-format 未安装，跳过" >> "$report"
    fi

    echo "" >> "$report"

    # 检查潜在问题
    echo "## 潜在问题" >> "$report"

    # 检查 TODO
    local todo_count=$(grep -r "TODO" "$PROJECT_ROOT/$target" 2>/dev/null | wc -l)
    echo "- TODO 注释：$todo_count 个" >> "$report"

    # 检查 FIXME
    local fixme_count=$(grep -r "FIXME" "$PROJECT_ROOT/$target" 2>/dev/null | wc -l)
    echo "- FIXME 注释：$fixme_count 个" >> "$report"

    # 检查长函数
    echo "" >> "$report"
    echo "## 代码统计" >> "$report"
    local loc=$(find "$PROJECT_ROOT/$target" -name "*.c" -o -name "*.h" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
    echo "- 代码行数：$loc" >> "$report"

    echo "" >> "$report"
    echo "## 建议" >> "$report"
    echo "1. 修复所有 clang-format 格式问题" >> "$report"
    echo "2. 处理 FIXME 标记的问题" >> "$report"
    echo "3. 完成 TODO 标记的功能" >> "$report"

    echo -e "${GREEN}审查报告：$report${NC}"
}

# 主函数
main() {
    case "$1" in
        backup)
            do_backup
            ;;
        sync)
            do_sync "$2" "$3"
            ;;
        init-project)
            do_init_project "$2"
            ;;
        install-deps)
            do_install_deps
            ;;
        run-tests)
            do_run_tests
            ;;
        csv-convert)
            do_csv_convert "$2" "$3"
            ;;
        gen-docs)
            do_gen_docs
            ;;
        code-review)
            do_code_review "$2"
            ;;
        help|"")
            show_help
            ;;
        *)
            echo -e "${RED}未知命令：$1${NC}"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
