# XinYi 设备组件架构设计 (完整版)

## 1. 架构概述

XinYi 设备组件架构结合了 RT-Thread 的易用性和 Zephyr 的规范性，旨在为嵌入式系统提供统一、高效、可维护的设备管理框架。

### 1.1 设计目标

- **统一接口**: 所有设备使用统一的访问接口
- **模块化设计**: 按功能分类，松耦合设计
- **可扩展性**: 支持新设备类型和驱动的动态添加
- **可配置性**: 通过 Kconfig 进行编译时裁剪
- **易用性**: 简单的 API，清晰的文档
- **兼容性**: 与现有 HAL 框架无缝集成
- **性能优化**: 针对嵌入式系统优化
- **安全性**: 完善的参数验证和错误处理

### 1.2 架构层次

```
┌─────────────────────────────────────────────────────────────────┐
│                    应用层 (Applications)                        │
│            使用 xy_device_* 统一接口访问设备                    │
├─────────────────────────────────────────────────────────────────┤
│                设备管理层 (xy_device.h)                         │
│          统一设备结构 + 设备管理 + 注册/查找机制                │
├─────────────────────────────────────────────────────────────────┤
│              能力接口层 (xy_dev_i2c.h/xy_dev_spi.h/...)         │
│        xy_dev_api.h 仅作为 legacy 兼容聚合头                    │
├─────────────────────────────────────────────────────────────────┤
│            设备驱动实现层 (xy_dev_*.c)                          │
│              针对特定硬件的驱动实现                            │
├─────────────────────────────────────────────────────────────────┤
│              硬件抽象层 (xy_hal_*.h)                           │
│            与 MCU 无关的统一硬件接口                           │
└─────────────────────────────────────────────────────────────────┘
```

## 2. 核心数据结构设计

### 2.1 统一设备结构

