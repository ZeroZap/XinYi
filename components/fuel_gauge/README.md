# FUEL_GAUGE Component

**状态**: ✅ 主线可用 / host 已验证

## 📋 功能

- Standalone Fuel Gauge 核心：设备注册、初始化、数据抓取、通道读取、告警配置。
- Host 覆盖驱动：BQ27Z746、BQ40Z50、MAX17043、BQ27Z561。
- BQ40Z50 覆盖 SMBus transient NACK bounded retry、atomic fetch snapshot、cached status helper 与 direct balance-status read helper。
- 扩展模块：状态查询、安全阈值/事件、安全认证与透传加解密接口。
- 错误返回：驱动和扩展 API 使用公开 `XY_FG_ERROR_*` 状态码。

## 📁 目录

- `inc/` - 公共 API 头文件
- `core/` - standalone core 与扩展模块实现
- `drivers/` - 具体 Fuel Gauge 芯片驱动

## ✅ Host 验证契约

Focused CTest targets:

- `fuel_gauge_core`
- `fg_bq27z746`
- `fg_bq40z50`
- `fg_max17043`
- `fg_bq27z561`

Host 覆盖当前固定以下公共契约，避免后续驱动回归：

- Core API 拒绝 `NULL`/未初始化设备/缺失回调，并在失败路径保留调用者输出。
- `xy_fuel_gauge_fetch()` 只有在驱动抓取成功后才更新时间戳。
- 芯片驱动 `init()` 失败时不能留下可见的 stale private initialized/status/cache 状态。
- 芯片驱动 `fetch()` 失败时保持上一份完整快照和状态位，不提交半更新数据。
- inline getter 与芯片专用 getter 在底层失败时保持调用者传入的 sentinel 输出值。
- 告警阈值 API 在 host 侧使用本地 cache；真实硬件阈值编程仍归入硬件验证项。

Run all unit coverage with:

```bash
make test-unit
```

## ⚠️ 后续硬件项

- 在真实 SMBus/I2C 硬件上验证 clock stretching、放电期 NACK/retry 和告警阈值硬件编程。
- 硬件证据需使用 `docs/validation/xinyi-fuel-gauge-smbus-hardware-validation-record-template-2026-08-06.md` 记录；host fake-I2C 或 STM32U5 compile-only 结果不能替代真实 SMBus/电池包日志。
- 按具体电池包参数调整 safety 默认阈值与状态映射。
