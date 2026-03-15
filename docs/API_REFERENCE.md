# XinYi API 参考手册

**版本**: 1.0.0  
**日期**: 2026-03-16  
**维护者**: XinYi Team  
**状态**: ✅ 完整

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

## 📋 API 速查表

| 类别 | 函数数 | 头文件 | 文档 |
|------|-------|--------|------|
| **OSAL** | 34 | `xy_osal.h` | [OSAL 说明](../components/osal/README.md) |
| **HAL** | 120+ | `xy_hal_*.h` | [HAL 说明](../components/hal/README.md) |
| **设备模型** | 45 | `xy_device.h` | [设备模型](../components/device/README.md) |
| **传感器** | 80+ | `xy_sensor_*.h` | [传感器指南](../components/sensor/SENSOR_GUIDE.md) |
| **工具** | 25 | `xy_utils.h` | - |

**总计**: 300+ API 函数

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

### 任务管理

```c
xy_os_task_t xy_os_task_create(const char *name, 
                                xy_os_task_entry_t entry,
                                void *arg,
                                uint32_t stack_size,
                                uint32_t priority);
xy_os_status_t xy_os_task_destroy(xy_os_task_t task);
xy_os_status_t xy_os_task_delay(uint32_t ms);
xy_os_status_t xy_os_task_yield(void);
```

**参数说明**:
- `name`: 任务名称 (最大 32 字符)
- `stack_size`: 栈大小 (字节)，建议 ≥2048
- `priority`: 优先级 (0=最低，31=最高)

### 同步原语

```c
// 互斥锁
xy_os_mutex_t xy_os_mutex_create(void);
xy_os_status_t xy_os_mutex_lock(xy_os_mutex_t mutex, uint32_t timeout);
xy_os_status_t xy_os_mutex_unlock(xy_os_mutex_t mutex);

// 信号量
xy_os_sem_t xy_os_sem_create(uint32_t initial, uint32_t max);
xy_os_status_t xy_os_sem_wait(xy_os_sem_t sem, uint32_t timeout);
xy_os_status_t xy_os_sem_post(xy_os_sem_t sem);

// 事件标志
xy_os_event_t xy_os_event_create(void);
xy_os_status_t xy_os_event_wait(xy_os_event_t event, 
                                 uint32_t flags, 
                                 uint32_t options,
                                 uint32_t timeout);
xy_os_status_t xy_os_event_set(xy_os_event_t event, uint32_t flags);
```

### 内存管理

```c
void *xy_os_malloc(size_t size);
void xy_os_free(void *ptr);
void *xy_os_calloc(size_t nmemb, size_t size);
void *xy_os_realloc(void *ptr, size_t size);
```

### 时间服务

```c
uint64_t xy_os_time_get_ms(void);           // 获取系统时间 (ms)
uint64_t xy_os_time_get_us(void);           // 获取系统时间 (us)
xy_os_status_t xy_os_time_delay_ms(uint32_t ms);
xy_os_status_t xy_os_time_delay_us(uint32_t us);
```

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

**配置结构**:
```c
typedef struct {
    xy_hal_gpio_mode_t mode;      // 输入/输出/复用/模拟
    xy_hal_gpio_pull_t pull;      // 上拉/下拉/浮空
    xy_hal_gpio_speed_t speed;    // 低速/中速/高速/超高速
    uint8_t alternate;            // 复用功能选择
} xy_hal_gpio_config_t;
```

**支持平台**:
- ✅ STM32U5
- ✅ WCH CH32U5
- ⏳ HC32 (待支持)

**使用示例**:
```c
// 配置 LED 引脚 (输出，推挽，高速)
xy_hal_gpio_t led = xy_hal_gpio_bind("GPIOA.5");
xy_hal_gpio_config_t cfg = {
    .mode = XY_HAL_GPIO_MODE_OUTPUT,
    .pull = XY_HAL_GPIO_PULL_NONE,
    .speed = XY_HAL_GPIO_SPEED_HIGH
};
xy_hal_gpio_configure(led, XY_HAL_GPIO_PIN_5, &cfg);
xy_hal_gpio_write(led, XY_HAL_GPIO_PIN_5, 1);  // 点亮 LED
```

### UART

