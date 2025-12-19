 本文定义了对 XinYi 嵌入式框架的**强制性规则**，适用于人类开发者与 AI 代码生成器提交的所有代码。  
> 目标：一致性、可维护性、可移植性与整体质量。

---

## 目录

1. [目的](#目的)  
2. [编码标准](#1-编码标准)  
3. [日志与调试](#2-日志与调试)  
4. [文档要求](#3-文档要求)  
5. [头文件指南](#4-头文件指南)  
6. [错误处理](#5-错误处理)  
7. [内存管理](#6-内存管理)  
8. [命名约定](#7-命名约定)  
9. [配置与可移植性](#8-配置与可移植性)  
10. [AI 代码生成规则](#9-ai-代码生成规则)  
11. [测试要求](#10-测试要求)  
12. [构建系统集成](#11-构建系统集成)  
13. [版本控制](#12-版本控制)  
14. [常见错误避免](#13-常见错误避免)  
15. [快速参考](#14-快速参考)  
16. [执行与约束](#15-执行与约束)

---

## 目的

本规则手册适用于本仓库内的**所有改动**，包括但不限于：
- C 源代码（`.c`/`.h`）
- 构建与集成（`Makefile`/`CMakeLists.txt`/`Kconfig`/脚本）
- 单元测试与示例代码
- 文档中涉及的 API 使用约定

当本文与其他文档出现冲突时，优先级如下：
1. `RULEBOOK.md`（仓库根目录的规则手册为最高优先级；若本文与其内容不一致，以 `RULEBOOK.md` 为准并同步修订本文）
2. `xy_code_style.md`
3. `.clang-format`

> 优化补充：仓库可能存在历史遗留文档或结构描述不一致的情况。遇到冲突时，优先以**代码现状**与上述优先级文档为准，并及时补齐/修正文档。

---

## 1. 编码标准

### 1.1 主要参考

**所有代码必须遵循**：
- `xy_code_style.md`

其中关键要求（非穷举）：
- 使用 **C99** 标准
- 每级缩进 **4 空格**（禁止 Tab）
- 函数/变量名小写、可选下划线（`my_function`、`my_var`）
- 左花括号与关键字同一行（`if`/`for`/`while`）
- 复合语句必须使用花括号（即使只有一行）
- 对不应修改的参数/指针使用 `const`
- 禁止 VLA（可变长度数组），使用 `malloc()`（或仓库提供的内存分配器）替代

### 1.2 代码格式化

**所有代码必须**使用 `.clang-format` 进行格式化。

提交前请执行（示例）：

```/dev/null/formatting_zh.sh#L1-16
# 格式化单个文件
clang-format -i path/to/file.c

# 格式化所有暂存文件
./utils/script/format_staged.sh

# 格式化指定目录
./utils/script/format_path.sh path/to/directory
```

AI 生成的代码在提交前也必须经过格式化。

> 优化补充：如果 clang-format 的结果看起来“怪”，不要用手工排版去对抗格式化结果；需要调整风格时，应通过维护者评审后修改 `.clang-format`。

---

## 2. 日志与调试

### 2.1 强制日志 API

**严禁直接使用 `printf()` 或 `puts()`。**

**必须使用 `xy_log` API**（见 `components/trace/xy_log/inc/xy_log.h`）：

```/dev/null/xy_log_example_zh.c#L1-17
#include "xy_log.h"

/* 设置模块日志级别 */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

xy_log_e("错误: %s 失败，错误码 %d\n", func_name, error_code);
xy_log_w("警告: 内存不足（剩余 %zu 字节）\n", available);
xy_log_i("信息: 初始化成功\n");
xy_log_d("调试: 值 = 0x%08X\n", value);
xy_log_v("详细: 进入函数 %s\n", __FUNCTION__);
```

### 2.2 日志级别

可用日志级别（从低到高优先级）：
- `XY_LOG_LEVEL_VERBOSE`（详细跟踪）
- `XY_LOG_LEVEL_DEBUG`（调试信息）
- `XY_LOG_LEVEL_INFO`（关键事件）
- `XY_LOG_LEVEL_WARN`（可恢复问题）
- `XY_LOG_LEVEL_ERROR`（失败/错误）
- `XY_LOG_LEVEL_NEVER`（关闭日志）

**按文件设置日志级别：**

```/dev/null/local_log_level_zh.c#L1-6
/* 在每个 .c 文件顶部，在 include xy_log.h 之前 */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
```

### 2.3 何时使用每个级别

| 级别 | 使用时机 | 示例 |
|------|---------|------|
| `xy_log_e()` | 致命错误、操作失败 | `xy_log_e("UART 初始化失败: %d\n", ret);` |
| `xy_log_w()` | 可恢复问题 | `xy_log_w("缓冲区接近满: %d/%d\n", used, total);` |
| `xy_log_i()` | 重要事件 | `xy_log_i("系统初始化完成\n");` |
| `xy_log_d()` | 调试细节 | `xy_log_d("处理数据包: len=%d\n", len);` |
| `xy_log_v()` | 高频跟踪（较吵） | `xy_log_v("进入 %s()\n", __FUNCTION__);` |

> 优化补充：日志必须“可行动”。请包含关键上下文（模块/句柄/长度/返回码/状态机状态/重试次数等），避免只有“failed”这类不可定位信息。

---

## 3. 文档要求

### 3.1 函数文档

**每个函数都必须有 Doxygen 风格文档**，包括 `static` 函数。

```/dev/null/doxygen_func_example_zh.h#L1-18
/**
 * @brief 通过 UART 发送数据
 *
 * 此函数通过指定的 UART 接口发送数据，
 * 支持阻塞行为与超时控制。
 *
 * @param uart       UART 句柄
 * @param data       数据缓冲区指针
 * @param len        发送字节数
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功时返回发送字节数，失败返回负错误码
 */
int xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms);
```

### 3.2 文件头

**每个源文件/头文件必须包含**文件级 Doxygen 头：

```/dev/null/file_header_example_zh.c#L1-7
/**
 * @file filename.c
 * @brief 模块简述
 * @version X.Y.Z
 * @date YYYY-MM-DD
 */
```

> 优化补充：如果仓库未定义版本策略，不要凭空填写 `@version`；可省略或由维护者统一引入版本方案。

### 3.3 模块文档

每个主要组件**必须包含**：
- `README.md`：API 说明、使用示例、配置指南
- 配置头文件（`*_cfg.h`）：带注释的编译期选项（如适用）
- 至少一个示例文件：最小可运行用法

---

## 4. 头文件指南

### 4.1 包含保护

使用传统 include guard（禁止 `#pragma once`）：

```/dev/null/include_guard_zh.h#L1-9
#ifndef XY_MODULE_NAME_H
#define XY_MODULE_NAME_H

/* ... 内容 ... */

#endif /* XY_MODULE_NAME_H */
```

### 4.2 C++ 兼容性

加入 `extern "C"` 保护：

```/dev/null/extern_c_zh.h#L1-11
#ifdef __cplusplus
extern "C" {
#endif

/* ... 声明 ... */

#ifdef __cplusplus
}
#endif
```

### 4.3 include 路径

对项目内头文件使用**相对路径**：

```/dev/null/include_paths_zh.c#L1-10
/* 推荐 */
#include "../../xy_clib/xy_typedef.h"
#include "../../../bsp/xy_hal/inc/xy_hal_uart.h"

/* 不推荐：可能在不同工具链/集成方式下失败 */
#include "xy_typedef.h"
#include "xy_hal_uart.h"
```

> 优化补充：如果组件通过构建系统导出稳定的 include 根目录（CMake `target_include_directories` / Make `-I`），可以减少深层相对路径；但若仓库当前强制使用相对路径，请在组件内部保持一致。

---

## 5. 错误处理

### 5.1 返回值约定

函数必须返回：
- 成功：**0 或正值**（例如传输字节数）
- 失败：**负值**（使用标准化错误码）

```/dev/null/return_conventions_zh.c#L1-24
/* 推荐 */
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config) {
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

/* 不推荐：使用 -1 这类“泛错误码” */
int bad_function(void *ptr) {
    if (!ptr) {
        return -1;
    }
    return 0;
}
```

> 优化补充：不要混用不同模块域的错误码；如果模块定义了 `xy_xxx_error_t`，请在该模块中保持一致。

### 5.2 参数验证

必须在函数开头验证参数：

```/dev/null/param_validation_zh.c#L1-17
int my_function(void *handle, const uint8_t *data, size_t len) {
    if (!handle || !data || len == 0) {
        xy_log_e("无效参数\n");
        return -EINVAL;
    }
    return 0;
}
```

### 5.3 错误日志

返回错误码前必须记录错误（按需增加上下文）：

```/dev/null/error_logging_zh.c#L1-8
if (ret != XY_HAL_OK) {
    xy_log_e("UART 初始化失败: 错误码 %d\n", ret);
    return ret;
}
```

> 优化补充：避免上下层重复输出同一错误。如果底层已经记录了完整上下文，上层只补充“额外上下文”（调用者、操作类型、重试策略等）。

---

## 6. 内存管理

### 6.1 动态分配

使用标准 C 的 `malloc/free`，或仓库提供的统一分配器（如存在）。

```/dev/null/malloc_example_zh.c#L1-18
uint8_t *buffer = malloc(sizeof(*buffer) * size);
if (!buffer) {
    xy_log_e("内存分配失败\n");
    return -ENOMEM;
}
/* ... */
free(buffer);

/* 禁止：VLA */
void bad_function(size_t size) {
    uint8_t vla[size];
}
```

### 6.2 全局变量

不要在声明处初始化全局变量，应在 `init()` 中初始化：

```/dev/null/global_init_zh.c#L1-17
static int32_t global_counter;
static uint8_t *global_buffer;

void my_module_init(void) {
    global_counter = 0;
    global_buffer = malloc(1024);
}

/* 不推荐 */
static int32_t global_counter_wrong = 0;
```

> 优化补充：若模块存在 `deinit()`，应释放资源并重置状态，确保测试中可反复 init/deinit。

---

## 7. 命名约定

### 7.1 模块前缀

所有对外函数/类型必须使用模块前缀：

```/dev/null/module_prefix_zh.c#L1-10
xy_hal_error_t xy_hal_uart_init(...);
xy_hal_uart_config_t config;

xy_iso7816_error_t xy_iso7816_init(...);
xy_iso7816_handle_t handle;
```

### 7.2 类型后缀

使用一致后缀：
- `*_t`：typedef 的结构体/枚举
- `*_fn`：函数指针类型

```/dev/null/type_suffixes_zh.h#L1-6
typedef struct { /* ... */ } my_struct_t;
typedef enum { /* ... */ } my_enum_t;
typedef void (*my_callback_fn)(void *arg);
```

### 7.3 私有函数

模块内私有 `static` 函数使用 `prv_` 前缀：

```/dev/null/private_fn_zh.c#L1-6
static void prv_internal_helper(void) {
}
```

---

## 8. 配置与可移植性

### 8.1 配置头文件

当模块存在可调参数时，应提供 `*_cfg.h`：

```/dev/null/module_cfg_zh.h#L1-22
#ifndef XY_MODULE_CFG_H
#define XY_MODULE_CFG_H

#ifndef XY_MODULE_CFG_DEBUG
#define XY_MODULE_CFG_DEBUG 1
#endif

#ifndef XY_MODULE_CFG_TIMEOUT
#define XY_MODULE_CFG_TIMEOUT 1000
#endif

#endif /* XY_MODULE_CFG_H */
```

### 8.2 平台抽象

平台相关操作必须通过 HAL/抽象接口完成：

```/dev/null/platform_abstraction_zh.c#L1-10
/* 推荐：使用 HAL 抽象 */
xy_hal_uart_send(uart, data, len, timeout);

/* 不推荐：直接寄存器访问（不可移植） */
UART1->DR = data[0];
```

---

## 9. AI 代码生成规则

### 9.1 生成前检查清单

AI 在生成代码前必须：
1. 阅读 `xy_code_style.md`
2. 阅读 `RULEBOOK.md`
3. 查看 `.clang-format`
4. 阅读仓库内相似模块以保持一致

### 9.2 生成后检查清单

AI 在生成代码后必须：
1. 应用 clang-format
2. 将所有 `printf()` 替换为 `xy_log_*()`
3. 为所有函数补齐 Doxygen 注释
4. 确保 include 路径符合仓库规则
5. 使用标准化错误码
6. 添加文件头注释
7. 必要时创建或更新模块 `README.md`

### 9.3 代码生成模板

```/dev/null/ai_template_zh.c#L1-26
/**
 * @brief [简要描述]
 * @param [参数] [描述]
 * @return [返回值描述]
 */
[返回类型] [模块前缀_函数名]([参数]) {
    if ([无效条件]) {
        xy_log_e("[错误消息]\n");
        return [错误码];
    }

    [实现代码]

    xy_log_d("[成功消息]\n");
    return [成功值];
}
```

---

## 10. 测试要求

### 10.1 单元测试

新模块应提供单元测试：

```/dev/null/unit_test_example_zh.c#L1-17
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

每个模块必须提供至少一个示例：

```/dev/null/example_usage_zh.c#L1-15
#include "xy_module.h"

void example_basic_usage(void) {
    xy_module_init();
    xy_module_do_something();
    xy_module_deinit();
}
```

---

## 11. 构建系统集成

### 11.1 Makefile

若组件提供 Makefile（示例约定）：

```/dev/null/makefile_example_zh.mk#L1-12
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
INCLUDES = -I. -I../../xy_clib

all: lib
clean:
install:
```

### 11.2 CMakeLists.txt

若组件提供 CMake 支持（示例约定）：

```/dev/null/cmake_example_zh.txt#L1-8
cmake_minimum_required(VERSION 3.10)
project(xy_module C)

set(CMAKE_C_STANDARD 99)
add_library(xy_module STATIC module.c)
target_include_directories(xy_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

---

## 12. 版本控制

### 12.1 提交信息

提交信息必须清晰、可追踪：

```/dev/null/commit_message_zh.txt#L1-11
[组件] 简要描述

必要时补充详细说明

- 变更 1
- 变更 2
```

示例：

```/dev/null/commit_message_example_zh.txt#L1-9
[xy_iso7816] 添加 SIM 卡认证支持

根据 3GPP TS 31.102 实现 3G/4G 双向认证

- 添加 xy_iso7816_authenticate() 函数
- 支持 RAND/AUTN 挑战-响应
- 返回 RES、CK、IK 密钥
```

---

## 13. 常见错误避免

### ❌ 不要

```/dev/null/dont_examples_zh.c#L1-26
printf("你好\n");            /* 不要使用 printf */
 int x = 5;                  /* 不要使用 Tab */
void func(int n) { int a[n]; }/* 不要使用 VLA */
static int counter = 0;       /* 不要初始化全局变量 */
return -1;                    /* 不要使用泛错误码 */
```

### ✅ 要

```/dev/null/do_examples_zh.c#L1-31
xy_log_i("你好\n");          /* 使用 xy_log */
    int x = 5;               /* 使用空格缩进 */

void func(int n) {
    int *arr = malloc(sizeof(*arr) * n);
    if (!arr) {
        return;
    }
    free(arr);
}

static int counter;
void init(void) { counter = 0; }

return XY_HAL_ERROR_TIMEOUT;
```

---

## 14. 快速参考

| 规则 | 要求 |
|------|------|
| 代码风格 | 遵循 `xy_code_style.md` |
| 格式化 | 使用 `.clang-format` |
| 日志 | 使用 `xy_log_*()`，禁止 `printf()` |
| 文档 | 所有函数必须 Doxygen 注释 |
| 缩进 | 4 空格，禁止 Tab |
| 命名 | `lowercase_with_underscores` |
| 错误码 | 使用特定错误码，不用 `-1` |
| 内存 | malloc/free，禁止 VLA |
| include | 遵循仓库 include 路径规则 |
| 头文件 | include guard + C++ extern |

---

## 15. 执行与约束

违反本规则可能导致：
- Code Review 拒绝
- 要求修改后重新提交
- 必要时回滚提交

如需澄清：
- 优先查阅 `xy_code_style.md`
- 参考仓库已有模块实现
- 与维护者确认并统一口径
