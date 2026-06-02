# XinYi 设备驱动开发模板

**版本**: 1.0  
**最后更新**: 2026-03-01  
**参考**: RT-Thread 设备框架、Zephyr 驱动模型

---

## 📋 概述

本模板提供 XinYi 项目设备驱动开发的标准结构和最佳实践。

---

## 🏗️ 驱动结构

### 基础架构

```
┌─────────────────────────────────────┐
│         应用层 (Application)         │
├─────────────────────────────────────┤
│         设备接口 (Device API)        │
├─────────────────────────────────────┤
│         驱动框架 (Driver Framework)  │
├─────────────────────────────────────┤
│    HAL 层 (Hardware Abstraction)     │
├─────────────────────────────────────┤
│         硬件 (Hardware)              │
└─────────────────────────────────────┘
```

---

## 📁 目录结构

```
components/device/
├── inc/
│   ├── xy_device.h          # 设备框架核心头文件
│   ├── xy_dev_api.h         # 兼容聚合头（新代码优先包含 xy_device.h/typed capability headers）
│   ├── xy_dev_i2c.h         # I2C 设备接口
│   ├── xy_dev_spi.h         # SPI 设备接口
│   └── xy_dev_uart.h        # UART 设备接口
│
├── src/
│   └── xy_device.c          # 设备框架实现
│
├── drivers/
│   ├── sensor/
│   │   ├── xy_mpu6050.c/h   # MPU6050 驱动
│   │   ├── xy_bmp280.c/h    # BMP280 驱动
│   │   └── xy_sht30.c/h     # SHT30 驱动
│   │
│   ├── display/
│   │   ├── xy_oled.c/h      # OLED 驱动
│   │   └── xy_lcd.c/h       # LCD 驱动
│   │
│   └── storage/
│       ├── xy_eeprom.c/h    # EEPROM 驱动
│       └── xy_flash.c/h     # Flash 驱动
│
└── tests/
    ├── test_device.c        # 设备框架测试
    └── test_mpu6050.c       # MPU6050 测试
```

---

## 🔧 驱动模板

### 模板 1: I2C 设备驱动

#### 头文件 (xy_xxx.h)

```c
/**
 * @file xy_xxx.h
 * @brief XXX 设备驱动
 * @version 1.0.0
 * @date 2026-03-01
 */

#ifndef XY_XXX_H
#define XY_XXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief 设备寄存器地址
 */
#define XXX_REG_ID          0x00
#define XXX_REG_CTRL        0x01
#define XXX_REG_DATA        0x02

/**
 * @brief 设备 ID
 */
#define XXX_ID_VALUE        0xAA

/**
 * @brief 错误码
 */
#define XXX_OK              0
#define XXX_ERROR           (-1)
#define XXX_INVALID_PARAM   (-2)
#define XXX_NOT_FOUND       (-3)

/**
 * @brief 设备数据结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /**< I2C 设备 */
    uint8_t device_id;          /**< 设备 ID */
    int16_t data;               /**< 传感器数据 */
    uint8_t initialized;        /**< 初始化标志 */
} xy_xxx_t;

/**
 * @brief 初始化设备
 * @param dev 设备句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @return XXX_OK 成功，其他值失败
 */
int xy_xxx_init(xy_xxx_t *dev, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化设备
 * @param dev 设备句柄
 * @return XXX_OK 成功，其他值失败
 */
int xy_xxx_deinit(xy_xxx_t *dev);

/**
 * @brief 读取数据
 * @param dev 设备句柄
 * @param data 数据指针
 * @return XXX_OK 成功，其他值失败
 */
int xy_xxx_read(xy_xxx_t *dev, int16_t *data);

/**
 * @brief 读取设备 ID
 * @param dev 设备句柄
 * @param id ID 指针
 * @return XXX_OK 成功，其他值失败
 */
int xy_xxx_read_id(xy_xxx_t *dev, uint8_t *id);

#ifdef __cplusplus
}
#endif

#endif /* XY_XXX_H */
```

