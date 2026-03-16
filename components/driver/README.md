# XinYi Driver Components - 外围芯片驱动

**版本**: 1.0.0  
**日期**: 2026-03-17  
**维护者**: XinYi Team

---

## 📋 概述

Driver 层提供外围芯片的驱动程序，基于 HAL 层实现，为 Device 层提供统一的硬件访问接口。

---

## 🏗️ 架构分层

```
┌─────────────────────────────────┐
│      应用层 (Application)        │
└───────────────┬─────────────────┘
                │ 调用
┌───────────────▼─────────────────┐
│    Device 层 (设备管理框架)       │
│  xy_device_register()           │
│  xy_device_read/write()         │
└───────────────┬─────────────────┘
                │ 使用
┌───────────────▼─────────────────┐
│    Driver 层 (外围芯片驱动) ⭐    │
│  xy_dht11_init()                │
│  xy_bq25620_charge()            │
│  xy_rc522_read_card()           │
└───────────────┬─────────────────┘
                │ 依赖
┌───────────────▼─────────────────┐
│     HAL 层 (MCU 外设抽象)         │
│  xy_hal_i2c_transfer()          │
│  xy_hal_gpio_write()            │
└───────────────┬─────────────────┘
                │ 访问
┌───────────────▼─────────────────┐
│   MCU SDK (STM32/WCH/HC32)      │
│  寄存器定义/底层驱动             │
└─────────────────────────────────┘
```

---

## 📁 目录结构

```
driver/
├── sensor/              # 传感器驱动
│   ├── xy_dht11.c/h    # DHT11/DHT22 温湿度传感器
│   ├── xy_sht30.c/h    # SHT30 温湿度传感器 (待实现)
│   └── ...
│
├── charger/             # 充电管理驱动
│   ├── xy_bq25620.c/h  # TI BQ25620 充电管理 (待实现)
│   └── ...
│
├── storage/             # 存储驱动
│   ├── xy_w25qxx.c/h   # W25Qxx SPI Flash
│   └── ...
│
├── rfid/                # RFID 驱动
│   ├── xy_rc522.c/h    # MFRC522 RFID 读卡器
│   └── ...
│
└── README.md           # 本文档
```

---

## 🔧 驱动开发规范

### 1. 文件命名
- 格式：`xy_<chip_model>.c/h`
- 示例：`xy_dht11.c`, `xy_bq25620.h`

### 2. 依赖关系
- ✅ 允许依赖：HAL 层
- ❌ 禁止依赖：Device 层、其他 Driver

### 3. 接口规范
所有驱动提供标准初始化接口：

```c
/**
 * @brief 初始化驱动
 * @param dev 设备句柄
 * @param ... 硬件参数 (如 I2C 句柄、GPIO 等)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_xxx_init(xy_xxx_t *dev, ...);
```

### 4. 错误码
使用统一的设备错误码：
- `XY_DEVICE_OK` - 成功
- `XY_DEVICE_ERROR` - 通用错误
- `XY_DEVICE_INVALID_PARAM` - 无效参数
- `XY_DEVICE_TIMEOUT` - 超时
- `XY_DEVICE_NOT_FOUND` - 设备未找到

---

## 📦 现有驱动清单

### 传感器驱动 (sensor/)

| 驱动 | 芯片 | 接口 | 状态 |
|------|------|------|------|
| xy_dht11 | DHT11/DHT22 | GPIO | ✅ 完成 |

### 充电管理驱动 (charger/)

| 驱动 | 芯片 | 接口 | 状态 |
|------|------|------|------|
| xy_bq25620 | BQ25620 | I2C | 📝 待实现 |

### 存储驱动 (storage/)

| 驱动 | 芯片 | 接口 | 状态 |
|------|------|------|------|
| xy_w25qxx | W25Q16/32/64 | SPI | ✅ 完成 |

### RFID 驱动 (rfid/)

| 驱动 | 芯片 | 接口 | 状态 |
|------|------|------|------|
| xy_rc522 | MFRC522 | SPI | ✅ 完成 |

---

## 🚀 使用示例

### DHT11 温湿度传感器

```c
#include "driver/sensor/xy_dht11.h"

xy_dht11_t dht11;
xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.0");

/* 初始化 */
xy_dht11_init(&dht11, gpio);

/* 读取数据 */
xy_dht11_data_t data;
xy_dht11_read(&dht11, &data);

printf("温度：%.1f°C, 湿度：%.1f%%\n", 
       data.temperature, data.humidity);
```

### BQ25620 充电管理

```c
#include "driver/charger/xy_bq25620.h"

xy_bq25620_t charger;
xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");

/* 初始化 */
xy_bq25620_init(&charger, i2c, 0x6B);

/* 配置充电参数 */
xy_bq25620_set_charge_current(&charger, 1000);  // 1A
xy_bq25620_set_charge_voltage(&charger, 4200);  // 4.2V

/* 启动充电 */
xy_bq25620_start_charge(&charger);
```

---

## 🔗 相关文档

- [HAL 层文档](../hal/README.md)
- [Device 层文档](../device/README.md)
- [驱动开发指南](../docs/driver_development_guide.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0  
**最后更新**: 2026-03-17
