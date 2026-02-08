# 毫欧表 (Milliohm Meter)

## 概述
基于单片机的高精度毫欧表，用于测量低阻值电阻（1mΩ - 1000mΩ）。

## 测量原理
采用四线法（Kelvin测量）：
- 恒流源提供测试电流（10mA - 100mA可调）
- 差分放大器测量电阻两端电压
- 通过欧姆定律计算电阻值：R = V / I

## 硬件要求
- MCU: STM32/ESP32等（带ADC）
- 恒流源: LM334或运放+MOS管
- 差分放大器: INA226/INA229或运放（增益100-1000倍）
- ADC: 12位以上，参考电压稳定
- 显示: OLED/LCD

## 测量范围
- 电阻: 1mΩ - 1000mΩ
- 精度: ±1% ± 0.5mΩ
- 测试电流: 10mA/50mA/100mA

## 文件说明
- [`milliohm_meter.h`](milliohm_meter.h:1): 头文件
- [`milliohm_meter.c`](milliohm_meter.c:1): 核心实现
- [`config.h`](config.h:1): 配置参数