#### 源文件 (xy_xxx.c)

```c
/**
 * @file xy_xxx.c
 * @brief XXX 设备驱动实现
 * @version 1.0.0
 * @date 2026-03-01
 */

#include "xy_xxx.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

int xy_xxx_init(xy_xxx_t *dev, void *i2c_handle, uint8_t addr)
{
    int ret;
    uint8_t id;
    
    if (!dev || !i2c_handle) {
        return XXX_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    /* 初始化 I2C 设备 */
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000);
    
    /* 读取设备 ID */
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XXX_REG_ID, &id, 1);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to read device ID\n");
        return XXX_NOT_FOUND;
    }
    
    if (id != XXX_ID_VALUE) {
        xy_log_e("Device ID mismatch: expected 0x%02X, got 0x%02X\n", 
                 XXX_ID_VALUE, id);
        return XXX_NOT_FOUND;
    }
    
    dev->device_id = id;
    dev->initialized = 1;
    
    xy_log_i("XXX device initialized successfully\n");
    return XXX_OK;
}

int xy_xxx_deinit(xy_xxx_t *dev)
{
    if (!dev) {
        return XXX_INVALID_PARAM;
    }
    
    dev->initialized = 0;
    xy_log_d("XXX device deinitialized\n");
    return XXX_OK;
}

int xy_xxx_read(xy_xxx_t *dev, int16_t *data)
{
    int ret;
    uint8_t buf[2];
    
    if (!dev || !data || !dev->initialized) {
        return XXX_INVALID_PARAM;
    }
    
    /* 读取数据寄存器 */
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XXX_REG_DATA, buf, 2);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to read data\n");
        return XXX_ERROR;
    }
    
    /* 组合数据 (小端) */
    *data = (int16_t)((buf[1] << 8) | buf[0]);
    dev->data = *data;
    
    xy_log_d("Read data: %d\n", *data);
    return XXX_OK;
}

int xy_xxx_read_id(xy_xxx_t *dev, uint8_t *id)
{
    if (!dev || !id || !dev->initialized) {
        return XXX_INVALID_PARAM;
    }
    
    *id = dev->device_id;
    return XXX_OK;
}
```

---

### 模板 2: SPI 设备驱动

#### 头文件 (xy_spi_xxx.h)

```c
/**
 * @file xy_spi_xxx.h
 * @brief SPI XXX 设备驱动
 * @version 1.0.0
 * @date 2026-03-01
 */

#ifndef XY_SPI_XXX_H
#define XY_SPI_XXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_spi.h"
#include <stdint.h>

/**
 * @brief SPI 设备结构
 */
typedef struct {
    xy_spi_device_t spi_dev;    /**< SPI 设备 */
    uint8_t cs_pin;             /**< 片选引脚 */
    uint8_t initialized;        /**< 初始化标志 */
} xy_spi_xxx_t;

/**
 * @brief 初始化设备
 * @param dev 设备句柄
 * @param spi_handle SPI 句柄
 * @param cs_pin 片选引脚
 * @return 0 成功，负数失败
 */
int xy_spi_xxx_init(xy_spi_xxx_t *dev, void *spi_handle, uint8_t cs_pin);

/**
 * @brief 写入数据
 * @param dev 设备句柄
 * @param data 数据指针
 * @param len 数据长度
 * @return 0 成功，负数失败
 */
int xy_spi_xxx_write(xy_spi_xxx_t *dev, const uint8_t *data, size_t len);

/**
 * @brief 读取数据
 * @param dev 设备句柄
 * @param data 数据指针
 * @param len 数据长度
 * @return 0 成功，负数失败
 */
int xy_spi_xxx_read(xy_spi_xxx_t *dev, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* XY_SPI_XXX_H */
```

---

### 模板 3: GPIO 设备驱动

#### 头文件 (xy_gpio_xxx.h)

