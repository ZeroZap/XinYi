# Fuel Gauge 组件说明

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📋 概述

Fuel Gauge (电量计) 组件提供电池管理功能，包括电压/电流监测、电量百分比 (SOC)、健康度 (SOH) 等。

### 参考

- [Zephyr fuel_gauge drivers](https://github.com/zephyrproject-rtos/zephyr/tree/main/drivers/fuel_gauge)

---

## 🎯 为什么独立？

### 电量计 vs 传感器

| 特性 | 传感器 | 电量计 |
|------|--------|--------|
| **数据类型** | 环境数据 | 电池数据 |
| **采样频率** | 秒级 | 毫秒级 |
| **精度要求** | 中等 | 高精度 |
| **安全关键** | 低 | 高 (电池安全) |
| **校准需求** | 简单 | 复杂 (库仑计数) |
| **电源管理** | 被动 | 主动 (充放电控制) |

### Zephyr 设计

在 Zephyr 中，fuel_gauge 与 sensor 是平级组件：
```
zephyr/drivers/
├── sensor/          # 传感器
├── fuel_gauge/      # 电量计 (独立!)
├── adc/             # ADC
└── ...
```

---

## 📁 目录结构

```
fuel_gauge/
├── inc/
│   └── xy_fuel_gauge.h      # 统一 API
├── core/
│   └── fuel_gauge_core.c    # 核心实现
└── drivers/
    ├── xy_fg_max17043.c     # MAX17043 驱动
    ├── xy_fg_bq27z561.c     # BQ27 系列驱动
    └── ...
```

---

## 🚀 快速开始

### 1. 初始化电量计

```c
#include "xy_fuel_gauge.h"

/* 注册电量计 */
xy_fuel_gauge_max17043_register(i2c_handle, 0x36);

/* 获取设备 */
xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("MAX17043");

/* 初始化 */
xy_fuel_gauge_init(fg);
```

### 2. 读取电池数据

```c
/* 读取电压 */
uint16_t voltage;
xy_fuel_gauge_get_voltage(fg, &voltage);
xy_log_i("Voltage: %d mV\n", voltage);

/* 读取电流 */
int16_t current;
xy_fuel_gauge_get_current(fg, &current);
xy_log_i("Current: %d mA\n", current);

/* 读取电量百分比 */
uint8_t soc;
xy_fuel_gauge_get_soc(fg, &soc);
xy_log_i("SOC: %d%%\n", soc);

/* 读取健康度 */
uint8_t soh;
xy_fuel_gauge_get_soh(fg, &soh);
xy_log_i("SOH: %d%%\n", soh);
```

### 3. 设置告警

```c
xy_fuel_gauge_alert_t alert = {
    .low_soc_threshold = 20,      /* 低电量 20% */
    .high_soc_threshold = 90,     /* 高电量 90% */
    .low_voltage_mv = 3300,       /* 低电压 3.3V */
    .high_voltage_mv = 4200,      /* 高电压 4.2V */
    .over_current_ma = 2000,      /* 过流 2A */
    .over_temp_c = 600,           /* 过温 60°C */
};

xy_fuel_gauge_set_alert(fg, &alert);
```

---

## 📖 API 参考

### 核心 API

| 函数 | 说明 |
|------|------|
| `xy_fuel_gauge_init(fg)` | 初始化电量计 |
| `xy_fuel_gauge_deinit(fg)` | 反初始化 |
| `xy_fuel_gauge_fetch(fg)` | 获取最新数据 |
| `xy_fuel_gauge_get(fg, type, val)` | 读取指定数据 |

### 便捷 API

| 函数 | 说明 |
|------|------|
| `xy_fuel_gauge_get_voltage(fg, &voltage)` | 读取电压 (mV) |
| `xy_fuel_gauge_get_current(fg, &current)` | 读取电流 (mA) |
| `xy_fuel_gauge_get_soc(fg, &soc)` | 读取电量百分比 |
| `xy_fuel_gauge_get_soh(fg, &soh)` | 读取健康度 |
| `xy_fuel_gauge_get_temperature(fg, &temp)` | 读取温度 |

### 数据类型

| 类型 | 说明 | 单位 |
|------|------|------|
| `XY_FG_DATA_VOLTAGE` | 电池电压 | mV |
| `XY_FG_DATA_CURRENT` | 电流 | mA (正=充电，负=放电) |
| `XY_FG_DATA_SOC` | 电量百分比 | % (0-100) |
| `XY_FG_DATA_SOH` | 健康度 | % (0-100) |
| `XY_FG_DATA_TEMPERATURE` | 温度 | 0.1°C |
| `XY_FG_DATA_CYCLE_COUNT` | 循环次数 | 次 |

---

## 🔋 支持的芯片

| 芯片 | 厂商 | 状态 |
|------|------|------|
| **MAX17043** | Maxim | ✅ |
| **MAX17048** | Maxim | ⏳ |
| **BQ27z561** | TI | ⏳ |
| **BQ28z610** | TI | ⏳ |
| **BQ40z50** | TI | ⏳ |
| **SBS Gauge** | SBS | ⏳ |

---

## ⚠️ 注意事项

### 安全考虑

1. **电池安全** - 电量计涉及电池安全，需正确配置告警
2. **精度校准** - 首次使用需校准容量参数
3. **温度补偿** - 低温环境影响测量精度

### 使用建议

1. **定期校准** - 建议每月进行一次完整充放电校准
2. **告警配置** - 配置低电量/过流/过温告警
3. **数据存储** - 保存循环次数和 SOH 数据到 NVM

---

## 📚 相关文档

- [COMPONENT_ARCHITECTURE.md](../COMPONENT_ARCHITECTURE.md) - 组件架构
- [Zephyr fuel_gauge](https://github.com/zephyrproject-rtos/zephyr/tree/main/drivers/fuel_gauge) - 参考设计

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