```c
/**
 * @brief 设备类型枚举
 */
typedef enum {
    XY_DEV_TYPE_ADC = 0,        /**< ADC 设备 */
    XY_DEV_TYPE_DAC,            /**< DAC 设备 */
    XY_DEV_TYPE_UART,           /**< UART 设备 */
    XY_DEV_TYPE_SPI,            /**< SPI 设备 */
    XY_DEV_TYPE_I2C,            /**< I2C 设备 */
    XY_DEV_TYPE_GPIO,           /**< GPIO 设备 */
    XY_DEV_TYPE_PWM,            /**< PWM 设备 */
    XY_DEV_TYPE_TIMER,          /**< 定时器设备 */
    XY_DEV_TYPE_RTC,            /**< RTC 设备 */
    XY_DEV_TYPE_WDG,            /**< 看门狗设备 */
    XY_DEV_TYPE_FLASH,          /**< Flash 设备 */
    XY_DEV_TYPE_SENSOR,         /**< 传感器设备 */
    XY_DEV_TYPE_STORAGE,        /**< 存储设备 */
    XY_DEV_TYPE_BUS,            /**< 总线设备 */
    XY_DEV_TYPE_MISC,           /**< 杂项设备 */
    XY_DEV_TYPE_MAX
} xy_dev_type_t;

/**
 * @brief 设备状态枚举
 */
typedef enum {
    XY_DEV_STATE_INIT = 0,      /**< 初始化状态 */
    XY_DEV_STATE_READY,         /**< 准备就绪 */
    XY_DEV_STATE_OPENED,        /**< 已打开 */
    XY_DEV_STATE_BUSY,          /**< 忙碌状态 */
    XY_DEV_STATE_ERROR,         /**< 错误状态 */
    XY_DEV_STATE_SUSPENDED,     /**< 挂起状态 */
    XY_DEV_STATE_CLOSED,        /**< 关闭状态 */
} xy_dev_state_t;

/**
 * @brief 设备标志枚举
 */
typedef enum {
    XY_DEV_FLAG_RDWR      = 0x0001, /**< 可读写 */
    XY_DEV_FLAG_RDONLY    = 0x0002, /**< 只读 */
    XY_DEV_FLAG_WRONLY    = 0x0004, /**< 只写 */
    XY_DEV_FLAG_STREAM    = 0x0008, /**< 流设备 */
    XY_DEV_FLAG_BLOCK     = 0x0010, /**< 块设备 */
    XY_DEV_FLAG_INT       = 0x0020, /**< 支持中断 */
    XY_DEV_FLAG_DMA       = 0x0040, /**< 支持 DMA */
    XY_DEV_FLAG_ASYNC     = 0x0080, /**< 支持异步操作 */
    XY_DEV_FLAG_POLL      = 0x0100, /**< 支持轮询 */
    XY_DEV_FLAG_EVENT     = 0x0200, /**< 支持事件 */
    XY_DEV_FLAG_POWER_MGMT = 0x0400, /**< 支持电源管理 */
    XY_DEV_FLAG_REENTRANT  = 0x0800, /**< 可重入 */
    XY_DEV_FLAG_THREAD_SAFE = 0x1000, /**< 线程安全 */
} xy_dev_flag_t;

/**
 * @brief 通用设备操作集
 */
typedef struct xy_dev_api {
    xy_error_t (*init)(struct xy_device *dev, const void *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*open)(struct xy_device *dev, uint32_t flags);
    xy_error_t (*close)(struct xy_device *dev);
    int32_t (*read)(struct xy_device *dev, uint32_t pos, void *buf, size_t size);
    int32_t (*write)(struct xy_device *dev, uint32_t pos, const void *buf, size_t size);
    xy_error_t (*control)(struct xy_device *dev, uint32_t cmd, void *args);
    xy_error_t (*async_read)(struct xy_device *dev, uint32_t pos, void *buf, 
                            size_t size, xy_async_callback_t cb, void *arg);
    xy_error_t (*async_write)(struct xy_device *dev, uint32_t pos, const void *buf,
                             size_t size, xy_async_callback_t cb, void *arg);
    xy_error_t (*ioctl)(struct xy_device *dev, uint32_t cmd, void *args);
    xy_error_t (*power_control)(struct xy_device *dev, uint8_t power_mode);
    xy_error_t (*register_callback)(struct xy_device *dev, 
                                   xy_async_callback_t callback, void *arg);
} xy_dev_api_t;

/**
 * @brief 设备结构
 */
typedef struct xy_device {
    const char *name;                 /**< 设备名称 */
    xy_dev_type_t type;               /**< 设备类型 */
    uint32_t flags;                   /**< 设备标志 */
    xy_dev_state_t state;             /**< 设备状态 */
    const xy_dev_api_t *api;          /**< 驱动 API 结构 */
    const void *config;               /**< 设备配置 (编译时) */
    void *data;                       /**< 设备私有数据 (运行时) */
    uint8_t ref_count;                /**< 引用计数 */
    uint8_t power_mode;               /**< 电源模式 */
    struct xy_device *next;           /**< 链表指针 */
} xy_device_t;
```

### 2.2 设备控制命令

