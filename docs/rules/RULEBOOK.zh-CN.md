# XinYi 开发规则手册（简体中文）

> 本文定义了对 XinYi 嵌入式框架的**强制性规则**，适用于人类开发者与 AI 代码生成器提交的所有代码。  
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

代码风格、示例与工具设置仅由 [code_style.md](../../code_style.md) 维护；本文负责仓库流程和项目集成。

---

## 1. 编码标准

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 2. 日志与调试

### 2.1 强制日志 API

日志约定统一见 [code_style.md](../../code_style.md) 第 6.7 节。下列级别名称仅作为项目 API 集成速查。

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

日志约定统一见 [code_style.md](../../code_style.md) 第 6.7 节。下列级别名称仅作为项目 API 集成速查。

## 3. 文档要求

### 3.1 函数文档

文档、配置与代码示例统一见 [code_style.md](../../code_style.md)，不在本节复制模板或另订风格。

### 3.2 文件头

文档、配置与代码示例统一见 [code_style.md](../../code_style.md)，不在本节复制模板或另订风格。

### 3.3 模块文档

每个主要组件**必须包含**：
- `README.md`：API 说明、使用示例、配置指南
- 配置头文件（`*_cfg.h`）：带注释的编译期选项（如适用）
- 至少一个示例文件：最小可运行用法

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

## 8. 配置与可移植性

编码要求与示例统一由 [code_style.md](../../code_style.md) 维护。

项目集成要求：可配置功能应在 Kconfig 与 CMake 中提供对应入口，并明确平台依赖。

## 9. AI 代码生成规则

### 9.1 生成前检查清单

AI 在生成代码前必须：
1. 阅读 `code_style.md`
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

文档、配置与代码示例统一见 [code_style.md](../../code_style.md)，不在本节复制模板或另订风格。

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

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 14. 快速参考

要求与示例统一见 [code_style.md](../../code_style.md)，本节不再另行维护编码约定。

## 15. 执行与约束

违反本规则可能导致：
- Code Review 拒绝
- 要求修改后重新提交
- 必要时回滚提交

如需澄清：
- 优先查阅 `code_style.md`
- 参考仓库已有模块实现
- 与维护者确认并统一口径
