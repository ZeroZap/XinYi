# XinYi Analog Devices 组件

**状态**: host-guarded / root build integrated for current active sources
**范围**: ADC/DAC helper、MCP3008 SPI ADC、HX711 load-cell ADC

## 当前事实源

`components/analog_devices/` 当前不是完整外设生态包，而是一组低层模拟器件 helper/driver：

- `xy_adc.{h,c}`：host-safe ADC/DAC helper，提供通道配置、模拟采样、raw/voltage 转换和 DAC 设置 API。
- `inc/xy_adc_ext.h`：外部 ADC driver umbrella，当前 active host coverage 只覆盖 MCP3008 与 HX711；其它声明（ADS1015/ADS1118/ADS1248/MAX11100/MCP3208 等）仍是历史/待实现 API 形状。
- `src/xy_mcp3008.c`：MCP3008 SPI command/result contract，依赖 mockable `xy_hal_spi_transmit_receive()` 与 GPIO CS seam。
- `src/xy_hx711.c`：HX711 GPIO bitbang/read/tare/calibrate helper，当前只证明 host fake GPIO/timing contract。

## Host 单元测试

活跃 focused CTest：

| CTest | 覆盖范围 |
| --- | --- |
| `analog_devices` | ADC/DAC helper lifecycle/conversion、多通道采样，MCP3008 SPI command/result/guard，HX711 init/read timeout/bitbang smoke |

常用验证：

```bash
cmake --build build/tests/unit --target test_analog_devices -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^analog_devices$'
make test-unit
cmake --build build/pc --target xy_adc -j$(nproc)
make HAL_PLATFORM=STM32U5 -j$(nproc)                # MCU compile probe；仍非硬件验证
```

## 构建/配置边界

- `components/analog_devices/CMakeLists.txt` 现在只把当前已有实现且已有 host CTest 的源文件纳入 root build：`xy_adc.c`、`src/xy_mcp3008.c`、`src/xy_hx711.c`。
- root target include roots 同步到这些源码实际使用的 HAL/Trace/CLIB/Device/Sensor public headers；`inc/xy_adc_ext.h` 仍会经历史 ADS1115 umbrella 间接需要 `components/device/inc`。
- root auto-discovery 会生成 `xy_adc` 静态库；同时提供 `analog_devices_component` CMake ALIAS，方便未来 component-style consumer 迁移，但当前安装/runtime target 名仍保持 `xy_adc`。
- `components/analog_devices/Kconfig` 仍只提供 nested `COMPONENT_ADDC` 历史开关；当前 root `Kconfig` 不导出专门的 analog devices component enablement。
- `tests/unit/analog_devices/test_analog_devices.c` 与 root `xy_adc` target 使用同一组 active 源文件；focused host CTest 与 root PC/STM32U5 compile gate 共同证明当前 active 子集，不代表所有历史外部 ADC 声明已实现。

后续如要扩展 analog devices，应先写小步 proposal：为单个已声明但未实现的 ADS*/MAX*/MCP* driver 明确 source/header/API 归属，补 focused host CTest，再纳入 root target。不要同轮复活所有历史声明。

## 硬件验证边界

现有 host fake 只证明软件契约：

- MCP3008：SPI transaction shape、CS toggling 与 10-bit result extraction。
- HX711：GPIO clock/read sequence、ready timeout 与默认 gain pulse count。
- ADC/DAC helper：simulation/math contract。

这些结果不能替代真实 SPI ADC、电桥传感器、GPIO 时序、噪声/校准、参考电压误差或板级电气验证。真实硬件结论必须来自 board/project validation record。

## 回滚

本轮 root-build 集成只改动 CMake 与 README。未提交前可回滚：

```bash
git checkout -- components/analog_devices/CMakeLists.txt components/analog_devices/README.md
```
