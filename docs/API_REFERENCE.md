# XinYi API 参考手册

**版本**: 1.0.0  
**日期**: 2026-03-16  
**维护者**: XinYi Team  
**状态**: 🟡 持续更新中

---

## 📖 导航

### 核心 API
- [OSAL API](#osal-api) - 操作系统抽象层
- [HAL API](#hal-api) - 硬件抽象层
- [设备模型 API](#设备模型-api)
- [传感器 API](#传感器-api)

### 工具 API
- [CRC 校验](#crc-校验)
- [加密算法](#加密算法)
- [状态机](#状态机)

---

## OSAL API

### 内核控制

```c
xy_os_status_t xy_os_kernel_init(void);
xy_os_status_t xy_os_kernel_start(void);
int32_t xy_os_kernel_lock(void);
int32_t xy_os_kernel_unlock(void);
```

**支持后端**:
- ✅ FreeRTOS
- ✅ RT-Thread
- ✅ CMSIS-RTX
- ✅ RTX5
- ✅ Bare-metal (ARM/RISC-V/ARC/x86)

---

## HAL API

### GPIO

```c
xy_hal_gpio_t xy_hal_gpio_bind(const char *name);
xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, 
                                     xy_hal_gpio_pin_t pin,
                                     const xy_hal_gpio_config_t *config);
xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t gpio, 
                                 xy_hal_gpio_pin_t pin, 
                                 uint8_t value);
int32_t xy_hal_gpio_read(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin);
```

**支持平台**:
- ✅ STM32U5
- ✅ WCH CH32U5
- ⏳ HC32 (待支持)

### UART

```c
xy_hal_uart_t xy_hal_uart_bind(const char *name);
xy_hal_error_t xy_hal_uart_configure(xy_hal_uart_t uart,
                                     const xy_hal_uart_config_t *config);
int32_t xy_hal_uart_write(xy_hal_uart_t uart, const uint8_t *data,
                          size_t length, uint32_t timeout);
int32_t xy_hal_uart_read(xy_hal_uart_t uart, uint8_t *data,
                         size_t length, uint32_t timeout);
```

### SPI

```c
xy_hal_spi_t xy_hal_spi_bind(const char *name);
xy_hal_error_t xy_hal_spi_configure(xy_hal_spi_t spi,
                                    const xy_hal_spi_config_t *config);
int32_t xy_hal_spi_transfer(xy_hal_spi_t spi, const uint8_t *tx_data,
                            uint8_t *rx_data, size_t length, uint32_t timeout);
```

### I2C

```c
xy_hal_i2c_t xy_hal_i2c_bind(const char *name);
xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c,
                                    const xy_hal_i2c_config_t *config);
int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length,
                                   uint32_t timeout);
```

---

## 设备模型 API

### 电源管理

```c
int xy_device_pm_init(xy_device_t *dev, const xy_device_pm_ops_t *pm_ops);
int xy_device_pm_set_state(xy_device_t *dev, xy_device_pm_state_t state);
int xy_device_pm_get_state(xy_device_t *dev, xy_device_pm_state_t *state);
int xy_device_pm_sleep(xy_device_t *dev);
int xy_device_pm_wakeup(xy_device_t *dev);
```

**电源状态**:
- `XY_DEVICE_PM_STATE_ACTIVE` - 全功率运行
- `XY_DEVICE_PM_STATE_SLEEP` - 睡眠模式
- `XY_DEVICE_PM_STATE_DEEP_SLEEP` - 深度睡眠
- `XY_DEVICE_PM_STATE_OFF` - 关闭

### 异步操作

```c
int xy_device_async_init(xy_device_t *dev);
int xy_device_async_read(xy_device_t *dev, void *buffer, size_t length,
                         xy_device_async_callback_t callback, void *user_data,
                         uint32_t timeout_ms);
int xy_device_async_write(xy_device_t *dev, const void *buffer, size_t length,
                          xy_device_async_callback_t callback, void *user_data,
                          uint32_t timeout_ms);
int xy_device_async_cancel(xy_device_t *dev);
```

---

## 传感器 API

### 通用传感器接口

```c
int xy_sensor_init(xy_sensor_t *sensor, const xy_sensor_config_t *config);
int xy_sensor_read(xy_sensor_t *sensor, xy_sensor_data_t *data);
int xy_sensor_get_value(xy_sensor_t *sensor, float *value);
```

### 已实现驱动

| 传感器 | 类型 | API 文档 |
|--------|------|---------|
| DHT11 | 温湿度 | [xy_dht11.h](../components/device/src/xy_dht11.h) |
| W25Qxx | SPI Flash | [xy_w25qxx.h](../components/device/src/xy_w25qxx.h) |
| WS2812 | RGB LED | [xy_ws2812.h](../components/device/src/xy_ws2812.h) |
| RC522 | RFID | [xy_rc522.h](../components/device/src/xy_rc522.h) |

---

## 工具 API

### CRC 校验

```c
uint32_t xy_crc32(const uint8_t *data, size_t length);
uint16_t xy_crc16(const uint8_t *data, size_t length);
uint8_t xy_crc8(const uint8_t *data, size_t length);
```

### 加密算法

```c
int xy_chacha20_encrypt(const uint8_t *key, const uint8_t *nonce,
                        const uint8_t *plaintext, size_t len,
                        uint8_t *ciphertext);
int xy_curve25519_keypair(uint8_t *public_key, uint8_t *private_key);
```

### 状态机

```c
void xy_sm_init(xy_state_machine_t *sm, const xy_sm_state_t *states,
                size_t state_count);
void xy_sm_transition(xy_state_machine_t *sm, uint8_t event);
xy_sm_state_t xy_sm_get_current_state(xy_state_machine_t *sm);
```

---

## 📝 更新日志

### 2026-03-16
- ✅ 初始版本发布
- ✅ OSAL API 文档
- ✅ HAL API 文档
- ✅ 设备模型 API 文档

---

## 🔗 相关文档

- [开发者指南](getting-started/DEVELOPER_GUIDE.md)
- [快速开始](getting-started/QUICK_START.md)
- [工具链配置](getting-started/toolchain.md)

---

**最后更新**: 2026-03-16  
**许可证**: Apache License 2.0
