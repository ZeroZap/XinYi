# FUEL_GAUGE Component

**状态**: ✅ 主线可用 / host 已验证；SMBus 硬件验证待真实板卡证据

## 📋 功能

- Standalone Fuel Gauge 核心：设备注册、初始化、数据抓取、通道读取、告警配置。
- Host 覆盖驱动：BQ27Z746、BQ40Z50、MAX17043、BQ27Z561。
- BQ40Z50 覆盖 SMBus transient NACK bounded retry、atomic fetch snapshot、cached status helper 与 direct balance-status read helper。
- `test_fuel_gauge_smbus_hardware_smoke_example` 固化真实板级验证前的 init/fetch/snapshot smoke 流程，但只代表 fake-I2C contract coverage。
- 扩展模块：状态查询、安全阈值/事件及 fail-closed 安全模式接口；未接入受审查 provider
  时，AES/SHA 配置不会把明文作为“密文”成功返回。
- 错误返回：驱动和扩展 API 使用公开 `XY_FG_ERROR_*` 状态码。

## 📁 目录

- `inc/` - 公共 API 头文件
- `core/` - standalone core 与扩展模块实现
- `drivers/` - 具体 Fuel Gauge 芯片驱动

Driver public headers:

- `drivers/xy_fg_bq27z746.h`
- `drivers/xy_fg_bq27z561.h`
- `drivers/xy_fg_bq40z50.h`
- `drivers/xy_fg_max17043.h`

## ✅ Host 验证契约

Focused CTest targets:

- `fuel_gauge_core`
- `fg_bq27z746`
- `fg_bq40z50`
- `fg_max17043`
- `fg_bq27z561`
- `fuel_gauge_smbus_hardware_smoke_example`

Host 覆盖当前固定以下公共契约，避免后续驱动回归：

- Core API 拒绝 `NULL`/未初始化设备/缺失回调，并在失败路径保留调用者输出。
- `xy_fuel_gauge_fetch()` 只有在驱动抓取成功后才更新时间戳。
- 芯片驱动 `init()` 失败时不能留下可见的 stale private initialized/status/cache 状态。
- 芯片驱动 `fetch()` 失败时保持上一份完整快照和状态位，不提交半更新数据。
- inline getter 与芯片专用 getter 在底层失败时保持调用者传入的 sentinel 输出值。
- 告警阈值 API 在 host 侧使用本地 cache；真实硬件阈值编程仍归入硬件验证项。
- `XY_FG_SECURITY_NONE` 保留显式明文复制兼容行为；AES/SHA 等安全模式在 provider
  尚未实现时返回 `XY_FG_ERROR_NOT_SUPPORTED` 并保持输出不变。
- BQ40Z50 cached status helper 不触发新 I2C 读；direct balance-status read helper 可单独 bounded-read 且失败时保留调用者输出。
- SMBus smoke skeleton 的 `record_template_must_stay_pending` 用例明确：没有真实板级 UART/SMBus/逻辑分析仪日志时，硬件验证记录必须保持 `pending`。

Run all unit coverage with:

```bash
make test-unit
```

## ⚠️ 后续硬件项

- 在真实 SMBus/I2C 硬件上验证 clock stretching、放电期 NACK/retry 和告警阈值硬件编程。
- 硬件证据需使用 `docs/validation/xinyi-fuel-gauge-smbus-hardware-validation-record-template-2026-08-06.md` 记录；host fake-I2C、`test_fuel_gauge_smbus_hardware_smoke_example` 或 STM32U5 compile-only 结果不能替代真实 SMBus/电池包日志。
- 后续若继续自动推进，应优先补 board/project smoke 记录或真实硬件日志入口；没有硬件证据时不要再增加等价 fake-I2C 证明。
- 按具体电池包参数调整 safety 默认阈值与状态映射。
