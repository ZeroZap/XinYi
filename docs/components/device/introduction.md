# Device 组件 - 设备驱动框架

**状态**: host-guarded / 软件契约已护栏（实板 pending） | **版本**: 1.0

> 当前证据边界：本文件是较早的用户向简介，仅作历史参考；若与 `components/device/README.md`、
> 当前 CMake/Kconfig 或组件证据台账冲突，以 canonical README 与当前源码为准。Pandora
> STM32L475VE 是正式 reference board；STM32U5/M33 仅保留 enhancement compile compatibility。
> SSD1306 deferred，不选择、不推进、不作为 Sprint 1–4 基础验收依赖；Host/compile-only 证据
> 不构成实板、安全或 production-ready 声明。

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
| EEPROM | 24xx 系列 | I2C | Host contract；Pandora 无板载 24xx，实板 pending |
| OLED | SSD1306 | I2C | deferred（Host contract 已有，实板不推进） |
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

### 驱动迁移状态（历史 TODO 已收敛）

早期列为「待添加」的 MPU6050、BMP280、SHT30、ADS1115 已迁移为 `components/drivers/` 下的
canonical Device owners（详见组件证据台账与 Sprint 跟踪看板）；MAX31855、PCF8563、AT24MAC
等仍为未验证的规划项，不作为本 Sprint 承诺，也不代表 root product target 已接入。

---

*最后更新：2026-10-27（事实源校准） | 维护者：XinYi Team*
