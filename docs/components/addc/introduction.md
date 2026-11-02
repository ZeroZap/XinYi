# ADDC 组件 - ADC/DAC 辅助库

**状态**: 辅助库是台账 + 换算层（DAC 输出为占位），Host 测试 `analog_devices`；真实采样/输出要 HAL 后端 | **本页代码块**: 由 CTest `doc_samples_compile` 编译 | **版本**: 2.0

---

## 📖 简介

`components/analog_devices/xy_adc.h`（CMake 目标 `xy_adc`，别名 `analog_devices_component`）提供
`xy_adc_t` / `xy_dac_t`：分辨率、通道使能、原始值 ↔ 电压换算。它**不是驱动**，两条边界必须先说清：

1. **它不碰硬件。** `xy_adc_sample()` 填的是满量程的 50%（源码注释原文「模拟采样值 (实际应用中需要
   读取硬件)」），`xy_dac_set_raw()` 丢掉 raw 值直接返回 `XY_ADC_OK`，`xy_dac_get_voltage()` 无论之前
   设过什么都返回 `ref_voltage_mv / 2`。要真的采到电压、真的把电平推出去，走 HAL 的
   `xy_hal_adc_*` / `xy_hal_dac_*`。
2. **同名头文件陷阱。** `components/device/inc/xy_adc.h` 与本页这个头**同名且共用 include guard
   `XY_ADC_H`**：两支里先被包含的那支生效，另一支静默失效。device 那支声明的是 `xy_adc_dev_*`
   设备封装，**全仓没有任何实现**——照它写调用编译能过、链接一定失败。链 `xy_adc` 目标时
   PUBLIC include 顺序把 `components/analog_devices` 排在 `components/device/inc` 之前，所以
   `#include "xy_adc.h"` 才解析到本页这一支；如果你的 include 路径反过来，拿到的是那支空头。
   这个碰撞由 `check_include_guard_collisions.py` 钉住并写明理由（「谁拥有这个名字是 owner 决策」）。

## 📊 平台可用性（实测）

辅助库的换算与台账与平台无关，但**真实采样落到 HAL 后端**，而后端的覆盖并不均匀。下表的数字是
`nm -g --defined-only <build>/components/hal/libxy_hal.a` 在真实归档上数出来的定义数，并由 CTest
`hal_adc_dac_surface` 常驻守着——每一格与守卫里钉住的集合逐项比对，多实现一个（表格该缩）或少实现
一个（回归）都会变红：

| 平台 | ADC（头声明 40） | DAC（头声明 14） |
|------|-----------------|-----------------|
| STM32F4 | 28 | 14 |
| STM32U5 | 12 | 14 |
| STM32L4（Pandora 参考板） | **0 —— 该分支不构建 ADC 后端** | 14（复用 F4 包装） |
| CH32V307（WCH） | 40 | 14 |
| PC（Host 仿真） | 40 | 14 |

两条推论：

- **Pandora 参考板上没有 ADC 后端**。L4 分支按设计排除 `stm32/stm32f4/xy_hal_adc.c`（F4 的
  ADC->CCR 与采样时间契约是 F4 专有的），排除了却还没有 L4 替代实现；这个取舍记录在
  `check_hal_platform_build_contract.py` 的 `EXPECTED_L4_F4_EXCLUDED` 里。该板的电平读取只能走
  外接 ADC 模块（如 ADS1115/MCP3008，走 I2C/SPI 而不是片上 ADC）。
- **40 个 ADC 函数在四支后端里没有交集**：F4 / U5 / CH32V307 三支都实现的只有 9 个（`init` /
  `deinit` / `config_channels` / `start` / `stop` / `read` / `read_nb` / `read_dma` /
  `register_callback`），PC 仿真实现了全部 40 个，但那只是契约可调用，不代表任何一颗芯片的采样。
  `xy_hal_adc_value_to_mv()` 在 U5 上不存在，`set_sampling_time()` 只有 CH32V307 和 PC 有。**换平台前
  先查自己那一支实现了什么**，不要以头文件为准。

---

## 🚀 快速开始

### 辅助库：把原始值换成电压

这一层能放心用的就是换算与台账（它在 Host 上可测，CTest `analog_devices`）：

```c
#include "xy_adc.h"
#include <stdio.h>

static void report_reading(uint32_t raw)
{
    xy_adc_t adc;

    if (xy_adc_init(&adc, 3300, ADC_RESOLUTION_12BIT) != XY_ADC_OK) {
        return;
    }
    if (xy_adc_config_channel(&adc, 0, true) != XY_ADC_OK) {
        return;
    }

    /* 与分辨率、参考电压一致：raw / 4095 * 3300 mV。 */
    printf("raw %lu -> %lu mV\n", (unsigned long)raw,
           (unsigned long)xy_adc_raw_to_voltage(&adc, raw));
}
```

要反向换算（目标电压 → 应写入的 raw）用 `xy_adc_voltage_to_raw()`。
`xy_adc_sample()` 也在这一层，但它填的是满量程一半的占位值，别拿它当采样结果。

### HAL：真的读一次通道

只使用上面那 9 个三平台共有的函数，因此换到任何一支 ADC 后端都能链接：