```c
/**
 * @file xy_gpio_xxx.h
 * @brief GPIO XXX 设备驱动
 * @version 1.0.0
 * @date 2026-03-01
 */

#ifndef XY_GPIO_XXX_H
#define XY_GPIO_XXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_gpio.h"
#include <stdint.h>

/**
 * @brief GPIO 设备结构
 */
typedef struct {
    uint16_t gpio_pin;          /**< GPIO 引脚 */
    void *gpio_port;            /**< GPIO 端口 */
    uint8_t direction;          /**< 方向：0=输入，1=输出 */
    uint8_t initialized;        /**< 初始化标志 */
} xy_gpio_xxx_t;

/**
 * @brief 初始化 GPIO 设备
 * @param dev 设备句柄
 * @param port GPIO 端口
 * @param pin GPIO 引脚
 * @param direction 方向
 * @return 0 成功，负数失败
 */
int xy_gpio_xxx_init(xy_gpio_xxx_t *dev, void *port, uint16_t pin, uint8_t direction);

/**
 * @brief 写入 GPIO
 * @param dev 设备句柄
 * @param value 值：0 或 1
 * @return 0 成功，负数失败
 */
int xy_gpio_xxx_write(xy_gpio_xxx_t *dev, uint8_t value);

/**
 * @brief 读取 GPIO
 * @param dev 设备句柄
 * @param value 值指针
 * @return 0 成功，负数失败
 */
int xy_gpio_xxx_read(xy_gpio_xxx_t *dev, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif /* XY_GPIO_XXX_H */
```

---

## 🧪 测试模板

### 单元测试

```c
/**
 * @file test_xxx.c
 * @brief XXX 设备单元测试
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "xy_xxx.h"

/* ==================== Test Fixtures ==================== */

static xy_xxx_t dev;
static void *mock_i2c_handle;

void setUp(void)
{
    memset(&dev, 0, sizeof(dev));
    mock_i2c_handle = (void*)0x12345678;  /* Mock I2C handle */
}

void tearDown(void)
{
    xy_xxx_deinit(&dev);
}

/* ==================== Init Tests ==================== */

void test_xxx_init_null_param(void)
{
    int ret = xy_xxx_init(NULL, mock_i2c_handle, 0x50);
    TEST_ASSERT_EQUAL(XXX_INVALID_PARAM, ret);
}

void test_xxx_init_valid_param(void)
{
    /* 实际测试需要真实硬件 */
    /* int ret = xy_xxx_init(&dev, mock_i2c_handle, 0x50); */
    /* TEST_ASSERT_EQUAL(XXX_OK, ret); */
}

/* ==================== Read Tests ==================== */

void test_xxx_read_null_param(void)
{
    int ret = xy_xxx_read(NULL, NULL);
    TEST_ASSERT_EQUAL(XXX_INVALID_PARAM, ret);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_xxx_init_null_param);
    RUN_TEST(test_xxx_read_null_param);
    
    return UNITY_END();
}
```

---

## 📝 开发 checklist

### 驱动开发前

- [ ] 确认设备通信接口（I2C/SPI/UART/GPIO）
- [ ] 阅读设备数据手册
- [ ] 确认寄存器地址和位定义
- [ ] 准备开发板和测试环境

### 驱动开发中

- [ ] 创建头文件（结构体、API 声明）
- [ ] 实现初始化函数
- [ ] 实现读写函数
- [ ] 添加错误处理
- [ ] 添加日志输出

### 驱动开发后

- [ ] 编写单元测试
- [ ] 在真实硬件上测试
- [ ] 编写使用文档
- [ ] 代码审查

---

## 🔗 参考资源

| 资源 | 说明 |
|------|------|
| [RT-Thread 设备框架](https://www.rt-thread.io/document/site/programming/rt-thread-device/) | RT-Thread 设备驱动开发指南 |
| [Zephyr 驱动模型](https://docs.zephyrproject.org/latest/hardware/drivers/index.html) | Zephyr 驱动开发指南 |
| [Linux 设备驱动](https://lwn.net/Kernel/LDD3/) | Linux 设备驱动开发（经典） |

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
