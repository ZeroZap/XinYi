# ⚠️ Historical deprecation notice / 历史弃用说明

**原记录日期**: 2024-12-19
**事实校准日期**: 2026-08-29
**当前结论**: 本文件是 `historical notice`，原迁移目标并未落地

---

## 当前 ownership 决策

- `components/drivers/power/charger/` 当前不存在，不能作为可用替代实现。
- BQ25620 的 canonical implementation owner 仍是
  `components/charger/src/xy_bq25620.c`，并由 focused Host CTest 守护。
- 在真实替代 owner、兼容 API 和迁移测试落地前，现有调用方**不得迁移**到不存在的路径。
- 当前组件按 `legacy-maintained` 维护；不因 Host 测试升级硬件、安全或产品声明。

### 可用路径

```
components/charger/       # BQ25620 standalone owner（legacy-maintained）
components/fuel_gauge/    # standalone Fuel Gauge owner；保持独立
components/pm/            # 电源策略层；不是 BQ25620 驱动替代品
```

### 当前代码
```c
#include "xy_bq25620.h"

xy_bq25620_t charger;
int ret = xy_bq25620_init(&charger, i2c_handle, 0x6A);
```

后续若建立 `components/drivers/power/charger/`，必须先定义唯一 owner、兼容层、
Kconfig/CMake 接入和 focused migration contract，再更新本说明；不得只创建空目录或文档即宣称迁移完成。

---

## 需要帮助？

当前事实与证据请参考：
- [Charger README](README.md)
- [组件证据台账](../../docs/validation/component-evidence-matrix.md)
- [Sprint 跟踪看板](../../docs/plans/SPRINT_TRACKER.md)

---

**最后更新**: 2026-08-29
