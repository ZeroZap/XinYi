# SEI CERT C 编码标准

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: SEI CERT C Coding Standard

---

## 📋 概述

SEI CERT C 编码标准是由卡内基梅隆大学软件工程研究所 (SEI) 开发的安全编码规范，旨在预防在使用 C 语言开发软件时可能引入的安全漏洞。

---

## 🗂️ 规则分类

SEI CERT C 标准将安全编码规则分为 **17 个类别**：

| 编号 | 分类 | 英文全称 | 规则数 |
|------|------|----------|--------|
| PRE | 预处理器 | Preprocessor | 15+ |
| DCL | 声明和初始化 | Declarations and Initialization | 20+ |
| EXP | 表达式 | Expressions | 40+ |
| INT | 整数 | Integers | 15+ |
| FLP | 浮点数 | Floating Point | 10+ |
| ARR | 数组 | Arrays | 15+ |
| STR | 字符和字符串 | Characters and Strings | 20+ |
| MEM | 内存管理 | Memory Management | 25+ |
| FIO | 输入输出 | Input Output | 25+ |
| ENV | 环境 | Environment | 10+ |
| SIG | 信号 | Signals | 15+ |
| ERR | 错误处理 | Error Handling | 10+ |
| API | 应用程序接口 | Application Programming Interfaces | 15+ |
| CON | 并发 | Concurrency | 40+ |
| MSC | 杂项 | Miscellaneous | 30+ |
| POS | POSIX | POSIX | 20+ |
| WIN | Microsoft Windows | Microsoft Windows | 10+ |

---

## 🔴 最重要的 30 条规则

### 预处理器 (PRE)

#### PRE31-C - 避免宏定义中的副作用

```c
/* ❌ 不安全 - 参数被多次评估 */
#define SQUARE(x) x * x
int result = SQUARE(++i);  /* 展开为 ++i * ++i，未定义行为 */

/* ✅ 安全 - 括号保护 */
#define SQUARE(x) ((x) * (x))
int result = SQUARE(i);  /* 安全使用 */
```

#### PRE00-C - 避免复杂宏

```c
/* ❌ 不安全 - 多语句宏 */
#define LOG_AND_INC(x) log(x); x++

/* ✅ 安全 - 使用 do-while(0) */
#define LOG_AND_INC(x) do { log(x); (x)++; } while (0)
```

---

### 声明和初始化 (DCL)

#### DCL30-C - 不要声明具有外部链接的标识符与内部标识符冲突

```c
/* ❌ 不安全 - 与标准库 errno 冲突 */
int errno;

/* ✅ 安全 - 使用静态或唯一命名 */
static int my_errno;
```

#### DCL02-C - 不使用变长数组 (VLA)

```c
/* ❌ 不安全 - VLA 可能导致栈溢出 */
void func(size_t size) {
    int array[size];
}

/* ✅ 安全 - 使用动态分配 */
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

### 表达式 (EXP)

#### EXP33-C - 不要解引用空指针

```c
/* ❌ 不安全 */
int *ptr = get_pointer();
*ptr = 42;  /* ptr 可能为 NULL */

/* ✅ 安全 */
int *ptr = get_pointer();
if (ptr != NULL) {
    *ptr = 42;
}
```

#### EXP42-C - 不要使用有符号整数的位移位产生未定义行为

```c
/* ❌ 不安全 */
int32_t x = -1;
x <<= 31;  /* 有符号整数左移是未定义行为 */

/* ✅ 安全 */
uint32_t x = 1;
x <<= 31;  /* 使用无符号整数 */
```

#### EXP40-C - 不要修改易失性变量

```c
/* ❌ 不安全 */
volatile int flag;
flag = flag + 1;  /* 多次访问 volatile */

/* ✅ 安全 */
volatile int flag;
int temp = flag;
temp = temp + 1;
flag = temp;
```

---

### 整数 (INT)

#### INT30-C - 确保无符号整数不溢出

```c
/* ❌ 不安全 */
unsigned int x = UINT_MAX;
x += 1;  /* 溢出 */

/* ✅ 安全 - 检查溢出 */
if (UINT_MAX - x >= 1) {
    x += 1;
}
```

#### INT32-C - 确保有符号整数运算不会溢出

```c
/* ❌ 不安全 */
int a = INT_MAX;
int b = a + 1;  /* 溢出 */

/* ✅ 安全 - 检查溢出 */
if (INT_MAX - a >= 1) {
    int b = a + 1;
}
```

#### INT36-C - 将整数转换为指针时保持符号扩展

```c
/* ❌ 不安全 */
intptr_t addr = get_address();
void *ptr = (void*)(uint32_t)addr;  /* 可能丢失符号位 */

/* ✅ 安全 */
intptr_t addr = get_address();
void *ptr = (void*)addr;
```

---

### 数组 (ARR)

#### ARR30-C - 确保数组索引在有效范围内

```c
/* ❌ 不安全 */
int arr[10];
arr[index] = 5;  /* index 未检查 */

