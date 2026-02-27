#!/bin/bash
# XinYi Quick Deploy Script - Linux/macOS/WSL
# 快速部署所有依赖

set -e

echo "=== XinYi 快速部署 ==="
echo ""

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# 检测系统
detect_system() {
    if command -v apt-get &> /dev/null; then
        SYSTEM="debian"
        echo -e "${GREEN}检测到:${NC} Debian/Ubuntu"
    elif command -v yum &> /dev/null; then
        SYSTEM="centos"
        echo -e "${GREEN}检测到:${NC} CentOS/RHEL"
    elif command -v dnf &> /dev/null; then
        SYSTEM="fedora"
        echo -e "${GREEN}检测到:${NC} Fedora"
    elif command -v brew &> /dev/null; then
        SYSTEM="macos"
        echo -e "${GREEN}检测到:${NC} macOS"
    else
        SYSTEM="unknown"
        echo -e "${RED}未知系统${NC}"
    fi
}

# 安装依赖
install_deps() {
    case $SYSTEM in
        debian)
            echo "更新包管理器..."
            sudo apt-get update

            echo "安装核心依赖..."
            sudo apt-get install -y cmake gcc make

            echo "安装可选依赖..."
            sudo apt-get install -y clang-format clang-tidy doxygen graphviz rsync gcov lcov 2>/dev/null || true
            ;;

        centos|fedora)
            echo "安装核心依赖..."
            sudo yum install -y cmake gcc make

            echo "安装可选依赖..."
            sudo yum install -y clang-tools-extra doxygen graphviz rsync 2>/dev/null || true
            ;;

        macos)
            # 检查 Homebrew
            if ! command -v brew &> /dev/null; then
                echo -e "${YELLOW}Homebrew 未安装${NC}"
                echo "正在安装 Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            fi

            echo "安装核心依赖..."
            brew install cmake

            echo "安装可选依赖..."
            brew install llvm doxygen graphviz lcov 2>/dev/null || true
            ;;

        *)
            echo -e "${RED}不支持的系统，请手动安装依赖${NC}"
            return 1
            ;;
    esac
}

# 赋予权限
setup_permissions() {
    echo "设置脚本权限..."

    chmod +x .qwen/skills/project-manager/pm.sh 2>/dev/null || true
    chmod +x .qwen/skills/automation/auto.sh 2>/dev/null || true
    chmod +x .qwen/skills/automation/check_deps.sh 2>/dev/null || true

    echo -e "${GREEN}权限设置完成${NC}"
}

# 验证安装
verify_install() {
    echo ""
    echo "=== 验证安装 ==="

    if command -v cmake &> /dev/null; then
        echo -e "${GREEN}✅${NC} CMake: $(cmake --version | head -1)"
    else
        echo -e "${RED}❌${NC} CMake: 未安装"
    fi

    if command -v gcc &> /dev/null; then
        echo -e "${GREEN}✅${NC} GCC: $(gcc --version | head -1)"
    else
        echo -e "${RED}❌${NC} GCC: 未安装"
    fi

    if command -v make &> /dev/null; then
        echo -e "${GREEN}✅${NC} Make: $(make --version | head -1)"
    else
        echo -e "${RED}❌${NC} Make: 未安装"
    fi

    echo ""
    echo "=== 部署完成 ==="
    echo ""
    echo "使用示例:"
    echo "  # 查看项目状态"
    echo "  ./.qwen/skills/project-manager/pm.sh status"
    echo ""
    echo "  # 自动备份"
    echo "  ./.qwen/skills/automation/auto.sh backup"
    echo ""
    echo "  # 依赖检查"
    echo "  ./.qwen/skills/automation/check_deps.sh"
    echo ""
}

# 主流程
main() {
    detect_system
    install_deps
    setup_permissions
    verify_install
}

main "$@"
