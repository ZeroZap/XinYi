# 测试工程师 Skill

**名称**: tester

**角色**: 单元测试、集成测试、系统测试

**职责**:
- 测试计划制定
- 测试用例编写
- 自动化测试
- 缺陷跟踪
- 质量报告

---

## 使用方式

```bash
# 查看测试状态
/skill tester status

# 查看测试用例
/skill tester cases

# 运行测试
/skill tester run <component>

# 生成测试报告
/skill tester report
```

---

## 测试框架

### Unity 测试框架

**位置**: `third_party/unity/`

**基本断言**:
```c
TEST_ASSERT(condition)
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)
TEST_ASSERT_NULL(ptr)
TEST_ASSERT_NOT_NULL(ptr)
TEST_ASSERT_EQUAL(expected, actual)
TEST_ASSERT_EQUAL_INT(e, a)
TEST_ASSERT_EQUAL_STRING(e, a)
TEST_ASSERT_EQUAL_MEMORY(e, a, len)
```

---

## 测试流程

### 1. 测试计划

确定测试范围：
- 单元测试
- 集成测试
- 系统测试

### 2. 测试用例设计

```c
// 测试正常路径
void test_<module>_normal_case(void);

// 测试边界条件
void test_<module>_boundary_case(void);

// 测试错误处理
void test_<module>_error_case(void);

// 测试空指针
void test_<module>_null_ptr(void);
```

### 3. 测试执行

```bash
# 运行单个组件测试
./pm.sh test run crypto

# 运行所有测试
./pm.sh test run all

# 生成覆盖率报告
./pm.sh test coverage
```

### 4. 缺陷报告

记录缺陷信息：
- 缺陷描述
- 复现步骤
- 期望结果
- 实际结果
- 严重程度

---

## 测试模板

### 单元测试模板

```c
/**
 * @file test_<module>.c
 * @brief <Module> Unit Tests
 */

#include "unity.h"
#include "xy_<module>.h"

void setUp(void) { }
void tearDown(void) { }

// 测试空指针处理
void test_<module>_init_null_ptr(void)
{
    xy_hal_error_t ret;
    
    ret = xy_<module>_init(NULL, NULL);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

// 测试正常初始化
void test_<module>_init_normal(void)
{
    xy_hal_error_t ret;
    xy_<module>_config_t config = { 0 };
    
    // 准备测试数据
    
    ret = xy_<module>_init(&<module>, &config);
    TEST_ASSERT_EQUAL(XY_HAL_OK, ret);
}

// 测试重复初始化
void test_<module>_init_twice(void)
{
    xy_hal_error_t ret;
    xy_<module>_config_t config = { 0 };
    
    ret = xy_<module>_init(&<module>, &config);
    TEST_ASSERT_EQUAL(XY_HAL_OK, ret);
    
    ret = xy_<module>_init(&<module>, &config);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_ALREADY_INIT, ret);
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_<module>_init_null_ptr);
    RUN_TEST(test_<module>_init_normal);
    RUN_TEST(test_<module>_init_twice);
    
    return UNITY_END();
}
```

---

## 测试覆盖率

### 覆盖率目标

| 组件类型 | 目标覆盖率 |
|----------|------------|
| 核心组件 | > 90% |
| 驱动组件 | > 80% |
| 工具组件 | > 70% |

### 覆盖率检查

```bash
# 使用 gcov
make coverage

# 查看报告
open coverage/index.html
```

---

## 测试检查清单

### 测试前

- [ ] 测试环境准备
- [ ] 测试数据准备
- [ ] 测试脚本验证

### 测试中

- [ ] 记录测试结果
- [ ] 截图/日志保存
- [ ] 缺陷记录

### 测试后

- [ ] 测试报告编写
- [ ] 缺陷跟踪
- [ ] 回归测试计划

---

## 自动化测试

### CI/CD 集成

```yaml
# .github/workflows/test.yml
name: Run Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run Tests
        run: |
          mkdir build && cd build
          cmake .. -DBUILD_TESTING=ON
          make
          ctest --output-on-failure
```

---

## 常用命令

```bash
# 运行测试
./pm.sh test run <component>

# 生成覆盖率
./pm.sh test coverage

# 查看测试报告
./pm.sh test report
```

---

## 相关文件

- [测试布局分析](../../docs/test_layout_analysis.md)
- [Unity 框架](../../third_party/unity/README.md)
- [组件状态](../../COMPONENTS_STATUS.md)

---

**维护者**: 测试团队  
**更新频率**: 测试流程变更时