```c
/**
 * @brief 设备控制命令
 */
typedef enum {
    XY_DEV_CMD_CONFIG = 0,          /**< 配置设备 */
    XY_DEV_CMD_ENABLE,              /**< 使能设备 */
    XY_DEV_CMD_DISABLE,             /**< 禁用设备 */
    XY_DEV_CMD_RESET,               /**< 复位设备 */
    XY_DEV_CMD_GET_INFO,            /**< 获取设备信息 */
    XY_DEV_CMD_SET_CALLBACK,        /**< 设置回调 */
    XY_DEV_CMD_GET_STATE,           /**< 获取设备状态 */
    XY_DEV_CMD_SET_POWER,           /**< 设置电源模式 */
    XY_DEV_CMD_GET_POWER,           /**< 获取电源模式 */
    XY_DEV_CMD_SUSPEND,             /**< 挂起设备 */
    XY_DEV_CMD_RESUME,              /**< 恢复设备 */
    XY_DEV_CMD_LOCK,                /**< 锁定设备 */
    XY_DEV_CMD_UNLOCK,              /**< 解锁设备 */
    XY_DEV_CMD_FLUSH,               /**< 刷新设备 */
    XY_DEV_CMD_SET_TIMEOUT,         /**< 设置超时 */
    XY_DEV_CMD_GET_TIMEOUT,         /**< 获取超时 */
    XY_DEV_CMD_SET_BAUDRATE,        /**< 设置波特率 */
    XY_DEV_CMD_GET_BAUDRATE,        /**< 获取波特率 */
    XY_DEV_CMD_SET_FREQUENCY,       /**< 设置频率 */
    XY_DEV_CMD_GET_FREQUENCY,       /**< 获取频率 */
    XY_DEV_CMD_SET_DUTY_CYCLE,      /**< 设置占空比 */
    XY_DEV_CMD_GET_DUTY_CYCLE,      /**< 获取占空比 */
    XY_DEV_CMD_SET_MODE,            /**< 设置模式 */
    XY_DEV_CMD_GET_MODE,            /**< 获取模式 */
    XY_DEV_CMD_SET_SPEED,           /**< 设置速度 */
    XY_DEV_CMD_GET_SPEED,           /**< 获取速度 */
    XY_DEV_CMD_SET_POLARITY,        /**< 设置极性 */
    XY_DEV_CMD_GET_POLARITY,        /**< 获取极性 */
} xy_dev_cmd_t;
```

## 3. 分类驱动 API 结构

### 3.1 UART 驱动 API

```c
/**
 * @brief UART 配置结构
 */
typedef struct {
    uint32_t baudrate;              /**< 波特率 */
    uint8_t wordlen;                /**< 字长 (8/9 位) */
    uint8_t stopbits;               /**< 停止位 (1/2) */
    uint8_t parity;                 /**< 校验 (0=无, 1=奇, 2=偶) */
    uint8_t flowctrl;               /**< 流控制 (0=无, 1=RTS, 2=CTS, 3=RTS+CTS) */
    uint8_t mode;                   /**< 模式 (TX/RX/双工) */
} xy_uart_config_t;

/**
 * @brief UART 驱动 API 结构
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_uart_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*send)(struct xy_device *dev, const uint8_t *data, size_t len, uint32_t timeout);
    int32_t (*recv)(struct xy_device *dev, uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*flush)(struct xy_device *dev);
    xy_error_t (*set_baudrate)(struct xy_device *dev, uint32_t baudrate);
    uint32_t (*get_baudrate)(struct xy_device *dev);
    xy_error_t (*enable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*disable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*async_send)(struct xy_device *dev, const uint8_t *data, size_t len,
                            xy_async_callback_t cb, void *arg);
    xy_error_t (*async_recv)(struct xy_device *dev, uint8_t *data, size_t len,
                            xy_async_callback_t cb, void *arg);
} xy_uart_api_t;
```

### 3.2 SPI 驱动 API

```c
/**
 * @brief SPI 模式
 */
typedef enum {
    XY_SPI_MODE_0 = 0,              /**< CPOL=0, CPHA=0 */
    XY_SPI_MODE_1,                  /**< CPOL=0, CPHA=1 */
    XY_SPI_MODE_2,                  /**< CPOL=1, CPHA=0 */
    XY_SPI_MODE_3,                  /**< CPOL=1, CPHA=1 */
} xy_spi_mode_t;

/**
 * @brief SPI 配置结构
 */
typedef struct {
    xy_spi_mode_t mode;              /**< SPI 模式 */
    uint8_t direction;               /**< 传输方向 */
    uint8_t datasize;                /**< 数据大小 */
    uint8_t bitorder;                /**< 位顺序 */
    uint8_t nss_mode;                /**< NSS 模式 */
    uint32_t baudrate;               /**< 波特率 */
    uint8_t is_master;               /**< 是否主机 */
} xy_spi_config_t;

/**
 * @brief SPI 驱动 API 结构
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_spi_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*transfer)(struct xy_device *dev, const uint8_t *tx_data,
                       uint8_t *rx_data, size_t size, uint32_t timeout);
    xy_error_t (*set_speed)(struct xy_device *dev, uint32_t speed);
    uint32_t (*get_speed)(struct xy_device *dev);
    xy_error_t (*set_mode)(struct xy_device *dev, xy_spi_mode_t mode);
    xy_error_t (*async_transfer)(struct xy_device *dev, const uint8_t *tx_data,
                                uint8_t *rx_data, size_t size,
                                xy_async_callback_t cb, void *arg);
} xy_spi_api_t;
```

