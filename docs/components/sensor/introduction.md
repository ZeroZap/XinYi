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
#include "sensor_core.h"

int main(void) {
    sensor_device_t sensor;
    
    // 初始化传感器
    xy_sensor_init(&sensor, SENSOR_TYPE_TEMPERATURE);
    
    // 读取数据
    sensor_data_t data;
    xy_sensor_read(&sensor, &data);
    
    printf("Temperature: %.2f°C\n", data.value.val_float);
    
    return 0;
}
```

---

## 📋 API 参考

| 函数 | 说明 |
|------|------|
| `xy_sensor_init()` | 初始化传感器 |
| `xy_sensor_read()` | 读取传感器数据 |
| `xy_sensor_calibrate()` | 校准传感器 |
| `xy_sensor_set_odr()` | 设置输出数据率 |

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
