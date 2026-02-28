# Device 组件 - 设备驱动框架

**状态**: ✅ 完成 | **版本**: 1.0

---

## 📖 简介

XinYi Device 组件提供标准化的设备驱动框架，基于 HAL 层封装常用外设驱动。

### 核心特性

- ✅ **统一接口** - 标准化的设备 API
- ✅ **设备管理** - 设备注册/查找
- ✅ **常用驱动** - EEPROM/OLED 等
- ✅ **易于扩展** - 简单的驱动开发接口

### 支持的设备

| 类别 | 设备 | 接口 | 状态 |
|------|------|------|------|
| EEPROM | 24xx 系列 | I2C | ✅ |
| OLED | SSD1306 | I2C | ✅ |
| GPIO | 通用 GPIO | GPIO | ✅ |
| UART | 串口设备 | UART | ✅ |
| SPI | SPI 设备 | SPI | ✅ |

---

## 🚀 快速开始

### I2C 设备示例

```c
#include "xy_device.h"

int main(void) {
    xy_i2c_device_t i2c_dev;
    
    // 初始化 I2C 设备
    xy_i2c_device_init(&i2c_dev, I2C1, 0x50, 1000);
    
    // 写入数据
    uint8_t data[] = {0x01, 0x02, 0x03};
    xy_i2c_device_write(&i2c_dev, data, sizeof(data));
    
    // 读取数据
    uint8_t rx[3];
    xy_i2c_device_read(&i2c_dev, rx, sizeof(rx));
    
    return 0;
}
```

### EEPROM 示例

```c
#include "xy_eeprom_24xx.h"

int main(void) {
    xy_eeprom_24xx_t eeprom;
    
    // 初始化 EEPROM (24C256, 32KB)
    xy_eeprom_24xx_init(&eeprom, I2C1, 0x50, 64, 32768);
    
    // 写入数据
    uint8_t tx[] = "Hello, EEPROM!";
    xy_eeprom_24xx_write(&eeprom, 0, tx, sizeof(tx));
    
    // 读取数据
    uint8_t rx[32];
    xy_eeprom_24xx_read(&eeprom, 0, rx, sizeof(rx));
    
    return 0;
}
```

### OLED 示例

```c
#include "xy_oled_ssd1306.h"

int main(void) {
    xy_oled_ssd1306_t oled;
    
    // 初始化 OLED (128x64)
    xy_oled_ssd1306_init(&oled, I2C1, 128, 64);
    
    // 清屏
    xy_oled_ssd1306_clear(&oled);
    
    // 绘制字符串
    xy_oled_ssd1306_draw_string(&oled, 0, 0, "Hello!", true);
    
    // 刷新屏幕
    xy_oled_ssd1306_refresh(&oled);
    
    return 0;
}
```

---

## 📋 API 参考

### 设备框架

| 函数 | 说明 |
|------|------|
| `xy_device_manager_init()` | 初始化设备管理器 |
| `xy_device_manager_register()` | 注册设备 |
| `xy_device_manager_unregister()` | 注销设备 |
| `xy_device_manager_find()` | 查找设备 |
| `xy_device_manager_foreach()` | 遍历设备 |

### I2C 设备

| 函数 | 说明 |
|------|------|
| `xy_i2c_device_init()` | 初始化 I2C 设备 |
| `xy_i2c_device_read()` | 读取数据 |
| `xy_i2c_device_write()` | 写入数据 |
| `xy_i2c_device_read_reg()` | 读寄存器 |
| `xy_i2c_device_write_reg()` | 写寄存器 |

### SPI 设备

| 函数 | 说明 |
|------|------|
| `xy_spi_device_init()` | 初始化 SPI 设备 |
| `xy_spi_device_transfer()` | 传输数据 |
| `xy_spi_device_send()` | 发送数据 |
| `xy_spi_device_recv()` | 接收数据 |
| `xy_spi_device_cs()` | 片选控制 |

### EEPROM

| 函数 | 说明 |
|------|------|
| `xy_eeprom_24xx_init()` | 初始化 EEPROM |
| `xy_eeprom_24xx_read()` | 读取 EEPROM |
| `xy_eeprom_24xx_write()` | 写入 EEPROM |
| `xy_eeprom_24xx_write_page()` | 页写入 |

### OLED

| 函数 | 说明 |
|------|------|
| `xy_oled_ssd1306_init()` | 初始化 OLED |
| `xy_oled_ssd1306_clear()` | 清屏 |
| `xy_oled_ssd1306_refresh()` | 刷新屏幕 |
| `xy_oled_ssd1306_draw_pixel()` | 绘制像素 |
| `xy_oled_ssd1306_draw_line()` | 绘制直线 |
| `xy_oled_ssd1306_draw_char()` | 绘制字符 |
| `xy_oled_ssd1306_draw_string()` | 绘制字符串 |

---

## 📝 开发自定义驱动

### 1. 定义设备结构

```c
typedef struct {
    xy_i2c_device_t i2c_dev;
    // 设备特定参数
    uint16_t param1;
    uint16_t param2;
} xy_my_device_t;
```

### 2. 实现初始化

```c
int xy_my_device_init(xy_my_device_t *dev, void *i2c_handle, uint16_t addr)
{
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000);
    // 设备特定初始化
    return XY_DEVICE_OK;
}
```

### 3. 实现设备操作

```c
int xy_my_device_read(xy_my_device_t *dev, uint8_t *data, size_t len)
{
    return xy_i2c_device_read(&dev->i2c_dev, data, len);
}

int xy_my_device_write(xy_my_device_t *dev, const uint8_t *data, size_t len)
{
    return xy_i2c_device_write(&dev->i2c_dev, data, len);
}
```

---

## 🎯 下一步计划

### 待添加的驱动

- [ ] MPU6050 (加速度计/陀螺仪)
- [ ] BMP280 (气压计)
- [ ] SHT30 (温湿度)
- [ ] ADS1115 (ADC)
- [ ] MAX31855 (热电偶)
- [ ] PCF8563 (RTC)
- [ ] AT24MAC (EEPROM+MAC)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
