# PID 组件 - PID 控制器

**状态**: ✅ 可用 | **测试**: host Unity/CTest 覆盖 | **版本**: 2.0.0

---

## 📖 简介

XinYi PID 组件提供基于 `float` 的比例 - 积分 - 微分控制器实现，适合在温控、充放电控制、执行器闭环等场景中作为上层控制算法使用。

### 核心特性

- ✅ **手动/自动模式**：手动模式保持当前输出，自动模式执行闭环计算。
- ✅ **输出限幅**：通过 `output_min` / `output_max` 限制控制输出。
- ✅ **积分限幅与抗饱和**：启用 anti-windup 后积分限幅跟随输出限幅，防止积分累积失控。
- ✅ **微分滤波**：可选 `0.0F..1.0F` 微分滤波系数。
- ✅ **输入校验**：公开 API 对 NULL、非法模式、NaN/Inf、无效上下限返回错误并保持已有状态。

---

## 🚀 快速开始

```c
#include "xy_pid.h"

static xy_pid_config_t make_pid_config(void)
{
    xy_pid_config_t config = {
        .kp = 2.0F,
        .ki = 0.5F,
        .kd = 0.25F,
        .output_min = 0.0F,
        .output_max = 100.0F,
        .integral_min = 0.0F,
        .integral_max = 100.0F,
        .derivative_filter = 0.1F,
    };
    return config;
}

void pid_task(void)
{
    xy_pid_t pid;
    float output = 0.0F;
    xy_pid_config_t config = make_pid_config();

    if (xy_pid_init(&pid, &config) != XY_PID_OK) {
        return;
    }

    xy_pid_set_setpoint(&pid, 100.0F);
    xy_pid_enable_anti_windup(&pid, true);
    xy_pid_enable_derivative_filter(&pid, true, 0.1F);
    xy_pid_set_mode(&pid, XY_PID_MODE_AUTO);

    while (1) {
        float measurement = get_sensor_value();
        if (xy_pid_compute(&pid, measurement, &output) == XY_PID_OK) {
            set_actuator(output);
        }
    }
}
```

> 注意：旧文档中的 `PID_FLOAT_TO_FIXED()`、`pid_fixed_t`、`xy_pid_output_t` 和多参数 `xy_pid_init()` 已不属于当前公开 API；当前头文件以 `xy_pid_config_t` + `float` 为准。

---

## 📋 API 参考

| 函数 | 说明 |
|------|------|
| `xy_pid_init()` | 使用 `xy_pid_config_t` 初始化 PID 控制器 |
| `xy_pid_reset()` | 重置运行状态、积分/微分项与首次运行标志 |
| `xy_pid_set_tuning()` | 设置 `kp` / `ki` / `kd` |
| `xy_pid_set_output_limits()` | 设置输出限幅；anti-windup 开启时同步积分限幅 |
| `xy_pid_set_setpoint()` | 设置目标值 |
| `xy_pid_set_input()` | 设置输入缓存值 |
| `xy_pid_compute()` | 根据当前 tick、设定值与输入计算输出 |
| `xy_pid_get_error()` | 获取当前误差 |
| `xy_pid_get_integral()` | 获取积分项 |
| `xy_pid_get_derivative()` | 获取微分项 |
| `xy_pid_set_mode()` / `xy_pid_get_mode()` | 设置/读取手动或自动模式 |
| `xy_pid_enable_anti_windup()` | 启用/关闭积分抗饱和 |
| `xy_pid_enable_derivative_filter()` | 启用/关闭微分滤波 |

---

## 🧪 测试与验证

PID 当前由 `tests/unit/pid/` 下的 host Unity/CTest 目标守护：

| CTest 名称 | 覆盖重点 |
|----------|----------|
| `pid_core` | 生命周期、调参、限幅、模式切换、tick wraparound、输出/状态保持 |
| `pid_auto` | 自整定初始化、采样、应用结果与错误路径 |
| `pid_example_basic` / `pid_example_auto` / `pid_example_charging` / `pid_example_log_stub` | 示例源码与当前公开 API 的编译对齐 |

常用验证命令：

```bash
cmake --build build/tests/unit --target test_pid_core test_pid_auto -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(pid_core|pid_auto)$'
make test-unit
```

---

*最后更新：2026-08-04 | 维护者：XinYi Team*
