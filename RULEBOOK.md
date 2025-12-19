# XinYi Development Rulebook

[English](#english) | [中文](#chinese)

---

<a name="english"></a>

## English

### Purpose

This rulebook defines **mandatory rules** for all code contributions to the XinYi embedded framework, including code written by human developers and AI code generators. Adherence to these rules ensures consistency, maintainability, and quality across the entire codebase.

---

## 1. Coding Standards

### 1.1 Primary Reference

**ALL code MUST follow** the detailed conventions in **[xy_code_style.md](xy_code_style.md)**. This is the authoritative coding standard document.

Key requirements from xy_code_style.md:
- Use **C99 standard**
- **4 spaces** per indent level (NO tabs)
- Lowercase function/variable names with optional underscores (`my_function`, `my_var`)
- Opening braces on same line as keywords (`if`, `for`, `while`)
- Always use braces for compound statements (even single-line)
- Use `const` for pointers and parameters that should not be modified
- Never use Variable Length Arrays (VLA) - use `malloc()` instead

### 1.2 Code Formatting

**ALL code MUST be formatted** using the **[.clang-format](.clang-format)** configuration file.

**Before committing:**
```bash
# Format a single file
clang-format -i path/to/file.c

# Format all staged files
./utils/script/format_staged.sh

# Format specific directory
./utils/script/format_path.sh path/to/directory
```

**AI Code Generators:** Always apply clang-format to generated code before output.

---

## 2. Logging and Debugging

### 2.1 Mandatory Logging API

**NEVER use `printf()` or `puts()` directly.**

**ALWAYS use the `xy_log` API** from `components/trace/xy_log/inc/xy_log.h`:

```c
#include "xy_log.h"

/* Set module log level */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* Use appropriate log level */
xy_log_e("Error: %s failed with code %d\n", func_name, error_code);  /* Error */
xy_log_w("Warning: low memory (%zu bytes)\n", available);            /* Warning */
xy_log_i("Info: initialized successfully\n");                        /* Info */
xy_log_d("Debug: value = 0x%08X\n", value);                          /* Debug */
xy_log_v("Verbose: entering function %s\n", __FUNCTION__);           /* Verbose */
```

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

| Level | When to Use | Example |
|-------|-------------|---------|
| `xy_log_e()` | Fatal errors, failures | `xy_log_e("UART init failed: %d\n", ret);` |
| `xy_log_w()` | Recoverable issues | `xy_log_w("Buffer nearly full: %d/%d\n", used, total);` |
| `xy_log_i()` | Important events | `xy_log_i("System initialized\n");` |
| `xy_log_d()` | Debug information | `xy_log_d("Processing packet: len=%d\n", len);` |
| `xy_log_v()` | Verbose traces | `xy_log_v("Entering %s()\n", __FUNCTION__);` |

---

## 3. Documentation Requirements

### 3.1 Function Documentation

**EVERY function MUST have Doxygen-style documentation**, even static functions.

```c
/**
 * @brief Send data via UART
 *
 * This function transmits data through the specified UART interface
 * with blocking behavior and timeout support.
 *
 * @param uart UART handle
 * @param data Pointer to data buffer
 * @param len Number of bytes to send
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes sent on success, negative error code on failure
 */
int xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms);
```

### 3.2 File Headers

**EVERY source/header file MUST include:**

```c
/**
 * @file filename.c
 * @brief Brief description of the module
 * @version X.Y.Z
 * @date YYYY-MM-DD
 */
```

### 3.3 Module Documentation

Each major component **MUST include**:
- **README.md** - API reference, usage examples, configuration guide
- **Configuration header** (`*_cfg.h`) - Compile-time options with comments
- **Example file** - At least one usage example

---

## 4. Header File Guidelines

### 4.1 Include Guards

Always use traditional include guards (not `#pragma once`):

```c
#ifndef XY_MODULE_NAME_H
#define XY_MODULE_NAME_H

/* ... content ... */

#endif /* XY_MODULE_NAME_H */
```

### 4.2 C++ Compatibility

Always include C++ extern guards:

```c
#ifdef __cplusplus
extern "C" {
#endif

/* ... declarations ... */

#ifdef __cplusplus
}
#endif
```

### 4.3 Include Paths

Use **relative paths** for project headers:

```c
/* Good */
#include "../../xy_clib/xy_typedef.h"
#include "../../../bsp/xy_hal/inc/xy_hal_uart.h"

/* Wrong - will fail in different toolchains */
#include "xy_typedef.h"
#include "xy_hal_uart.h"
```

---

## 5. Error Handling

### 5.1 Return Value Conventions

**Functions MUST return:**
- **0 or positive** for success (or bytes transferred)
- **Negative values** for errors (using standardized error codes)

```c
/* Good */
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config) {
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;  /* Negative error code */
    }
    /* ... */
    return XY_HAL_OK;  /* 0 for success */
}

/* Wrong - using -1 instead of specific error code */
int bad_function(void *ptr) {
    if (!ptr) {
        return -1;  /* Wrong: use specific error codes */
    }
    return 0;
}
```

### 5.2 Parameter Validation

**ALWAYS validate function parameters** at the beginning:

```c
int my_function(void *handle, const uint8_t *data, size_t len) {
    /* Validate ALL parameters first */
    if (!handle || !data || len == 0) {
        xy_log_e("Invalid parameters\n");
        return -EINVAL;
    }

    /* ... implementation ... */
}
```

### 5.3 Error Logging

**ALWAYS log errors** before returning error codes:

```c
if (ret != XY_HAL_OK) {
    xy_log_e("UART init failed: error code %d\n", ret);
    return ret;
}
```

---

## 6. Memory Management

### 6.1 Dynamic Allocation

**Use standard C memory functions:**

```c
/* Good */
uint8_t *buffer = malloc(sizeof(*buffer) * size);
if (!buffer) {
    xy_log_e("Memory allocation failed\n");
    return -ENOMEM;
}
/* ... use buffer ... */
free(buffer);

/* Wrong - VLA is forbidden */
void bad_function(size_t size) {
    uint8_t buffer[size];  /* NEVER use VLA */
}
```

### 6.2 Global Variables

**DO NOT initialize global variables** at declaration. Initialize in dedicated `init()` function:

```c
/* Good */
static int32_t global_counter;
static uint8_t *global_buffer;

void my_module_init(void) {
    global_counter = 0;
    global_buffer = malloc(1024);
}

/* Wrong - may not work correctly in embedded systems */
static int32_t global_counter = 0;  /* Don't initialize here */
```

---

## 7. Naming Conventions

### 7.1 Module Prefix

**ALL public functions/types MUST use module prefix:**

```c
/* HAL module */
xy_hal_error_t xy_hal_uart_init(...);
xy_hal_uart_config_t config;

/* ISO7816 module */
xy_iso7816_error_t xy_iso7816_init(...);
xy_iso7816_handle_t handle;
```

### 7.2 Type Suffixes

**Use consistent type suffixes:**

```c
typedef struct { ... } my_struct_t;         /* _t for typedef structs */
typedef enum { ... } my_enum_t;             /* _t for typedef enums */
typedef void (*my_callback_fn)(void *arg); /* _fn for function pointers */
```

### 7.3 Private Functions

**Use `prv_` prefix for static (private) functions:**

```c
static void prv_internal_helper(void) {
    /* ... */
}
```

---

## 8. Configuration and Portability

### 8.1 Configuration Headers

**Each module SHOULD provide a configuration header** (`*_cfg.h`):

```c
/* xy_module_cfg.h */
#ifndef XY_MODULE_CFG_H
#define XY_MODULE_CFG_H

/**
 * @brief Enable debug mode
 */
#ifndef XY_MODULE_CFG_DEBUG
#define XY_MODULE_CFG_DEBUG    1
#endif

/**
 * @brief Default timeout in milliseconds
 */
#ifndef XY_MODULE_CFG_TIMEOUT
#define XY_MODULE_CFG_TIMEOUT  1000
#endif

#endif /* XY_MODULE_CFG_H */
```

### 8.2 Platform Abstraction

**Use HAL interfaces for platform-specific operations:**

```c
/* Good - uses HAL abstraction */
xy_hal_uart_send(uart, data, len, timeout);

/* Wrong - direct hardware access */
UART1->DR = data[0];  /* Platform-specific, not portable */
```

---

## 9. AI Code Generation Rules

### 9.1 Pre-Generation Checklist

Before generating code, AI MUST:
1. ✅ Review [xy_code_style.md](xy_code_style.md)
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

**Function template:**
```c
/**
 * @brief [Brief description]
 * @param [param] [description]
 * @return [return value description]
 */
[return_type] [module_prefix_function_name]([parameters]) {
    /* Validate parameters */
    if ([invalid_condition]) {
        xy_log_e("[Error message]\n");
        return [ERROR_CODE];
    }

    /* Implementation */
    [implementation_code]

    /* Log success if appropriate */
    xy_log_d("[Success message]\n");

    return [SUCCESS_VALUE];
}
```

---

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

### ❌ DON'T

```c
/* Don't use printf */
printf("Hello\n");

/* Don't use tabs */
 int x = 5;

/* Don't use VLA */
void func(int n) {
    int arr[n];
}

/* Don't initialize globals */
static int counter = 0;

/* Don't use generic error codes */
return -1;

/* Don't skip parameter validation */
void func(void *ptr) {
    ptr->data = 0;  /* No NULL check! */
}
```

### ✅ DO

```c
/* Use xy_log */
xy_log_i("Hello\n");

/* Use spaces */
    int x = 5;

/* Use malloc */
void func(int n) {
    int *arr = malloc(sizeof(*arr) * n);
    if (!arr) return;
    /* ... */
    free(arr);
}

/* Initialize in init function */
static int counter;
void init(void) { counter = 0; }

/* Use specific error codes */
return XY_HAL_ERROR_TIMEOUT;

/* Always validate */
void func(void *ptr) {
    if (!ptr) {
        xy_log_e("NULL pointer\n");
        return;
    }
    ptr->data = 0;
}
```

---

## 14. Quick Reference

| Rule | Requirement |
|------|-------------|
| **Code Style** | Follow [xy_code_style.md](xy_code_style.md) |
| **Formatting** | Use [.clang-format](.clang-format) |
| **Logging** | Use `xy_log_*()`, NOT `printf()` |
| **Documentation** | Doxygen comments for ALL functions |
| **Indentation** | 4 spaces, NO tabs |
| **Naming** | lowercase_with_underscores |
| **Errors** | Specific error codes, not -1 |
| **Memory** | malloc/free, NO VLA |
| **Includes** | Relative paths |
| **Headers** | Include guards + C++ extern |

---

## 15. Enforcement

**Violations of this rulebook will result in:**
- Code review rejection
- Request for modifications
- Potential revert of commits

**For questions or clarifications:**
- Refer to [xy_code_style.md](xy_code_style.md) first
- Check existing code for examples
- Contact project maintainers

---

<a name="chinese"></a>

## 中文

### 目的

本规则手册为 XinYi 嵌入式框架的所有代码贡献定义了**强制性规则**，包括人类开发者和 AI 代码生成器编写的代码。遵守这些规则可确保整个代码库的一致性、可维护性和质量。

---

## 1. 编码标准

### 1.1 主要参考

**所有代码必须遵循** **[xy_code_style.md](xy_code_style.md)** 中的详细约定。这是权威的编码标准文档。

xy_code_style.md 的关键要求：
- 使用 **C99 标准**
- 每个缩进级别 **4 个空格**（禁止使用制表符）
- 函数/变量名小写，可选下划线（`my_function`、`my_var`）
- 左花括号与关键字在同一行（`if`、`for`、`while`）
- 复合语句始终使用花括号（即使是单行）
- 对不应修改的指针和参数使用 `const`
- 永远不要使用可变长度数组（VLA）- 使用 `malloc()` 代替

### 1.2 代码格式化

**所有代码必须** 使用 **[.clang-format](.clang-format)** 配置文件进行格式化。

**提交前：**
```bash
# 格式化单个文件
clang-format -i path/to/file.c

# 格式化所有暂存文件
./utils/script/format_staged.sh

# 格式化特定目录
./utils/script/format_path.sh path/to/directory
```

**AI 代码生成器：** 在输出前始终对生成的代码应用 clang-format。

---

## 2. 日志和调试

### 2.1 强制日志 API

**永远不要直接使用 `printf()` 或 `puts()`。**

**始终使用 `xy_log` API**，来自 `components/trace/xy_log/inc/xy_log.h`：

```c
#include "xy_log.h"

/* 设置模块日志级别 */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 使用适当的日志级别 */
xy_log_e("错误: %s 失败，代码 %d\n", func_name, error_code);    /* 错误 */
xy_log_w("警告: 内存不足 (%zu 字节)\n", available);             /* 警告 */
xy_log_i("信息: 初始化成功\n");                                /* 信息 */
xy_log_d("调试: 值 = 0x%08X\n", value);                        /* 调试 */
xy_log_v("详细: 进入函数 %s\n", __FUNCTION__);                 /* 详细 */
```

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

| 级别 | 使用时机 | 示例 |
|------|---------|------|
| `xy_log_e()` | 致命错误、失败 | `xy_log_e("UART 初始化失败: %d\n", ret);` |
| `xy_log_w()` | 可恢复的问题 | `xy_log_w("缓冲区接近满: %d/%d\n", used, total);` |
| `xy_log_i()` | 重要事件 | `xy_log_i("系统已初始化\n");` |
| `xy_log_d()` | 调试信息 | `xy_log_d("处理数据包: len=%d\n", len);` |
| `xy_log_v()` | 详细跟踪 | `xy_log_v("进入 %s()\n", __FUNCTION__);` |

---

## 3. 文档要求

### 3.1 函数文档

**每个函数都必须有 Doxygen 风格的文档**，即使是静态函数。

```c
/**
 * @brief 通过 UART 发送数据
 *
 * 此函数通过指定的 UART 接口传输数据，
 * 具有阻塞行为和超时支持。
 *
 * @param uart UART 句柄
 * @param data 数据缓冲区指针
 * @param len 要发送的字节数
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功时返回发送的字节数，失败时返回负错误代码
 */
int xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms);
```

### 3.2 文件头

**每个源文件/头文件必须包含：**

```c
/**
 * @file filename.c
 * @brief 模块的简要描述
 * @version X.Y.Z
 * @date YYYY-MM-DD
 */
```

### 3.3 模块文档

每个主要组件**必须包括**：
- **README.md** - API 参考、使用示例、配置指南
- **配置头文件**（`*_cfg.h`）- 带注释的编译时选项
- **示例文件** - 至少一个使用示例

---

## 4. 头文件指南

### 4.1 包含保护

始终使用传统的包含保护（不使用 `#pragma once`）：

```c
#ifndef XY_MODULE_NAME_H
#define XY_MODULE_NAME_H

/* ... 内容 ... */

#endif /* XY_MODULE_NAME_H */
```

### 4.2 C++ 兼容性

始终包含 C++ extern 保护：

```c
#ifdef __cplusplus
extern "C" {
#endif

/* ... 声明 ... */

#ifdef __cplusplus
}
#endif
```

### 4.3 包含路径

对项目头文件使用**相对路径**：

```c
/* 好的 */
#include "../../xy_clib/xy_typedef.h"
#include "../../../bsp/xy_hal/inc/xy_hal_uart.h"

/* 错误 - 在不同的工具链中会失败 */
#include "xy_typedef.h"
#include "xy_hal_uart.h"
```

---

## 5. 错误处理

### 5.1 返回值约定

**函数必须返回：**
- **0 或正值** 表示成功（或传输的字节数）
- **负值** 表示错误（使用标准化的错误代码）

```c
/* 好的 */
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config) {
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;  /* 负错误代码 */
    }
    /* ... */
    return XY_HAL_OK;  /* 0 表示成功 */
}

/* 错误 - 使用 -1 而不是特定错误代码 */
int bad_function(void *ptr) {
    if (!ptr) {
        return -1;  /* 错误：使用特定错误代码 */
    }
    return 0;
}
```

### 5.2 参数验证

**始终在开始时验证函数参数**：

```c
int my_function(void *handle, const uint8_t *data, size_t len) {
    /* 首先验证所有参数 */
    if (!handle || !data || len == 0) {
        xy_log_e("无效参数\n");
        return -EINVAL;
    }

    /* ... 实现 ... */
}
```

### 5.3 错误日志

**在返回错误代码前始终记录错误**：

```c
if (ret != XY_HAL_OK) {
    xy_log_e("UART 初始化失败: 错误代码 %d\n", ret);
    return ret;
}
```

---

## 6. 内存管理

### 6.1 动态分配

**使用标准 C 内存函数：**

```c
/* 好的 */
uint8_t *buffer = malloc(sizeof(*buffer) * size);
if (!buffer) {
    xy_log_e("内存分配失败\n");
    return -ENOMEM;
}
/* ... 使用 buffer ... */
free(buffer);

/* 错误 - 禁止使用 VLA */
void bad_function(size_t size) {
    uint8_t buffer[size];  /* 永远不要使用 VLA */
}
```

### 6.2 全局变量

**不要在声明时初始化全局变量**。在专用的 `init()` 函数中初始化：

```c
/* 好的 */
static int32_t global_counter;
static uint8_t *global_buffer;

void my_module_init(void) {
    global_counter = 0;
    global_buffer = malloc(1024);
}

/* 错误 - 在嵌入式系统中可能无法正常工作 */
static int32_t global_counter = 0;  /* 不要在这里初始化 */
```

---

## 7. 命名约定

### 7.1 模块前缀

**所有公共函数/类型必须使用模块前缀：**

```c
/* HAL 模块 */
xy_hal_error_t xy_hal_uart_init(...);
xy_hal_uart_config_t config;

/* ISO7816 模块 */
xy_iso7816_error_t xy_iso7816_init(...);
xy_iso7816_handle_t handle;
```

### 7.2 类型后缀

**使用一致的类型后缀：**

```c
typedef struct { ... } my_struct_t;         /* typedef 结构体用 _t */
typedef enum { ... } my_enum_t;             /* typedef 枚举用 _t */
typedef void (*my_callback_fn)(void *arg); /* 函数指针用 _fn */
```

### 7.3 私有函数

**静态（私有）函数使用 `prv_` 前缀：**

```c
static void prv_internal_helper(void) {
    /* ... */
}
```

---

## 8. 配置和可移植性

### 8.1 配置头文件

**每个模块应该提供配置头文件**（`*_cfg.h`）：

```c
/* xy_module_cfg.h */
#ifndef XY_MODULE_CFG_H
#define XY_MODULE_CFG_H

/**
 * @brief 启用调试模式
 */
#ifndef XY_MODULE_CFG_DEBUG
#define XY_MODULE_CFG_DEBUG    1
#endif

/**
 * @brief 默认超时时间（毫秒）
 */
#ifndef XY_MODULE_CFG_TIMEOUT
#define XY_MODULE_CFG_TIMEOUT  1000
#endif

#endif /* XY_MODULE_CFG_H */
```

### 8.2 平台抽象

**对平台特定操作使用 HAL 接口：**

```c
/* 好的 - 使用 HAL 抽象 */
xy_hal_uart_send(uart, data, len, timeout);

/* 错误 - 直接硬件访问 */
UART1->DR = data[0];  /* 平台特定，不可移植 */
```

---

## 9. AI 代码生成规则

### 9.1 生成前检查清单

生成代码前，AI 必须：
1. ✅ 审查 [xy_code_style.md](xy_code_style.md)
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

**函数模板：**
```c
/**
 * @brief [简要描述]
 * @param [参数] [描述]
 * @return [返回值描述]
 */
[返回类型] [模块前缀_函数名]([参数]) {
    /* 验证参数 */
    if ([无效条件]) {
        xy_log_e("[错误消息]\n");
        return [错误代码];
    }

    /* 实现 */
    [实现代码]

    /* 如适当，记录成功 */
    xy_log_d("[成功消息]\n");

    return [成功值];
}
```

---

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

### ❌ 不要

```c
/* 不要使用 printf */
printf("你好\n");

/* 不要使用制表符 */
 int x = 5;

/* 不要使用 VLA */
void func(int n) {
    int arr[n];
}

/* 不要初始化全局变量 */
static int counter = 0;

/* 不要使用通用错误代码 */
return -1;

/* 不要跳过参数验证 */
void func(void *ptr) {
    ptr->data = 0;  /* 没有 NULL 检查！ */
}
```

### ✅ 要

```c
/* 使用 xy_log */
xy_log_i("你好\n");

/* 使用空格 */
    int x = 5;

/* 使用 malloc */
void func(int n) {
    int *arr = malloc(sizeof(*arr) * n);
    if (!arr) return;
    /* ... */
    free(arr);
}

/* 在 init 函数中初始化 */
static int counter;
void init(void) { counter = 0; }

/* 使用特定错误代码 */
return XY_HAL_ERROR_TIMEOUT;

/* 始终验证 */
void func(void *ptr) {
    if (!ptr) {
        xy_log_e("空指针\n");
        return;
    }
    ptr->data = 0;
}
```

---

## 14. 快速参考

| 规则 | 要求 |
|------|------|
| **代码风格** | 遵循 [xy_code_style.md](xy_code_style.md) |
| **格式化** | 使用 [.clang-format](.clang-format) |
| **日志** | 使用 `xy_log_*()`，不用 `printf()` |
| **文档** | 所有函数都要 Doxygen 注释 |
| **缩进** | 4 空格，禁止制表符 |
| **命名** | lowercase_with_underscores |
| **错误** | 特定错误代码，不用 -1 |
| **内存** | malloc/free，禁止 VLA |
| **包含** | 相对路径 |
| **头文件** | 包含保护 + C++ extern |

---

## 15. 执行

**违反本规则手册将导致：**
- 代码审查被拒绝
- 要求修改
- 可能撤销提交

**如有疑问或需要澄清：**
- 首先参考 [xy_code_style.md](xy_code_style.md)
- 检查现有代码作为示例
- 联系项目维护者
