# PM 组件 - 电源管理

**状态**: 充电器/PM 核心与电量计分开实现 | **本页代码块**: 由 CTest `doc_samples_compile` 编译 | **版本**: 2.0

---

## 📖 简介

电源管理在本仓分成三块，彼此不共享一个头文件：

| 部分 | 头文件 | 说明 |
|------|--------|------|
| 充电器 | `components/pm/charger/xy_charger.h` | `xy_charger_*`，板级单实例 |
| PM 核心 | `components/pm/inc/xy_pm.h` | `xy_pm_*`：模式、休眠、低电、关机 |
| 电量计 | `components/fuel_gauge/inc/xy_fuel_gauge.h` | **独立组件**，驱动 own 的 `xy_fuel_gauge_t` 实例 |

三块之间没有共享状态：充电器状态里的 `soc_percent` 来自 PM 的**电压估算**
（`xy_pm_get_battery_percent()` → `xy_pm_estimate_soc_from_voltage()`，读的是 `xy_pm_adc.c` 的
电池电压通道），而电量计组件给的是驱动级 SOC 缓存。两者目前**没有接起来**——想让充电器状态显示电量计
的 SOC，需要自己搬数据。

> **Pandora 参考板没有 charger IC。** `xy_charger_hw_init()` 在没有 `XY_PM_ENABLE_TEST_HOOKS` 的构建里
> 直接返回 `XY_PM_ERROR_NOT_SUPPORTED`，于是 `xy_charger_init()` 在该板上按设计返回 `XY_CHARGER_ERROR`，
> `xy_pm_init()` 随之拒绝初始化。这是 fail-closed 的软件边界，不是缺陷；本板的 PM 证据是「边界成立」，
> 不是「充电能跑」。下面的充电器样张描述的是 API 形状，不代表该板会成功。
>
> **PM 上报的电池电压在参考板上是常量（实测）。** `xy_pm_adc.c` 只为 `STM32U5` / `STM32F4` / `STM32F1`
> 编译真实读取分支，其余平台（含 **STM32L4** 与 PC）落到通用分支——`xy_pm_adc_read_battery_voltage()`
> 直接 `return 3700`。在本机 Pandora 的 L4 归档里反汇编就是 `movw r0, #3700; bx lr`，所以
> `xy_pm_get_battery_voltage_mV()` 恒为 3700、`xy_pm_get_battery_percent()` 按电压表恒为 **40%**。
> 两个后果：这类读数**不能当电池证据**；U5/F4/F1 那支是绕过 HAL 直接调厂商 `HAL_ADC_*`
> （`extern ADC_HandleTypeDef hadc1`），与「组件只走 `xy_hal_*`」的规则不一致。

---

## 🚀 快速开始

### 充电器示例

```c
#include "xy_charger.h"
#include <stdio.h>

static void charger_bring_up(void)
{
    xy_charger_config_t config = {
        .cell_count = 1,
        .charge_current_mA = 1000,
        .charge_voltage_mV = 4200,
    };

    if (xy_charger_init(&config) != XY_CHARGER_OK) {
        return; /* 没有 charger IC 的板上会停在这里 */
    }
    if (xy_charger_start() != XY_CHARGER_OK) {
        return;
    }

    xy_charger_state_t state;
    if (xy_charger_get_state(&state) == XY_CHARGER_OK) {
        printf("status=%d soc=%u%% charging=%d\n", (int)state.status,
               (unsigned)state.soc_percent, (int)state.charging);
    }
}
```

### 电量计示例

电量计不是「填一个 config 再问全局 SOC」。真实契约是：**驱动注册一个自己拥有的
`xy_fuel_gauge_t` 实例，使用者按名字取回，`init` 之后 `fetch` 刷新缓存，再从缓存里取字段**。

> PM 内部还留着一份**同名旧头** `components/pm/fuel-gauge/xy_fuel_gauge.h`（131 行，单例式
> `xy_fuel_gauge_update(voltage, current, temp)` + 无参 `get_soc()`），由 `components/pm/src/
> xy_fuel_gauge.c` 实现，`xy_pm_system.c` 自己还在调它。两支头**共用 include guard
> `XY_FUEL_GAUGE_H`**，同一个翻译单元里只能存在一支；旧头随 PM slice 一起删除
> （`check_include_guard_collisions.py` 把它钉住，理由是「让这次删除保持可见」）。新代码用下面
> 这份独立组件，不要用 PM 里的单例。

