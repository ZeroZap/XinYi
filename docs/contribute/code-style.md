# 代码风格规范

**版本**: 1.0  
**最后更新**: 2026-02-28

---

## 📋 基本要求

### 语言标准

- 使用 **C99** 标准
- 兼容 C++ 编译器（使用 `extern "C"`）

### 缩进和空格

- 使用 **4 空格** 缩进（**禁用制表符**）
- 关键字和左括号之间使用 1 空格
- 运算符两侧使用 1 空格

```c
/* ✅ 正确 */
if (condition) {
    do_something();
}

for (int i = 0; i < 10; i++) {
    /* ... */
}

/* ❌ 错误 */
if(condition) {
    do_something();
}

if (condition){
    do_something();
}
```

---

## 📝 命名规范

### 基本原则

- 使用 **小写字母** + **下划线** 分隔
- 禁止使用 `__` 或 `_` 前缀（保留给 C 语言）
- 宏定义使用全大写

### 命名示例

```c
/* ✅ 正确 */
int my_function(void);
int my_var;
struct my_struct;
#define MY_MACRO  100

/* ❌ 错误 */
int MyFunction(void);
int myVar;
int __private_var;
int _private_var;
```

### 文件命名

- 源文件：小写 + 下划线（`my_module.c`）
- 头文件：小写 + 下划线（`my_module.h`）
- 测试文件：`test_<module>.c`

---

## 📐 代码布局

### 函数定义

```c
/**
 * @brief 函数描述
 * @param param1 参数 1 描述
 * @param param2 参数 2 描述
 * @return 返回值描述
 */
int my_function(int param1, const char *param2) {
    xy_log_d("进入函数\n");
    
    if (condition) {
        /* 处理逻辑 */
    }
    
    return 0;
}
```

### 控制结构

```c
/* if-else */
if (condition) {
    /* ... */
} else if (other_condition) {
    /* ... */
} else {
    /* ... */
}

/* switch */
switch (value) {
    case 0:
        /* ... */
        break;
    case 1:
        /* ... */
        break;
    default:
        /* ... */
        break;
}

/* for 循环 */
for (size_t i = 0; i < count; i++) {
    /* ... */
}

/* while 循环 */
while (condition) {
    /* ... */
}

/* do-while 循环 */
do {
    /* ... */
} while (condition);
```

### 括号规则

- 左括号与关键字同行
- 所有复合语句必须使用括号

```c
/* ✅ 正确 */
if (condition) {
    do_something();
}

/* ❌ 错误 */
if (condition)
    do_something();

if (condition)
{
    do_something();
}
```

---

## 💬 注释规范

### 文件头注释

```c
/**
 * @file my_module.c
 * @brief 模块简要描述
 * @version 1.0.0
 * @date 2026-02-28
 * @author XinYi Team
 */
```

### 函数注释

```c
/**
 * @brief 函数简要描述
 * 
 * 详细描述（可选）
 * 
 * @param param1 参数 1 描述
 * @param param2 参数 2 描述
 * @return 返回值描述
 * 
 * @note 注意事项（可选）
 * @warning 警告（可选）
 * @example 示例（可选）
 */
```

### 行内注释

```c
/* 单行注释 */
int value = 42;  /* 行尾注释 */

/*
 * 多行注释
 * 每行以空格 + 星号开头
 */
```

### 日志注释

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

void my_function(void) {
    xy_log_d("调试信息：%d\n", value);
    xy_log_i("信息：%s\n", str);
    xy_log_w("警告：%p\n", ptr);
    xy_log_e("错误：操作失败\n");
}
```

---

## 🔧 变量和类型

### 变量声明

```c
/* 在同一行声明同类型变量 */
int a, b, c;

/* 在块开头声明局部变量 */
void my_function(void) {
    int counter;
    char *buffer;
    
    /* 可执行语句 */
    counter = 0;
}
```

### 类型定义

```c
/* 结构体定义 */
typedef struct {
    int x;
    int y;
} point_t;

/* 枚举定义 */
typedef enum {
    MY_ENUM_VALUE1 = 0,
    MY_ENUM_VALUE2,
    MY_ENUM_VALUE3,
} my_enum_t;

/* 函数指针类型 */
typedef int (*my_callback_fn)(void *arg);
```

### 常量定义

```c
/* 使用 enum 或 #define */
#define MAX_BUFFER_SIZE  256

typedef enum {
    MY_CONST_A = 100,
    MY_CONST_B = 200,
} my_constants_t;
```

---

## 🛡️ 错误处理

### 返回值约定

- **0 或正值** 表示成功
- **负值** 表示错误

```c
#define MY_OK               0
#define MY_ERROR            (-1)
#define MY_INVALID_PARAM    (-2)

int my_function(int param) {
    if (param < 0) {
        return MY_INVALID_PARAM;
    }
    
    /* 处理逻辑 */
    
    return MY_OK;
}
```

### 错误日志

```c
int my_function(void) {
    int result;
    
    result = do_something();
    if (result < 0) {
        xy_log_e("操作失败：%d\n", result);
        return result;
    }
    
    return MY_OK;
}
```

---

## 📦 头文件规范

### 头文件保护

```c
#ifndef MY_MODULE_H
#define MY_MODULE_H

/* 头文件内容 */

#endif /* MY_MODULE_H */
```

### 包含顺序

```c
/* 1. 自身头文件 */
#include "my_module.h"

/* 2. 项目内头文件 */
#include "xy_log.h"
#include "xy_os.h"

/* 3. 第三方库头文件 */
#include <stdio.h>
#include <string.h>
```

### C++ 兼容

```c
#ifdef __cplusplus
extern "C" {
#endif

/* 函数声明 */

#ifdef __cplusplus
}
#endif
```

---

## 🧪 测试代码规范

### 测试文件结构

```c
/**
 * @file test_my_module.c
 * @brief My Module Unit Tests
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "my_module.h"

void setUp(void) { }
void tearDown(void) { }

void test_my_function(void) {
    int result = my_function(42);
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_my_function);
    return UNITY_END();
}
```

### 测试命名

```c
void test_<module>_<feature>(void) {
    /* 测试代码 */
}

/* 示例 */
void test_aes_encrypt_decrypt(void);
void test_md5_basic(void);
void test_pid_init(void);
```

---

## 🔍 代码检查

### 使用 clang-format

```bash
# 格式化代码
./tools/scripts/format_code.sh

# 检查风格
./tools/scripts/check_style.sh
```

### 使用 clang-tidy

```bash
# 运行静态分析
clang-tidy source.c -- -Iinclude
```

### CI/CD 检查

GitHub Actions 会自动检查：
- 代码格式化
- 静态分析
- 编译警告

---

## 📚 参考资源

- [xy_code_style.md](../docs/rules/100-code_style/xy_code_style.md) - 详细编码规范
- [.clang-format](../.clang-format) - 格式化配置
- [.editorconfig](../.editorconfig) - 编辑器配置

---

## ✅ 检查清单

提交代码前请确认：

- [ ] 遵循代码风格规范
- [ ] 使用 clang-format 格式化
- [ ] 所有函数有 Doxygen 注释
- [ ] 使用 `xy_log_*()` 进行日志记录
- [ ] 实现错误处理
- [ ] 包含单元测试
- [ ] 无编译器警告
- [ ] 在目标平台测试

---

*维护者：XinYi Team | 许可证：Apache License 2.0*
