# PID 组件 - PID 控制器

**状态**: ✅ 完善 | **测试**: 20 用例 | **版本**: 1.0

---

## 📖 简介

XinYi PID 组件提供比例 - 积分 - 微分（PID）控制器实现。

### 核心特性

- ✅ **定点/浮点支持** - 可选数据类型
- ✅ **抗积分饱和** - 防止积分累积
- ✅ **输出限幅** - 限制输出范围
- ✅ **积分限幅** - 限制积分累积

---

## 🚀 快速开始

```c
#include "xy_pid.h"

int main(void) {
    xy_pid_t pid;
    xy_pid_output_t output;
    
    // 初始化 PID (Kp=2.0, Ki=0.5, Kd=1.0)
    xy_pid_init(&pid, 
        PID_FLOAT_TO_FIXED(2.0),
        PID_FLOAT_TO_FIXED(0.5),
        PID_FLOAT_TO_FIXED(1.0),
        100  // 100ms 采样时间
    );
    
    xy_pid_set_setpoint(&pid, PID_FLOAT_TO_FIXED(100));  // 目标值
    
    // 控制循环
    while (1) {
        pid_fixed_t measurement = get_sensor_value();
        xy_pid_compute(&pid, measurement, &output);
        set_actuator(output.output);
    }
    
    return 0;
}
```

---

## 📋 API 参考

| 函数 | 说明 |
|------|------|
| `xy_pid_init()` | 初始化 PID |
| `xy_pid_set_gains()` | 设置增益 |
| `xy_pid_set_setpoint()` | 设置设定点 |
| `xy_pid_compute()` | PID 计算 |
| `xy_pid_reset()` | 重置 PID |

---

## 🧪 测试用例

PID 组件包含 20 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| 初始化 | 4 |
| 增益/设定点 | 4 |
| PID 计算 | 5 |
| 重置/抗饱和 | 4 |
| 状态/饱和 | 3 |

运行测试：
```bash
ctest -R test_pid --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