```c
xy_hal_uart_t xy_hal_uart_bind(const char *name);
xy_hal_error_t xy_hal_uart_configure(xy_hal_uart_t uart,
                                     const xy_hal_uart_config_t *config);
int32_t xy_hal_uart_write(xy_hal_uart_t uart, const uint8_t *data,
                          size_t length, uint32_t timeout);
int32_t xy_hal_uart_read(xy_hal_uart_t uart, uint8_t *data,
                         size_t length, uint32_t timeout);
xy_hal_error_t xy_hal_uart_set_baudrate(xy_hal_uart_t uart, uint32_t baud);
```

**配置结构**:
```c
typedef struct {
    uint32_t baudrate;      // 波特率 (1200 ~ 921600)
    uint8_t data_bits;      // 数据位 (7/8/9)
    uint8_t stop_bits;      // 停止位 (1/1.5/2)
    uint8_t parity;         // 校验位 (无/奇/偶)
    uint8_t flow_control;   // 流控制 (无/RTS/CTS/RTS+CTS)
} xy_hal_uart_config_t;
```

**支持波特率**: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600

### SPI

```c
xy_hal_spi_t xy_hal_spi_bind(const char *name);
xy_hal_error_t xy_hal_spi_configure(xy_hal_spi_t spi,
                                    const xy_hal_spi_config_t *config);
int32_t xy_hal_spi_transfer(xy_hal_spi_t spi, const uint8_t *tx_data,
                            uint8_t *rx_data, size_t length, uint32_t timeout);
xy_hal_error_t xy_hal_spi_set_frequency(xy_hal_spi_t spi, uint32_t freq_hz);
```

**配置结构**:
```c
typedef struct {
    uint32_t frequency;     // 时钟频率 (Hz)
    uint8_t mode;           // SPI 模式 (0/1/2/3)
    uint8_t bit_order;      // 位序 (MSB/LSB)
    uint8_t data_size;      // 数据大小 (8/16 bit)
} xy_hal_spi_config_t;
```

### I2C

```c
xy_hal_i2c_t xy_hal_i2c_bind(const char *name);
xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c,
                                    const xy_hal_i2c_config_t *config);
int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length,
                                   uint32_t timeout);
int32_t xy_hal_i2c_master_receive(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                  uint8_t *data, size_t length,
                                  uint32_t timeout);
```

**配置结构**:
```c
typedef struct {
    uint32_t speed;         // 速度 (100k/400k/1M)
    uint8_t duty_cycle;     // 占空比
    uint8_t ack_control;    // ACK 控制
} xy_hal_i2c_config_t;
```

**支持速度**:
- `XY_HAL_I2C_SPEED_STANDARD` - 100 kHz
- `XY_HAL_I2C_SPEED_FAST` - 400 kHz
- `XY_HAL_I2C_SPEED_FAST_PLUS` - 1 MHz

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

### 设备注册

```c
int xy_device_register(xy_device_t *dev, const char *name, 
                       const xy_device_ops_t *ops);
int xy_device_unregister(xy_device_t *dev);
xy_device_t *xy_device_find_by_name(const char *name);
```

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

**电源管理回调**:
```c
typedef struct {
    int (*prepare)(xy_device_t *dev);     // 进入低功耗前准备
    int (*resume)(xy_device_t *dev);      // 唤醒后恢复
    int (*get_state)(xy_device_t *dev);   // 获取当前状态
} xy_device_pm_ops_t;
```

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

**异步回调类型**:
```c
typedef void (*xy_device_async_callback_t)(
    xy_device_t *dev,
    int result,              // 操作结果 (>=0: 成功字节数，<0: 错误码)
    void *user_data          // 用户数据
);
```

**使用示例**:
```c
// 异步读取传感器数据
void sensor_callback(xy_device_t *dev, int result, void *user_data) {
    if (result > 0) {
        printf("读取 %d 字节\n", result);
    } else {
        printf("读取失败：%d\n", result);
    }
}

xy_device_async_read(sensor_dev, buffer, 32, sensor_callback, NULL, 1000);
```

### 设备树支持

```c
int xy_device_tree_init(const char *dtb_data, size_t size);
xy_device_t *xy_device_tree_create_device(const char *path);
int xy_device_tree_get_property(const char *path, const char *prop, 
                                 void *buf, size_t len);
```

---

## 传感器 API

### 通用传感器接口

