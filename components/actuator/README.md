# XinYi Actuator Component

**版本**: 1.0.0
**状态**: 🟡 完善中

---

## 📋 概述

执行器框架 (Actuator Framework)，统一管理舵机、继电器、直流电机、步进电机、PWM 输出等执行设备。提供标准化的设备注册、控制和状态监控接口。

---

## 🎯 特性

- ✅ 支持多种执行器类型（继电器、舵机、电机、PWM、LED、蜂鸣器等）
- ✅ 统一的设备注册和管理接口
- ✅ 标准化操作 API
- ✅ 紧急停止功能
- ✅ 批量操作支持

---

## 📁 文件结构

```
actuator/
├── xy_actuator.h    # 执行器框架头文件
├── xy_actuator.c    # 执行器框架实现
├── examples/
│   └── example_relay_servo_pwm.c # 继电器/舵机/PWM build-guarded smoke 示例
├── CMakeLists.txt
└── README.md        # 本文件
```

---

## 🔧 API 概述

### 设备注册

```c
#include "xy_actuator.h"

// 定义执行器设备
actuator_device_t my_servo = ACTUATOR_DEVICE_INIT(
    "servo_1",
    ACTUATOR_TYPE_SERVO,
    &servo_ops,
    bus_handle,
    NULL
);

// 注册设备
actuator_register(&my_servo);

// 查找设备
actuator_device_t *dev = actuator_find("servo_1");
actuator_device_t *dev = actuator_find_by_type(ACTUATOR_TYPE_SERVO);
```

### 通用操作

```c
// 初始化/反初始化
actuator_init(dev);
actuator_deinit(dev);

// 使能/禁用
actuator_enable(dev, true);

// 读取/写入值
actuator_value_t value;
value.servo.target_angle = 90.0f;
actuator_write(dev, &value);
actuator_read(dev, &value);

// 重置和急停
actuator_reset(dev);
actuator_emergency_stop(dev);

// 获取状态
actuator_status_t status = actuator_get_status(dev);
```

### 继电器操作

```c
#include "xy_actuator.h"

actuator_device_t *relay = actuator_find("relay_1");

// 继电器控制
relay_on(relay);           // 打开
relay_off(relay);          // 关闭
relay_toggle(relay);       // 翻转
relay_get(relay, &state);  // 获取状态
relay_pulse(relay, 100);   // 脉冲（100ms）
```

### 舵机操作

```c
#include "xy_actuator.h"

actuator_device_t *servo = actuator_find("servo_1");

// 初始化舵机
servo_init(servo);

// 设置角度 (-90° 到 +90°)
servo_set_angle(servo, 45.0f);

// 获取角度
float angle;
servo_get_angle(servo, &angle);

// 设置角度范围
servo_set_range(servo, -45.0f, 45.0f);

// 设置速度 (度/秒)
servo_set_speed(servo, 90);

// 往复摆动
servo_sweep(servo, -45.0f, 45.0f, 1000);  // 每步 1 秒

// 停止和回中
servo_stop(servo);
servo_center(servo);
```

### PWM 操作

```c
#include "xy_actuator.h"

actuator_device_t *pwm_dev = actuator_find("pwm_1");

// 设置占空比 (0-65535)
pwm_set_duty(pwm_dev, 32768);  // 50%

// 设置频率 (Hz)
pwm_set_frequency(pwm_dev, 1000);  // 1kHz
```

### 批量操作

```c
// 关闭所有执行器
actuator_all_off();

// 紧急停止所有
actuator_emergency_stop_all();
```

---

## 🚀 简单使用示例

```c
#include "xy_actuator.h"

// 定义舵机操作接口
static actuator_err_t my_servo_init(actuator_device_t *dev) {
    // 硬件初始化代码
    return ACTUATOR_EOK;
}

static actuator_err_t my_servo_write(actuator_device_t *dev, const actuator_value_t *value) {
    // 设置 PWM 占空比到目标角度
    uint16_t pwm = servo_angle_to_pwm(
        value->servo.target_angle,
        dev->config.servo_min_angle,
        dev->config.servo_max_angle,
        dev->config.servo_pwm_min,
        dev->config.servo_pwm_max
    );
    set_pwm_duty(dev->config.pwm_channel, pwm);
    return ACTUATOR_EOK;
}

static const actuator_ops_t servo_ops = {
    .init = my_servo_init,
    .write = my_servo_write,
    // ... 其他可选接口
};

int main(void)
{
    // 定义舵机设备
    actuator_device_t servo1 = SERVO_DEVICE_INIT(
        "servo_1",
        &servo_ops,
        1,              // PWM 通道
        -90.0f,         // 最小角度
        90.0f,          // 最大角度
        90              // 速度 (度/秒)
    );

    // 注册设备
    actuator_register(&servo1);

    // 使用舵机
    servo_device_t *servo = actuator_find("servo_1");
    servo_init(servo);
    servo_set_angle(servo, 45.0f);

    // 清理
    actuator_unregister(&servo1);

    return 0;
}
```

---

## 🏗️ 构建说明

### Kconfig 配置

