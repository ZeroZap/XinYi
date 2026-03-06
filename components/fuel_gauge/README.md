# Fuel Gauge 电量计组件

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📋 概述

Fuel Gauge 电量计组件提供电池电量监测、安全认证、状态查询和安全保护功能。

### 特性

- ✅ 支持多种电量计芯片 (MAX17043, BQ27z561, BQ27Z746, BQ40Z50)
- ✅ 支持单节和多节电池 (1-4 节串联)
- ✅ 高精度电量计 (Impedance Track™ 技术)
- ✅ Security 安全认证 (SHA256/AES)
- ✅ Status 状态查询 (充电状态/健康状态)
- ✅ Safety 安全保护 (过压/欠压/过流/过温)

---

## 📁 目录结构

```
fuel_gauge/
├── inc/
│   ├── xy_fuel_gauge.h           # 统一 API
│   ├── xy_fuel_gauge_security.h  # 安全认证接口
│   ├── xy_fuel_gauge_status.h    # 状态查询接口
│   └── xy_fuel_gauge_safety.h    # 安全保护接口
├── core/
│   ├── fuel_gauge_core.c         # 核心实现
│   ├── xy_fuel_gauge_security.c  # 安全认证实现
│   ├── xy_fuel_gauge_status.c    # 状态查询实现
│   └── xy_fuel_gauge_safety.c    # 安全保护实现
├── drivers/
│   ├── xy_fg_max17043.c          # MAX17043 驱动
│   ├── xy_fg_bq27z561.c          # BQ27z561 驱动
│   ├── xy_fg_bq27z746.c          # BQ27Z746 驱动
│   └── xy_fg_bq40z50.c           # BQ40Z50 驱动
└── README.md                     # 本文档
```

---

## 🚀 快速开始

### 1. 初始化电量计

```c
#include "xy_fuel_gauge.h"

/* 注册电量计驱动 */
xy_fuel_gauge_max17043_register(i2c_handle, 0x36);

/* 获取设备 */
xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("MAX17043");

/* 初始化 */
xy_fuel_gauge_init(fg);
```

### 2. 读取电量

```c
/* 读取 SOC */
uint8_t soc;
xy_fuel_gauge_get_soc(fg, &soc);
xy_log_i("Battery SOC: %d%%\n", soc);

/* 读取电压 */
uint16_t voltage;
xy_fuel_gauge_get_voltage(fg, &voltage);
xy_log_i("Battery Voltage: %dmV\n", voltage);

/* 读取电流 */
int16_t current;
xy_fuel_gauge_get_current(fg, &current);
xy_log_i("Battery Current: %dmA\n", current);
```

### 3. 安全认证

```c
#include "xy_fuel_gauge_security.h"

/* 配置安全认证 */
xy_fg_security_config_t sec_cfg = {
    .type = XY_FG_SECURITY_SHA256,
    .key = auth_key,
    .key_len = 32,
};
xy_fuel_gauge_security_config(fg, &sec_cfg);

/* 执行认证 */
xy_fg_auth_result_t auth = xy_fuel_gauge_authenticate(fg);
if (auth == XY_FG_AUTH_OK) {
    xy_log_i("Authentication successful\n");
}

/* 验证设备真伪 */
if (xy_fuel_gauge_verify_device(fg)) {
    xy_log_i("Genuine device\n");
}
```

### 4. 状态查询

```c
#include "xy_fuel_gauge_status.h"

/* 获取充电状态 */
xy_fg_charging_state_t chg_state = xy_fuel_gauge_get_charging_state(fg);
xy_log_i("Charging state: %s\n", 
         xy_fuel_gauge_state_to_string(chg_state));

/* 获取电池健康状态 */
xy_fg_battery_health_t health;
xy_fuel_gauge_get_battery_health(fg, &health);
xy_log_i("SOH: %d%%, Cycles: %d\n", health.soh_percent, health.cycle_count);

/* 快速状态检查 */
if (xy_fuel_gauge_is_charging(fg)) {
    xy_log_i("Battery is charging\n");
}

if (xy_fuel_gauge_is_full(fg)) {
    xy_log_i("Battery is full\n");
}
```

### 5. 安全检查

