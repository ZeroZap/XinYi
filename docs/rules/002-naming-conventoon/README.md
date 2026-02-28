# 命名约定 (Naming Conventions)

**状态**: ⚠️ 待完善

---

## 📋 概述

本文档详细说明 XinYi 项目的命名约定。

---

## 📝 基本规则

### 1. 文件和目录

| 类型 | 规则 | 示例 |
|------|------|------|
| 源文件 | 小写 + 下划线 | `xy_uart.c`, `xy_spi.h` |
| 头文件 | 小写 + 下划线 | `xy_uart.h`, `xy_types.h` |
| 目录 | 小写 + 下划线 | `components/`, `getting-started/` |

### 2. 函数

| 类型 | 规则 | 示例 |
|------|------|------|
| 公共函数 | 小写 + 下划线，模块前缀 | `xy_uart_init()`, `xy_spi_send()` |
| 私有函数 | `prv_` 前缀 | `prv_initialize_buffer()` |
| Getter | `module_get_xxx()` | `battery_get_voltage()` |
| Setter | `module_set_xxx()` | `battery_set_threshold()` |

### 3. 变量

| 类型 | 规则 | 示例 |
|------|------|------|
| 局部变量 | 小写 + 下划线 | `counter`, `buffer_size` |
| 全局变量 | 小写 + 下划线，模块前缀 | `g_uart_buffer`, `g_spi_ready` |
| 常量 | 小写 + 下划线，`k_` 前缀 | `k_max_buffer_size` |
| 指针 | 明确表达指向 | `buffer_ptr`, `node_list` |

### 4. 类型

| 类型 | 规则 | 示例 |
|------|------|------|
| 结构体（命名） | 小写 + 下划线 | `struct my_struct` |
| 结构体（typedef） | 小写 + 下划线，`_t` 后缀 | `my_struct_t` |
| 枚举（类型） | 小写 + 下划线，`_t` 后缀 | `my_enum_t` |
| 枚举（成员） | 全大写 + 下划线 | `MY_ENUM_VALUE_A` |
| 宏定义 | 全大写 + 下划线 | `MAX_BUFFER_SIZE` |

### 5. 宏

| 类型 | 规则 | 示例 |
|------|------|------|
| 常量宏 | 全大写 + 下划线 | `XY_VERSION_MAJOR` |
| 函数宏 | 全大写 + 下划线 | `XY_MIN(x, y)` |
| 条件编译 | 全大写 + 下划线 | `XY_HAL_UART_ENABLED` |

---

## ⚠️ 禁止事项

- ❌ 使用 `__` 或 `_` 前缀（C 语言保留）
- ❌ 混合大小写（驼峰命名）
- ❌ 无意义的缩写
- ❌ 过长的名称（>32 字符）

---

## 📚 参考

- [代码风格指南](../100-code_style/xy_code_style.md)
- [RULEBOOK](../RULEBOOK.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
