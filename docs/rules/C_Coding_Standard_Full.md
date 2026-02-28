# C 语言编码规范完整版

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: xy_code_style.md (1681 行), memory_safety.md, safety_overview.md, SEI CERT

---

## 📋 概述

本文档是 XinYi 项目的完整 C 语言编码规范，整合了：
- **xy_code_style.md** (1681 行) - 详细代码风格
- **memory_safety.md** - 内存安全指南
- **safety_overview.md** - 安全规则概览
- **SEI CERT C** - 安全编码标准

**适用范围**: 所有 XinYi 项目 C 语言代码

**关键字说明**:
- **MUST/必须**: 强制性要求
- **MUST NOT/禁止**: 强制性禁止
- **SHOULD/应该**: 推荐性建议
- **MAY/可以**: 允许性选项

---

## 📚 目录

### 第一部分：一般规则 (General Rules)
1. [语言标准](#1-语言标准)
2. [格式规范](#2-格式规范)
3. [命名规范](#3-命名规范)
4. [变量声明](#4-变量声明)
5. [指针使用](#5-指针使用)

### 第二部分：代码风格 (Code Style)
6. [函数风格](#6-函数风格)
7. [注释规范](#7-注释规范)
8. [宏定义](#8-宏定义)
9. [预处理](#9-预处理)
10. [结构体/枚举/联合体](#10-结构体枚举联合体)

### 第三部分：内存安全 (Memory Safety)
11. [动态内存管理](#11-动态内存管理)
12. [缓冲区操作](#12-缓冲区操作)
13. [栈使用](#13-栈使用)

### 第四部分：安全规则 (Security Rules)
14. [整数安全](#14-整数安全)
15. [字符串安全](#15-字符串安全)
16. [并发安全](#16-并发安全)
17. [错误处理](#17-错误处理)

---

## 第一部分：一般规则

### 1. 语言标准

#### 1.1 C99 标准
- **必须** 使用 C99 标准
- **禁止** 使用编译器特定扩展（除非必要）

```c
/* ✅ 正确 */
#include <stdint.h>
#include <stdbool.h>  /* C99 支持 */

/* ❌ 错误 - 使用非标准扩展 */
#pragma pack(1)  /* 除非必要 */
```

#### 1.2 禁止制表符
- **必须** 使用空格代替制表符 (Tab)
- **必须** 使用 4 个空格缩进

```c
/* ✅ 正确 - 4 空格缩进 */
void func(void) {
    if (condition) {
        do_something();
    }
}

/* ❌ 错误 - 使用 Tab */
void func(void) {
	if (condition) {  /* Tab 缩进 */
	    do_something();
	}
}
```

---

### 2. 格式规范

#### 2.1 关键字空格
- **必须** 在关键字和左括号之间使用 1 个空格

```c
/* ✅ 正确 */
if (condition) {
    /* ... */
}

while (condition) {
    /* ... */
}

for (init; condition; step) {
    /* ... */
}

do {
    /* ... */
} while (condition);

/* ❌ 错误 */
if(condition) {  /* 缺少空格 */
    /* ... */
}

if (condition){  /* 括号位置错误 */
    /* ... */
}
```

#### 2.2 函数调用空格
- **禁止** 在函数名和左括号之间使用空格

```c
/* ✅ 正确 */
int32_t a = sum(4, 3);

/* ❌ 错误 */
int32_t a = sum (4, 3);
```

#### 2.3 运算符空格
- **必须** 在运算符前后使用单个空格

```c
/* ✅ 正确 */
int32_t a;
a = 3 + 4;
for (a = 0; a < 5; ++a) {
    /* ... */
}

/* ❌ 错误 */
a=3+4;
for(a=0;a<5;++a){
    /* ... */
}
```

#### 2.4 逗号空格
- **必须** 在每个逗号后使用一个空格

```c
/* ✅ 正确 */
func_name(5, 4);
int a, b, c;

/* ❌ 错误 */
func_name(4,3);
int a,b,c;
```

#### 2.5 括号位置
- **必须** 将左花括号放在与关键字同一行

```c
/* ✅ 正确 */
size_t i;
for (i = 0; i < 5; ++i) {
    /* ... */
}

if (condition) {
    /* ... */
}

/* ❌ 错误 */
for (i = 0; i < 5; ++i)
{
    /* ... */
}

if (condition)
{
    /* ... */
}
```

#### 2.6 复合语句括号
- **必须** 为所有复合语句包含括号，即使单条语句

```c
/* ✅ 正确 */
if (condition) {
    do_something();
}

/* ❌ 错误 */
if (condition)
    do_something();
```

---

### 3. 命名规范

#### 3.1 基本原则
- **必须** 使用小写字母 + 下划线
- **禁止** 使用 `__` 或 `_` 前缀（C 语言保留）
- **禁止** 混合大小写（驼峰命名）

```c
/* ✅ 正确 */
int my_function(void);
int my_variable;
#define MAX_BUFFER_SIZE 256

/* ❌ 错误 */
int MyFunction(void);      /* 驼峰命名 */
int myVariable;            /* 驼峰命名 */
int __my_var;              /* 保留前缀 */
int _my_var;               /* 保留前缀 */
```

#### 3.2 前缀规范
- **应该** 对私有函数使用 `prv_` 前缀
- **应该** 对库内部函数使用 `libname_int_` 前缀

```c
/* ✅ 正确 */
static void prv_initialize(void);
int32_t xy_int_internal_function(void);

/* ❌ 错误 */
static void _initialize(void);  /* 保留前缀 */
```

#### 3.3 函数命名

| 函数类型 | 命名规则 | 示例 |
|---------|---------|------|
| 普通函数 | 小写 + 下划线 | `xy_uart_init()` |
| Getter | `module_get_xxx()` | `battery_get_voltage()` |
| Setter | `module_set_xxx()` | `battery_set_threshold()` |
| 私有函数 | `prv_` 前缀 | `prv_initialize_buffer()` |
| 回调函数 | `xxx_callback` | `uart_rx_callback` |

#### 3.4 变量命名

| 变量类型 | 命名规则 | 示例 |
|---------|---------|------|
| 局部变量 | 小写 + 下划线 | `counter`, `buffer_size` |
| 全局变量 | `g_` 前缀 | `g_uart_buffer` |
| 静态变量 | `s_` 前缀 | `s_initialized` |
| 常量 | `k_` 前缀 | `k_max_retries` |
| 指针 | 明确表达指向 | `buffer_ptr`, `node_list` |

#### 3.5 类型命名

| 类型 | 命名规则 | 示例 |
|------|---------|------|
| 结构体（命名） | 小写 + 下划线 | `struct my_struct` |
| 结构体（typedef） | 小写 + 下划线，`_t` 后缀 | `my_struct_t` |
| 枚举（类型） | 小写 + 下划线，`_t` 后缀 | `my_enum_t` |
| 枚举（成员） | 全大写 + 下划线 | `MY_ENUM_VALUE_A` |
| 联合体 | 小写 + 下划线，`_t` 后缀 | `data_union_t` |

#### 3.6 宏命名

| 宏类型 | 命名规则 | 示例 |
|--------|---------|------|
| 常量宏 | 全大写 + 下划线 | `MAX_BUFFER_SIZE` |
| 函数宏 | 全大写 + 下划线 | `XY_MIN(x, y)` |
| 条件编译 | 全大写 + 下划线 | `XY_HAL_UART_ENABLED` |
| 头文件保护 | 全大写 + 下划线 | `XY_UART_H` |

#### 3.7 禁止事项

```c
/* ❌ 禁止使用 __ 或 _ 前缀 */
int __my_var;
int _my_var;

/* ❌ 禁止混合大小写 */
int myVar;
int MyVar;

/* ❌ 禁止无意义缩写 */
int buf_sz;      /* 除非 sz 是团队共识 */
int tmp;

/* ❌ 禁止过长名称 (>32 字符) */
int this_is_a_very_long_variable_name_that_exceeds_32_characters;
```

---

### 4. 变量声明

#### 4.1 同类型变量同行
- **必须** 在同一行声明同一类型的所有局部变量

```c
void my_func(void) {
    /* ✅ 正确 */
    char a;
    char a, b;

    /* ❌ 错误 */
    char a;
    char b;  /* char 类型已存在 */
}
```

#### 4.2 声明顺序
- **必须** 在块的开头、第一个可执行语句之前声明局部变量
- **应该** 按以下顺序声明：
  1. 自定义结构和枚举
  2. 整数类型（宽的无符号优先）
  3. 单/双精度浮点

```c
int my_func(void) {
    /* 1. 自定义结构 */
    my_struct_t my;
    my_struct_ptr_t *p;

    /* 2. 整数类型 */
    uint32_t a;
    int32_t b;
    uint16_t c;
    int16_t g;
    char h;

    /* 3. 浮点类型 */
    double d;
    float f;
}
```

#### 4.3 禁止 VLA
- **禁止** 使用可变长度数组 (VLA)
- **应该** 使用动态内存分配

```c
/* ✅ 正确 */
#include <stdlib.h>
void my_func(size_t size) {
    int32_t *arr;
    arr = malloc(sizeof(*arr) * size);
    if (arr == NULL) {
        /* 错误处理 */
        return;
    }
    /* 使用 arr */
    free(arr);
}

/* ❌ 错误 */
void my_func(size_t size) {
    int32_t arr[size];  /* VLA - 禁止使用 */
}
```

#### 4.4 禁止全局变量初始化
- **禁止** 将全局变量初始化为任何默认值（或 NULL）
- **应该** 在专用 init 函数中实现

```c
/* ✅ 正确 */
static int32_t a;

void my_module_init(void) {
    a = 0;
}

/* ❌ 错误 */
static int32_t b = 4;  /* 可能在 linker script 中未正确处理 */
```

#### 4.5 避免函数调用赋值
- **应该** 避免在声明中使用函数调用进行变量赋值

```c
/* ✅ 正确 */
void func(void) {
    int32_t a, b;
    b = sum(1, 2);
}

/* ❌ 不推荐 */
void func(void) {
    int32_t a, b = sum(1, 2);
}

/* ✅ 可以接受 */
uint8_t a = 3, b = 4;
```

---

### 5. 指针使用

#### 5.1 const 指针
- **必须** 如果函数不应该修改指向的内存，使用 const 指针
- **必须** 如果不应修改，使用 const 函数参数或变量

```c
/* ✅ 正确 */
void my_func(const void *d) {
    /* d 可以修改，但 d 指向的数据不能修改 */
}

void my_func(const void * const d) {
    /* d 和 d 指向的数据都不能修改 */
}

void my_func(void * const d) {
    /* d 不能修改，但 d 指向的数据可以修改 */
}
```

#### 5.2 void 指针
- **必须** 当函数可以接受任何类型的指针时，使用 void *

```c
/* ✅ 正确 */
void send_data(const void *data, size_t len) {
    /* 不转换 void * 或 const void * */
    const uint8_t *d = data;  /* 函数内部处理正确类型 */
}

/* ❌ 错误 */
void send_data(const void *data, int len) {  /* 应该使用 size_t */
}
```

#### 5.3 sizeof 运算符
- **必须** 始终使用带有 sizeof 运算符的括号

```c
/* ✅ 正确 */
arr = malloc(sizeof(*arr) * n);
arr = malloc(sizeof(int) * n);

/* ❌ 错误 */
arr = malloc(sizeof *arr * n);  /* 缺少括号 */
```

#### 5.4 空指针比较
- **必须** 始终将指针与 NULL 值进行比较
- **禁止** 与 true 比较

```c
/* ✅ 正确 */
if (ptr != NULL) {
    /* ... */
}

if (ptr == NULL) {
    /* ... */
}

/* ❌ 错误 */
if (ptr == true) {
    /* ... */
}
```

#### 5.5 自增自减
- **应该** 优先使用预增（减）量而不是后增（减）量

```c
/* ✅ 推荐 */
++a;
for (size_t j = 0; j < 10; ++j) {
    /* ... */
}

/* ❌ 不推荐 */
a++;
for (size_t j = 0; j < 10; j++) {
    /* ... */
}
```

#### 5.6 size_t 使用
- **必须** 始终用于 `size_t` 长度或大小变量

```c
/* ✅ 正确 */
size_t length;
size_t buffer_size;

/* ❌ 错误 */
int length;
int buffer_size;
```

---

### 6. 函数风格

#### 6.1 函数声明
- **必须** 为所有公共函数提供原型
- **必须** 使用 Doxygen 注释

```c
/**
 * @brief 初始化 UART 模块
 * @param baudrate 波特率
 * @return 0 成功，负数失败
 */
int xy_uart_init(uint32_t baudrate);
```

#### 6.2 函数参数
- **必须** 对只读参数使用 `const`
- **应该** 限制参数数量（不超过 5 个）

```c
/* ✅ 正确 */
void process_data(const uint8_t *data, size_t len);

/* ❌ 不推荐 */
void process_data(uint8_t *data, size_t len);  /* 可能修改数据 */
```

#### 6.3 函数长度
- **应该** 限制函数长度（不超过 50 行）
- **应该** 一个函数只做一件事

---

### 7. 注释规范

#### 7.1 注释风格
- **必须** 使用 `/* */` 风格，禁止使用 `//`
- **必须** 使用英文或中文，保持统一
- **应该** 在句尾添加句号

```c
/* ✅ 正确 */
/* This is a single-line comment. */

/*
 * This is a multi-line comment
 * written in multiple lines.
 */

/* ❌ 错误 */
// This is C++ style comment
```

#### 7.2 Doxygen 文档

```c
/**
 * @brief   函数简要描述
 * @details 详细描述（可选）
 *
 * @param[in]     input_param   输入参数描述
 * @param[out]    output_param  输出参数描述
 * @param[in,out] inout_param   输入输出参数描述
 *
 * @return  返回值描述
 * @retval  0       成功
 * @retval  -1      失败：参数无效
 * @retval  -2      失败：内存不足
 *
 * @note    注意事项
 * @warning 警告信息
 * @see     相关函数
 *
 * @code
 * // 使用示例
 * int result = function_name(input, &output);
 * if (result != 0) {
 *     // 错误处理
 * }
 * @endcode
 *
 * @author  作者名
 * @date    2026-02-25
 * @version 1.0
 */
int function_name(int input_param, int *output_param);
```

#### 7.3 结构体成员注释
- **必须** 使用 `/**< member comment */` 格式

```c
typedef struct {
    int x;              /**< X 坐标 */
    int y;              /**< Y 坐标 */
    char *name;         /**< 名称 */
} point_t;
```

#### 7.4 注释对齐
- **应该** 对齐注释在 12 缩进级别（12 * 4 空格）
- **应该** 如果语句超过 12 缩进，对齐到下一个可用的 4 空格缩进

```c
void my_func(void) {
    char a, b;

    a = call_func_returning_char_a(a);          /* 12*4 空格缩进 */
    b = call_func_returning_char_a_but_func_name_is_very_long(a);
                                                        /* 对齐到下一个 4 空格缩进 */
}
```

---

### 8. 宏定义

#### 8.1 基本规则
1. **必须** 参数与最终展开结果使用括号保护
2. **必须** 多语句宏使用 `do { } while (0)` 包装
3. **必须** 宏名使用全大写
4. **禁止** 重复评估有副作用的参数
5. **应该** 对值范围进行限制时使用 CLAMP 宏

#### 8.2 数值宏

```c
/* ✅ 正确 */
#define XY_MIN(x, y)            ((x) < (y) ? (x) : (y))
#define XY_MAX(x, y)            ((x) > (y) ? (x) : (y))
#define XY_ABS(x)               (((x) < 0) ? -(x) : (x))

/* ❌ 错误 */
#define MY_MIN(x, y) x < y ? x : y  /* 缺少括号 */
```

#### 8.3 位操作宏

```c
/* ✅ 正确 */
#define XY_BIT(pos)             (1UL << (pos))
#define XY_BIT_SET(val, bit)    ((val) |= (1UL << (bit)))
#define XY_BIT_CLEAR(val, bit)  ((val) &= ~(1UL << (bit)))
#define XY_BIT_IS_SET(val, bit) (((val) >> (bit)) & 1UL)

/* 使用示例 */
uint32_t mask = XY_BIT(5);  /* mask == 0x20 */
```

#### 8.4 数组宏

```c
/* ✅ 正确 */
#define XY_ARRAY_SIZE(arr)      (sizeof(arr) / sizeof((arr)[0]))

static const int32_t values[] = {1, 3, 5};
size_t count = XY_ARRAY_SIZE(values);  /* count == 3 */

/* ❌ 错误 */
#define BAD_ARR_SZ(a) sizeof a / sizeof a[0]  /* 缺少括号 */
```

#### 8.5 值限制宏

```c
/* ✅ 正确 */
#define XY_CLAMP(v, min, max)   (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))

int32_t speed = XY_CLAMP(input_speed, 0, 100);  /* 保证范围在 0..100 */

/* ❌ 错误 */
#define MY_CLAMP(v, min, max)   (v < min ? min : v > max ? max : v)  /* 缺少括号 */
```

#### 8.6 多语句宏

```c
/* ✅ 正确 */
#define XY_SET_POINT(p, x, y)   do { (p)->px = (x); (p)->py = (y); } while (0)
#define XY_SWAP(a, b)           do { typeof(a) _t = (a); (a) = (b); (b) = _t; } while (0)
#define XY_UNUSED(x)            do { (void)(x); } while (0)

/* 使用示例 */
xy_point_t pt;
XY_SET_POINT(&pt, 10, 20);

int a = 1, b = 2;
XY_SWAP(a, b);

void handler(int code) {
    XY_UNUSED(code);  /* 消除未使用警告 */
}

/* ❌ 错误 */
#define SET_POINT(p, x, y) (p)->px = (x); (p)->py = (y)  /* 多语句缺少 do-while */
```

#### 8.7 宏 vs 内联函数
- **应该** 当逻辑超过 1~2 个操作或需要类型安全时，优先使用 `static inline` 函数

```c
/* ✅ 推荐 - 内联函数 */
static inline int32_t clamp_i32(int32_t v, int32_t min, int32_t max) {
    return v < min ? min : (v > max ? max : v);
}

/* ❌ 不推荐 - 复杂宏 */
#define CLAMP(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))
```

#### 8.8 避免副作用

```c
/* ✅ 正确 */
int i = 0;
int tmp = i++;
int ok = XY_MIN(tmp, 5);

/* ❌ 错误 */
int i = 0;
int wrong = XY_MIN(i++, 5);  /* i 被评估两次 */
```

---

### 9. 预处理

#### 9.1 条件编译

```c
/* ✅ 正确 */
#if defined(CONFIG_FEATURE)
    /* 功能代码 */
#else /* !defined(CONFIG_FEATURE) */
    /* 备选代码 */
#endif /* !defined(CONFIG_FEATURE) */

/* ❌ 错误 */
#ifdef CONFIG_FEATURE
    /* 功能代码 */
#else
    /* 备选代码 */
#endif
```

#### 9.2 头文件保护

```c
/* ✅ 正确 */
#ifndef XY_UART_H
#define XY_UART_H

/* 头文件内容 */

#endif /* XY_UART_H */

/* ❌ 错误 */
#ifndef XY_UART_H
#define XY_UART_H

/* 头文件内容 */
#endif
```

#### 9.3 嵌套条件编译

```c
/* ✅ 正确 */
#if DLEVEL > 5
    #define SIGNAL 1
    #if STACKUSE == 1
        #define STACK 200
    #else
        #define STACK 100
    #endif
#else
    #define SIGNAL 0
    #if STACKUSE == 1
        #define STACK 100
    #else
        #define STACK 50
    #endif
#endif
```

---

### 10. 结构体/枚举/联合体

#### 10.1 结构体定义方式

**方式 1：仅命名结构体（无 typedef）**
```c
struct struct_name {
    char *a;
    char b;
};

/* 使用 */
struct struct_name obj;
```

**方式 2：仅 typedef（匿名结构体）**
```c
typedef struct {
    char *a;
    char b;
} struct_name_t;

/* 使用 */
struct_name_t obj;
```

**方式 3：命名结构体 + typedef（推荐）**
```c
typedef struct struct_name {    /* 无 _t 后缀 */
    char *a;
    char b;
} struct_name_t;                /* _t 后缀 */

/* 使用 */
struct_name_t obj;
```

#### 10.2 结构体初始化
- **必须** 使用 C99 指定初始化器风格
- **应该** 为复杂结构体添加尾随逗号

```c
/* ✅ 正确 */
typedef struct {
    int a, b;
} str_t;

str_t s = {
    .a = 1,
    .b = 2,  /* 尾随逗号 */
};

/* 复杂结构体示例 */
static const my_struct_t my_var_1 = {
    .type = TYPE1,
    .type_data = {
        .type1 = {
            .par1 = 0,
            .par2 = 1,  /* 尾随逗号 */
        },  /* 尾随逗号 */
    },  /* 尾随逗号 */
};

/* ❌ 不推荐 - 缺少尾随逗号 */
static const my_struct_t my_var_2 = {
    .type = TYPE2,
    .type_data = {
        .type2 = {
            .par1 = 0,
            .par2 = 1
        }
    }
};
```

#### 10.3 枚举定义

```c
/* ✅ 正确 */
typedef enum {
    ERR_OK = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
} error_code_t;

/* 枚举成员必须全大写 */
typedef enum {
    MODE_A = 0,
    MODE_B,
    MODE_C,
} mode_t;

/* ❌ 错误 */
typedef enum {
    my_enum_testa,  /* 应该全大写 */
    my_enum_testb,
} my_enum_t;
```

#### 10.4 联合体定义

```c
/* ✅ 正确 */
typedef union {
    uint32_t value;
    struct {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
    } bytes;
} data_union_t;

/* 使用 */
data_union_t data;
data.value = 0x12345678;
/* data.bytes.b0 == 0x78 (小端) */
```

#### 10.5 位域定义

```c
/* ✅ 正确 */
typedef struct {
    uint32_t flag1 : 1;      /* 1 位 */
    uint32_t flag2 : 1;      /* 1 位 */
    uint32_t mode  : 4;      /* 4 位 */
    uint32_t reserved : 26;  /* 保留 26 位 */
} control_reg_t;
```

#### 10.6 位域注意事项
- **禁止** 在多线程环境使用相邻位域

```c
/* ❌ 错误 - 数据竞争风险 */
struct multi_threaded_flags {
    unsigned int flag1 : 2;
    unsigned int flag2 : 2;  /* 可能与 flag1 共享存储单元 */
};

struct multi_threaded_flags flags;
flags.flag1 = 1;  /* 线程 1 */
flags.flag2 = 2;  /* 线程 2 */

/* ✅ 正确 - 使用非位域成员分隔 */
struct safe_flags {
    unsigned char flag1;
    unsigned char flag2;
};
```

**原因**: 编译器可能将多个相邻位域存储在同一个存储单元中，导致多线程访问时发生数据竞争。

---

## 第三部分：内存安全

### 11. 动态内存管理

#### 11.1 分配内存
- **必须** 检查 malloc 返回值
- **应该** 使用 calloc 初始化数组

```c
/* ✅ 正确 */
int *buffer = malloc(sizeof(int) * size);
if (buffer == NULL) {
    xy_log_e("Failed to allocate memory\n");
    return XY_ERROR;
}

/* ❌ 错误 */
int *buffer = malloc(sizeof(int) * size);
/* 未检查返回值 */
```

#### 11.2 释放内存
- **必须** 配对使用 malloc/free
- **必须** 释放后立即设为 NULL

```c
/* ✅ 正确 */
free(ptr);
ptr = NULL;

/* ❌ 错误 */
free(ptr);
/* ptr 仍然是野指针 */
*ptr = 42;  /* 使用已释放的内存 */
```

#### 11.3 禁止事项
- **禁止** 使用可变长度数组 (VLA)
- **禁止** 重复释放 (double free)
- **禁止** 释放后继续使用 (use after free)

```c
/* ❌ 禁止使用 VLA */
void func(size_t size) {
    int array[size];  /* 栈溢出风险 */
}

/* ✅ 正确 - 动态分配 */
void func(size_t size) {
    int *array = malloc(sizeof(int) * size);
    if (array == NULL) {
        return;
    }
    /* 使用 array */
    free(array);
}
```

---

### 12. 缓冲区操作

#### 12.1 数组边界
- **必须** 检查数组索引在有效范围内

```c
/* ✅ 正确 */
if (index >= 0 && index < 10) {
    arr[index] = 5;
}

/* ❌ 错误 */
arr[index] = 5;  /* index 未检查 */
```

#### 12.2 字符串操作
- **必须** 确保字符串以空字符终止
- **必须** 使用安全函数（带长度参数）

```c
/* ✅ 正确 */
char buf[10];
memcpy(buf, src, 9);
buf[9] = '\0';

/* ❌ 错误 */
char buf[10];
memcpy(buf, src, 10);  /* 可能没有 '\0' */
```

#### 12.3 安全函数替代

| 不安全函数 | 安全替代 |
|-----------|---------|
| `strcpy()` | `strncpy()` |
| `strcat()` | `strncat()` |
| `sprintf()` | `snprintf()` |
| `gets()` | `fgets()` |
| `scanf()` | `fgets()` + `sscanf()` |

```c
/* ✅ 正确 */
char buffer[100];
snprintf(buffer, sizeof(buffer), "Value: %d", value);

/* ❌ 错误 */
char buffer[100];
sprintf(buffer, "Value: %d", value);  /* 可能溢出 */
```

---

### 13. 栈使用

#### 13.1 局部变量大小
- **应该** 限制局部变量大小（<1KB）
- **应该** 大缓冲区使用动态分配

```c
/* ✅ 正确 */
void func(void) {
    uint8_t buffer[256];  /* 256 字节，安全 */
}

/* ❌ 错误 */
void func(void) {
    uint8_t buffer[4096];  /* 4KB，可能栈溢出 */
}
```

#### 13.2 递归
- **应该** 避免深度递归
- **应该** 使用迭代代替递归

```c
/* ✅ 正确 - 使用迭代 */
int factorial_iterative(int n) {
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

/* ❌ 不推荐 - 深度递归 */
int factorial_recursive(int n) {
    if (n <= 1) return 1;
    return n * factorial_recursive(n - 1);  /* 可能栈溢出 */
}
```

---

## 第四部分：安全规则

### 14. 整数安全

#### 14.1 溢出检查
- **必须** 确保无符号整数不溢出
- **必须** 确保有符号整数运算不会溢出

```c
/* ✅ 正确 - 检查溢出 */
if (UINT_MAX - x >= 1) {
    x += 1;
}

if (INT_MAX - a >= 1) {
    int b = a + 1;
}

/* ❌ 错误 */
unsigned int x = UINT_MAX;
x += 1;  /* 溢出 */

int a = INT_MAX;
int b = a + 1;  /* 溢出 */
```

#### 14.2 类型转换
- **必须** 检查整数转换不丢失信息

```c
/* ✅ 正确 */
if (value <= INT_MAX) {
    int32_t converted = (int32_t)value;
}

/* ❌ 错误 */
int32_t converted = (int32_t)large_value;  /* 可能丢失信息 */
```

---

### 15. 字符串安全

#### 15.1 字符串终止
- **必须** 确保字符串以空字符终止

```c
/* ✅ 正确 */
char dest[20];
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';

/* ❌ 错误 */
char dest[5];
strcpy(dest, "long string");  /* 缓冲区溢出 */
```

#### 15.2 wchar_t 字符串
- **必须** 确保 wchar_t 字符串以空宽字符终止

```c
/* ✅ 正确 */
wchar_t wbuf[10];
wmemcpy(wbuf, wsrc, 9);
wbuf[9] = L'\0';

/* ❌ 错误 */
wchar_t wbuf[10];
wmemcpy(wbuf, wsrc, 10);  /* 可能没有 L'\0' */
```

---

### 16. 并发安全

#### 16.1 位域数据竞争
- **必须** 防止位域数据竞争

```c
/* ❌ 错误 - 数据竞争风险 */
struct multi_threaded_flags {
    unsigned int flag1 : 2;
    unsigned int flag2 : 2;
};

struct multi_threaded_flags flags;
flags.flag1 = 1;  /* 线程 1 */
flags.flag2 = 2;  /* 线程 2 */

/* ✅ 正确 - 使用非位域成员分隔 */
struct safe_flags {
    unsigned char flag1;
    unsigned char flag2;
};
```

#### 16.2 互斥锁顺序
- **必须** 按正确顺序解锁互斥锁

```c
/* ✅ 正确 */
pthread_mutex_lock(&mutex1);
pthread_mutex_lock(&mutex2);
/* ... */
pthread_mutex_unlock(&mutex2);
pthread_mutex_unlock(&mutex1);

/* ❌ 错误 - 可能导致死锁 */
pthread_mutex_lock(&mutex1);
pthread_mutex_lock(&mutex2);
/* ... */
pthread_mutex_lock(&mutex1);  /* 重复锁定 */
```

#### 16.3 信号处理
- **禁止** 在信号处理函数中调用异步信号不安全函数

```c
/* ❌ 错误 */
void handler(int sig) {
    printf("Signal received\n");  /* printf 不是异步信号安全的 */
}

/* ✅ 正确 */
void handler(int sig) {
    write(STDOUT_FILENO, "Signal received\n", 16);  /* write 是异步信号安全的 */
}
```

---

### 17. 错误处理

#### 17.1 标准库函数错误
- **必须** 检测并处理标准库函数的错误

```c
/* ✅ 正确 */
FILE *f = fopen("file.txt", "r");
if (f == NULL) {
    perror("fopen failed");
    return -1;
}
size_t n = fread(buf, 1, size, f);
if (ferror(f)) {
    /* 处理读取错误 */
}

/* ❌ 错误 */
FILE *f = fopen("file.txt", "r");
fread(buf, 1, size, f);  /* 未检查 fopen 是否成功 */
```

#### 17.2 搜索和排序函数
- **必须** 检测搜索和排序函数中的错误

```c
/* ✅ 正确 */
int *result = bsearch(key, array, n, sizeof(int), cmp);
if (result != NULL) {
    process(*result);
} else {
    /* 处理未找到的情况 */
}

/* ❌ 错误 */
int *result = bsearch(key, array, n, sizeof(int), cmp);
process(*result);  /* 未检查 result 是否为 NULL */
```

---

## 📚 大厂规范参考

### Google C 风格指南
- 类型：大驼峰 (`MyClass`)
- 函数：小写 + 下划线 (`my_function`)
- 常量：全大写 (`MAX_SIZE`)

### Linux 内核编码风格
- 函数/变量：小写 + 下划线 (`my_function`)
- 宏：全大写 (`MAX_SIZE`)
- 结构体：小写 + 下划线 (`my_struct`)

### FreeRTOS 编码规范
- 类型：大驼峰带前缀 (`PortBASE_TYPE`)
- 函数：大驼峰带前缀 (`xTaskCreate`)
- 宏：全大写带前缀 (`portCONFIGURE_INTERRUPT()`)

### RT-Thread 编码规范
- 函数/变量：小写 + 下划线 (`rt_thread_create`)
- 类型：小写 + 下划线，`_t` 后缀 (`rt_thread_t`)
- 宏：全大写 + 下划线 (`RT_THREAD_PRIORITY_MAX`)

### Zephyr 编码规范
- 函数/变量：小写 + 下划线 (`k_thread_create`)
- 类型：小写 + 下划线，`_t` 后缀 (`k_thread_t`)
- 宏：全大写 + 下划线 (`K_THREAD_STACK_SIZE`)

---

## 🔗 相关文档

- [SEI CERT C 编码标准](SEI_CERT_C_Coding_Standard.md)
- [GoF 设计模式](../design/GoF_Design_Patterns.md)
- [TAOCP 学习笔记](../reference/TAOCP_编程生涯学习笔记.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
