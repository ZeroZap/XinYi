# 内存安全指南 (Memory Safety Guide)

**状态**: ⚠️ 待完善

---

## 📋 概述

本文档提供 XinYi 项目的内存安全最佳实践。

---

## ⚠️ 内存安全规则

### 1. 动态内存分配

#### 必须遵守

- ✅ 总是检查 `malloc()` 返回值
- ✅ 总是配对使用 `malloc()` 和 `free()`
- ✅ 释放后立即将指针设为 `NULL`
- ✅ 使用 `calloc()` 初始化数组

```c
/* ✅ 正确示例 */
int *buffer = malloc(sizeof(int) * size);
if (buffer == NULL) {
    xy_log_e("Failed to allocate memory\n");
    return XY_ERROR;
}

/* 使用完毕后 */
free(buffer);
buffer = NULL;
```

#### 禁止事项

- ❌ 使用可变长度数组 (VLA)
- ❌ 忽略 `malloc()` 返回值
- ❌ 重复释放 (double free)
- ❌ 释放后继续使用 (use after free)

```c
/* ❌ 错误示例 */
int buffer[size];  /* VLA - 禁止使用 */

int *ptr = malloc(sizeof(int));
/* 未检查返回值 */

free(ptr);
free(ptr);  /* 重复释放 */
*ptr = 42;  /* 释放后使用 */
```

### 2. 指针使用

#### 必须遵守

- ✅ 使用前检查指针有效性
- ✅ 数组访问检查边界
- ✅ 使用 `const` 标记只读指针
- ✅ 指针运算时注意类型大小

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
```

### 3. 缓冲区操作

#### 必须遵守

- ✅ 总是传递缓冲区大小
- ✅ 使用安全函数（带长度参数）
- ✅ 检查字符串终止符

```c
/* ✅ 正确示例 */
int copy_string(char *dest, const char *src, size_t dest_size)
{
    if (dest == NULL || src == NULL || dest_size == 0) {
        return XY_INVALID_PARAM;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';  /* 确保终止 */
    
    return XY_OK;
}
```

#### 禁止事项

- ❌ 使用 `strcpy()`、`strcat()`、`sprintf()` 等不安全函数
- ❌ 假设缓冲区足够大
- ❌ 忽略字符串终止符

### 4. 栈使用

#### 最佳实践

- ✅ 限制局部变量大小（<1KB）
- ✅ 大缓冲区使用动态分配
- ✅ 避免深度递归

```c
/* ✅ 正确示例 */
void process_large_data(void)
{
    /* 大缓冲区使用动态分配 */
    uint8_t *buffer = malloc(4096);
    if (buffer == NULL) {
        return;
    }
    
    /* 使用 buffer */
    
    free(buffer);
}

/* ❌ 错误示例 */
void process_large_data(void)
{
    uint8_t buffer[4096];  /* 栈上分配大缓冲区 */
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

---

## 🛠️ 检测工具

### 静态分析

- Clang Static Analyzer
- Cppcheck
- Coverity

### 动态分析

- Valgrind (Linux)
- AddressSanitizer (ASan)
- MemorySanitizer (MSan)

---

## 📚 参考

- [RULEBOOK](../RULEBOOK.md)
- [代码风格指南](../100-code_style/xy_code_style.md)
- [安全规则](../300-security-rules/safety_overview.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
