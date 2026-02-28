# C 语言编码规范

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: xy_code_style.md, memory_safety.md, safety_overview.md, SEI CERT

---

## 📋 概述

本文档整合了 XinYi 项目的完整 C 语言编码规范，包括代码风格、内存安全、安全规则三大部分。

**适用范围**: 所有 XinYi 项目 C 语言代码

---

## 📚 目录

### 第一部分：代码风格
1. [基本原则](#1-基本原则)
2. [命名规范](#2-命名规范)
3. [代码格式](#3-代码格式)
4. [注释规范](#4-注释规范)
5. [函数风格](#5-函数风格)
6. [预处理](#6-预处理)

### 第二部分：内存安全
7. [动态内存管理](#7-动态内存管理)
8. [指针使用](#8-指针使用)
9. [缓冲区操作](#9-缓冲区操作)
10. [栈使用](#10-栈使用)

### 第三部分：安全规则
11. [整数安全](#11-整数安全)
12. [字符串安全](#12-字符串安全)
13. [并发安全](#13-并发安全)
14. [错误处理](#14-错误处理)

---

## 第一部分：代码风格

### 1. 基本原则

#### 1.1 语言标准
- **必须** 使用 C99 标准
- **禁止** 使用编译器特定扩展（除非必要）

#### 1.2 缩进和空格
- **必须** 使用 4 个空格缩进
- **禁止** 使用制表符 (Tab)
- **必须** 在关键字和左括号之间使用 1 个空格

```c
/* ✅ 正确 */
if (condition) {
    do_something();
}

for (int i = 0; i < 10; i++) {
    /* ... */
}

/* ❌ 错误 */
if(condition) {  /* 缺少空格 */
    do_something();
}

if (condition){  /* 括号位置错误 */
    do_something();
}
```

#### 1.3 运算符空格
- **必须** 在运算符前后使用单个空格

```c
/* ✅ 正确 */
int a = 3 + 4;
for (int i = 0; i < 5; i++) {
    a = b + c;
}

/* ❌ 错误 */
int a=3+4;
for(int i=0;i<5;i++){
    a=b+c;
}
```

---

### 2. 命名规范

#### 2.1 基本原则
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
```

#### 2.2 函数命名

| 函数类型 | 命名规则 | 示例 |
|---------|---------|------|
| 普通函数 | 小写 + 下划线 | `xy_uart_init()` |
| Getter | `module_get_xxx()` | `battery_get_voltage()` |
| Setter | `module_set_xxx()` | `battery_set_threshold()` |
| 私有函数 | `prv_` 前缀 | `prv_initialize_buffer()` |
| 回调函数 | `xxx_callback` | `uart_rx_callback` |

#### 2.3 变量命名

| 变量类型 | 命名规则 | 示例 |
|---------|---------|------|
| 局部变量 | 小写 + 下划线 | `counter`, `buffer_size` |
| 全局变量 | `g_` 前缀 | `g_uart_buffer` |
| 静态变量 | `s_` 前缀 | `s_initialized` |
| 常量 | `k_` 前缀 | `k_max_retries` |
| 指针 | 明确表达指向 | `buffer_ptr`, `node_list` |

#### 2.4 类型命名

| 类型 | 命名规则 | 示例 |
|------|---------|------|
| 结构体（typedef） | 小写 + 下划线，`_t` 后缀 | `my_struct_t` |
| 枚举（类型） | 小写 + 下划线，`_t` 后缀 | `my_enum_t` |
| 枚举（成员） | 全大写 + 下划线 | `MY_ENUM_VALUE_A` |
| 联合体 | 小写 + 下划线，`_t` 后缀 | `data_union_t` |

#### 2.5 宏命名

| 宏类型 | 命名规则 | 示例 |
|--------|---------|------|
| 常量宏 | 全大写 + 下划线 | `MAX_BUFFER_SIZE` |
| 函数宏 | 全大写 + 下划线 | `XY_MIN(x, y)` |
| 条件编译 | 全大写 + 下划线 | `XY_HAL_UART_ENABLED` |
| 头文件保护 | 全大写 + 下划线 | `XY_UART_H` |

#### 2.6 禁止事项

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

### 3. 代码格式

#### 3.1 括号位置
- **必须** 将左花括号放在与关键字同一行

```c
/* ✅ 正确 */
if (condition) {
    do_something();
}

for (int i = 0; i < 10; i++) {
    /* ... */
}

/* ❌ 错误 */
if (condition)
{
    do_something();
}

if (condition){
    do_something();
}
```

#### 3.2 复合语句
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

#### 3.3 switch 语句

```c
/* ✅ 正确 */
switch (value) {
    case 0:
        handle_zero();
        break;
    case 1:
        handle_one();
        break;
    default:
        handle_default();
        break;
}
```

---

### 4. 注释规范

#### 4.1 注释风格
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

#### 4.2 Doxygen 文档

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

#### 4.3 结构体成员注释

```c
typedef struct {
    int x;              /**< X 坐标 */
    int y;              /**< Y 坐标 */
    char *name;         /**< 名称 */
} point_t;
```

---

### 5. 函数风格

#### 5.1 函数声明
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

#### 5.2 函数参数
- **必须** 对只读参数使用 `const`
- **应该** 限制参数数量（不超过 5 个）

```c
/* ✅ 正确 */
void process_data(const uint8_t *data, size_t len);

/* ❌ 错误 */
void process_data(uint8_t *data, size_t len);  /* 可能修改数据 */
```

#### 5.3 函数长度
- **应该** 限制函数长度（不超过 50 行）
- **应该** 一个函数只做一件事

---

### 6. 预处理

#### 6.1 宏定义

```c
/* ✅ 正确 - 括号保护 */
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define SQUARE(x) ((x) * (x))

/* ❌ 错误 - 缺少括号 */
#define MIN(x, y) x < y ? x : y
#define SQUARE(x) x * x
```

#### 6.2 多语句宏
- **必须** 使用 `do { } while (0)` 包装

```c
/* ✅ 正确 */
#define LOG_AND_INC(x) do { log(x); (x)++; } while (0)

/* ❌ 错误 */
#define LOG_AND_INC(x) log(x); x++
```

#### 6.3 条件编译

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

#### 6.4 头文件保护

```c
/* ✅ 正确 */
#ifndef XY_UART_H
#define XY_UART_H

/* 头文件内容 */

#endif /* XY_UART_H */
```

---

## 第二部分：内存安全

### 7. 动态内存管理

#### 7.1 分配内存
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

#### 7.2 释放内存
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

#### 7.3 禁止事项
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

### 8. 指针使用

#### 8.1 空指针检查
- **必须** 使用前检查指针有效性

```c
/* ✅ 正确 */
int *ptr = get_pointer();
if (ptr != NULL) {
    *ptr = 42;
}

/* ❌ 错误 */
int *ptr = get_pointer();
*ptr = 42;  /* ptr 可能为 NULL */
```

#### 8.2 const 指针
- **必须** 对只读指针使用 `const`

```c
/* ✅ 正确 */
void print_data(const char *data);

/* ❌ 不推荐 */
void print_data(char *data);  /* 意图不明确 */
```

#### 8.3 指针运算
- **必须** 注意类型大小
- **应该** 使用无符号类型进行位移

```c
/* ✅ 正确 */
uint32_t x = 1;
x <<= 31;  /* 使用无符号整数 */

/* ❌ 错误 */
int32_t x = -1;
x <<= 31;  /* 有符号整数左移是未定义行为 */
```

---

### 9. 缓冲区操作

#### 9.1 数组边界
- **必须** 检查数组索引在有效范围内

```c
/* ✅ 正确 */
if (index >= 0 && index < 10) {
    arr[index] = 5;
}

/* ❌ 错误 */
arr[index] = 5;  /* index 未检查 */
```

#### 9.2 字符串操作
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

#### 9.3 安全函数替代

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

### 10. 栈使用

#### 10.1 局部变量大小
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

#### 10.2 递归
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

## 第三部分：安全规则

### 11. 整数安全

#### 11.1 溢出检查
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

#### 11.2 类型转换
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

### 12. 字符串安全

#### 12.1 字符串终止
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

#### 12.2 wchar_t 字符串
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

### 13. 并发安全

#### 13.1 位域数据竞争
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

#### 13.2 互斥锁顺序
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

#### 13.3 信号处理
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

### 14. 错误处理

#### 14.1 标准库函数错误
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

#### 14.2 搜索和排序函数
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
