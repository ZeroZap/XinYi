#!/bin/bash
# XinYi Dependency Checker
# 检查自动化脚本所需依赖

set -e

echo "=== XinYi 依赖检查 ==="
echo ""

# 颜色定义
if [ "$(uname)" != "Windows_NT" ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[1;33m'
    NC='\033[0m'
else
    GREEN=''
    RED=''
    YELLOW=''
    NC=''
fi

# 检查命令
check_cmd() {
    local cmd=$1
    local name=$2

    if command -v $cmd &> /dev/null; then
        local version=$($cmd --version 2>&1 | head -1)
        echo -e "${GREEN}✅${NC} $name: $version"
        return 0
    else
        echo -e "${RED}❌${NC} $name: 未安装"
        return 1
    fi
}

# 统计
total=0
installed=0

echo "核心依赖 (必需):"
echo "----------------"
check_cmd "cmake" "CMake" && ((installed++)) || true
((total++))
check_cmd "gcc" "GCC" && ((installed++)) || true
((total++))
check_cmd "make" "Make" && ((installed++)) || true
((total++))
echo ""

echo "推荐依赖 (可选但推荐):"
echo "---------------------"
check_cmd "clang-format" "clang-format" && ((installed++)) || true
((total++))
check_cmd "clang-tidy" "clang-tidy" && ((installed++)) || true
((total++))
check_cmd "doxygen" "Doxygen" && ((installed++)) || true
((total++))
echo ""

echo "可选依赖:"
echo "---------"
check_cmd "rsync" "rsync" && ((installed++)) || true
((total++))
check_cmd "gcov" "gcov" && ((installed++)) || true
((total++))
check_cmd "cppcheck" "cppcheck" && ((installed++)) || true
((total++))
echo ""

echo "================"
echo "已安装：$installed / $total"

if [ $installed -lt $total ]; then
    echo ""
    echo -e "${YELLOW}提示:${NC}"
    echo ""

    # 检测系统并给出安装建议
    if command -v apt-get &> /dev/null; then
        echo "Ubuntu/Debian 安装命令:"
        echo "sudo apt-get install cmake gcc make clang-format clang-tidy doxygen rsync gcov cppcheck"
    elif command -v yum &> /dev/null; then
        echo "CentOS/RHEL 安装命令:"
        echo "sudo yum install cmake gcc make clang-tools-extra doxygen rsync gcc-c++ cppcheck"
    elif command -v brew &> /dev/null; then
        echo "macOS 安装命令:"
        echo "brew install cmake llvm doxygen lcov"
    else
        echo "请根据系统手动安装缺失的依赖"
    fi

    echo ""
    echo "或使用一键安装:"
    echo "./.qwen/skills/automation/auto.sh install-deps"
fi

echo ""
