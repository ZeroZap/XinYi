# FUEL_GAUGE Component

**状态**: ✅ 主线可用 / host 已验证

## 📋 功能

- Standalone Fuel Gauge 核心：设备注册、初始化、数据抓取、通道读取、告警配置。
- Host 覆盖驱动：BQ27Z746、BQ40Z50、MAX17043、BQ27Z561。
- 扩展模块：状态查询、安全阈值/事件、安全认证与透传加解密接口。
- 错误返回：驱动和扩展 API 使用公开 `XY_FG_ERROR_*` 状态码。

## 📁 目录

- `inc/` - 公共 API 头文件
- `core/` - standalone core 与扩展模块实现
- `drivers/` - 具体 Fuel Gauge 芯片驱动

## ✅ 验证

Focused CTest targets:

- `fuel_gauge_core`
- `fg_bq27z746`
- `fg_bq40z50`
- `fg_max17043`
- `fg_bq27z561`

Run all unit coverage with:

```bash
make test-unit
```

## ⚠️ 后续硬件项

- 在真实 SMBus/I2C 硬件上验证 clock stretching、放电期 NACK/retry 和告警阈值硬件编程。
- 按具体电池包参数调整 safety 默认阈值与状态映射。
