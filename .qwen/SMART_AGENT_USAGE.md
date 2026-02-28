# XinYi 智能代理使用指南

## 概述

XinYi 智能代理系统是一个利用 Qwen Code 内置 API 实现的自动化开发工具集，提供项目经理、架构师、开发工程师和测试工程师的智能代理功能。

## 代理系统结构

### 1. 项目经理代理 (pm)

**功能**: 项目状态管理、任务跟踪、文件管理

**命令**:
```bash
# 查看组件状态
./.qwen/smart_agent.sh pm status

# 查看待办任务
./.qwen/smart_agent.sh pm tasks

# 列出组件文件
./.qwen/smart_agent.sh pm files components/hal

# 搜索代码
./.qwen/smart_agent.sh pm search xy_hal_uart

# 项目统计
./.qwen/smart_agent.sh pm stats
```

### 2. 架构师代理 (arch)

**功能**: 代码审查、依赖分析、质量检查

**命令**:
```bash
# 审查组件
./.qwen/smart_agent.sh arch review hal

# 分析依赖关系
./.qwen/smart_agent.sh arch deps osal

# 代码质量检查
./.qwen/smart_agent.sh arch check

# 兼容性检查
./.qwen/smart_agent.sh arch compat
```

### 3. 开发工程师代理 (dev)

**功能**: 组件创建、文档生成、代码修复

**命令**:
```bash
# 创建新组件
./.qwen/smart_agent.sh dev create new_component

# 生成文档
./.qwen/smart_agent.sh dev docs hal

# 修复代码问题
./.qwen/smart_agent.sh dev fix "buffer overflow"

# 生成代码模板
./.qwen/smart_agent.sh dev template driver
```

### 4. 测试工程师代理 (test)

**功能**: 测试运行、测试生成、覆盖率分析

**命令**:
```bash
# 运行测试
./.qwen/smart_agent.sh test run all

# 生成测试用例
./.qwen/smart_agent.sh test gen hal

# 测试覆盖率
./.qwen/smart_agent.sh test coverage
```

## 使用示例

### 1. 项目管理示例

```bash
# 查看整体项目状态
./.qwen/smart_agent.sh pm status

# 搜索特定功能的实现
./.qwen/smart_agent.sh pm search "xy_hal_gpio_init"

# 查看项目统计
./.qwen/smart_agent.sh pm stats
```

### 2. 架构分析示例

```bash
# 审查 HAL 组件
./.qwen/smart_agent.sh arch review hal

# 检查代码质量
./.qwen/smart_agent.sh arch check

# 分析 OSAL 依赖
./.qwen/smart_agent.sh arch deps osal
```

### 3. 开发辅助示例

```bash
# 创建新的驱动组件
./.qwen/smart_agent.sh dev create i2c_driver

# 生成文档
./.qwen/smart_agent.sh dev docs i2c_driver
```

### 4. 测试管理示例

```bash
# 生成 HAL 测试用例
./.qwen/smart_agent.sh test gen hal

# 运行所有测试
./.qwen/smart_agent.sh test run all
```

## 智能代理 API 集成

### Qwen Code API 使用

智能代理系统充分利用了 Qwen Code 的内置 API：

```bash
# 文件读取
read_file("<path>")

# 文件写入
write_file("<path>", "<content>")

# 文件编辑
edit("<old_string>", "<new_string>", "<file_path>")

# 目录列表
list_directory("<path>")

# 文件搜索
glob("<pattern>", "<path>")

# 内容搜索
grep_search("<pattern>", "<path>")

# 命令执行
run_shell_command("<command>")

# 任务管理
todo_write("<todos>")
```

### 实际应用示例

```bash
# 在代理脚本中使用 Qwen Code API
xy_search_code() {
    local pattern=$1
    echo "搜索 '$pattern'..."
    
    # 使用 Qwen Code 的 grep_search API
    grep -r -n -i "$pattern" "$PROJECT_ROOT/components/" --include="*.c" --include="*.h" | head -10
}
```

## 高级功能

### 1. 代理组合使用