/* ✅ 安全 */
if (index >= 0 && index < 10) {
    arr[index] = 5;
}
```

#### ARR38-C - 保证变参列表中数组到指针的转换

```c
/* ❌ 不安全 */
void func(int arr[]);  /* 实际上传递的是指针 */

/* ✅ 安全 - 明确是指针 */
void func(int *arr);
```

---

### 字符串 (STR)

#### STR30-C - 确保字符串以空字符终止

```c
/* ❌ 不安全 */
char buf[10];
memcpy(buf, src, 10);  /* 可能没有 '\0' */

/* ✅ 安全 */
char buf[10];
memcpy(buf, src, 9);
buf[9] = '\0';
```

#### STR31-C - 确保字符串操作有足够的空间

```c
/* ❌ 不安全 */
char dest[5];
strcpy(dest, "long string");  /* 缓冲区溢出 */

/* ✅ 安全 */
char dest[20];
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

#### STR32-C - 确保 wchar_t 字符串以空宽字符终止

```c
/* ❌ 不安全 */
wchar_t wbuf[10];
wmemcpy(wbuf, wsrc, 10);  /* 可能没有 L'\0' */

/* ✅ 安全 */
wchar_t wbuf[10];
wmemcpy(wbuf, wsrc, 9);
wbuf[9] = L'\0';
```

---

### 内存管理 (MEM)

#### MEM30-C - 不要释放已释放的内存

```c
/* ❌ 不安全 - 双重释放 */
free(ptr);
free(ptr);

/* ✅ 安全 */
free(ptr);
ptr = NULL;  /* 释放后立即置空 */
```

#### MEM35-C - 充分分配内存

```c
/* ❌ 不安全 */
char *buf = malloc(strlen(str));  /* 缺少 '\0' 空间 */

/* ✅ 安全 */
char *buf = malloc(strlen(str) + 1);  /* 为 '\0' 预留空间 */
```

#### MEM00-C - 正确分配内存

```c
/* ❌ 不安全 */
int *ptr = malloc(10);  /* 未检查返回值 */

/* ✅ 安全 */
int *ptr = malloc(10 * sizeof(int));
if (ptr == NULL) {
    /* 处理错误 */
    return;
}
```

#### MEM01-C - 释放后不访问内存

```c
/* ❌ 不安全 */
free(ptr);
*ptr = 42;  /* 使用已释放的内存 */

/* ✅ 安全 */
free(ptr);
ptr = NULL;
/* 不再使用 ptr */
```

---

### 输入输出 (FIO)

#### FIO34-C - 区分字符和字节

```c
/* ❌ 不安全 */
char c = getchar();
if (c == EOF) { ... }  /* char 可能无法表示 EOF */

/* ✅ 安全 */
int c = getchar();  /* 使用 int 存储 getchar 返回值 */
if (c == EOF) { ... }
```

#### FIO00-C - 不操作未打开文件

```c
/* ❌ 不安全 */
FILE *f = fopen("file.txt", "r");
fread(buf, 1, size, f);  /* 未检查 fopen 是否成功 */

/* ✅ 安全 */
FILE *f = fopen("file.txt", "r");
if (f == NULL) {
    perror("fopen failed");
    return -1;
}
fread(buf, 1, size, f);
```

#### FIO02-C - 验证 fopen 返回值

```c
/* ❌ 不安全 */
FILE *f = fopen("file.txt", "r");
/* 直接使用 f */

/* ✅ 安全 */
FILE *f = fopen("file.txt", "r");
if (f == NULL) {
    /* 处理错误 */
}
```

---

### 信号 (SIG)

#### SIG30-C - 不要在信号处理函数中调用异步信号不安全函数

```c
/* ❌ 不安全 */
void handler(int sig) {
    printf("Signal received\n");  /* printf 不是异步信号安全的 */
}

/* ✅ 安全 */
void handler(int sig) {
    write(STDOUT_FILENO, "Signal received\n", 16);  /* write 是异步信号安全的 */
}
```

#### SIG31-C - 不要在信号处理函数中访问非原子全局变量

```c
/* ❌ 不安全 */
volatile int flag;
void handler(int sig) {
    flag = flag + 1;  /* 非原子操作 */
}

/* ✅ 安全 */
sig_atomic_t flag;
void handler(int sig) {
    flag = 1;  /* sig_atomic_t 是原子类型 */
}
```

---

### 错误处理 (ERR)

#### ERR33-C - 检测并处理标准库函数的错误

```c
/* ❌ 不安全 */
FILE *f = fopen("file.txt", "r");
fread(buf, 1, size, f);

/* ✅ 安全 */
FILE *f = fopen("file.txt", "r");
if (f == NULL) {
    perror("fopen failed");
    return -1;
}
size_t n = fread(buf, 1, size, f);
if (ferror(f)) {
    /* 处理读取错误 */
}
```