```c
int xy_sensor_init(xy_sensor_t *sensor, const xy_sensor_config_t *config);
int xy_sensor_read(xy_sensor_t *sensor, xy_sensor_data_t *data);
int xy_sensor_get_value(xy_sensor_t *sensor, float *value);
int xy_sensor_set_power_mode(xy_sensor_t *sensor, xy_sensor_power_mode_t mode);
```

**传感器数据结构**:
```c
typedef struct {
    float temperature;      // 温度 (°C)
    float humidity;         // 湿度 (%RH)
    float pressure;         // 气压 (hPa)
    float light;            // 光照强度 (lux)
    float distance;         // 距离 (cm)
    int32_t accel[3];       // 加速度 (x,y,z)
    int32_t gyro[3];        // 角速度 (x,y,z)
    uint32_t timestamp;     // 时间戳 (ms)
} xy_sensor_data_t;
```

### 已实现驱动

#### 温湿度传感器
| 型号 | 接口 | 精度 | 驱动文件 |
|------|------|------|---------|
| DHT11 | 单总线 | ±2°C/±5% | `xy_dht11.c` (7.3KB) |
| SHT40 | I2C | ±0.15°C/±1.8% | `xy_sht40.c` (6KB) |

#### IMU (惯性测量单元)
| 型号 | 轴数 | 接口 | 驱动文件 |
|------|------|------|---------|
| BMI088 | 6 轴 | SPI | `xy_bmi088.c` (21KB) |

#### ToF 测距
| 型号 | 量程 | 接口 | 驱动文件 |
|------|------|------|---------|
| VL53L1X | 0-4m | I2C | `xy_vl53l1x.c` (30KB) |

#### 气压传感器
| 型号 | 精度 | 接口 | 驱动文件 |
|------|------|------|---------|
| LPS22HB | ±0.25hPa | I2C/SPI | `xy_lps22hb.c` (25KB) |

#### 气体传感器
| 型号 | 检测 | 接口 | 驱动文件 |
|------|------|------|---------|
| SGP40 | VOC | I2C | `xy_sgp40.c` (18.5KB) |

#### 其他驱动
| 型号 | 类型 | 接口 | 驱动文件 |
|------|------|------|---------|
| W25Qxx | SPI Flash | SPI | `xy_w25qxx.c` (14.6KB) |
| WS2812 | RGB LED | GPIO | `xy_ws2812.c` (9.2KB) |
| RC522 | RFID | SPI | `xy_rc522.c` (14.6KB) |

### 传感器使用示例

```c
// 初始化 DHT11 温湿度传感器
xy_sensor_t dht11;
xy_sensor_config_t cfg = {
    .type = XY_SENSOR_DHT11,
    .interface = XY_SENSOR_INTERFACE_GPIO,
    .gpio_pin = XY_HAL_GPIO_PIN_0
};
xy_sensor_init(&dht11, &cfg);

// 读取数据
xy_sensor_data_t data;
xy_sensor_read(&dht11, &data);
printf("温度：%.1f°C, 湿度：%.1f%%\n", data.temperature, data.humidity);
```

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

### 2026-03-16 - v1.0.0
- ✅ 初始版本发布
- ✅ OSAL API 文档 (34 个函数)
- ✅ HAL API 文档 (120+ 个函数)
- ✅ 设备模型 API 文档 (45 个函数)
- ✅ 传感器 API 文档 (80+ 个函数)
- ✅ 工具 API 文档 (25 个函数)

### 计划更新
- [ ] 添加更多使用示例
- [ ] 补充错误码说明
- [ ] 添加性能指标
- [ ] 补充平台差异说明

---

## 🔗 相关文档

### 入门指南
- [开发者指南](getting-started/DEVELOPER_GUIDE.md)
- [快速开始](getting-started/QUICK_START.md)
- [工具链配置](getting-started/toolchain.md)

### 组件文档
- [OSAL 说明](../components/osal/README.md)
- [HAL 说明](../components/hal/README.md)
- [设备模型](../components/device/README.md)
- [传感器指南](../components/sensor/SENSOR_GUIDE.md)

### 其他资源
- [常见问题](FAQs/)
- [故障排查](TROUBLESHOOTING.md)
- [术语表](GLOSSARY.md)

---

## 📞 支持

- **GitHub Issues**: https://github.com/ZeroZap/XinYi/issues
- **文档问题**: 提交 Issue 或 PR
- **社区讨论**: Discord / Feishu

---

**最后更新**: 2026-03-16  
**版本**: 1.0.0  
**许可证**: Apache License 2.0
