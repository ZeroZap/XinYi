# 内存安全指南 (Memory Safety Guide)

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: TAOCP/CYouAgain/0-Code, 2-CBuildTrace

---

## 📋 概述

本文档提供 XinYi 项目的内存安全最佳实践，所有代码贡献必须遵循此指南。

---

## ⚠️ 动态内存分配

### 必须遵守的规则

#### 1. 总是检查 malloc 返回值

```c
/* ✅ 正确示例 */
int *buffer = malloc(sizeof(int) * size);
if (buffer == NULL) {
    xy_log_e("Failed to allocate memory\n");
    return XY_ERROR;
}

/* ❌ 错误示例 */
int *buffer = malloc(sizeof(int) * size);
/* 未检查返回值 */
buffer[0] = 42;  /* 可能崩溃 */
```

#### 2. 总是配对使用 malloc 和 free

```c
/* ✅ 正确示例 */
char *str = malloc(100);
if (str == NULL) {
    return XY_ERROR;
}

/* 使用 str */

free(str);
str = NULL;  /* 防止野指针 */

/* ❌ 错误示例 */
char *str = malloc(100);
/* 使用后未释放 - 内存泄漏 */
```

#### 3. 释放后立即设为 NULL

```c
/* ✅ 正确做法 */
free(ptr);
ptr = NULL;

/* ❌ 错误做法 */
free(ptr);
/* ptr 仍然是野指针 */
*ptr = 42;  /* 未定义行为 */
```

#### 4. 使用 calloc 初始化数组

```c
/* ✅ 推荐 - 自动清零 */
int *array = calloc(100, sizeof(int));

/* ❌ 不推荐 - 需要手动初始化 */
int *array = malloc(100 * sizeof(int));
memset(array, 0, 100 * sizeof(int));
```

---

## 🚫 禁止事项

### 1. 禁止使用可变长度数组 (VLA)

```c
/* ❌ 错误 - VLA */
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

### 2. 禁止重复释放 (Double Free)

```c
/* ❌ 错误 */
free(ptr);
free(ptr);  /* 重复释放 - 未定义行为 */

/* ✅ 正确 */
free(ptr);
ptr = NULL;
free(ptr);  /* free(NULL) 是安全的，什么都不做 */
```

### 3. 禁止释放后继续使用 (Use After Free)

```c
/* ❌ 错误 */
free(ptr);
*ptr = 42;  /* 使用已释放的内存 - 未定义行为 */

/* ✅ 正确 */
free(ptr);
ptr = NULL;
/* 不再使用 ptr */
```

### 4. 禁止内存泄漏

```c
/* ❌ 错误 - 内存泄漏 */
void leak_memory(void) {
    char *buffer = malloc(1024);
    /* 忘记释放 */
}

/* ✅ 正确 */
void no_leak(void) {
    char *buffer = malloc(1024);
    if (buffer == NULL) {
        return;
    }
    
    /* 使用 buffer */
    
    free(buffer);
}
```

---

## 🔍 指针使用规范

### 1. 使用前检查指针有效性

```c
/* ✅ 正确示例 */
int process_data(const int *data, size_t len)
{
    if (data == NULL || len == 0) {
        return XY_INVALID_PARAM;
    }
    
    for (size_t i = 0; i < len; i++) {
        if (data[i] > MAX_VALUE) {
            return XY_ERROR;
        }
    }
    
    return XY_OK;
}

/* ❌ 错误示例 */
int process_data(const int *data, size_t len)
{
    /* 未检查 NULL */
    for (size_t i = 0; i < len; i++) {
        /* 可能崩溃 */
    }
}
```

### 2. 数组访问检查边界

```c
/* ✅ 正确示例 */
int get_element(int *array, size_t size, size_t index)
{
    if (array == NULL || index >= size) {
        return XY_INVALID_PARAM;
    }
    return array[index];
}

/* ❌ 错误示例 */
int get_element(int *array, size_t size, size_t index)
{
    /* 未检查边界 */
    return array[index];  /* 可能越界 */
}
```

### 3. 使用 const 标记只读指针

```c
/* ✅ 正确 - 明确意图 */
void print_data(const char *data)
{
    /* 只读访问 */
    printf("%s\n", data);
}

