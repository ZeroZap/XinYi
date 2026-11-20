# Trace 组件 - 日志系统

**状态**: ✅ 完善 | **测试**: 10 用例 | **版本**: 1.0

---

## 📖 简介

XinYi Trace 组件提供统一的日志系统（xy_log）和命令处理（xy_cmd）。

### 核心特性

- ✅ **多日志级别** - VERBOSE/DEBUG/INFO/WARN/ERROR
- ✅ **动态日志级别** - 运行时调整
- ✅ **低开销** - 编译时优化
- ✅ **多后端支持** - UART/RTT/自定义

---

## 🚀 快速开始

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

int main(void) {
    int value = 42;
    const char *str = "串口已就绪";

    xy_log_init();

    xy_log_str("Direct message\n");
    xy_log_d("Debug message: %d\n", value);
    xy_log_i("Info message: %s\n", str);
    xy_log_w("Warning message\n");
    xy_log_e("Error message\n");

    return 0;
}
```

`xy_log_init()` 之后用 `xy_log_set_dynamic_level()` 打开运行时阈值；四个等级宏
`xy_log_e()` / `xy_log_w()` / `xy_log_i()` / `xy_log_d()` 由 `XY_LOG_LEVEL` 在编译期裁剪。

---

## 📋 API 参考

### 日志宏

| 宏 | 说明 |
|------|------|
| `xy_log_d()` | DEBUG 级别 |
| `xy_log_i()` | INFO 级别 |
| `xy_log_w()` | WARN 级别 |
| `xy_log_e()` | ERROR 级别 |

### 日志函数

| 函数 | 说明 |
|------|------|
| `xy_log_init()` | 初始化日志 |
| `xy_log_str()` | 输出字符串 |
| `xy_log_raw()` | 输出原始数据 |
| `xy_log_set_dynamic_level()` | 设置动态级别 |

---

## 🧪 测试用例

Trace 组件包含 10 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| 日志级别 | 2 |
| 日志函数 | 5 |
| 日志宏 | 2 |
| 其他 | 1 |

运行测试：
```bash
ctest -R test_trace --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
