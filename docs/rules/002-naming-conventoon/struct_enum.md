# 结构体与枚举定义规范

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: TAOCP/CYouAgain/0-Ctyle

---

## 📋 概述

本文档规定 XinYi 项目中结构体和枚举的定义规范。

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

---

## 🔗 结构体与函数

### 作为参数传递

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

### 作为返回值

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

---

## 🎯 联合体与位域

### 联合体定义

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

### 位域定义

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

## 📊 结构体内存布局

### 内存对齐

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

### 结构体存储

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

## 🔗 相关文档

- [命名规范](naming.md)
- [代码风格指南](../design/Code_Style_Design_Guide.md)
- [TAOCP 学习笔记](../reference/TAOCP_编程生涯学习笔记.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