### 3.3 I2C 驱动 API

```c
/**
 * @brief I2C 地址模式
 */
typedef enum {
    XY_I2C_ADDR_7BIT = 0,           /**< 7 位地址 */
    XY_I2C_ADDR_10BIT,              /**< 10 位地址 */
} xy_i2c_addr_mode_t;

/**
 * @brief I2C 配置结构
 */
typedef struct {
    uint32_t clock_speed;           /**< 时钟速度 (Hz) */
    xy_i2c_addr_mode_t addr_mode;   /**< 地址模式 */
    uint8_t duty_cycle;             /**< 占空比 */
    uint16_t own_address;           /**< 自己地址 */
    uint8_t general_call_mode;      /**< 通用呼叫模式 */
} xy_i2c_config_t;

/**
 * @brief I2C 驱动 API 结构
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_i2c_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*master_send)(struct xy_device *dev, uint16_t dev_addr,
                             const uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*master_recv)(struct xy_device *dev, uint16_t dev_addr,
                             uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*mem_write)(struct xy_device *dev, uint16_t dev_addr,
                           uint16_t mem_addr, const uint8_t *data, size_t len,
                           uint32_t timeout);
    xy_error_t (*mem_read)(struct xy_device *dev, uint16_t dev_addr,
                          uint16_t mem_addr, uint8_t *data, size_t len,
                          uint32_t timeout);
    xy_error_t (*is_device_ready)(struct xy_device *dev, uint16_t dev_addr,
                                  uint32_t trials, uint32_t timeout);
    xy_error_t (*async_transfer)(struct xy_device *dev, uint16_t dev_addr,
                                const uint8_t *tx_data, uint8_t *rx_data, size_t size,
                                xy_async_callback_t cb, void *arg);
} xy_i2c_api_t;
```

## 4. 总线模型设计

### 4.1 总线设备结构

```c
/**
 * @brief 总线操作 API 结构
 */
typedef struct xy_bus_api {
    xy_error_t (*take_bus)(struct xy_device *bus);
    xy_error_t (*release_bus)(struct xy_device *bus);
    xy_error_t (*transfer)(struct xy_device *bus, struct xy_device *node,
                          const void *send_buf, void *recv_buf, size_t length);
    xy_error_t (*configure)(struct xy_device *bus, struct xy_device *node,
                           const void *config);
    xy_error_t (*set_speed)(struct xy_device *bus, struct xy_device *node, uint32_t speed);
    uint32_t (*get_speed)(struct xy_device *bus, struct xy_device *node);
} xy_bus_api_t;

/**
 * @brief 总线设备结构
 */
typedef struct xy_bus_device {
    xy_device_t parent;              /**< 父设备 */
    const xy_bus_api_t *bus_api;     /**< 总线操作 API */
    uint32_t speed;                  /**< 总线速度 */
    void *bus_data;                  /**< 总线私有数据 */
    uint8_t node_count;              /**< 节点数量 */
} xy_bus_device_t;

/**
 * @brief 总线节点结构
 */
typedef struct xy_bus_node {
    xy_device_t parent;              /**< 设备节点 */
    xy_bus_device_t *bus;            /**< 所属总线 */
    uint32_t addr;                   /**< 设备地址 */
    void *node_data;                 /**< 节点私有数据 */
} xy_bus_node_t;

/* 总线操作函数 */
xy_error_t xy_bus_take(xy_bus_device_t *bus);
xy_error_t xy_bus_release(xy_bus_device_t *bus);
xy_error_t xy_bus_transfer(xy_bus_device_t *bus, xy_bus_node_t *node,
                          const void *send_buf, void *recv_buf, size_t length);
```

