# XinYi 代码风格规范

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: TAOCP/CYouAgain, SEI CERT, xy_code_style.md

---

## 📋 概述

本文档整合了 XinYi 项目的完整代码风格规范，包括命名、结构体、枚举、联合体、位域等所有编码约定。

---

## 🎯 基本原则

### 1. 使用小写字母 + 下划线

**✅ 正确**:
```c
int my_function(void);
int my_variable;
#define MAX_BUFFER_SIZE 256
```

**❌ 错误**:
```c
int MyFunction(void);      /* 驼峰命名 */
int myVariable;            /* 驼峰命名 */
int MY_VARIABLE;           /* 全大写用于变量 */
```

### 2. 类型使用小写 + 下划线，typedef 带 `_t` 后缀

**✅ 正确**:
```c
typedef struct {
    int x;
    int y;
} point_t;

typedef enum {
    MODE_A = 0,
    MODE_B,
} mode_t;
```

### 3. 常量大写下划线

**✅ 正确**:
```c
#define MAX_RETRY_COUNT 3
#define DEFAULT_TIMEOUT_MS 1000
```

### 4. 全局变量加 `g_` 前缀

**✅ 正确**:
```c
static int g_system_ready = 0;
static uint32_t g_config_flags;
```

---

## 📝 命名层次结构

### 命名格式

```
Part_Element_Class
```

### Part 分类

| Part | 说明 | 示例 |
|------|------|------|
| NAM | 命名 | `NAM_variable.md` |
| CMT | 注释 | `CMT_doxygen.md` |
| FMT | 格式 | `FMT_braces.md` |
| PRE | 预处理 | `PRE_macro.md` |
| DCL | 声明 | `DCL_function.md` |
| EXP | 表达式 | `EXP_condition.md` |
| INT | 整数 | `INT_overflow.md` |
| FLP | 浮点 | `FLP_precision.md` |
| ARR | 数组 | `ARR_bounds.md` |
| STR | 字符串 | `STR_null_terminate.md` |
| MEM | 内存 | `MEM_alloc.md` |
| FIO | 文件 IO | `FIO_error.md` |
| ENV | 环境 | `ENV_init.md` |
| SIG | 信号 | `SIG_handler.md` |
| ERR | 错误 | `ERR_code.md` |
| API | API | `API_version.md` |
| CON | 控制 | `CON_flow.md` |
| POS | 位置 | `POS_offset.md` |
| MSC | 杂项 | `MSC_utility.md` |

---

## 🔤 具体命名规则

### 1. 函数命名

| 函数类型 | 命名规则 | 示例 |
|---------|---------|------|
| 普通函数 | 小写 + 下划线 | `xy_uart_init()` |
| Getter | `module_get_xxx()` | `battery_get_voltage()` |
| Setter | `module_set_xxx()` | `battery_set_threshold()` |
| 私有函数 | `prv_` 前缀 | `prv_initialize_buffer()` |
| 回调函数 | `xxx_callback` | `uart_rx_callback` |

### 2. 变量命名

| 变量类型 | 命名规则 | 示例 |
|---------|---------|------|
| 局部变量 | 小写 + 下划线 | `counter`, `buffer_size` |
| 全局变量 | `g_` 前缀 | `g_uart_buffer` |
| 静态变量 | `s_` 前缀 | `s_initialized` |
| 常量 | `k_` 前缀 | `k_max_retries` |
| 指针 | 明确表达指向 | `buffer_ptr`, `node_list` |

### 3. 类型命名

| 类型 | 命名规则 | 示例 |
|------|---------|------|
| 结构体（命名） | 小写 + 下划线 | `struct my_struct` |
| 结构体（typedef） | 小写 + 下划线，`_t` 后缀 | `my_struct_t` |
| 枚举（类型） | 小写 + 下划线，`_t` 后缀 | `my_enum_t` |
| 枚举（成员） | 全大写 + 下划线 | `MY_ENUM_VALUE_A` |
| 联合体 | 小写 + 下划线，`_t` 后缀 | `data_union_t` |

### 4. 宏命名

| 宏类型 | 命名规则 | 示例 |
|--------|---------|------|
| 常量宏 | 全大写 + 下划线 | `MAX_BUFFER_SIZE` |
| 函数宏 | 全大写 + 下划线 | `XY_MIN(x, y)` |
| 条件编译 | 全大写 + 下划线 | `XY_HAL_UART_ENABLED` |
| 头文件保护 | 全大写 + 下划线 | `XY_UART_H` |

---

## 🏗️ 结构体定义

### 三种定义方式

#### 方式 1：仅命名结构体（无 typedef）

```c
struct dev_obj_xxx
{
    int member1;
    int member2;
};

/* 使用 */
struct dev_obj_xxx obj;
```

#### 方式 2：仅 typedef（匿名结构体）

```c
typedef struct {
    int member1;
    int member2;
} my_data_t;

/* 使用 */
my_data_t data;
```

#### 方式 3：命名结构体 + typedef（推荐）

```c
typedef struct dev_obj_xxx {
    int member1;
    int member2;
} dev_obj_t;

/* 使用 */
dev_obj_t obj;
```

### 推荐做法

**✅ 让调用者显著看到数据类型**:
```c
typedef struct animal_obj_pig {
    int weight;
    int age;
} pig_t;
```

**✅ 链表节点定义**:
```c
typedef struct timer_list_node {
    timer_list_info_t info;
    struct timer_list_node *next;
} timer_list_node_t;
```

### 结构体与函数

#### 作为参数传递

