# Sensor 组件 - 传感器框架

**状态**: ✅ 完善 | **测试**: 18 用例 | **版本**: 1.0

---

## 📖 简介

XinYi 传感器框架提供统一的传感器接口，支持多种传感器类型。

### 核心特性

- ✅ **统一接口** - 标准化传感器 API
- ✅ **多传感器支持** - 温度/湿度/压力/光照等
- ✅ **校准支持** - 自动/手动校准
- ✅ **低功耗** - 睡眠模式管理
- ✅ **中断支持** - 阈值中断

### 支持的传感器

| 类型 | 传感器 | 状态 |
|------|--------|------|
| 温度 | ADT7420, TMP36 | ✅ |
| 运动 | 加速度计，陀螺仪 | ✅ |
| 光照 | APDS9960 | ✅ |
| 压力 | BMP280 | ✅ |
| 湿度 | SHT30 | ✅ |

---

## 🚀 快速开始

```c
#include "xy_sensor.h"
#include "xy_log.h"

void read_temperature(void) {
    xy_sensor_device_t *dev = xy_sensor_device_get("sht30");
    xy_sensor_value_t value;

    if (dev == NULL) {
        return;
    }

    // 初始化传感器
    xy_sensor_init(dev);

    // 采集并读取温度通道；定点值 = val1 + val2 / 1000000
    if (xy_sensor_sample_fetch(dev, XY_SENSOR_CHAN_AMBIENT_TEMP) == XY_SENSOR_OK &&
        xy_sensor_channel_get(dev, XY_SENSOR_CHAN_AMBIENT_TEMP, &value) == XY_SENSOR_OK) {
        xy_log_i("Temperature: %ld.%06ld C\n", (long)value.val1, (long)value.val2);
    }
}
```

---

## 📋 API 参考

| 函数 | 说明 |
|------|------|
| `xy_sensor_init()` | 初始化传感器 |
| `xy_sensor_sample_fetch()` | 采集一次数据（通道或 `XY_SENSOR_CHAN_ALL`） |
| `xy_sensor_channel_get()` | 读取通道值（`xy_sensor_value_t` 定点数） |
| `xy_sensor_attr_set()` | 设置通道采样属性（ODR、量程等） |

校准、滤波、FIFO 与中断等扩展契约在 legacy `sensor_*()` 家族（`sensor_core.h`，例如 `sensor_calibrate()`、`sensor_fifo_read()`）；`xy_sensor.h` 是面向应用的统一门面，驱动通过 `xy_sensor_device.h` 的 ops 注册。

---

## 🧪 测试用例

Sensor 组件包含 18 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| 传感器类型 | 4 |
| 错误码 | 1 |
| 传感器信息 | 2 |
| 设备 | 2 |
| 配置 | 2 |
| 条件功能 | 7 |

运行测试：
```bash
ctest -R test_sensor --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