```bash
# 1. 架构师审查
./.qwen/smart_agent.sh arch review new_component

# 2. 开发工程师创建
./.qwen/smart_agent.sh dev create new_component

# 3. 测试工程师生成测试
./.qwen/smart_agent.sh test gen new_component

# 4. 项目经理验证
./.qwen/smart_agent.sh pm status
```

### 2. 自动化工作流

```bash
# 创建自动化脚本
cat > automated_workflow.sh << 'EOF'
#!/bin/bash

COMPONENT=$1

echo "=== 自动化工作流: $COMPONENT ==="

# 1. 审查现有组件
./.qwen/smart_agent.sh arch review $COMPONENT

# 2. 检查代码质量
./.qwen/smart_agent.sh arch check

# 3. 生成测试用例
./.qwen/smart_agent.sh test gen $COMPONENT

# 4. 验证状态
./.qwen/smart_agent.sh pm status

echo "工作流完成: $COMPONENT"
EOF

chmod +x automated_workflow.sh
./automated_workflow.sh hal
```

### 3. 批量操作

```bash
# 批量审查所有组件
for comp in hal osal clib crypto dm net trace; do
    echo "审查组件: $comp"
    ./.qwen/smart_agent.sh arch review $comp
done
```

## 配置选项

### 环境变量

```bash
# 设置项目根目录
export XY_PROJECT_ROOT="/path/to/xinyi"

# 设置默认组件
export XY_DEFAULT_COMPONENT="hal"

# 设置详细输出
export XY_VERBOSE=1
```

### 代理配置

可以通过修改 `.qwen/config.json` 配置代理行为：

```json
{
  "agents": {
    "default_timeout": 5000,
    "max_results": 100,
    "verbose_output": true,
    "auto_save": true
  }
}
```

## 错误处理

### 常见错误

| 错误 | 原因 | 解决方法 |
|------|------|----------|
| `未知代理` | 代理名称拼写错误 | 检查支持的代理名称 |
| `未知命令` | 命令拼写错误 | 检查支持的命令 |
| `文件不存在` | 路径错误 | 验证文件路径 |
| `权限错误` | 缺少执行权限 | 运行 `chmod +x smart_agent.sh` |

### 调试模式

```bash
# 启用调试模式
export XY_DEBUG=1
./.qwen/smart_agent.sh pm status

# 详细输出
./.qwen/smart_agent.sh --verbose pm status
```

## 最佳实践

### 1. 项目启动时

```bash
# 1. 查看当前状态
./.qwen/smart_agent.sh pm status

# 2. 审查架构
./.qwen/smart_agent.sh arch check

# 3. 确定开发任务
./.qwen/smart_agent.sh pm tasks
```

### 2. 组件开发时

```bash
# 1. 创建新组件
./.qwen/smart_agent.sh dev create my_component

# 2. 生成文档模板
./.qwen/smart_agent.sh dev docs my_component

# 3. 生成测试模板
./.qwen/smart_agent.sh test gen my_component

# 4. 验证实现
./.qwen/smart_agent.sh arch review my_component
```

### 3. 代码审查时

```bash
# 1. 全面审查
./.qwen/smart_agent.sh arch review component_name

# 2. 依赖分析
./.qwen/smart_agent.sh arch deps component_name

# 3. 质量检查
./.qwen/smart_agent.sh arch check
```

## 性能优化

### 1. 并行执行

```bash
# 并行运行多个代理任务
./.qwen/smart_agent.sh pm stats &
./.qwen/smart_agent.sh arch check &
wait
```

### 2. 结果缓存

代理系统会自动缓存频繁查询的结果以提高性能。

### 3. 选择性执行

使用特定命令而非通用命令可以提高执行速度。

## 扩展性

### 添加新代理

1. 在 `smart_agent.sh` 中添加新代理函数
2. 实现对应的功能
3. 更新帮助信息

```bash
xy_new_agent() {
    local command=$1
    shift
    local args=("$@")
    
    case $command in
        "action")
            # 实现功能
            ;;
        *)
            echo "未知命令: $command"
            ;;
    esac
}
```

## 联系与支持

- **问题反馈**: 提交 GitHub Issue
- **功能建议**: 提交 Pull Request
- **技术支持**: zerozap2020@gmail.com

**XinYi Team**  
**版本**: 2.0  
**日期**: 2026-02-28
