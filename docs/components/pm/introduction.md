# PM 组件 - 电源管理

**状态**: ✅ 完善 | **测试**: 19 用例 | **版本**: 1.0

---

## 📖 简介

XinYi 电源管理（PM）组件提供充电器管理、电量计量等功能。

### 核心特性

- ✅ **充电管理** - 多阶段充电
- ✅ **电量计量** - SOC/SOH 估算
- ✅ **低功耗模式** - 睡眠/待机管理
- ✅ **温度监控** - 过温保护

---

## 🚀 快速开始

### 充电器示例

```c
#include "xy_charger.h"

int main(void) {
    xy_charger_config_t config = {
        .cell_count = 1,
        .charge_current_mA = 1000,
        .charge_voltage_mV = 4200,
    };
    
    xy_charger_init(&config);
    xy_charger_start();
    
    // 获取状态
    xy_charger_state_t state;
    xy_charger_get_state(&state);
    
    printf("SOC: %d%%\n", state.soc_percent);
    
    return 0;
}
```

### 电量计示例

```c
#include "xy_fuel_gauge.h"

int main(void) {
    xy_fuel_gauge_config_t config = {
        .design_capacity_mAh = 2000,
        .nominal_voltage_mV = 3700,
    };
    
    xy_fuel_gauge_init(&config);
    
    // 更新数据
    xy_fuel_gauge_update(3800, 500, 25);
    
    // 获取 SOC
    uint8_t soc = xy_fuel_gauge_get_soc();
    printf("SOC: %d%%\n", soc);
    
    return 0;
}
```

---

## 📋 API 参考

### Charger

| 函数 | 说明 |
|------|------|
| `xy_charger_init()` | 初始化充电器 |
| `xy_charger_start()` | 开始充电 |
| `xy_charger_stop()` | 停止充电 |
| `xy_charger_get_state()` | 获取状态 |

### Fuel Gauge

| 函数 | 说明 |
|------|------|
| `xy_fuel_gauge_init()` | 初始化电量计 |
| `xy_fuel_gauge_update()` | 更新数据 |
| `xy_fuel_gauge_get_soc()` | 获取 SOC |
| `xy_fuel_gauge_get_soh()` | 获取 SOH |

---

## 🧪 测试用例

PM 组件包含 19 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| Charger | 11 |
| Fuel Gauge | 8 |

运行测试：
```bash
ctest -R test_pm --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
