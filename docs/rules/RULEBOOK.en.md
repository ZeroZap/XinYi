# XinYi Development Rulebook (English)

> This document defines **mandatory rules** for all code contributions to the XinYi embedded framework, including code written by human developers and AI code generators.  
> These rules exist to ensure consistency, maintainability, portability, and overall code quality.

---

## Table of Contents

1. [Purpose](#purpose)  
2. [Coding Standards](#1-coding-standards)  
3. [Logging and Debugging](#2-logging-and-debugging)  
4. [Documentation Requirements](#3-documentation-requirements)  
5. [Header File Guidelines](#4-header-file-guidelines)  
6. [Error Handling](#5-error-handling)  
7. [Memory Management](#6-memory-management)  
8. [Naming Conventions](#7-naming-conventions)  
9. [Configuration and Portability](#8-configuration-and-portability)  
10. [AI Code Generation Rules](#9-ai-code-generation-rules)  
11. [Testing Requirements](#10-testing-requirements)  
12. [Build System Integration](#11-build-system-integration)  
13. [Version Control](#12-version-control)  
14. [Common Mistakes to Avoid](#13-common-mistakes-to-avoid)  
15. [Quick Reference](#14-quick-reference)  
16. [Enforcement](#15-enforcement)

---

## Purpose

This rulebook applies to **all code** in this repository:
- C source code (`.c`, `.h`)
- Build scripts and integration (`Makefile`, `CMakeLists.txt`, `Kconfig`, scripts)
- Unit tests and examples

If this document conflicts with other documents, follow this priority:
1. `RULEBOOK.md` (this rulebook)
2. `xy_code_style.md`
3. `.clang-format`

> Minor improvement: The repository may also contain older documentation. If you see inconsistencies, prefer **code reality** + this rulebook, and update docs accordingly.

---

## 1. Coding Standards

### 1.1 Primary Reference

**ALL code MUST follow** the detailed conventions in:
- `xy_code_style.md`

Key requirements (non-exhaustive):
- Use **C99**
- **4 spaces** per indent level (**NO tabs**)
- Lowercase function/variable names with optional underscores (`my_function`, `my_var`)
- Opening braces on same line as keywords (`if`, `for`, `while`)
- Always use braces for compound statements (even single-line)
- Use `const` for pointers and parameters that should not be modified
- Never use Variable Length Arrays (VLA) — use `malloc()` (or project allocator) instead

### 1.2 Code Formatting

**ALL code MUST be formatted** using:
- `.clang-format`

Before committing:
```/dev/null/formatting.sh#L1-16
# Format a single file
clang-format -i path/to/file.c

# Format all staged files
./utils/script/format_staged.sh

# Format a directory
./utils/script/format_path.sh path/to/directory
```

AI code generators must ensure formatted output before submission.

> Minor improvement: If clang-format produces a surprising result, do not hand-format to fight it. Adjust `.clang-format` only with maintainer agreement.

---

## 2. Logging and Debugging

### 2.1 Mandatory Logging API

**NEVER use `printf()` or `puts()` directly.**

**ALWAYS use `xy_log` API** (see `components/trace/xy_log/inc/xy_log.h`):

```/dev/null/xy_log_example.c#L1-17
#include "xy_log.h"

/* Set module log level */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

xy_log_e("Error: %s failed with code %d\n", func_name, error_code);
xy_log_w("Warning: low memory (%zu bytes)\n", available);
xy_log_i("Info: initialized successfully\n");
xy_log_d("Debug: value = 0x%08X\n", value);
xy_log_v("Verbose: entering function %s\n", __FUNCTION__);
```

### 2.2 Log Levels

Available log levels (lowest to highest priority):
- `XY_LOG_LEVEL_VERBOSE`
- `XY_LOG_LEVEL_DEBUG`
- `XY_LOG_LEVEL_INFO`
- `XY_LOG_LEVEL_WARN`
- `XY_LOG_LEVEL_ERROR`
- `XY_LOG_LEVEL_NEVER`

Set per-file log level:
```/dev/null/local_log_level.c#L1-6
/* At the top of each .c file, before including xy_log.h */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
```

### 2.3 When to Use Each Level

| Level | When to Use | Example |
|------:|-------------|---------|
| `xy_log_e()` | fatal errors / operation failure | `xy_log_e("UART init failed: %d\n", ret);` |
| `xy_log_w()` | recoverable issues | `xy_log_w("Buffer nearly full: %d/%d\n", used, total);` |
| `xy_log_i()` | important events | `xy_log_i("System initialized\n");` |
| `xy_log_d()` | debugging details | `xy_log_d("Processing packet: len=%d\n", len);` |
| `xy_log_v()` | noisy tracing | `xy_log_v("Entering %s()\n", __FUNCTION__);` |

> Minor improvement: Logs should be **actionable**: include identifiers, lengths, return codes, state transitions, and avoid vague messages like “failed”.

---

## 3. Documentation Requirements

### 3.1 Function Documentation

**EVERY function MUST have Doxygen-style documentation**, including `static` functions.

```/dev/null/doxygen_func_example.h#L1-18
/**
 * @brief Send data via UART
 *
 * This function transmits data through the specified UART interface
 * with blocking behavior and timeout support.
 *
 * @param uart       UART handle
 * @param data       Pointer to data buffer
 * @param len        Number of bytes to send
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes sent on success, negative error code on failure
 */
int xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms);
```

### 3.2 File Headers

**EVERY source/header file MUST include** a file-level Doxygen header:

```/dev/null/file_header_example.c#L1-7
/**
 * @file filename.c
 * @brief Brief description of the module
 * @version X.Y.Z
 * @date YYYY-MM-DD
 */
```

> Minor improvement: Use the repository’s actual versioning strategy. If no versioning is defined, omit `@version` rather than inventing values.

### 3.3 Module Documentation

Each major component **MUST include**:
- `README.md` — API reference, usage examples, configuration guide
- Configuration header (`*_cfg.h`) — compile-time options with comments (if applicable)
- At least one example file — minimal, working usage

---

## 4. Header File Guidelines

### 4.1 Include Guards

Use traditional include guards (do not use `#pragma once`):

```/dev/null/include_guard.h#L1-9
#ifndef XY_MODULE_NAME_H
#define XY_MODULE_NAME_H

/* ... content ... */

#endif /* XY_MODULE_NAME_H */
```

### 4.2 C++ Compatibility

Include `extern "C"` guards:

```/dev/null/extern_c.h#L1-11
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

```/dev/null/include_paths.c#L1-10
/* Good */
#include "../../xy_clib/xy_typedef.h"
#include "../../../bsp/xy_hal/inc/xy_hal_uart.h"

/* Wrong - can fail in different toolchains/projects */
#include "xy_typedef.h"
#include "xy_hal_uart.h"
```

> Minor improvement: Prefer stable include roots where possible (e.g., exported include directories via CMake/Make). If the project mandates relative includes, keep them consistent within a component.

---

## 5. Error Handling

### 5.1 Return Value Conventions

Functions MUST return:
- **0 or positive** for success (or bytes transferred)
- **Negative values** for errors (using standardized error codes)

```/dev/null/return_conventions.c#L1-24
/* Good */
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config) {
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

/* Wrong - using -1 instead of a specific error code */
int bad_function(void *ptr) {
    if (!ptr) {
        return -1;
    }
    return 0;
}
```

> Minor improvement: Do not mix unrelated error code domains. If a module defines `xy_xxx_error_t`, use it consistently.

### 5.2 Parameter Validation

Validate parameters at function start:

```/dev/null/param_validation.c#L1-17
int my_function(void *handle, const uint8_t *data, size_t len) {
    if (!handle || !data || len == 0) {
        xy_log_e("Invalid parameters\n");
        return -EINVAL;
    }
    return 0;
}
```

### 5.3 Error Logging

Log errors before returning:

```/dev/null/error_logging.c#L1-8
if (ret != XY_HAL_OK) {
    xy_log_e("UART init failed: error code %d\n", ret);
    return ret;
}
```

> Minor improvement: Avoid double-logging the same failure across layers. If a lower layer already logs the full context, upper layers should only add *additional* context (caller, operation, retry policy).

---

## 6. Memory Management

### 6.1 Dynamic Allocation

Use standard C allocation (`malloc/free`) or the project allocator if provided.

```/dev/null/malloc_example.c#L1-18
uint8_t *buffer = malloc(sizeof(*buffer) * size);
if (!buffer) {
    xy_log_e("Memory allocation failed\n");
    return -ENOMEM;
}
/* ... */
free(buffer);

/* Wrong - VLA is forbidden */
void bad_function(size_t size) {
    uint8_t vla[size];
}
```

### 6.2 Global Variables

Do not initialize globals at declaration; initialize in an `init()` function:

```/dev/null/global_init.c#L1-17
static int32_t global_counter;
static uint8_t *global_buffer;

void my_module_init(void) {
    global_counter = 0;
    global_buffer = malloc(1024);
}

/* Wrong */
static int32_t global_counter_wrong = 0;
```

> Minor improvement: If a module has a deinit path, free globals and reset state there to support repeated init/deinit in tests.

---

## 7. Naming Conventions

### 7.1 Module Prefix

All public functions/types MUST use a module prefix:

```/dev/null/module_prefix.c#L1-10
xy_hal_error_t xy_hal_uart_init(...);
xy_hal_uart_config_t config;

xy_iso7816_error_t xy_iso7816_init(...);
xy_iso7816_handle_t handle;
```

### 7.2 Type Suffixes

Use consistent suffixes:

```/dev/null/type_suffixes.h#L1-6
typedef struct { /* ... */ } my_struct_t;
typedef enum { /* ... */ } my_enum_t;
typedef void (*my_callback_fn)(void *arg);
```

### 7.3 Private Functions

Use `prv_` prefix for private static functions:

```/dev/null/private_fn.c#L1-6
static void prv_internal_helper(void) {
}
```

---

## 8. Configuration and Portability

### 8.1 Configuration Headers

Each module SHOULD provide `*_cfg.h` when it has tunables:

```/dev/null/module_cfg.h#L1-22
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

### 8.2 Platform Abstraction

Use HAL interfaces for platform-specific operations:

```/dev/null/platform_abstraction.c#L1-10
/* Good */
xy_hal_uart_send(uart, data, len, timeout);

/* Wrong - direct register access */
UART1->DR = data[0];
```

---

## 9. AI Code Generation Rules

### 9.1 Pre-Generation Checklist

Before generating code, AI MUST:
1. Review `xy_code_style.md`
2. Review this rulebook
3. Review `.clang-format`
4. Review similar modules for repository consistency

### 9.2 Post-Generation Checklist

After generating code, AI MUST:
1. Apply clang-format
2. Replace all `printf()` with `xy_log_*()`
3. Add Doxygen comments to all functions
4. Validate include paths follow repository rules
5. Use standardized error codes
6. Add file header comments
7. Create or update module `README.md` if needed

### 9.3 Code Generation Template

```/dev/null/ai_template.c#L1-26
/**
 * @brief [Brief description]
 * @param [param] [description]
 * @return [return value description]
 */
[return_type] [module_prefix_function_name]([parameters]) {
    if ([invalid_condition]) {
        xy_log_e("[Error message]\n");
        return [ERROR_CODE];
    }

    [implementation_code]

    xy_log_d("[Success message]\n");
    return [SUCCESS_VALUE];
}
```

---

## 10. Testing Requirements

### 10.1 Unit Tests

New modules SHOULD include unit tests:

```/dev/null/unit_test_example.c#L1-17
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

Each module MUST provide at least one example:

```/dev/null/example_usage.c#L1-15
#include "xy_module.h"

void example_basic_usage(void) {
    xy_module_init();
    xy_module_do_something();
    xy_module_deinit();
}
```

---

## 11. Build System Integration

### 11.1 Makefile

If providing a Makefile:
```/dev/null/makefile_example.mk#L1-12
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
INCLUDES = -I. -I../../xy_clib

all: lib
clean:
install:
```

### 11.2 CMakeLists.txt

If providing CMake support:
```/dev/null/cmake_example.txt#L1-8
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

```/dev/null/commit_message.txt#L1-11
[component] Brief description

Detailed explanation if needed

- Change 1
- Change 2
```

Example:
```/dev/null/commit_message_example.txt#L1-9
[xy_iso7816] Add SIM card authentication support

Implement 3G/4G mutual authentication according to 3GPP TS 31.102

- Add xy_iso7816_authenticate() function
- Support RAND/AUTN challenge-response
- Return RES, CK, IK keys
```

---

## 13. Common Mistakes to Avoid

### ❌ DON'T

```/dev/null/dont_examples.c#L1-26
printf("Hello\n");          /* Don't use printf */
 int x = 5;                 /* Don't use tabs */
void func(int n) { int a[n]; } /* Don't use VLA */
static int counter = 0;     /* Don't initialize globals */
return -1;                  /* Don't use generic errors */
```

### ✅ DO

```/dev/null/do_examples.c#L1-31
xy_log_i("Hello\n");        /* Use xy_log */
    int x = 5;              /* Use spaces */

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

## 14. Quick Reference

| Rule | Requirement |
|------|-------------|
| Code Style | Follow `xy_code_style.md` |
| Formatting | Use `.clang-format` |
| Logging | Use `xy_log_*()`, NOT `printf()` |
| Documentation | Doxygen comments for ALL functions |
| Indentation | 4 spaces, NO tabs |
| Naming | `lowercase_with_underscores` |
| Errors | Specific error codes, not `-1` |
| Memory | malloc/free, NO VLA |
| Includes | Follow repository include path rules |
| Headers | Include guards + C++ extern |

---

## 15. Enforcement

Violations of this rulebook may result in:
- Code review rejection
- Requested modifications
- Potential revert of commits

For clarifications:
- Check `xy_code_style.md` first
- Check existing code for patterns
- Ask maintainers to resolve ambiguities