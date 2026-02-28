# 单元测试规范 (Unit Test Specification)

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: TAOCP/CYouAgain/8-CTest

---

## 📋 概述

本文档规定 XinYi 项目的单元测试规范和最佳实践。

---

## 🎯 测试框架

### 使用的框架

XinYi 项目使用 **Unity** 单元测试框架。

**位置**: `third_party/unity/`

**文档**: [Unity 官方文档](https://github.com/ThrowTheSwitch/Unity)

### 测试目录结构

```
tests/                          # 统一测试目录
├── CMakeLists.txt              # 测试构建配置
├── test_osal.c                 # OSAL 测试
├── test_crypto.c               # Crypto 测试
├── test_xy_clib.c              # CLib 测试
├── test_device.c               # Device 测试
├── test_fota.c                 # FOTA 测试
└── test_gui.c                  # GUI 测试
```

---

## 📝 测试编写规范

### 1. 测试文件结构

```c
/**
 * @file test_<module>.c
 * @brief <Module> Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* 被测模块头文件 */
#include "<module>.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* 每个测试前执行 */
}

void tearDown(void)
{
    /* 每个测试后执行 */
}

/* ==================== Test Cases ==================== */

void test_<module>_<feature>(void)
{
    /* 测试代码 */
    TEST_ASSERT_EQUAL(expected, actual);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_<module>_<feature>);
    
    return UNITY_END();
}
```

### 2. 测试命名规范

| 类型 | 命名规则 | 示例 |
|------|---------|------|
| 测试文件 | `test_<module>.c` | `test_crypto.c` |
| 测试函数 | `test_<module>_<feature>` | `test_aes_encrypt_decrypt` |
| 辅助函数 | `test_<module>_helper_<action>` | `test_crypto_helper_encode` |

### 3. 常用断言

#### 相等性断言

```c
TEST_ASSERT_EQUAL(expected, actual)
TEST_ASSERT_EQUAL_INT(expected, actual)
TEST_ASSERT_EQUAL_HEX(expected, actual)
TEST_ASSERT_EQUAL_STRING(expected, actual)
TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)
```

#### 布尔断言

```c
TEST_ASSERT(condition)
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)
```

#### 空指针断言

```c
TEST_ASSERT_NULL(pointer)
TEST_ASSERT_NOT_NULL(pointer)
```

#### 范围断言

```c
TEST_ASSERT_GREATER_THAN(threshold, value)
TEST_ASSERT_LESS_THAN(threshold, value)
TEST_ASSERT_WITHIN(threshold, value)
```

---

## 🎯 测试覆盖要求

### 覆盖率目标

| 组件类型 | 行覆盖率 | 分支覆盖率 |
|---------|---------|-----------|
| 核心组件 | >90% | >80% |
| 一般组件 | >80% | >70% |
| 辅助工具 | >70% | >60% |

### 必须测试的场景

- ✅ 正常路径（happy path）
- ✅ 边界条件
- ✅ 错误处理
- ✅ 空指针/无效参数
- ✅ 资源分配/释放

---

## 📊 测试组织

### 按功能分组

```c
/* ==================== Init Tests ==================== */
void test_module_init(void);
void test_module_init_invalid_params(void);

/* ==================== Operation Tests ==================== */
void test_module_read(void);
void test_module_write(void);

/* ==================== Error Handling Tests ==================== */
void test_module_null_pointer(void);
void test_module_out_of_range(void);
```

### 测试数据管理

```c
/* 测试数据放在文件开头或单独文件 */
static const uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
static const size_t test_data_len = sizeof(test_data);
```

---

## 🧪 运行测试

### 本地运行

```bash
# 构建并运行所有测试
cd build
make test

# 运行特定测试
ctest -R test_crypto --output-on-failure

# 详细输出
ctest --verbose
```

### 生成覆盖率

```bash
# 启用覆盖率
cmake .. -DTEST_COVERAGE=ON

# 运行测试后生成报告
make coverage

# 查看 HTML 报告
open coverage-report.html  # macOS
xdg-open coverage-report.html  # Linux
start coverage-report.html  # Windows
```

---

## 📚 参考

- [Unity 文档](https://github.com/ThrowTheSwitch/Unity)
- [测试布局分析](../design/test_layout_analysis.md)
- [TAOCP 学习笔记](../reference/TAOCP_编程生涯学习笔记.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