```c
/* 值传递 - 复制整个结构体 */
void func_by_value(struct MyStruct s)
{
    /* 修改不影响原数据 */
    s.member = 42;
}

/* 指针传递 - 传递地址，可修改 */
void func_by_pointer(struct MyStruct *s)
{
    /* 修改影响原数据 */
    s->member = 42;
}

/* const 指针传递 - 传递地址，不可修改 */
void func_by_const_pointer(const struct MyStruct *s)
{
    /* 只读访问 */
    int value = s->member;
}
```

#### 作为返回值

```c
/* 返回结构体值（小结构体） */
point_t get_origin(void)
{
    point_t origin = {0, 0};
    return origin;
}

/* 返回指针（大结构体或需要修改） */
config_t* get_config(void)
{
    static config_t config;
    return &config;
}
```

### 结构体内存布局

#### 内存对齐

```c
/* 自然对齐 */
struct natural {
    char a;      /* 1 byte + 3 bytes padding */
    int b;       /* 4 bytes */
    short c;     /* 2 bytes + 2 bytes padding */
};               /* Total: 12 bytes */

/* 打包结构体 */
struct __attribute__((packed)) packed {
    char a;      /* 1 byte */
    int b;       /* 4 bytes */
    short c;     /* 2 bytes */
};               /* Total: 7 bytes */
```

#### 结构体存储

```c
/* 栈上分配 */
void func(void) {
    my_struct_t local;  /* 在栈上 */
}

/* 堆上分配 */
my_struct_t* create_struct(void) {
    my_struct_t *obj = malloc(sizeof(my_struct_t));
    return obj;
}

/* 静态存储 */
static my_struct_t g_instance;  /* 在全局/静态区 */
```

---

## 🔖 枚举定义

### 基本定义方式

枚举可视为已知常量集合：

```c
enum XXX_MODE
{
    MODE_A = 0,
    MODE_B,
    MODE_C,
};
```

### 推荐定义方式

**✅ 带 typedef 的枚举**:
```c
typedef enum {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK,
} led_mode_t;
```

**✅ 命名枚举 + typedef**:
```c
typedef enum {
    ERR_OK = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
} error_code_t;
```

### 使用示例

```c
/* 定义 */
typedef enum {
    TAOCI_BANG = 0,
    TAOCI_PIAN,
    TAOCI_GUAN,
} taoci_type_t;

/* 使用 */
taoci_type_t type = TAOCI_BANG;
```

---

## 🔀 联合体定义

联合体成员共享同一块内存：

```c
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

---

## 🔢 位域定义

位域用于精确控制位：

```c
typedef struct {
    uint32_t flag1 : 1;      /* 1 位 */
    uint32_t flag2 : 1;      /* 1 位 */
    uint32_t mode  : 4;      /* 4 位 */
    uint32_t reserved : 26;  /* 保留 26 位 */
} control_reg_t;
```

### ⚠️ 位域使用注意事项

**多线程环境避免使用相邻位域**:

```c
/* ❌ 错误 - 数据竞争风险 */
struct multi_threaded_flags {
    unsigned int flag1 : 2;
    unsigned int flag2 : 2;  /* 可能与 flag1 共享存储单元 */
};

/* ✅ 正确 - 使用非位域成员分隔 */
struct safe_flags {
    unsigned char flag1;
    unsigned char flag2;
};
```

**原因**: 编译器可能将多个相邻位域存储在同一个存储单元中，导致多线程访问时发生数据竞争。

---

## ⚠️ 禁止事项

### 1. 禁止使用 `__` 或 `_` 前缀

```c
/* ❌ 错误 - C 语言保留 */
int __my_var;
int _my_var;

/* ✅ 正确 */
int my_var;
```

### 2. 禁止混合大小写

```c
/* ❌ 错误 */
int myVar;
int MyVar;
int MYVar;

/* ✅ 正确 */
int my_var;
```

### 3. 禁止无意义缩写

```c
/* ❌ 错误 */
int buf_sz;      /* 除非 sz 是团队共识 */
int tmp;

/* ✅ 正确 */
int buffer_size;
int temporary_value;
```

### 4. 禁止过长名称 (>32 字符)

```c
/* ❌ 错误 */
int this_is_a_very_long_variable_name_that_exceeds_32_characters;

/* ✅ 正确 */
int long_config_value;
```

---

## 📝 最佳实践

### 1. 使用 typedef 简化使用

```c
/* ✅ 推荐 */
typedef struct {
    int x;
    int y;
} point_t;

point_t p1, p2;

/* ❌ 不推荐 - 每次都要写 struct */
struct point {
    int x;
    int y;
};

struct point p1, struct point p2;
```

### 2. 结构体成员命名清晰

```c
/* ✅ 推荐 */
typedef struct {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
} uart_config_t;

/* ❌ 不推荐 */
typedef struct {
    uint32_t b;
    uint8_t d;
    uint8_t s;
} cfg_t;
```

### 3. 添加注释说明

```c
/**
 * @brief 传感器数据结构
 */
typedef struct {
    int16_t temperature;    /**< 温度 (0.01°C) */
    uint16_t humidity;      /**< 湿度 (0.01%RH) */
    uint32_t pressure;      /**< 压力 (Pa) */
    uint32_t timestamp;     /**< 时间戳 (ms) */
} sensor_data_t;
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

- [代码风格指南](../design/Code_Style_Design_Guide.md)
- [RULEBOOK](RULEBOOK.md)
- [TAOCP 学习笔记](../reference/TAOCP_编程生涯学习笔记.md)
- [SEI CERT C 编码标准](SEI_CERT_C_Coding_Standard.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
