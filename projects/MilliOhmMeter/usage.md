# 毫欧表使用说明

## 快速开始

### 1. 硬件连接
```
MCU ----[I2C/SPI]---- INA229 ----[四线法]---- 被测电阻
                      或差分放大器
```

### 2. 软件集成
```c
#include "milliohm_meter.h"

// 初始化
milliohm_init();

// 设置测试电流
milliohm_set_current(CURRENT_50MA);

// 测量
measurement_t result = milliohm_measure();
if (result.valid) {
    printf("电阻: %.2f mΩ\n", result.resistance_mohm);
}
```

## API说明

### [`milliohm_init()`](milliohm_meter.c:6)
初始化毫欧表，配置ADC和恒流源

### [`milliohm_set_current()`](milliohm_meter.c:11)
设置测试电流档位
- `CURRENT_10MA`: 10mA (测量100-1000mΩ)
- `CURRENT_50MA`: 50mA (测量20-200mΩ)
- `CURRENT_100MA`: 100mA (测量1-100mΩ)

### [`milliohm_measure()`](milliohm_meter.c:20)
执行一次测量，返回测量结果

## 测量精度优化

### 1. 多次采样平均
[`config.h`](config.h:19) 中设置 `SAMPLE_COUNT = 16`

### 2. 自动量程选择
```c
if (result.resistance_mohm > 200) {
    milliohm_set_current(CURRENT_10MA);
} else if (result.resistance_mohm > 50) {
    milliohm_set_current(CURRENT_50MA);
} else {
    milliohm_set_current(CURRENT_100MA);
}
```

### 3. 温度补偿
测量环境温度，补偿电阻温度系数

## 注意事项
1. 测试前确保被测电阻无电压
2. 使用粗短的测试线减少接触电阻
3. 定期校准零点和满度
4. 大电流测试时注意散热
