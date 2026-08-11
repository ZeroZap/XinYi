# Device 组件

**状态**: host-guarded / 软件契约已护栏  
**事实源**: `components/device/`、root `Kconfig` 的 `COMPONENT_DEVICE`、root auto-discovery CMake、`tests/unit/device/*`

## 当前边界

Device 组件提供 XinYi 的统一设备生命周期、注册表、通用 I/O dispatch、PM hook 与 I2C/SPI/UART/GPIO compatibility bus helpers。它位于应用/组件和 HAL 之间：

```text
applications/projects/examples
    -> components/device public APIs
    -> HAL / OSAL backends
```

当前实现不是新的目录迁移目标；`src/xy_device.c`、`src/xy_device_bus_helpers.c`、`src/xy_device_pm.c`、`src/xy_device_async.c` 与 `xy_device_core.c` 的拆分已经是主线事实：

| 文件 | 职责 |
| --- | --- |
| `src/xy_device.c` | public lifecycle、open/close/read/write/control、utility forwarders |
| `xy_device_core.c` | static-array registry、find/count/stats、idle PM check |
| `src/xy_device_bus_helpers.c` | I2C/SPI/UART/GPIO child-device compatibility helpers |
| `src/xy_device_pm.c` | per-device sleep/wake/power-mode helpers |
| `src/xy_device_async.c` | optional caller-owned async helper state machine |

## 构建与配置

- 根 `CMakeLists.txt` 会 auto-discover `components/device/CMakeLists.txt`，并生成 `xy_device` 静态库。
- root `Kconfig` 中 `COMPONENT_DEVICE` 默认关闭；需要 Device 的组件可显式启用，`COMPONENT_CHARGER` 目前会 `select COMPONENT_DEVICE`。
- `components/device/CMakeLists.txt` 在 PC 平台会链接 PC HAL stub；非 PC 平台应依赖外部 `xy_hal` target，不在 Device 内直接引用 vendor SDK。

## Host 测试护栏

Device 当前由主线 `make test-unit` 中的 focused CTest 守护：

| CTest | 目标 |
| --- | --- |
| `device_framework` | registry init/register/find/duplicate/unregister、public `xy_device_find()` forwarding、PM stats |
| `spi_device` | SPI helper 与 mixed-bus registry contract |
| `auto_register` | `XY_DEVICE_REGISTER` GCC constructor auto-registration contract |
| `device_async_helper` | optional `xy_device_async_*_ex` pending/busy/poll/timeout/cancel helper contract |
| `device_registry_example` | `examples/device_registry_example.c` public API smoke |
| `device_driver_template` | `examples/device_driver_template.c` driver-template smoke |

常用验证命令：

```bash
cmake --build build/tests/unit --target test_device -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^device_framework$'
make test-unit
git diff --check
```

## 后续维护口径

1. 不再按旧文档中的“Device 70% / 设备注册待完善”重复做基线补齐；注册表、示例与 focused host CTest 已存在。
2. 不做低收益的大规模 `src/`/根文件移动，除非先有独立 proposal、consumer impact scan 与 PC/STM32U5 验证。
3. 只在真实失败或新增 consumer 暴露缺口时补最小回归测试，例如：
   - 新 typed bus/device helper 的 callback isolation；
   - `xy_device_manager_*` group API 的明确 consumer contract；
   - 真实 MCU/OSAL PM backend 的 board-level evidence。
4. Device host CTest 只证明软件契约；不能替代真实外设、板级 pinmux、IRQ、DMA 或低功耗硬件验证。

## 相关文档

- `components/device/DEVICE_ARCHITECTURE.md` — 设备模型和分层设计说明。
- `docs/device-framework-core.md` — registry/core helper 使用说明。
- `docs/components/device/introduction.md` — 较早的用户向简介；若与本 README 或当前 CMake/Kconfig 冲突，以当前源码与本 README 为准。