## 5. 目录结构设计

```
components/device/                 # 设备组件根目录
├── inc/                          # 公共头文件
│   ├── xy_device.h               # 设备框架接口
│   ├── xy_dev_api.h              # 兼容聚合头（通用设备 API 归属 xy_device.h）
│   ├── xy_dev_uart.h             # UART 驱动接口
│   ├── xy_dev_spi.h              # SPI 驱动接口
│   ├── xy_dev_i2c.h              # I2C 驱动接口
│   ├── xy_dev_adc.h              # ADC 驱动接口
│   ├── xy_dev_gpio.h             # GPIO 驱动接口
│   ├── xy_dev_pwm.h              # PWM 驱动接口
│   ├── xy_dev_timer.h            # Timer 驱动接口
│   ├── xy_dev_rtc.h              # RTC 驱动接口
│   ├── xy_dev_wdg.h              # Watchdog 驱动接口
│   ├── xy_dev_sensor.h           # 传感器驱动接口
│   └── xy_dev_bus.h              # 总线驱动接口
│
├── src/                          # 源文件
│   ├── xy_device.c               # 设备框架实现
│   ├── xy_dev_uart.c             # UART 通用实现
│   ├── xy_dev_spi.c              # SPI 通用实现
│   ├── xy_dev_i2c.c              # I2C 通用实现
│   └── ...
│
├── bus/                          # 总线驱动
│   ├── xy_bus_spi.c              # SPI 总线
│   ├── xy_bus_i2c.c              # I2C 总线
│   └── xy_bus_can.c              # CAN 总线
│
├── sensor/                       # 传感器驱动
│   ├── xy_sensor_temp.c          # 温度传感器
│   ├── xy_sensor_acc.c           # 加速度传感器
│   ├── xy_sensor_gyro.c          # 陀螺仪
│   └── ...
│
├── mcu/                          # MCU 相关驱动
│   ├── stm32/                    # STM32 系列
│   │   ├── stm32u5/              # STM32U5 系列
│   │   │   ├── xy_dev_uart_stm32u5.c
│   │   │   ├── xy_dev_spi_stm32u5.c
│   │   │   ├── xy_dev_i2c_stm32u5.c
│   │   │   ├── xy_dev_adc_stm32u5.c
│   │   │   ├── xy_dev_gpio_stm32u5.c
│   │   │   └── ...
│   │   └── ...
│   ├── gd32/                     # GD32 系列
│   ├── ch32/                     # CH32 系列
│   └── ...
│
├── tests/                        # 测试
│   ├── test_device.c             # 设备框架测试
│   ├── test_uart.c               # UART 测试
│   ├── test_spi.c                # SPI 测试
│   └── ...
│
├── examples/                     # 示例
│   ├── device_usage.c            # 设备使用示例
│   ├── uart_example.c            # UART 示例
│   └── ...
│
├── docs/                         # 文档
│   ├── device_model.md           # 设备模型文档
│   ├── driver_dev.md             # 驱动开发指南
│   └── ...
│
├── CMakeLists.txt                # CMake 构建配置
├── Kconfig                       # Kconfig 配置
└── Makefile                      # Make 构建配置
```

## 6. 构建系统设计

### 6.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.12)
project(xy_device C)

# Configuration options
option(XY_DEVICE_UART_ENABLED "Enable UART device support" ON)
option(XY_DEVICE_SPI_ENABLED "Enable SPI device support" ON)
option(XY_DEVICE_I2C_ENABLED "Enable I2C device support" ON)
option(XY_DEVICE_GPIO_ENABLED "Enable GPIO device support" ON)
option(XY_DEVICE_ADC_ENABLED "Enable ADC device support" ON)
option(XY_DEVICE_SENSOR_ENABLED "Enable sensor device support" OFF)

# Common sources
set(DEVICE_COMMON_SOURCES
    src/xy_device.c
)

