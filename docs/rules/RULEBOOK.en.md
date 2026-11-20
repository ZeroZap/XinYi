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

Coding conventions and examples are maintained only in [code_style.md](../../code_style.md). This rulebook covers workflow and project integration.

---

## 1. Coding Standards

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 2. Logging and Debugging

### 2.1 Mandatory Logging API

Logging conventions are defined in [code_style.md](../../code_style.md), section 6.7. The level names below are project API integration details.

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

Logging conventions are defined in [code_style.md](../../code_style.md), section 6.7. The level names below are project API integration details.

## 3. Documentation Requirements

### 3.1 Function Documentation

See [code_style.md](../../code_style.md) for documentation, configuration and code examples. Reuse the canonical rules rather than copying templates here.

### 3.2 File Headers

See [code_style.md](../../code_style.md) for documentation, configuration and code examples. Reuse the canonical rules rather than copying templates here.

### 3.3 Module Documentation

Each major component **MUST include**:
- `README.md` — API reference, usage examples, configuration guide
- Configuration header (`*_cfg.h`) — compile-time options with comments (if applicable)
- At least one example file — minimal, working usage

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
1. Review `code_style.md`
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

See [code_style.md](../../code_style.md) for documentation, configuration and code examples. Reuse the canonical rules rather than copying templates here.

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

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 14. Quick Reference

See [code_style.md](../../code_style.md) for the authoritative requirements and examples. This section does not maintain a separate set of coding conventions.

## 15. Enforcement

Violations of this rulebook may result in:
- Code review rejection
- Requested modifications
- Potential revert of commits

For clarifications:
- Check `code_style.md` first
- Check existing code for patterns
- Ask maintainers to resolve ambiguities