```c
#include "xy_fuel_gauge.h"
#include "xy_fg_bq27z561.h"
#include <stdio.h>

static void report_battery(void *i2c_handle)
{
    xy_fuel_gauge_t *fg;

    /* 注册驱动实例：addr 传 0 用驱动默认地址（BQ27Z561 是 0x55）。 */
    if (xy_fuel_gauge_bq27z561_register(i2c_handle, 0) != XY_FG_OK) {
        return;
    }

    fg = xy_fuel_gauge_device_get("BQ27z561");
    if (!fg) {
        return;
    }
    if (xy_fuel_gauge_init(fg) != XY_FG_OK) {
        return;
    }
    if (xy_fuel_gauge_fetch(fg) != XY_FG_OK) {
        return;
    }

    uint8_t soc = 0;
    if (xy_fuel_gauge_get_soc(fg, &soc) == XY_FG_OK) {
        printf("soc=%u%% voltage=%u mV\n", (unsigned)soc,
               (unsigned)fg->latest.voltage_mv);
    }
}
```

`xy_fuel_gauge_get_soc()` / `_get_soh()` / `_get_voltage()` 等都是 `static inline` 的
`xy_fuel_gauge_get(fg, XY_FG_DATA_*, &val)` 包装——读失败时**不写**调用者的输出变量。

---

## 📋 API 参考

### Charger（`xy_charger.h`）

| 函数 | 说明 |
|------|------|
| `xy_charger_init(const xy_charger_config_t *)` | 初始化（先过平台 hook，失败即返回） |
| `xy_charger_start()` / `xy_charger_stop()` | 启停充电 |
| `xy_charger_get_state(xy_charger_state_t *)` | 取状态快照 |
| `xy_charger_set_current(uint32_t)` | 改充电电流 |
| `xy_charger_enable(bool)` / `xy_charger_is_charging()` | 使能/查询 |

### Fuel Gauge 核心（`xy_fuel_gauge.h`）

| 函数 | 说明 |
|------|------|
| `xy_fuel_gauge_*_register(i2c_handle, addr)` | 各驱动自己的注册入口（BQ27Z561/BQ27Z746/BQ40Z50/MAX17043） |
| `xy_fuel_gauge_device_register(xy_fuel_gauge_t *)` | 驱动侧把实例挂进注册表（使用者一般不直接调） |
| `xy_fuel_gauge_device_get(const char *name)` | 按名字取实例，未注册返回 `NULL` |
| `xy_fuel_gauge_init()` / `_deinit()` | 走 `api->init` 并维护 `initialized` |
| `xy_fuel_gauge_fetch()` | 刷新 `fg->latest` 缓存 |
| `xy_fuel_gauge_get(fg, type, &val)` | 读单个数据通道 |
| `xy_fuel_gauge_get_soc()` / `_get_soh()` | 便捷包装 |

---

## 🧪 测试

| CTest | 覆盖 |
|-------|------|
| `pm_component` | PM 核心状态机 |
| `pm_platform_fallback` | 平台 hook 缺失时的 fallback |
| `charger_bq25620` | 充电器驱动（BQ25620 芯片级） |
| `charger_header_coexistence` | 两份 `xy_charger.h` 的共存边界 |
| `fuel_gauge_core` | 电量计核心：注册/遍历/缓存/失败保留 |
| `fg_bq27z561` / `fg_bq27z746` / `fg_bq40z50` / `fg_max17043` | 四支驱动的寄存器级契约 |

```bash
ctest -R 'pm_component|fg_bq27z561' --output-on-failure
```

> 本页此前写的「19 用例 / `ctest -R test_pm`」是拼出来的：没有 `test_pm` 这个目标，
> 现在这张表里的名字都是 `ctest -N` 里真的存在的。

---

*最后更新：2026-11-03 | 维护者：XinYi Team*