Actuator 在根 `Kconfig` 中提供两个兼容符号：

```kconfig
config COMPONENT_ACTUATOR
    bool "Actuator Framework"
    default y

config XY_ACTUATOR_ENABLE
    bool "Actuator Component"
    default y
```

`COMPONENT_ACTUATOR` 是新的组件入口；`XY_ACTUATOR_ENABLE` 保留给旧构建脚本兼容。组件
`CMakeLists.txt` 同时接受生成变量 `XY_COMPONENT_ACTUATOR` 和 `XY_XY_ACTUATOR_ENABLE`，因此旧
`.config` 覆盖仍可继续工作。

### CMake 构建

```cmake
# 在您的 CMakeLists.txt 中
add_subdirectory(components/actuator)
target_link_libraries(your_target xy_actuator)
```

### 依赖

- 硬件 PWM 输出
- GPIO 引脚控制（用于继电器）
- `xy_log` 日志系统（可选）

---

## 📊 执行器类型

| 类型                          | 描述             |
| ----------------------------- | ---------------- |
| `ACTUATOR_TYPE_RELAY`         | 继电器（开关）   |
| `ACTUATOR_TYPE_SERVO`         | 舵机（角度控制） |
| `ACTUATOR_TYPE_MOTOR_DC`      | 直流电机         |
| `ACTUATOR_TYPE_MOTOR_STEPPER` | 步进电机         |
| `ACTUATOR_TYPE_SOLENOID`      | 电磁阀           |
| `ACTUATOR_TYPE_PWM`           | 通用 PWM 输出    |
| `ACTUATOR_TYPE_LED`           | LED 控制         |
| `ACTUATOR_TYPE_BUZZER`        | 蜂鸣器           |
| `ACTUATOR_TYPE_VALVE`         | 阀门             |

---

## 📊 执行器状态

| 状态                       | 描述       |
| -------------------------- | ---------- |
| `ACTUATOR_STATUS_IDLE`     | 空闲       |
| `ACTUATOR_STATUS_READY`    | 就绪       |
| `ACTUATOR_STATUS_BUSY`     | 动作执行中 |
| `ACTUATOR_STATUS_ERROR`    | 错误       |
| `ACTUATOR_STATUS_DISABLED` | 禁用       |

---

## 📊 错误码

| 错误码              | 描述       |
| ------------------- | ---------- |
| `ACTUATOR_EOK`      | 成功       |
| `ACTUATOR_ERROR`    | 一般错误   |
| `ACTUATOR_EINVAL`   | 无效参数   |
| `ACTUATOR_ENODEV`   | 设备未找到 |
| `ACTUATOR_EBUSY`    | 设备忙     |
| `ACTUATOR_ETIMEOUT` | 超时       |
| `ACTUATOR_ENOMEM`   | 内存不足   |
| `ACTUATOR_ENOSYS`   | 不支持     |
| `ACTUATOR_EIO`      | I/O 错误   |
| `ACTUATOR_ELIMIT`   | 超出限制   |
| `ACTUATOR_EHW`      | 硬件错误   |

---

## 🧪 测试

### 测试文件

```
tests/unit/actuator/
└── test_actuator_framework.c # 统一 Actuator 框架、继电器、舵机、PWM 与批处理测试

components/actuator/examples/
└── example_relay_servo_pwm.c # 示例源文件，通过 CTest 编译/运行护栏保持 API 对齐
```

旧的 `tests/actuator/*` 独立测试入口已合并到 active `tests/unit` 套件，避免与 CTest 主入口重复。

### 构建与运行测试

```bash
# 运行全部 PC 单元测试
make test-unit

# 或只运行 Actuator 目标
cmake --build build/tests/unit --target test_actuator_framework -j"$(nproc)"
ctest --test-dir build/tests/unit -R '^actuator_framework$' --output-on-failure

# 只运行 Actuator 示例 smoke 护栏
cmake --build build/tests/unit --target test_actuator_example_relay_servo_pwm -j"$(nproc)"
ctest --test-dir build/tests/unit -R '^actuator_example_relay_servo_pwm$' --output-on-failure
```

### 测试覆盖

#### test_actuator_framework.c

- Actuator 类型、状态、错误码字符串与舵机 PWM/角度换算
- 设备注册/注销（含未注册设备 `ACTUATOR_ENODEV` 防御路径）、按名称/类型查找、计数、初始化/反初始化
- 通用读写、使能/禁用、缺失 ops 错误路径
- 继电器默认操作：开/关、翻转、脉冲
- 舵机默认操作：角度设置、回中、范围与 PWM 配置
- 批量操作：批量写入、序列执行、错误参数
- 类型专用 helper 防御路径：继电器/舵机/PWM helper 拒绝错误类型并保持调用者输出与缓存状态

#### example_relay_servo_pwm.c

- 使用 public `xy_actuator.h` API 注册继电器、舵机和 PWM 设备
- 演示继电器开/关、舵机角度设置、PWM duty/frequency 与批量 all-off
- 在 `make test-unit` 中作为 host smoke CTest 编译/运行，防止 README 示例与公共 API 漂移

---

**维护者**: XinYi Team