#### ERR34-C - 检测搜索和排序函数中的错误

```c
/* ❌ 不安全 */
int *result = bsearch(key, array, n, sizeof(int), cmp);
process(*result);  /* 未检查 result 是否为 NULL */

/* ✅ 安全 */
int *result = bsearch(key, array, n, sizeof(int), cmp);
if (result != NULL) {
    process(*result);
} else {
    /* 处理未找到的情况 */
}
```

---

### 并发 (CON)

#### CON30-C - 清理线程特定存储

```c
/* ❌ 不安全 - 可能导致资源泄漏 */
pthread_key_create(&key, NULL);

/* ✅ 安全 */
pthread_key_create(&key, cleanup_function);  /* 提供清理函数 */
```

#### CON32-C - 防止位域数据竞争

```c
/* ❌ 不安全 - 数据竞争 */
struct multi_threaded_flags {
    unsigned int flag1 : 2;
    unsigned int flag2 : 2;
};

struct multi_threaded_flags flags;
flags.flag1 = 1;  /* 线程 1 */
flags.flag2 = 2;  /* 线程 2 */

/* ✅ 安全 - 使用非位域成员分隔 */
struct safe_flags {
    unsigned char flag1;
    unsigned char flag2;
};
```

#### CON36-C - 按正确顺序解锁互斥锁

```c
/* ❌ 不安全 - 可能导致死锁 */
pthread_mutex_lock(&mutex1);
pthread_mutex_lock(&mutex2);
/* ... */
pthread_mutex_lock(&mutex1);  /* 重复锁定 */

/* ✅ 安全 */
pthread_mutex_lock(&mutex1);
pthread_mutex_lock(&mutex2);
/* ... */
pthread_mutex_unlock(&mutex2);
pthread_mutex_unlock(&mutex1);
```

---

### 杂项 (MSC)

#### MSC30-C - 不要使用 rand() 生成安全相关的值

```c
/* ❌ 不安全 */
int key = rand();  /* 可预测 */

/* ✅ 安全 */
/* 使用加密安全的随机数生成器 */
unsigned char key[16];
get_random_bytes(key, sizeof(key));  /* 平台特定的安全 RNG */
```

#### MSC24-C - 不要使用已弃用的函数

```c
/* ❌ 不安全 */
gets(buffer);  /* 已被移除，极度危险 */

/* ✅ 安全 */
fgets(buffer, sizeof(buffer), stdin);  /* 指定最大长度 */
```

#### MSC32-C - 正确定义可变参数函数

```c
/* ❌ 不安全 */
void func(int count, ...) {
    va_list args;
    va_start(args, count);  /* 未指定参数名 */
}

/* ✅ 安全 */
void func(int count, ...) {
    va_list args;
    va_start(args, count);  /* 正确指定最后一个命名参数 */
    /* 使用 va_arg */
    va_end(args);
}
```

---

### 环境 (ENV)

#### ENV33-C - 不要调用 system()

```c
/* ❌ 不安全 */
system("ls " + user_input);  /* 命令注入风险 */

/* ✅ 安全 */
/* 使用 exec 系列函数，避免 shell 解释 */
execlp("ls", "ls", user_arg, NULL);
```

#### ENV32-C - 在执行敏感操作前清理环境

```c
/* ❌ 不安全 */
system("ls");  /* 环境变量可能被篡改 */

/* ✅ 安全 */
unsetenv("LD_PRELOAD");
unsetenv("LD_LIBRARY_PATH");
system("ls");
```

---

### API (API)

#### API30-C - 不要使用已被弃用或移除的函数

```c
/* ❌ 不安全 */
asctime(time_ptr);  /* 返回静态缓冲区，线程不安全 */

/* ✅ 安全 */
asctime_r(time_ptr, buf);  /* 线程安全版本 */
```

#### API34-C - 不要调用 fork() 和 exec() 之间的异步信号不安全函数

```c
/* ❌ 不安全 */
pid_t pid = fork();
if (pid == 0) {
    malloc(100);  /* fork 后调用 malloc 可能死锁 */
    execvp(cmd, argv);
}

/* ✅ 安全 */
pid_t pid = fork();
if (pid == 0) {
    /* 只调用异步信号安全函数 */
    execvp(cmd, argv);
}
```

---

## 📊 规则优先级

SEI CERT 规则分为两类：

| 类型 | 说明 | 遵守要求 |
|------|------|---------|
| **规则 (Rules)** | 规范性要求 | 代码**必须**遵守 |
| **建议 (Recommendations)** | 指导性建议 | 遵循后可提高安全性 |

---

## 🔗 相关文档

- [内存安全指南](../200-memory-safety/memory_safety.md)
- [代码风格指南](../design/Code_Style_Design_Guide.md)
- [TAOCP PDF 核心知识提炼](../reference/TAOCP_PDF 核心知识提炼.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
