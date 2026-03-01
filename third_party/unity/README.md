# XinYi Unity 测试框架

**来源**: https://github.com/ThrowTheSwitch/Unity

**许可证**: MIT

## 使用方式

### 1. 在组件测试中引用

```cmake
# components/<your_component>/tests/CMakeLists.txt

# Unity 框架路径
set(UNITY_DIR ${CMAKE_SOURCE_DIR}/third_party/unity)

# 创建测试可执行文件
add_executable(test_<component>
    ${UNITY_DIR}/unity.c
    test_<module>.c
)

# 包含头文件
target_include_directories(test_<component> PRIVATE
    ${UNITY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 链接被测组件
target_link_libraries(test_<component>
    <component_library>
)

# 注册测试
add_test(NAME <component>_test COMMAND test_<component>)
```

### 2. 编写测试

```c
#include "unity.h"
#include "<your_component.h>"

void setUp(void) {
    // 每个测试前执行
}

void tearDown(void) {
    // 每个测试后执行
}

void test_<feature>(void) {
    TEST_ASSERT_EQUAL(expected, actual);
    TEST_ASSERT_TRUE(condition);
    TEST_ASSERT_NULL(pointer);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_<feature>);
    return UNITY_END();
}
```

### 3. 运行测试

```bash
# 单个组件测试
cd components/<component>/tests/build
./test_<component>

# 所有测试 (顶层)
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make test
```

## 常用断言

| 断言 | 描述 |
|------|------|
| `TEST_ASSERT(condition)` | 基本断言 |
| `TEST_ASSERT_TRUE(condition)` | 真值断言 |
| `TEST_ASSERT_FALSE(condition)` | 假值断言 |
| `TEST_ASSERT_NULL(ptr)` | NULL 指针断言 |
| `TEST_ASSERT_NOT_NULL(ptr)` | 非 NULL 指针断言 |
| `TEST_ASSERT_EQUAL(expected, actual)` | 相等断言 |
| `TEST_ASSERT_NOT_EQUAL(expected, actual)` | 不相等断言 |
| `TEST_ASSERT_EQUAL_INT(e, a)` | 整数相等 |
| `TEST_ASSERT_EQUAL_STRING(e, a)` | 字符串相等 |
| `TEST_ASSERT_EQUAL_MEMORY(e, a, len)` | 内存相等 |

## 文件结构

```
third_party/unity/
├── unity.c              # 框架实现
├── unity.h              # 公共头文件
├── unity_internals.h    # 内部头文件
└── README.md            # 本文档
```

## 资源

- **官网**: https://github.com/ThrowTheSwitch/Unity
- **文档**: https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md