/* ❌ 不推荐 - 意图不明确 */
void print_data(char *data)
{
    printf("%s\n", data);
}
```

### 4. 指针运算时注意类型大小

```c
/* ✅ 正确 */
int *array = malloc(10 * sizeof(int));
int *ptr = array;
ptr++;  /* 移动 sizeof(int) 字节 */

/* ❌ 错误 - 类型不匹配 */
char *ptr = (char*)array;
ptr++;  /* 只移动 1 字节，可能破坏对齐 */
```

---

## 📦 缓冲区操作规范

### 1. 总是传递缓冲区大小

```c
/* ✅ 正确 - 带大小参数 */
int copy_string(char *dest, const char *src, size_t dest_size)
{
    if (dest == NULL || src == NULL || dest_size == 0) {
        return XY_INVALID_PARAM;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';  /* 确保终止 */
    
    return XY_OK;
}

/* ❌ 错误 - 无大小参数 */
int copy_string(char *dest, const char *src)
{
    strcpy(dest, src);  /* 可能溢出 */
}
```

### 2. 使用安全函数（带长度参数）

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

### 3. 检查字符串终止符

```c
/* ✅ 正确 */
size_t safe_strlen(const char *str, size_t max_len)
{
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }
    return len;
}

/* ❌ 错误 */
size_t unsafe_strlen(const char *str)
{
    size_t len = 0;
    while (str[len] != '\0') {  /* 可能无限循环 */
        len++;
    }
    return len;
}
```

---

## 💾 栈使用规范

### 1. 限制局部变量大小

```c
/* ✅ 正确 - 小缓冲区在栈上 */
void func(void) {
    uint8_t buffer[256];  /* 256 字节，安全 */
}

/* ❌ 错误 - 大缓冲区在栈上 */
void func(void) {
    uint8_t buffer[4096];  /* 4KB，可能栈溢出 */
}
```

### 2. 大缓冲区使用动态分配

```c
/* ✅ 正确 */
void process_large_data(void)
{
    uint8_t *buffer = malloc(4096);
    if (buffer == NULL) {
        return;
    }
    
    /* 使用 buffer */
    
    free(buffer);
}

/* ❌ 错误 */
void process_large_data(void)
{
    uint8_t buffer[4096];  /* 栈上分配大缓冲区 */
}
```

### 3. 避免深度递归

```c
/* ✅ 正确 - 使用迭代 */
int factorial_iterative(int n)
{
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

/* ❌ 错误 - 深度递归 */
int factorial_recursive(int n)
{
    if (n <= 1) return 1;
    return n * factorial_recursive(n - 1);  /* 可能栈溢出 */
}
```

---

## 🔍 常见内存错误

| 错误 | 说明 | 预防方法 |
|------|------|---------|
| 内存泄漏 | 分配后未释放 | 配对使用 malloc/free |
| 重复释放 | 释放同一指针多次 | 释放后设为 NULL |
| 越界访问 | 访问数组边界外 | 检查索引范围 |
| 空指针解引用 | 解引用 NULL 指针 | 使用前检查 |
| 野指针 | 指向已释放内存 | 释放后设为 NULL |
| 栈溢出 | 局部变量过大 | 大缓冲区用 malloc |

---

## 🛠️ 检测工具

### 静态分析

- **Clang Static Analyzer**: `scan-build gcc -c file.c`
- **Cppcheck**: `cppcheck --enable=all file.c`
- **Coverity**: 商业工具

### 动态分析

- **Valgrind** (Linux): `valgrind --leak-check=full ./program`
- **AddressSanitizer** (ASan): `gcc -fsanitize=address file.c`
- **MemorySanitizer** (MSan): `clang -fsanitize=memory file.c`

---

## 📚 参考

- [RULEBOOK](RULEBOOK.md)
- [代码风格指南](../design/Code_Style_Design_Guide.md)
- [TAOCP 学习笔记](../reference/TAOCP_编程生涯学习笔记.md)
- [SEI CERT C Coding Standard](../CCG/SEI%20CERT%20C%20Coding%20Standard🚩/rules/)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
