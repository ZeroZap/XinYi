# C 内存与安全背景参考

更新日期：2026-11-20

代码风格与通用编码要求统一见 [code_style.md](../../code_style.md)。
本文保留内存、字符串、并发、标准库和搜索接口的安全背景案例，
不再定义命名、缩进、花括号、注释或工具配置规则；例子用于解释风险，不作为另一份排版规范。

## 内存安全背景

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

#### 9.3 字符串接口选择

下表不代表自动安全替换。仍需验证容量、源对象有效范围、终止符、返回值与截断；
`strncpy`、`strncat` 本身不保证接口调用安全。

| 易误用函数 | 可考虑的替代接口 |
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

## 其他安全背景

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

## 延伸阅读

- [SEI CERT C 安全编码参考](SEI_CERT_C_Coding_Standard.md)
