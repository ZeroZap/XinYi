# XinYi PID Controller - PID 控制算法

**版本**: 1.0.0  
**日期**: 2026-03-18  
**状态**: 🟡 完善中

---

## 📋 概述

PID (Proportional-Integral-Derivative) 控制器，用于闭环控制系统。

---

## 🎯 特性

- ✅ 标准 PID 算法
- ✅ 积分抗饱和
- ✅ 微分滤波
- ✅ 参数在线调整
- ✅ 位置式/增量式

---

## 🔧 API

### 初始化
```c
xy_pid_t pid;
xy_pid_init(&pid, Kp, Ki, Kd, output_min, output_max);
```

### 计算
```c
float output = xy_pid_calculate(&pid, setpoint, measurement);
```

### 参数调整
```c
xy_pid_set_params(&pid, Kp, Ki, Kd);
xy_pid_set_output_limit(&pid, min, max);
xy_pid_reset(&pid);  // 重置积分项
```

---

## 📊 完成度：85% 🟡

---

**维护者**: XinYi Team
