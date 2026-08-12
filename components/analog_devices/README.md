# XinYi Analog Devices 组件

**状态**: host-guarded / root build integration pending
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
cmake --build build/pc --target xy_adc -j$(nproc)   # 如果 root target 已配置存在
make HAL_PLATFORM=STM32U5 -j$(nproc)                # MCU compile probe；仍非硬件验证
```

## 构建/配置边界

- 当前 `components/analog_devices/CMakeLists.txt` 仍是历史 ADDC 片段，引用的 `src/xy_addc*.c` / `inc/xy_addc*.h` 在当前目录并不存在；不要把它视为已闭环 root component target。
- `components/analog_devices/Kconfig` 只提供 nested `COMPONENT_ADDC` 历史开关；当前 root `Kconfig` 不导出专门的 analog devices component enablement。
- `tests/unit/analog_devices/test_analog_devices.c` 直接链接当前 active 源文件，因此 focused host CTest 通过不等于 root firmware component integration 已完成。

后续如要让 analog devices 成为 root component，应先做小步 proposal/实现：明确 target 名称（例如 `xy_adc` / `analog_devices_component`）、只纳入当前存在并有 host coverage 的源文件，再补 root PC/STM32U5 compile gate。不要同轮复活所有历史 ADS*/MAX*/MCP* 声明。

## 硬件验证边界

现有 host fake 只证明软件契约：

- MCP3008：SPI transaction shape、CS toggling 与 10-bit result extraction。
- HX711：GPIO clock/read sequence、ready timeout 与默认 gain pulse count。
- ADC/DAC helper：simulation/math contract。

这些结果不能替代真实 SPI ADC、电桥传感器、GPIO 时序、噪声/校准、参考电压误差或板级电气验证。真实硬件结论必须来自 board/project validation record。

## 回滚

本 README 同步未改动实现。未提交前可回滚：

```bash
git checkout -- components/analog_devices/README.md
```