```c
#include "xy_fuel_gauge_safety.h"

/* 获取安全状态 */
xy_fg_safety_status_t safety = xy_fuel_gauge_get_safety_status(fg);
if (!xy_fuel_gauge_is_safe(fg)) {
    xy_log_e("Safety issue: %s\n", 
             xy_fuel_gauge_safety_status_to_string(safety));
}

/* 获取警告状态 */
xy_fg_warning_status_t warning = xy_fuel_gauge_get_warning_status(fg);
if (xy_fuel_gauge_has_warning(fg)) {
    xy_log_w("Warning detected\n");
}

/* 获取故障状态 */
xy_fg_fault_status_t fault = xy_fuel_gauge_get_fault_status(fg);
if (xy_fuel_gauge_has_fault(fg)) {
    xy_log_e("Fault detected\n");
    xy_fuel_gauge_clear_error(fg);
}
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
| `xy_fuel_gauge_get_soc(fg, &soc)` | 读取电量百分比 |
| `xy_fuel_gauge_get_voltage(fg, &voltage)` | 读取电压 |
| `xy_fuel_gauge_get_current(fg, &current)` | 读取电流 |

### Security API

| 函数 | 说明 |
|------|------|
| `xy_fuel_gauge_security_config(fg, cfg)` | 配置安全认证 |
| `xy_fuel_gauge_authenticate(fg)` | 执行安全认证 |
| `xy_fuel_gauge_verify_device(fg)` | 验证设备真伪 |
| `xy_fuel_gauge_encrypt_data(fg, ...)` | 加密数据 |
| `xy_fuel_gauge_decrypt_data(fg, ...)` | 解密数据 |

### Status API

| 函数 | 说明 |
|------|------|
| `xy_fuel_gauge_get_charging_state(fg)` | 获取充电状态 |
| `xy_fuel_gauge_get_charging_mode(fg)` | 获取充电模式 |
| `xy_fuel_gauge_get_battery_health(fg, &health)` | 获取健康状态 |
| `xy_fuel_gauge_is_charging(fg)` | 检查是否充电 |
| `xy_fuel_gauge_is_full(fg)` | 检查是否充满 |
| `xy_fuel_gauge_is_protected(fg)` | 检查保护状态 |

### Safety API

| 函数 | 说明 |
|------|------|
| `xy_fuel_gauge_get_safety_status(fg)` | 获取安全状态 |
| `xy_fuel_gauge_get_warning_status(fg)` | 获取警告状态 |
| `xy_fuel_gauge_get_fault_status(fg)` | 获取故障状态 |
| `xy_fuel_gauge_config_safety_thresholds(fg, th)` | 配置安全阈值 |
| `xy_fuel_gauge_is_safe(fg)` | 检查是否安全 |
| `xy_fuel_gauge_has_warning(fg)` | 检查警告 |
| `xy_fuel_gauge_has_fault(fg)` | 检查故障 |

---

## 🔋 支持的芯片

### 单节电池 (1S)

| 芯片 | 厂商 | 特性 | I2C 地址 |
|------|------|------|---------|
| **MAX17043** | Maxim | 小型低功耗 | 0x36 |
| **BQ27z561** | TI | 高精度 | 0x55 |
| **BQ27Z746** | TI | Impedance Track | 0x55 |

### 多节电池 (2-4S)

| 芯片 | 串联电池 | 特性 | I2C 地址 |
|------|---------|------|---------|
| **BQ40Z50** | 2-4 节 | 集成保护 | 0x0B |

---

## 📊 数据类型

### 电量计数据

```c
typedef struct {
    uint16_t voltage_mv;          /* 电池电压 (mV) */
    int16_t  current_ma;          /* 电流 (mA, 正=充电) */
    uint8_t  soc;                 /* 电量百分比 (0-100%) */
    uint8_t  soh;                 /* 健康度 (0-100%) */
    int16_t  temperature_c;       /* 温度 (0.1°C) */
    uint32_t cycle_count;         /* 循环次数 */
    uint16_t full_capacity_mah;   /* 满充容量 (mAh) */
    uint16_t remain_capacity_mah; /* 剩余容量 (mAh) */
    uint32_t timestamp;           /* 时间戳 */
} xy_fuel_gauge_data_t;
```

### 电池健康状态

```c
typedef struct {
    uint8_t soh_percent;          /* 健康度百分比 */
    uint8_t cycle_count;          /* 循环次数 */
    uint16_t design_capacity;     /* 设计容量 (mAh) */
    uint16_t full_charge_capacity;/* 满充容量 (mAh) */
    uint16_t remaining_capacity;  /* 剩余容量 (mAh) */
    uint8_t temperature;          /* 温度 (°C) */
    uint8_t age_days;             /* 使用天数 */
} xy_fg_battery_health_t;
```

---

## 🛡️ 安全保护

### 安全状态 (SAS)

| 标志 | 说明 | 默认阈值 |
|------|------|---------|
| **Cell OVP** | 单体过压 | 4.25V |
| **Cell UVP** | 单体欠压 | 2.8V |
| **Pack OVP** | 电池组过压 | 17V (4S) |
| **Pack UVP** | 电池组欠压 | 11.2V (4S) |
| **Chg OCP** | 充电过流 | 5A |
| **Dischg OCP** | 放电过流 | 10A |
| **Chg OCD** | 充电短路 | 15A |
| **Dischg OCD** | 放电短路 | 20A |
| **Cell OTP** | 单体过温 | 60°C |
| **Cell UTP** | 单体低温 | 0°C |
| **Pack OTC** | 充电过温 | 55°C |
| **Pack OTD** | 放电过温 | 60°C |
| **Pack UTP** | 电池组低温 | -10°C |

### 警告状态

| 标志 | 说明 |
|------|------|
| **Cell High/Low** | 单体电压高/低 |
| **Pack High/Low** | 电池组电压高/低 |
| **Chg/Dischg High** | 充电/放电电流高 |
| **Temp High/Low** | 温度高/低 |
| **SOC High/Low** | SOC 高/低 |
| **SOH Low** | SOH 低 |
| **Imbalance High** | 电芯不平衡 |

---

## ⚠️ 注意事项

### 1. I2C 通信

- 确保 I2C 总线速度适中 (100kHz 推荐)
- 添加上拉电阻 (4.7kΩ 推荐)
- 保持走线短，减少干扰

### 2. 安全认证

- 密钥需要安全存储
- 认证失败时限制电池使用
- 定期重新认证

### 3. 阈值配置

- 根据电池规格配置阈值
- 考虑温度补偿
- 定期校准

### 4. 多节电池

- 确保单体电压平衡
- 监控最大压差
- 启用平衡功能

---

## 📚 相关文档

- [Zephyr fuel_gauge](https://github.com/zephyrproject-rtos/zephyr/tree/main/drivers/fuel_gauge)
- [TI BQ40Z50 TRM](https://www.ti.com/product/BQ40Z50)
- [MAX17043 Datasheet](https://www.maximintegrated.com/en/products/power/battery-management/MAX17043.html)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
