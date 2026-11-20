# XinYi Development Rulebook

[English](#english) | [中文](#chinese)

---

<a name="english"></a>

## English

### Purpose

This rulebook defines **mandatory rules** for all code contributions to the XinYi embedded framework, including code written by human developers and AI code generators. Adherence to these rules ensures consistency, maintainability, and quality across the entire codebase.

---

## 1. Coding Standards

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 2. Logging and Debugging

### 2.1 Mandatory Logging API

Logging conventions are defined in [code_style.md](../../code_style.md), section 6.7. The level names below are project API integration details.

### 2.2 Log Levels

Available log levels (from lowest to highest priority):
- `XY_LOG_LEVEL_VERBOSE` - Detailed trace information
- `XY_LOG_LEVEL_DEBUG` - Debug information
- `XY_LOG_LEVEL_INFO` - Informational messages
- `XY_LOG_LEVEL_WARN` - Warning messages
- `XY_LOG_LEVEL_ERROR` - Error messages
- `XY_LOG_LEVEL_NEVER` - Disable all logging

**Set per-file log level:**
```c
/* At the top of each .c file, before including xy_log.h */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
```

### 2.3 When to Use Each Level

Logging conventions are defined in [code_style.md](../../code_style.md), section 6.7. The level names below are project API integration details.

## 3. Documentation Requirements

### 3.1 Function Documentation

See [code_style.md](../../code_style.md) for documentation, configuration and code examples. Reuse the canonical rules rather than copying templates here.

### 3.2 File Headers

See [code_style.md](../../code_style.md) for documentation, configuration and code examples. Reuse the canonical rules rather than copying templates here.

### 3.3 Module Documentation

Each major component **MUST include**:
- **README.md** - API reference, usage examples, configuration guide
- **Configuration header** (`*_cfg.h`) - Compile-time options with comments
- **Example file** - At least one usage example

---

## 4. Header File Guidelines

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 5. Error Handling

Coding requirements and examples are maintained in [code_style.md](../../code_style.md).

## 6. Memory Management

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

For components with init/deinit, test the documented repeated-call behavior, initialization rollback and resource release.

## 7. Naming Conventions

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 8. Configuration and Portability

Coding requirements and examples are maintained in [code_style.md](../../code_style.md).

Project integration: expose configurable features through Kconfig and CMake, and keep platform dependencies explicit.

## 9. AI Code Generation Rules

### 9.1 Pre-Generation Checklist

Before generating code, AI MUST:
1. ✅ Review [code_style.md](../../code_style.md)
2. ✅ Check this RULEBOOK.md
3. ✅ Examine [.clang-format](.clang-format) settings
4. ✅ Review existing similar modules for consistency

### 9.2 Post-Generation Checklist

After generating code, AI MUST:
1. ✅ Apply clang-format
2. ✅ Replace all `printf()` with `xy_log_*()`
3. ✅ Add Doxygen comments to all functions
4. ✅ Validate all include paths are relative
5. ✅ Ensure error codes are standardized
6. ✅ Add file header comments
7. ✅ Create or update README.md if needed

### 9.3 Code Generation Templates

See [code_style.md](../../code_style.md) for documentation, configuration and code examples. Reuse the canonical rules rather than copying templates here.

## 10. Testing Requirements

### 10.1 Unit Tests

**New modules SHOULD include unit tests:**

```c
/* xy_module_test.c */
#include "xy_module.h"
#include <assert.h>

void test_basic_functionality(void) {
    int ret = xy_module_init();
    assert(ret == 0);
}

int main(void) {
    test_basic_functionality();
    return 0;
}
```

### 10.2 Example Code

**Each module MUST provide at least one example:**

```c
/* xy_module_example.c */
#include "xy_module.h"

void example_basic_usage(void) {
    /* Initialize */
    xy_module_init();

    /* Use API */
    xy_module_do_something();

    /* Cleanup */
    xy_module_deinit();
}
```

---

## 11. Build System Integration

### 11.1 Makefile

If providing a Makefile:
```makefile
# Use standard variables
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
INCLUDES = -I. -I../../xy_clib

# Standard targets
all: lib
clean:
install:
```

### 11.2 CMakeLists.txt

If providing CMake support:
```cmake
cmake_minimum_required(VERSION 3.10)
project(xy_module C)

set(CMAKE_C_STANDARD 99)
add_library(xy_module STATIC module.c)
target_include_directories(xy_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

---

## 12. Version Control

### 12.1 Commit Messages

Use clear, descriptive commit messages:
```
[component] Brief description

Detailed explanation if needed

- Change 1
- Change 2
```

Example:
```
[xy_iso7816] Add SIM card authentication support

Implement 3G/4G mutual authentication according to 3GPP TS 31.102

- Add xy_iso7816_authenticate() function
- Support RAND/AUTN challenge-response
- Return RES, CK, IK keys
```

---

## 13. Common Mistakes to Avoid

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 14. Quick Reference

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 15. Enforcement

**Violations of this rulebook will result in:**
- Code review rejection
- Request for modifications
- Potential revert of commits

**For questions or clarifications:**
- Refer to [code_style.md](../../code_style.md) first
- Check existing code for examples
- Contact project maintainers

---

<a name="chinese"></a>

## 中文

### 目的

本规则手册为 XinYi 嵌入式框架的所有代码贡献定义了**强制性规则**，包括人类开发者和 AI 代码生成器编写的代码。遵守这些规则可确保整个代码库的一致性、可维护性和质量。

---

## 1. 编码标准

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 2. 日志和调试

### 2.1 强制日志 API

日志约定统一见 [code_style.md](../../code_style.md) 第 6.7 节。下列级别名称仅作为项目 API 集成速查。

### 2.2 日志级别

可用日志级别（从最低到最高优先级）：
- `XY_LOG_LEVEL_VERBOSE` - 详细跟踪信息
- `XY_LOG_LEVEL_DEBUG` - 调试信息
- `XY_LOG_LEVEL_INFO` - 信息性消息
- `XY_LOG_LEVEL_WARN` - 警告消息
- `XY_LOG_LEVEL_ERROR` - 错误消息
- `XY_LOG_LEVEL_NEVER` - 禁用所有日志

**设置每个文件的日志级别：**
```c
/* 在每个 .c 文件的顶部，在包含 xy_log.h 之前 */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
```

### 2.3 何时使用每个级别

日志约定统一见 [code_style.md](../../code_style.md) 第 6.7 节。下列级别名称仅作为项目 API 集成速查。

## 3. 文档要求

### 3.1 函数文档

文档、配置与代码示例统一见 [code_style.md](../../code_style.md)，不在本节复制模板或另订风格。

### 3.2 文件头

文档、配置与代码示例统一见 [code_style.md](../../code_style.md)，不在本节复制模板或另订风格。

### 3.3 模块文档

每个主要组件**必须包括**：
- **README.md** - API 参考、使用示例、配置指南
- **配置头文件**（`*_cfg.h`）- 带注释的编译时选项
- **示例文件** - 至少一个使用示例

---

## 4. 头文件指南

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 5. 错误处理

编码要求与示例统一由 [code_style.md](../../code_style.md) 维护。

## 6. 内存管理

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

组件若提供 init/deinit，应通过测试验证文档声明的重复调用语义、初始化失败回滚和资源释放。

## 7. 命名约定

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 8. 配置和可移植性

编码要求与示例统一由 [code_style.md](../../code_style.md) 维护。

项目集成要求：可配置功能应在 Kconfig 与 CMake 中提供对应入口，并明确平台依赖。

## 9. AI 代码生成规则

### 9.1 生成前检查清单

生成代码前，AI 必须：
1. ✅ 审查 [code_style.md](../../code_style.md)
2. ✅ 检查本 RULEBOOK.md
3. ✅ 检查 [.clang-format](.clang-format) 设置
4. ✅ 审查现有类似模块以确保一致性

### 9.2 生成后检查清单

生成代码后，AI 必须：
1. ✅ 应用 clang-format
2. ✅ 将所有 `printf()` 替换为 `xy_log_*()`
3. ✅ 为所有函数添加 Doxygen 注释
4. ✅ 验证所有包含路径都是相对路径
5. ✅ 确保错误代码标准化
6. ✅ 添加文件头注释
7. ✅ 如需要，创建或更新 README.md

### 9.3 代码生成模板

文档、配置与代码示例统一见 [code_style.md](../../code_style.md)，不在本节复制模板或另订风格。

## 10. 测试要求

### 10.1 单元测试

**新模块应该包含单元测试：**

```c
/* xy_module_test.c */
#include "xy_module.h"
#include <assert.h>

void test_basic_functionality(void) {
    int ret = xy_module_init();
    assert(ret == 0);
}

int main(void) {
    test_basic_functionality();
    return 0;
}
```

### 10.2 示例代码

**每个模块必须提供至少一个示例：**

```c
/* xy_module_example.c */
#include "xy_module.h"

void example_basic_usage(void) {
    /* 初始化 */
    xy_module_init();

    /* 使用 API */
    xy_module_do_something();

    /* 清理 */
    xy_module_deinit();
}
```

---

## 11. 构建系统集成

### 11.1 Makefile

如果提供 Makefile：
```makefile
# 使用标准变量
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
INCLUDES = -I. -I../../xy_clib

# 标准目标
all: lib
clean:
install:
```

### 11.2 CMakeLists.txt

如果提供 CMake 支持：
```cmake
cmake_minimum_required(VERSION 3.10)
project(xy_module C)

set(CMAKE_C_STANDARD 99)
add_library(xy_module STATIC module.c)
target_include_directories(xy_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

---

## 12. 版本控制

### 12.1 提交消息

使用清晰、描述性的提交消息：
```
[组件] 简要描述

详细说明（如需要）

- 更改 1
- 更改 2
```

示例：
```
[xy_iso7816] 添加 SIM 卡认证支持

根据 3GPP TS 31.102 实现 3G/4G 双向认证

- 添加 xy_iso7816_authenticate() 函数
- 支持 RAND/AUTN 挑战-响应
- 返回 RES、CK、IK 密钥
```

---

## 13. 常见错误避免

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 14. 快速参考

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 15. 执行

**违反本规则手册将导致：**
- 代码审查被拒绝
- 要求修改
- 可能撤销提交

**如有疑问或需要澄清：**
- 首先参考 [code_style.md](../../code_style.md)
- 检查现有代码作为示例
- 联系项目维护者