# Device-specific sources
if(XY_DEVICE_UART_ENABLED)
    list(APPEND DEVICE_COMMON_SOURCES src/xy_dev_uart.c)
endif()

if(XY_DEVICE_SPI_ENABLED)
    list(APPEND DEVICE_COMMON_SOURCES src/xy_dev_spi.c)
endif()

if(XY_DEVICE_I2C_ENABLED)
    list(APPEND DEVICE_COMMON_SOURCES src/xy_dev_i2c.c)
endif()

# Create library
add_library(xy_device STATIC ${DEVICE_COMMON_SOURCES})

target_include_directories(xy_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
)

target_link_libraries(xy_device PRIVATE
    xy_hal
)

# Compile definitions
target_compile_definitions(xy_device PUBLIC
    XY_DEVICE_ENABLED
    $<$<BOOL:${XY_DEVICE_UART_ENABLED}>:XY_DEVICE_UART_ENABLED>
    $<$<BOOL:${XY_DEVICE_SPI_ENABLED}>:XY_DEVICE_SPI_ENABLED>
    $<$<BOOL:${XY_DEVICE_I2C_ENABLED}>:XY_DEVICE_I2C_ENABLED>
    $<$<BOOL:${XY_DEVICE_GPIO_ENABLED}>:XY_DEVICE_GPIO_ENABLED>
    $<$<BOOL:${XY_DEVICE_ADC_ENABLED}>:XY_DEVICE_ADC_ENABLED>
    $<$<BOOL:${XY_DEVICE_SENSOR_ENABLED}>:XY_DEVICE_SENSOR_ENABLED>
)
```

### 6.2 Kconfig

```
menu "XY Device Configuration"

config XY_DEVICE_ENABLED
    bool "Enable XY Device Framework"
    default y
    help
      Enable the XY Device framework for unified device management.

if XY_DEVICE_ENABLED

config XY_DEVICE_MAX_COUNT
    int "Maximum number of devices"
    default 32
    range 8 256
    help
      Maximum number of devices that can be registered.

config XY_DEVICE_UART_ENABLED
    bool "Enable UART Device Support"
    default y
    help
      Enable UART device driver support.

config XY_DEVICE_SPI_ENABLED
    bool "Enable SPI Device Support"
    default y
    help
      Enable SPI device driver support.

config XY_DEVICE_I2C_ENABLED
    bool "Enable I2C Device Support"
    default y
    help
      Enable I2C device driver support.

config XY_DEVICE_GPIO_ENABLED
    bool "Enable GPIO Device Support"
    default y
    help
      Enable GPIO device driver support.

config XY_DEVICE_ADC_ENABLED
    bool "Enable ADC Device Support"
    default y
    help
      Enable ADC device driver support.

config XY_DEVICE_SENSOR_ENABLED
    bool "Enable Sensor Device Support"
    default n
    depends on XY_DEVICE_ADC_ENABLED
    help
      Enable sensor device support.

endif # XY_DEVICE_ENABLED

endmenu
```

## 7. 使用示例

### 7.1 设备使用示例

```c
/**
 * @brief XinYi 设备组件使用示例
 */
#include "xy_device.h"

void device_usage_example(void)
{
    // 1. 查找设备
    xy_device_t *uart1 = xy_device_find("uart1");
    if (!uart1) {
        xy_log_e("UART1 device not found\n");
        return;
    }

    // 2. 打开设备
    xy_device_t *dev = xy_device_open("uart1", XY_DEV_FLAG_RDWR);
    if (!dev) {
        xy_log_e("Failed to open UART1\n");
        return;
    }

    // 3. 发送数据
    const char *msg = "Hello from XinYi Device Framework!\r\n";
    int32_t ret = xy_device_write(dev, 0, msg, strlen(msg));
    if (ret < 0) {
        xy_log_e("UART write failed: %d\n", ret);
    }

    // 4. 关闭设备
    xy_device_close(dev);
}

