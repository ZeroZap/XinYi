# XinYi Demo Project

综合演示项目，展示 XinYi 框架的完整功能。

## 功能特性

- ✅ OSAL 多任务调度
- ✅ I2C 设备驱动
- ✅ EEPROM 数据存储
- ✅ OLED 显示
- ✅ 多传感器融合 (MPU6050/BMP280/SHT30)

## 硬件要求

- STM32U5 开发板
- 24C256 EEPROM
- SSD1306 OLED (128x64)
- MPU6050 加速度计
- BMP280 气压计
- SHT30 温湿度传感器

## 构建

```bash
mkdir build && cd build
cmake .. -DPROJECT=demo
make
```

## 连接说明

| 设备 | I2C 地址 | 引脚 |
|------|---------|------|
| EEPROM | 0x50 | PB6/PB7 |
| OLED | 0x3C | PB6/PB7 |
| MPU6050 | 0x68 | PB6/PB7 |
| BMP280 | 0x76 | PB6/PB7 |
| SHT30 | 0x44 | PB6/PB7 |
