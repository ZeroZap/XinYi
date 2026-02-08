测试开发指南 - 基于 Ceedling 框架
1. 概述
1.1 Ceedling 简介
Ceedling 是一个基于 Ruby 构建的 C 语言测试框架，集成了 Unity 测试框架和 CMock 模拟框架。它提供了一套完整的工具链，用于构建、测试和管理嵌入式 C 项目。

1.2 测试驱动开发（TDD）流程
text
编写测试用例 → 运行测试（失败） → 实现功能 → 运行测试（通过） → 重构
2. 环境搭建
2.1 安装要求
bash
# 安装 Ruby（Ceedling 依赖）
# 安装 Ceedling
gem install ceedling

# 验证安装
ceedling version
2.2 项目初始化
bash
# 在项目根目录创建 Ceedling 项目
ceedling new project_name

# 生成的项目结构：
project_name/
├── project.yml          # 主配置文件
├── src/                 # 生产代码
├── test/               # 测试代码
└── vendor/             # 第三方库
3. 项目配置
3.1 基本配置（project.yml）
yaml
:project:
  :use_exceptions: FALSE
  :use_test_preprocessor: TRUE
  :use_auxiliary_dependencies: TRUE

:paths:
  :test:
    - +:test/**
    - -:test/support
  :source:
    - src/**
  :support:
    - test/support

:defines:
  :test:
    - UNIT_TEST

:tools:
  :test_linker:
    :executable: gcc
    :arguments:
      - -o ${1}
      - ${2}
3.2 模块配置
yaml
:cmock:
  :mock_path: mocks
  :when_no_prototypes: :warn
  :enforce_strict_ordering: TRUE

:unity:
  :colorize: true
  :suite_setup: "custom_setup"
  :suite_teardown: "custom_teardown"
4. 测试用例编写
4.1 测试文件结构
c
// test/test_module.c
#include "unity.h"
#include "module.h"
#include "mock_dependency.h"

// 模块级设置/拆卸
void setUp(void) {
    // 每个测试前的初始化
}

void tearDown(void) {
    // 每个测试后的清理
}

// 测试分组
void test_GroupName_Functionality(void) {
    // 测试代码
}

// 测试用例示例
void test_Addition_ReturnsCorrectSum(void) {
    // 准备
    int a = 5;
    int b = 3;

    // 执行
    int result = add(a, b);

    // 验证
    TEST_ASSERT_EQUAL_INT(8, result);
}
4.2 常用断言方法
c
// 基本断言
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);

// 相等断言
TEST_ASSERT_EQUAL_INT(expected, actual);
TEST_ASSERT_EQUAL_HEX8(expected, actual);
TEST_ASSERT_EQUAL_STRING(expected, actual);

// 浮点数比较（带误差）
TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual);

// 数组比较
TEST_ASSERT_EQUAL_INT_ARRAY(expected, actual, num_elements);

// 内存比较
TEST_ASSERT_EQUAL_MEMORY(expected, actual, length);
5. 测试替身（Test Doubles）
5.1 CMock 使用
c
// 模拟函数调用
void test_WhenDependencyFails_ReturnsError(void) {
    // 设置模拟期望
    dependency_function_ExpectAndReturn(false);

    // 执行测试
    ErrorCode result = function_under_test();

    // 验证
    TEST_ASSERT_EQUAL(ERROR_CODE, result);
}

// 带参数的模拟
void test_WithSpecificParameters_CallsDependency(void) {
    dependency_process_Expect(42, "test");
    dependency_process_IgnoreArg_buffer(); // 忽略某个参数

    function_under_test();
}
5.2 回调函数模拟
c
void test_Callback_ExecutesProperly(void) {
    // 设置回调模拟
    register_callback_AddCallback(expected_callback);

    // 触发回调
    trigger_event();

    // 验证回调被调用
    TEST_ASSERT_TRUE(callback_called);
}
6. 测试执行与管理
6.1 命令行操作
bash
# 运行所有测试
ceedling test:all

# 运行特定模块测试
ceedling test:module_name

# 运行匹配模式的测试
ceedling test:pattern[*.c]

# 生成测试报告
ceedling gcov:all
ceedling utils:gcov

# 清理生成文件
ceedling clean
6.2 测试结果输出
bash
# 详细输出模式
ceedling verbosity[4] test:all

# 包含覆盖率报告
ceedling gcov:all
7. 高级特性
7.1 参数化测试
c
// 使用循环进行参数化测试
void test_Addition_MultipleCases(void) {
    struct TestCase {
        int a, b, expected;
    } cases[] = {
        {1, 1, 2},
        {2, 3, 5},
        {-1, 1, 0},
        {0, 0, 0}
    };

    for(int i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        char name[50];
        sprintf(name, "Case %d: %d + %d = %d",
                i, cases[i].a, cases[i].b, cases[i].expected);

        TEST_ASSERT_EQUAL_INT_MESSAGE(cases[i].expected,
                                     add(cases[i].a, cases[i].b),
                                     name);
    }
}
7.2 测试夹具
c
// 共享测试夹具
typedef struct {
    Module* module;
    Config config;
} TestFixture;

static TestFixture* fixture;

void testFixtureSetUp(void) {
    fixture = malloc(sizeof(TestFixture));
    fixture->module = module_create();
    fixture->config = get_default_config();
}

void testFixtureTearDown(void) {
    module_destroy(fixture->module);
    free(fixture);
}

void test_WithFixture_OperationSucceeds(void) {
    int result = module_operate(fixture->module, &fixture->config);
    TEST_ASSERT_EQUAL(SUCCESS, result);
}
8. 最佳实践
8.1 测试组织结构
text
tests/
├── unit/                    # 单元测试
│   ├── module_a/
│   │   ├── test_feature1.c
│   │   └── test_feature2.c
│   └── module_b/
├── integration/            # 集成测试
└── system/                # 系统测试
8.2 测试命名规范
文件名：test_<模块名>_<功能>.c

测试函数：test_<场景>_<预期行为>

分组：test_<模块>_<功能>_<具体情况>

8.3 CI/CD 集成
yaml
# GitHub Actions 示例
name: Ceedling Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v2

    - name: Setup Ruby
      uses: ruby/setup-ruby@v1
      with:
        ruby-version: '2.7'

    - name: Install Ceedling
      run: gem install ceedling

    - name: Run Tests
      run: ceedling test:all
9. 故障排除
9.1 常见问题
链接错误：检查源文件路径配置

头文件找不到：确认 include 路径设置

模拟函数未调用：验证 Expect 设置正确性

内存泄漏：使用 valgrind 检查内存管理

9.2 调试技巧
bash
# 生成详细构建日志
ceedling verbosity[4] test:problem_module

# 查看预处理后的代码
ceedling preprocess:test/test_file.c
10. 参考资料
10.1 官方文档
Ceedling 官方文档

Unity 断言参考

CMock 用户指南

10.2 相关工具
Gcov：代码覆盖率分析

Valgrind：内存泄漏检测

GDB：调试器