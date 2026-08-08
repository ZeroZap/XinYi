# XinYi Power Management Component

**版本**: 1.0.0
**状态**: 文档已补齐 / host-guarded PM core / 功耗与板级控制待实证

---

## 📋 概述

电源管理组件 (Power Management)，负责系统电源监控、电池状态聚合、充电控制与低功耗入口的框架层逻辑。

当前 PM 是 framework/host-guarded 状态：`tests/unit/pm/test_pm_core.c` 与 `test_pm_platform_fallback.c` 已覆盖 PM lifecycle、ADC fallback、charger state、fuel-gauge wrapper、platform tick/charger hook 契约。真实低功耗、板级 charger GPIO、ADC 通道、电池曲线和整机功耗结论仍必须来自 board/project 验证，不能用 host stub 结果替代。

> Product boundary: standalone `components/fuel_gauge/` 保持独立组件线；PM 侧的 `fuel-gauge/` 目录是历史/兼容入口，不应在没有单独 proposal 的情况下把 standalone Fuel Gauge 回并 PM。

---

## 🎯 特性

- ✅ Host 可验证的 PM lifecycle、状态查询、模拟 ADC 电压/SOC 估算
- ✅ Host 可验证的 charger state machine wrapper 与平台 charger hook 记录
- ✅ Host 可验证的 PM-local fuel-gauge wrapper 基础契约
- ⚠️ 真实低功耗/睡眠/关机效果仍待 board/project 功耗实测
- ⚠️ 真实 charger GPIO、ADC channel 与电池曲线仍待板级配置和验证记录
- ⚠️ Standalone Fuel Gauge 继续独立维护，不由 PM README 宣称硬件通过

---

## 📁 文件结构

```text
pm/
├── inc/
│   └── xy_pm.h              # PM 主头文件（含 Charger API）
├── src/
│   ├── xy_pm_system.c       # 电源系统管理
│   ├── xy_pm_adc.c          # ADC 采样 / host fallback
│   ├── xy_pm_platform.c     # 平台 tick 与 charger hook
│   ├── xy_charger.c         # PM-local 充电器状态机
│   └── xy_fuel_gauge.c      # PM-local 电量估算 wrapper
├── charger/                 # PM charger public header
├── fuel-gauge/              # 历史/兼容 PM-local fuel-gauge header
├── CMakeLists.txt
└── Kconfig
```

---

## 🔧 API 概述

### 电源管理初始化

```c
#include "xy_pm.h"

xy_pm_init();
```

### 电池状态查询

```c
uint32_t voltage = xy_pm_get_battery_voltage_mV();
uint8_t percent = xy_pm_get_battery_percent();
uint8_t soc = xy_pm_get_soc();
bool charging = xy_pm_is_charging();
```

### 充电器控制

```c
#include "xy_pm.h"

xy_charger_config_t charger_config = {
    .charge_current_mA = 500,
    .charge_voltage_mV = 4200,
    .cell_count = 1,
};

xy_charger_init(&charger_config);
xy_charger_start();
xy_charger_stop();

xy_charger_state_t state;
xy_charger_get_state(&state);
```

### PM-local 电量计操作

```c
xy_fuel_gauge_config_t fg_config = {
    .design_capacity_mAh = 2000,
    .full_capacity_mAh = 2000,
    .nominal_voltage_mV = 3700,
    .cells = 1,
};
xy_fuel_gauge_init(&fg_config);
xy_fuel_gauge_update(voltage_mV, current_mA, temperature_celsius);

uint8_t soc = xy_fuel_gauge_get_soc();
uint32_t remaining = xy_fuel_gauge_get_remaining_mAh();
```

真实芯片驱动请优先使用 standalone `components/fuel_gauge/`，PM-local wrapper 只作为 PM framework 的简化状态估算入口。

### 低功耗与关机入口

```c
xy_pm_enter_sleep();
xy_pm_wakeup();
xy_pm_enter_shutdown();
```

这些 API 当前是 framework entrypoint；实际 STOP/SLEEP/SHUTDOWN、外设恢复和功耗收益需要由 board/project backend 与硬件日志证明。

---

## 🏗️ 构建说明

### CMake 构建

```cmake
add_subdirectory(components/pm)
target_link_libraries(your_target xy_pm)
```

### Kconfig 配置

```text
CONFIG_XY_PM_ENABLE=y
CONFIG_XY_CHARGER_ENABLE=y       # 启用充电器 GPIO 控制
CONFIG_XY_CHARGER_GPIO_PORT=0    # 充电器使能 GPIO 端口
CONFIG_XY_CHARGER_GPIO_PIN=0     # 充电器使能 GPIO 引脚
CONFIG_XY_FUEL_GAUGE_ENABLE=y    # 启用 PM-local 电量估算 wrapper
```

