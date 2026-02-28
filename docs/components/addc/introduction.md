# ADDC 组件 - ADC/DAC 辅助库

**状态**: ✅ 完善 | **测试**: 24 用例 | **版本**: 1.0

---

## 📖 简介

XinYi ADDC 组件提供 ADC 和 DAC 的辅助库，简化模数/数模转换操作。

### 核心特性

- ✅ **ADC 辅助** - 多通道采样
- ✅ **DAC 辅助** - 电压输出
- ✅ **分辨率配置** - 8/10/12/14/16 位
- ✅ **电压转换** - 原始值/电压转换

---

## 🚀 快速开始

### ADC 示例

```c
#include "xy_adc.h"

int main(void) {
    xy_adc_t adc;
    xy_adc_sample_t sample;
    
    // 初始化 ADC (3.3V, 12 位)
    xy_adc_init(&adc, 3300, ADC_RESOLUTION_12BIT);
    
    // 配置通道
    xy_adc_config_channel(&adc, 0, true);
    
    // 采样
    xy_adc_sample(&adc, 0, &sample);
    
    printf("Voltage: %d mV\n", sample.voltage_mv);
    
    return 0;
}
```

### DAC 示例

```c
#include "xy_adc.h"

int main(void) {
    xy_dac_t dac;
    
    // 初始化 DAC (3.3V, 12 位)
    xy_dac_init(&dac, 3300, DAC_RESOLUTION_12BIT);
    
    // 配置通道
    xy_dac_config_channel(&dac, 0, true);
    
    // 设置输出电压 (1650mV = 50%)
    xy_dac_set_voltage(&dac, 0, 1650);
    
    return 0;
}
```

---

## 📋 API 参考

### ADC

| 函数 | 说明 |
|------|------|
| `xy_adc_init()` | 初始化 ADC |
| `xy_adc_config_channel()` | 配置通道 |
| `xy_adc_sample()` | 单次采样 |
| `xy_adc_sample_multi()` | 多通道采样 |
| `xy_adc_raw_to_voltage()` | 原始值转电压 |

### DAC

| 函数 | 说明 |
|------|------|
| `xy_dac_init()` | 初始化 DAC |
| `xy_dac_config_channel()` | 配置通道 |
| `xy_dac_set_voltage()` | 设置电压 |
| `xy_dac_set_raw()` | 设置原始值 |

---

## 🧪 测试用例

ADDC 组件包含 24 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| ADC 初始化 | 4 |
| ADC 通道 | 3 |
| ADC 采样 | 4 |
| ADC 转换 | 4 |
| DAC 初始化 | 4 |
| DAC 通道 | 2 |
| DAC 输出 | 3 |

运行测试：
```bash
ctest -R test_addc --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
