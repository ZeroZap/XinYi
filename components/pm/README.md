# XinYi Power Management Component

**版本**: 1.0.0
**状态**: 🟡 完善中

---

## 📋 概述

电源管理组件 (Power Management)，负责系统电源监控、电池管理、充电控制等功能。支持电量计 (Fuel Gauge) 和充电器 (Charger) 管理。

---

## 🎯 特性

- ✅ 电池状态监控（电压、电流、温度、SOC、SOH）
- ✅ 充电控制与管理
- ✅ 电量计功能
- ✅ 低功耗模式支持
- ✅ 系统电源状态管理

---

## 📁 文件结构

```
pm/
├── inc/
│   └── xy_pm.h              # PM 主头文件（含 Charger API）
├── src/
│   ├── xy_pm_system.c      # 电源系统管理
│   ├── xy_pm_adc.c         # ADC 采样
│   ├── xy_charger.c        # 充电器实现
│   └── xy_fuel_gauge.c     # 电量计实现
├── charger/                 # 充电器子模块
├── fuel-gauge/              # 电量计子模块
├── CMakeLists.txt
└── Kconfig
```

---

## 🔧 API 概述

### 电源管理初始化

```c
#include "xy_pm.h"

// 系统 PM 初始化
xy_pm_init();

// 设置电源模式
xy_pm_set_mode(XY_PM_MODE_ACTIVE);  // 或 XY_PM_MODE_SLEEP
```

### 电池状态查询

```c
// 获取电池电压 (mV)
uint32_t voltage = xy_pm_get_battery_voltage_mV();

// 获取电池电量百分比 (0-100%)
uint8_t soc = xy_pm_get_battery_percent();

// 获取 SOC (State of Charge)
uint8_t soc = xy_pm_get_soc();

// 检查是否在充电
bool charging = xy_pm_is_charging();
```

### 充电器控制

```c
#include "xy_pm.h"

xy_charger_config_t charger_config = {
    .charge_current_ma = 500,
    .target_voltage_mv = 4200,
    // ... 其他配置
};

// 初始化充电器
xy_charger_init(&charger_config);

// 开始充电
xy_charger_start();

// 停止充电
xy_charger_stop();

// 获取充电状态
xy_charger_state_t state;
xy_charger_get_status(&state);

// 检查是否正在充电
if (xy_charger_is_charging()) {
    // 正在充电
}
```

### 电量计操作

```c
#include "xy_pm.h"

// 初始化电量计
xy_fuel_gauge_config_t fg_config = {
    .design_capacity_mAh = 2000,
    .full_capacity_mAh = 2000,
    .nominal_voltage_mV = 3700,
    .cells = 1
};
xy_fuel_gauge_init(&fg_config);

// 更新电量计数据
xy_fuel_gauge_update(voltage_mV, current_mA, temperature_celsius);

// 获取 SOC
uint8_t soc = xy_fuel_gauge_get_soc();

// 获取剩余容量 (mAh)
uint32_t remaining = xy_fuel_gauge_get_remaining_mAh();

// 获取预计剩余时间（分钟）
uint32_t time_to_empty = xy_fuel_gauge_get_time_to_empty();

// 获取预计充满时间（分钟）
uint32_t time_to_full = xy_fuel_gauge_get_time_to_full();
```

### 低功耗与关机

```c
// 进入低功耗模式
xy_pm_set_low_power_mode(true);

// 关机
xy_pm_shutdown();
```

---

## 🚀 简单使用示例

```c
#include "xy_pm.h"

int main(void)
{
    // 初始化电源管理
    xy_pm_init();

    // 初始化充电器
    xy_charger_config_t charger_cfg = {
        .charge_current_ma = 1000,
        .target_voltage_mv = 4200,
    };
    xy_charger_init(&charger_cfg);
    xy_charger_start();

    // 初始化电量计
    xy_fuel_gauge_config_t fg_cfg = {
        .design_capacity_mAh = 3000,
        .full_capacity_mAh = 3000,
        .nominal_voltage_mV = 3700,
    };
    xy_fuel_gauge_init(&fg_cfg);

    // 主循环
    while (1) {
        uint32_t voltage = xy_pm_get_battery_voltage_mV();
        uint8_t soc = xy_pm_get_battery_percent();

        printf("Battery: %umV, %u%%\n", voltage, soc);

        // 更新电量计
        int32_t current = read_battery_current();
        int32_t temp = read_battery_temperature();
        xy_fuel_gauge_update(voltage, current, temp);

        delay_ms(1000);
    }

    return 0;
}
```

---

## 🏗️ 构建说明

### CMake 构建

```cmake
# 在您的 CMakeLists.txt 中
add_subdirectory(components/pm)
target_link_libraries(your_target xy_pm)
```

### Kconfig 配置

```
CONFIG_XY_PM=y
CONFIG_XY_CHARGER=y
CONFIG_XY_FUEL_GAUGE=y
```

### 依赖

- 硬件 ADC（电池电压、电流采样）
- 外部充电器 IC 驱动（如 LTC2944、LTC2945 等）

---

## 📊 状态码

| 状态码 | 描述 |
|--------|------|
| XY_PM_OK | 成功 |
| XY_PM_ERROR | 一般错误 |
| XY_PM_ERROR_INVALID_MODE | 无效模式 |
| XY_PM_ERROR_NOT_SUPPORTED | 不支持 |
| XY_PM_INVALID_PARAM | 无效参数 |
| XY_PM_NOT_INITIALIZED | 未初始化 |

充电器状态 (`xy_charger_state_t`)：
- `XY_CHARGER_STATE_IDLE` - 空闲
- `XY_CHARGER_STATE_CHARGING` - 充电中
- `XY_CHARGER_STATE_COMPLETE` - 充电完成
- `XY_CHARGER_STATE_FAULT` - 故障

---

## ⚠️ 注意事项

1. 电量计需要定期调用 `xy_fuel_gauge_update()` 以保持数据准确
2. 充电电流应根据电池规格合理配置
3. 低功耗模式需要在不用时及时启用以节省电量

---

**维护者**: XinYi Team
