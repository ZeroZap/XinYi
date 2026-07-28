# Fuel Gauge 迁移说明

**状态**: ✅ 主线保留 / standalone API 可用<br>
**更新日期**: 2026-07-28

> 历史版本曾把 `components/fuel_gauge` 标记为 Deprecated，并建议迁移到尚未落地的
> `drivers/power/fuel_gauge/`。当前仓库实际状态已经变化：standalone Fuel Gauge 组件
> 具备 `README.md`、`Kconfig`、`CMakeLists.txt`、核心实现、芯片驱动和 host Unity/CTest
> 覆盖，因此本文件仅保留为“历史迁移说明”，不再表示当前组件已弃用。

---

## 当前推荐用法

新代码可以直接使用 standalone Fuel Gauge 公共 API：

```c
#include "xy_fuel_gauge.h"

xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ27Z746");
if (fg != NULL && xy_fuel_gauge_init(fg) == XY_FG_OK) {
    uint8_t soc = 0;
    (void)xy_fuel_gauge_get_soc(fg, &soc);
}
```

当前已验证入口：

- `components/fuel_gauge/inc/xy_fuel_gauge.h`
- `components/fuel_gauge/core/fuel_gauge_core.c`
- `components/fuel_gauge/drivers/xy_fg_bq27z561.c`
- `components/fuel_gauge/drivers/xy_fg_bq27z746.c`
- `components/fuel_gauge/drivers/xy_fg_bq40z50.c`
- `components/fuel_gauge/drivers/xy_fg_max17043.c`

---

## Host 验证入口

Focused CTest targets:

```bash
cd build/tests/unit
ctest --output-on-failure -R '^(fuel_gauge_core|fg_bq27z746|fg_bq40z50|fg_max17043|fg_bq27z561)$'
```

完整 unit gate:

```bash
make test-unit
```

---

## 仍需注意的迁移边界

- `components/pm/fuel-gauge/` 是旧 PM 本地路径，仍可能被老文档或旧 include path 引用；不要在没有 proposal 的情况下把 standalone API 批量移动回 PM。
- 仓库目前没有可用的 `drivers/power/fuel_gauge/` 实现路径；如果未来要迁移到统一 driver tree，先写兼容 proposal，再做小步 API/目录迁移。
- 硬件侧仍需用真实 SMBus/I2C 覆盖 clock stretching、放电期 NACK/retry 和告警阈值硬件编程；host CTest 只覆盖公共契约和可模拟错误路径。

---

## 历史背景

旧文档在 2024-12-19 记录过以下方向：

- 将 charger/fuel_gauge 迁入 `drivers/power/*`。
- 由 PM 层提供更高层的电源策略 API。
- 避免 charger/fuel_gauge 在多个位置重复。

这些方向仍可作为未来架构讨论输入，但不代表当前 `components/fuel_gauge` 已弃用或应被移除。
