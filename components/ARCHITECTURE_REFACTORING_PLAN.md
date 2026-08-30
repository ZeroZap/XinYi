# XinYi 组件架构重组历史提案

**原始日期**：2024-12-19
**校准日期**：2026-08-30
**状态**：未执行历史提案（superseded；不得作为迁移指南）

> 本文原先提出一次性重排 `components/driver/`、`components/device/` 与
> `components/drivers/`，其中包含创建 `components/drivers/power/charger/`、迁移 Charger/Fuel
> Gauge 以及直接执行 `mv`/`rm` 的命令。该方案未按文中步骤执行，路径和 ownership 已与当前
> root Kconfig/CMake、组件证据台账及 Sprint 决策不符。**不得执行本文旧命令**，也不得根据本
> 历史提案新建平行 owner。

## 当前事实源

当前实现、构建选择与证据边界以以下文件为准：

- `docs/plans/SPRINT_TRACKER.md`
- `docs/validation/component-evidence-matrix.md`
- `components/README.md`
- root `Kconfig` 与各组件 `CMakeLists.txt`

历史提案不覆盖这些事实源。

## 已冻结的 ownership

### Charger

- BQ25620 canonical owner：`components/charger/src/xy_bq25620.c`。
- 当前状态为 `legacy-maintained`，由 root Kconfig/CMake 显式选择。
- `components/drivers/power/charger/` 当前不存在，不是迁移目标。
- 在产品需求、替代 owner 与迁移验证闭环前，不新增平行 Charger 实现。

### Fuel Gauge

- Fuel Gauge 保持 standalone：`components/fuel_gauge/`。
- 不回并 PM，也不迁入不存在的 power-driver owner。
- Host contract 不构成 SMBus 时序、clock stretching、放电期 NACK/retry、安全 provider 或实板证据。

### Sensor 与 Device

- Device model 是新驱动的 canonical migration destination。
- SHT30、ADS1115、MPU6050 与 BMP280 已有 Device-model canonical owner；legacy 入口只保留明确
  compatibility boundary。
- 不执行原提案中针对旧 `components/device/xy_*.c` 路径的批量移动，也不创建第四套生命周期。

### Display 与 Storage

- 现有 owner 由 root Kconfig/CMake 和组件证据台账决定。
- 目录或源码存在只表示 source inventory，不等于 root product target、硬件验证或 production-ready。

## 对原提案的保留结论

以下架构原则仍有效，但只能通过小型、受检、path-limited slice 落地：

1. HAL 只拥有 MCU 外设抽象；上层组件通过 HAL/Device 边界访问硬件。
2. Device 拥有通用 lifecycle、registry 与 dispatch；具体器件能力留在 typed driver API。
3. 新 owner 必须同时具备 root Kconfig/CMake 选择、focused Host contract 和明确的硬件 pending 状态。
4. 迁移必须先证明 active source/consumer，再保留兼容边界并移除 duplicate lifecycle。
5. 禁止通过批量 `mv`、软链接或复制源码制造并行实现。

## 历史方案处置

原“完全重构/渐进式重构”目录树、迁移脚本、分支命令、批量移动/删除命令和工期估算已删除，原因是：

- 引用多个不存在或已变更的路径；
- 将 Charger/Fuel Gauge ownership 写成未发生的迁移；
- 会绕过 root selection 和当前 canonical owner；
- 大爆炸式目录移动无法满足当前 focused/full/target gate 与 path-limited commit 规则。

如需继续架构收敛，应从 Sprint 看板选择一个依赖已满足的小 slice，执行
`probe/RED → implementation → focused → full/target gate → git diff --check → commit`，而不是恢复本历史提案。