```c
#include "xy_hal_adc.h"

/* `adc` 是板级给的句柄：STM32 上是 `ADC_HandleTypeDef *`，CH32V307 上是 WCH 的 ADC 实例。
 * HAL 不认厂商类型名，只按 void * 传递，所以调用者必须知道板上接的是哪个外设。 */
static int32_t adc_read_raw(void *adc, uint8_t channel)
{
    xy_hal_adc_config_t config = {
        .resolution = XY_HAL_ADC_RESOLUTION_12B,
        .align = XY_HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = XY_HAL_ADC_SCAN_DISABLE,
        .continuous = XY_HAL_ADC_CONTINUOUS_DISABLE,
        .trigger_src = XY_HAL_ADC_TRIGGER_SOFTWARE,
    };
    xy_hal_adc_channel_config_t channel_config = {
        .channel = channel,
        .rank = 1,
        .sampling_time = 0,
        .enabled = 1,
    };

    if (xy_hal_adc_init(adc, &config) != XY_HAL_OK) {
        return -1;
    }
    if (xy_hal_adc_config_channels(adc, &channel_config, 1) != XY_HAL_OK) {
        return -1;
    }

    return xy_hal_adc_read(adc, channel, 10); /* 超时 10 ms */
}
```

两层可以拼起来：HAL 读到的 `raw` 交给上一段的 `xy_adc_raw_to_voltage()`，换算逻辑就只写一遍。

### HAL：把电平推出去

```c
#include "xy_hal_dac.h"

static void dac_half_scale(void *dac)
{
    xy_hal_dac_config_t config = {
        .resolution = XY_HAL_DAC_RESOLUTION_12B,
        .align = XY_HAL_DAC_DATAALIGN_RIGHT,
        .trigger_src = XY_HAL_DAC_TRIGGER_SOFTWARE,
        .wave = XY_HAL_DAC_WAVE_NONE,
    };

    if (xy_hal_dac_init(dac, &config) != XY_HAL_OK) {
        return;
    }
    if (xy_hal_dac_enable_channel(dac, XY_HAL_DAC_CHANNEL_1) != XY_HAL_OK) {
        return;
    }

    /* 12 位满量程的一半；3.3 V 参考下约 1650 mV */
    if (xy_hal_dac_set_value(dac, XY_HAL_DAC_CHANNEL_1, 2048,
                             XY_HAL_DAC_DATAALIGN_RIGHT) != XY_HAL_OK) {
        return;
    }
    (void)xy_hal_dac_start_output(dac, XY_HAL_DAC_CHANNEL_1);
}
```

---

## 📋 API 参考

### 辅助库 ADC（`components/analog_devices/xy_adc.h`）

| 函数 | 说明 |
|------|------|
| `xy_adc_init(adc, ref_voltage_mv, resolution)` | 初始化台账：参考电压 + 分辨率 |
| `xy_adc_config_channel(adc, channel, enabled)` | 登记/使能通道 |
| `xy_adc_sample(adc, channel, sample)` | **占位**：返回满量程 50%，不读硬件 |
| `xy_adc_sample_multi(adc, samples, count)` | 对已启用通道逐个调 `sample()` |
| `xy_adc_raw_to_voltage(adc, raw)` | raw → mV（真实可用的换算） |
| `xy_adc_voltage_to_raw(adc, mv)` | mV → raw |

### 辅助库 DAC（同上头文件）

| 函数 | 说明 |
|------|------|
| `xy_dac_init(dac, ref_voltage_mv, resolution)` | 初始化台账 |
| `xy_dac_config_channel(dac, channel, enabled)` | 登记/使能通道 |
| `xy_dac_set_voltage(dac, channel, mv)` | 换算后转 `set_raw()` |
| `xy_dac_set_raw(dac, channel, raw)` | **占位**：丢弃参数并返回 `XY_ADC_OK` |
| `xy_dac_get_voltage(dac, channel)` | **占位**：恒返回 `ref_voltage_mv / 2` |

### HAL（`xy_hal_adc.h` / `xy_hal_dac.h`）

DAC 的 14 个函数在 F4/U5/CH32V307/PC 四支后端里全部有实现（L4 复用 F4 包装）；
ADC 的覆盖见上面的矩阵。这两支头文件都由 `xy_hal.h` 伞头导出，`#include "xy_hal.h"` 就能拿到整族。

---

## 🧪 测试

| CTest | 覆盖 |
|-------|------|
| `analog_devices` | 辅助库 ADC/DAC 台账与换算，加 MCP3008/HX711 的 SPI/时序契约（Host） |
| `hal_adc_dac_surface` | 上表每一格：每支后端定义了哪些 ADC/DAC 函数，逐项与钉住的集合比对 |
| `hal_pc` | PC 仿真后端本身的契约（满量程/连续转换/校准/看门狗/DAC 对齐与波形），不是任何一颗芯片的精度 |
| `hal_umbrella_coverage` | `xy_hal_adc.h`、`xy_hal_dac.h` 是否真的被 `xy_hal.h` 伞头导出 |
| `doc_samples_compile` | 本页三个代码块的签名：参数个数、类型、结构体字段名 |

> 本页此前写的「24 用例 / `ctest -R test_addc`」指向一个从来不存在的目标。真实覆盖是上表这几条：
> 辅助库与 PC 仿真后端都有 Host 测试；但 PC 的「转换」是由注入电压算出来的，**不代表任何芯片的
> 采样精度、时序、噪声或量程**，所以片上 ADC/DAC 的结论仍然只能来自目标平台的实测。

---

*最后更新：2026-11-03 | 维护者：XinYi Team*
