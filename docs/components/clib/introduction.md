# CLib 组件 - 自定义 C 库

**状态**: ✅ 完善 | **测试**: 21 用例 | **版本**: 1.0

---

## 📖 简介

XinYi CLib 提供字符串操作、数学工具、数据结构、滤波算法等实用功能。

### 核心特性

- ✅ **字符串操作** - 安全的字符串处理
- ✅ **数学工具** - 定点数学、位操作
- ✅ **数据结构** - 列表、队列
- ✅ **滤波算法** - IIR、FIR、中值滤波
- ✅ **排序算法** - 冒泡、快速、归并等

---

## 🚀 快速开始

### 滤波算法

```c
#include "xy_filter.h"

// 中值滤波
xy_median_filter_t filter;
uint16_t buffer[5];
xy_filter_median_init(&filter, buffer, 5);

uint16_t value = xy_filter_median(&filter, adc_value);
```

### 排序算法

```c
#include "xy_sort.h"

uint16_t arr[] = {64, 34, 25, 12, 22};
xy_quick_sort(arr, 5);
```

---

## 📋 API 参考

### 滤波

| 函数 | 说明 |
|------|------|
| `xy_filter_amplitude_limiting()` | 限幅滤波 |
| `xy_filter_median()` | 中值滤波 |
| `xy_filter_recursive_average()` | 递推平均滤波 |
| `xy_filter_first_order_lag()` | 一阶滞后滤波 |

### 排序

| 函数 | 说明 |
|------|------|
| `xy_bubble_sort()` | 冒泡排序 |
| `xy_selection_sort()` | 选择排序 |
| `xy_insertion_sort()` | 插入排序 |
| `xy_quick_sort()` | 快速排序 |
| `xy_heap_sort()` | 堆排序 |

### 数学

| 函数 | 说明 |
|------|------|
| `XY_MIN()` | 最小值 |
| `XY_MAX()` | 最大值 |
| `XY_CLAMP()` | 限幅 |
| `XY_SWAP()` | 交换 |
| `XY_BIT()` | 位掩码 |

---

## 🧪 测试用例

CLib 单元测试已并入统一 PC 测试套件，覆盖 common/string/stdio/ctype/ring-buffer/filter/sort 基础契约。

运行测试：
```bash
make test-unit

# 或只运行 CLib 目标
cmake --build build/tests/unit --target test_clib -j"$(nproc)"
ctest --test-dir build/tests/unit -R '^clib_component$' --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