/* 总线设备使用示例 */
void bus_device_example(void)
{
    // 1. 获取 SPI 总线
    xy_device_t *spi_bus = xy_device_find("spi1");
    if (!spi_bus) return;

    xy_bus_device_t *bus = (xy_bus_device_t *)spi_bus;

    // 2. 获取 SPI 设备 (节点)
    xy_device_t *spi_node = xy_device_find("spi_flash");
    if (!spi_node) return;

    xy_bus_node_t *node = (xy_bus_node_t *)spi_node;

    // 3. 配置 SPI 设备
    xy_spi_config_t spi_config = {
        .mode = XY_SPI_MODE_0,
        .direction = 0, // 2 线全双工
        .datasize = 8,  // 8 位数据
        .bitorder = 0,  // MSB 先传
        .baudrate = 1000000, // 1MHz
        .is_master = 1,
    };

    xy_device_control(spi_node, XY_SPI_CMD_SET_CONFIG, &spi_config);

    // 4. 传输数据
    const uint8_t tx_data[] = {0x9F}; // 读 JEDEC ID 命令
    uint8_t rx_data[3] = {0};

    xy_bus_take(bus);
    xy_bus_transfer(bus, node, tx_data, rx_data, sizeof(tx_data));
    xy_bus_release(bus);

    xy_log_i("JEDEC ID: 0x%02X%02X%02X\n", 
             rx_data[0], rx_data[1], rx_data[2]);
}
```

### 7.2 驱动开发示例

```c
/**
 * @file xy_dev_uart_stm32u5.c
 * @brief UART 驱动 STM32U5 实现
 */

#include "../inc/xy_dev_uart.h"
#include "xy_hal_uart.h"

/* UART 私有数据 */
typedef struct {
    UART_HandleTypeDef *huart;
    xy_uart_config_t config;
    uint8_t initialized;
} xy_uart_stm32u5_data_t;

/* UART 驱动 API */
static xy_error_t uart_stm32u5_init(struct xy_device *dev, const xy_uart_config_t *config);
static xy_error_t uart_stm32u5_deinit(struct xy_device *dev);
static int32_t uart_stm32u5_send(struct xy_device *dev, const uint8_t *data, 
                                size_t len, uint32_t timeout);
// ... 其他函数

static const xy_uart_api_t uart_stm32u5_api = {
    .init = uart_stm32u5_init,
    .deinit = uart_stm32u5_deinit,
    .send = uart_stm32u5_send,
    .recv = uart_stm32u5_recv,
    .flush = uart_stm32u5_flush,
    .set_baudrate = uart_stm32u5_set_baudrate,
    .get_baudrate = uart_stm32u5_get_baudrate,
};

/* 静态设备注册 */
XY_DEVICE_STATIC_REGISTER(uart1, XY_DEV_TYPE_UART, 
                         uart_stm32u5_init, &uart_stm32u5_api, &uart1_config);
```

## 8. 错误处理设计

### 8.1 统一错误码

```c
/* 继承自 xy_hal.h 的错误码 */
typedef enum {
    XY_OK = 0,                    /**< 成功 */
    XY_ERROR = -1,                /**< 通用错误 */
    XY_ERROR_INVALID_PARAM = -2,  /**< 无效参数 */
    XY_ERROR_NOT_SUPPORT = -3,    /**< 不支持 */
    XY_ERROR_TIMEOUT = -4,        /**< 超时 */
    XY_ERROR_BUSY = -5,           /**< 忙碌 */
    XY_ERROR_NO_MEMORY = -6,      /**< 内存不足 */
    XY_ERROR_IO = -7,             /**< I/O 错误 */
    XY_ERROR_NOT_INIT = -8,       /**< 未初始化 */
    XY_ERROR_ALREADY_INIT = -9,   /**< 已初始化 */
    XY_ERROR_NO_RESOURCE = -10,   /**< 无资源 */
    XY_ERROR_FAIL = -11,          /**< 失败 */
    // ... 更多错误码
} xy_error_t;
```

### 8.2 错误处理宏

```c
/* 错误处理宏 */
#define XY_RETURN_ON_ERROR(expr) \
    do { \
        xy_error_t _err = (expr); \
        if (_err != XY_OK) { \
            return _err; \
        } \
    } while(0)