### Host 验证入口

PM 当前纳入主线 Unity/CTest：

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_pm test_pm_platform_fallback -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(pm_component|pm_platform_fallback)$'

make test-unit
```

相关但独立的 standalone charger IC coverage 是 `charger_bq25620`；standalone Fuel Gauge coverage 在 `tests/unit/fuel_gauge/*`，不等同于 PM component 硬件验证。

### 平台特定配置

| 平台 | 宏定义 | Tick 来源 |
| --- | --- | --- |
| STM32 | `STM32U5`, `STM32F4`, `STM32F1`, `STM32L4` | `HAL_GetTick()` |
| WCH | `MCU_CH32`, `CH32V103`, `CH32V20X` | 内部 tick stub |
| HC32 | `MCU_HC32`, `HC32L021`, `HC32L110` | `xy_hal_sys_get_tick_count()` |
| PC | `CONFIG_PLATFORM_PC` / `PLATFORM_PC` | `GetTickCount()` / `clock()` |

---

## 📊 状态码

| 状态码 | 描述 |
| --- | --- |
| `XY_PM_OK` | 成功 |
| `XY_PM_ERROR` | 一般错误 |
| `XY_PM_ERROR_INVALID_MODE` | 无效模式 |
| `XY_PM_ERROR_NOT_SUPPORTED` | 不支持 |
| `XY_PM_INVALID_PARAM` | 无效参数 |
| `XY_PM_NOT_INITIALIZED` | 未初始化 |

充电器状态使用 `xy_charger_status_t`：

- `XY_CHARGER_STATUS_IDLE`
- `XY_CHARGER_STATUS_PRE_CHARGE`
- `XY_CHARGER_STATUS_FAST_CHARGE`
- `XY_CHARGER_STATUS_CONSTANT_VOLTAGE`
- `XY_CHARGER_STATUS_CHARGE_COMPLETE`
- `XY_CHARGER_STATUS_FAULT`

---

## 🔌 平台接口

| 函数 | 说明 |
| --- | --- |
| `xy_pm_tick_get()` | 获取 OS/platform tick 计数 (ms) |
| `xy_pm_get_platform_name()` | 返回当前编译平台名 |
| `xy_pm_is_platform()` | 检查当前编译平台 |
| `xy_charger_hw_init()` | 初始化 charger hook / board GPIO seam |
| `xy_charger_hw_enable()` | 使能 charger hook |
| `xy_charger_hw_disable()` | 禁用 charger hook |

---

## ⚠️ 注意事项

1. 电量计需要定期调用 `xy_fuel_gauge_update()` 以保持 PM-local 状态准确；真实电量计芯片请优先使用 standalone `components/fuel_gauge/`。
2. 充电电流、目标电压、温度窗口必须根据电池规格与 charger IC 数据手册配置。
3. 当前 `xy_pm_enter_sleep()` / `xy_pm_enter_shutdown()` 是框架入口；真实省电效果需用板级功耗日志验证。
4. 充电器 GPIO 引脚、极性、ADC 通道与分压比需要根据实际硬件连接配置；host 测试只验证软件契约。
5. 不要把 `pm_component` host CTest 的通过结果写成真实低功耗或电池硬件验证通过。

---

## 📝 更新记录

### v1.0.2 (2026-08-09)

- 同步 PM 当前状态为 host-guarded / 功耗待实证
- 记录 `pm_component`、`pm_platform_fallback` focused CTest 入口
- 明确 standalone Fuel Gauge 不回并 PM 的产品边界
- 明确 host stub 结果不能替代真实 charger/ADC/低功耗硬件验证

### v1.0.1 (2026-03-13)

- 新增 `xy_pm_platform.c` 平台特定实现
- 实现 `xy_charger_hw_enable()` GPIO 控制 seam
- 实现 `xy_charger_hw_disable()` GPIO 控制 seam
- 替换 `stub_tick_get()` 为平台 tick 函数
- 添加平台检测宏 (`XY_PLATFORM_STM32`, `XY_PLATFORM_WCH`, etc.)
- 新增 Kconfig 充电器配置选项
- 更新 `xy_pm.h` 平台接口声明

### v1.0.0 (2026-03-13)

- 初始版本
- 基本电源管理功能
- 充电器状态机
- 电量计基础

---

**维护者**: XinYi Team
