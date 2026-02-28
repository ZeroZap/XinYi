# 命名规范 (Naming Conventions)

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: TAOCP/CYouAgain/0-Ctyle

---

## 📋 概述

本文档规定 XinYi 项目的命名规范，所有代码贡献必须遵循此规范。

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

### 2. 类型使用驼峰命名

**✅ 正确**:
```c
typedef struct {
    int x;
    int y;
} Point_t;

typedef enum {
    MODE_A = 0,
    MODE_B,
} Mode_t;
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

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