#define XY_RETURN_VAL_ON_ERROR(expr, val) \
    do { \
        xy_error_t _err = (expr); \
        if (_err != XY_OK) { \
            return (val); \
        } \
    } while(0)

#define XY_BREAK_ON_ERROR(expr) \
    do { \
        xy_error_t _err = (expr); \
        if (_err != XY_OK) { \
            break; \
        } \
    } while(0)
```

## 9. 与现有系统集成

### 9.1 与 HAL 集成

```
设备层 (xy_device) ←→ 硬件抽象层 (xy_hal) ←→ MCU HAL
    ↑                    ↑                     ↑
  统一接口              统一接口                具体实现
```

### 9.2 与 OSAL 集成

- 设备操作可以在不同 RTOS 环境下运行
- 异步操作与 OSAL 事件系统集成
- 电源管理与 OSAL 低功耗模式集成

## 10. 智能代理集成

### 10.1 代理功能

```bash
# 项目经理代理
./.qwen/smart_agent.sh pm status

# 架构师代理
./.qwen/smart_agent.sh arch review device

# 开发工程师代理
./.qwen/smart_agent.sh dev create uart_driver

# 测试工程师代理
./.qwen/smart_agent.sh test gen device
```

### 10.2 自动化工作流

```bash
# 创建自动化工作流
cat > automated_device_workflow.sh << 'EOF'
#!/bin/bash
# 自动化设备组件开发工作流

DEVICE_NAME=$1

if [ -z "$DEVICE_NAME" ]; then
    echo "Usage: $0 <device_name>"
    exit 1
fi

echo "=== 自动化设备开发: $DEVICE_NAME ==="

# 1. 生成模板文件
xy_gen_device_template $DEVICE_NAME

# 2. 审查代码
./.qwen/smart_agent.sh arch review $DEVICE_NAME

# 3. 生成测试
./.qwen/smart_agent.sh test gen $DEVICE_NAME

# 4. 运行测试
./.qwen/smart_agent.sh test run $DEVICE_NAME

# 5. 更新文档
./.qwen/smart_agent.sh dev docs $DEVICE_NAME

echo "=== 设备开发完成: $DEVICE_NAME ==="
EOF
```

## 11. 性能优化

### 11.1 编译时优化

- 静态注册减少运行时开销
- 宏定义减少函数调用开销
- 代码裁剪减少 Flash 占用

### 11.2 运行时优化

- 链表操作优化
- 内存访问优化
- 中断响应优化

## 12. 安全性考虑

### 12.1 参数验证

- 所有函数进行参数验证
- 边界检查防止缓冲区溢出
- 空指针检查

### 12.2 访问控制

- 设备状态检查
- 引用计数管理
- 互斥访问控制

## 13. 可扩展性

### 13.1 新设备类型

- 通过添加新 API 结构支持
- 遵循统一接口规范

### 13.2 新 MCU 系列

- 在 mcu/ 目录下添加新子目录
- 实现对应的驱动适配层

### 13.3 新总线类型

- 在 bus/ 目录下添加实现
- 遵循总线模型规范

## 14. 维护策略

### 14.1 版本管理

- 与 XinYi 框架版本同步
- 保持向后兼容性

### 14.2 文档更新

- API 变更时同步更新文档
- 保持示例代码最新

### 14.3 测试覆盖

- 保持高测试覆盖率
- 定期运行回归测试

## 15. 未来扩展

### 15.1 功能扩展

- 支持更多设备类型 (Ethernet, USB, etc.)
- 高级电源管理
- 安全设备接口

### 15.2 平台扩展

- 支持更多 MCU 架构 (RISC-V, etc.)
- 支持更多 RTOS 后端

### 15.3 工具扩展

- 设备配置生成工具
- 驱动模板生成工具
- 自动化测试工具

---

**维护者**: XinYi Team  
**版本**: 2.0  
**日期**: 2026-02-28
