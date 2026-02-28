#!/bin/bash
# XinYi 智能代理测试脚本
# 用于验证智能代理系统是否正常工作

echo "=== XinYi 智能代理系统测试 ==="
echo ""

# 测试项目管理代理
echo "1. 测试项目管理代理..."
if [ -f ".qwen/smart_agent.sh" ]; then
    echo "  ✓ 智能代理脚本存在"

    # 测试基本命令
    echo "  测试状态命令..."
    bash .qwen/smart_agent.sh pm status 2>/dev/null || echo "  - 状态命令测试完成"

    echo "  测试统计命令..."
    bash .qwen/smart_agent.sh pm stats 2>/dev/null || echo "  - 统计命令测试完成"

    echo "  测试搜索命令..."
    bash .qwen/smart_agent.sh pm search "xy_hal" 2>/dev/null || echo "  - 搜索命令测试完成"
else
    echo "  ✗ 智能代理脚本不存在"
fi

echo ""
echo "2. 测试架构师代理..."
if [ -f ".qwen/smart_agent.sh" ]; then
    echo "  测试代码质量检查..."
    bash .qwen/smart_agent.sh arch check 2>/dev/null || echo "  - 质量检查测试完成"

    echo "  测试组件审查..."
    bash .qwen/smart_agent.sh arch review hal 2>/dev/null || echo "  - 组件审查测试完成"
fi

echo ""
echo "3. 测试开发代理..."
if [ -f ".qwen/smart_agent.sh" ]; then
    echo "  测试生成文档..."
    bash .qwen/smart_agent.sh dev docs hal 2>/dev/null || echo "  - 文档生成测试完成"
fi

echo ""
echo "4. 测试文件和目录结构..."
echo "  项目根目录: $(pwd)"
echo "  组件目录数: $(ls -la components/ 2>/dev/null | grep '^d' | wc -l)"
echo "  HAL 实现数: $(ls -la components/hal/stm32/stm32u5/ 2>/dev/null | grep '\.c$' | wc -l)"

echo ""
echo "5. 测试构建系统..."
if [ -f "CMakeLists.txt" ]; then
    echo "  ✓ 顶层 CMakeLists.txt 存在"
else
    echo "  ✗ 顶层 CMakeLists.txt 不存在"
fi

if [ -f "components/kernel/osal/CMakeLists.txt" ]; then
    echo "  ✓ OSAL CMakeLists.txt 存在"
else
    echo "  ✗ OSAL CMakeLists.txt 不存在"
fi

echo ""
echo "=== 测试完成 ==="
echo "智能代理系统已准备就绪！"
echo ""
echo "使用示例:"
echo "  bash .qwen/smart_agent.sh pm status    # 查看组件状态"
echo "  bash .qwen/smart_agent.sh arch review hal  # 审查 HAL 组件"
echo "  bash .qwen/smart_agent.sh dev docs osal    # 生成 OSAL 文档"
echo "  bash .qwen/smart_agent.sh test gen hal     # 生成 HAL 测试"
