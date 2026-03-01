# Zephyr 设备组件架构分析

## 1. Zephyr 设备模型

### 1.1 设备结构 (device.h)

```c
/* Zephyr 核心设备结构 */
struct device {
    const struct device_config *config;    /* 设备配置 */
    const struct device_state *state;      /* 设备状态 */
    const void *api;                       /* 驱动 API 指针 */
};

struct device_config {
    const char *name;                      /* 设备名称 */
    int (*init)(const struct device *dev); /* 初始化函数 */
    const void *pm;                        /* 电源管理配置 */
    uint32_t base;                         /* 寄存器基地址 */
    void *irq_config;                      /* 中断配置函数 */
};
```

### 1.2 设备注册机制

Zephyr 使用编译时静态设备注册：

```c
/* 静态设备注册宏 */
#define DEVICE_DEFINE(node_id, drv_name, init_fn, pm_control, api_ptr, level, prio, ...) \
    static const STRUCT_SECTION_ITERABLE(device, Z_DEVICE_NAME_GET(node_id)) = { \
        .config = &(const struct device_config){ \
            .name = drv_name, \
            .init = init_fn, \
            .pm = pm_control, \
            .base = DT_REG_ADDR(node_id), \
            .irq_config = &Z_IRQ_CONNECT_NAME(node_id), \
        }, \
        .state = &Z_DEVICE_STATE_NAME(node_id), \
        .api = api_ptr, \
    }

/* 使用设备树集成 */
#define DEVICE_DT_DEFINE(node_id, init, pm_control, data, config, level, prio, api) \
    static const struct device_config config_##node_id = { \
        .name = DT_LABEL(node_id), \
        .init = init, \
        .pm = pm_control, \
    }; \
    static const struct device device_##node_id = { \
        .config = &config_##node_id, \
        .data = data, \
        .config = config, \
        .api = api, \
    };
```

## 2. Zephyr 驱动分类

### 2.1 串行通信驱动

```
drivers/
├── serial/
│   ├── uart.h                    # UART 公共 API
│   ├── uart.c                    # UART 通用实现
│   ├── uart_stm32.c              # STM32 UART 驱动
│   ├── uart_nrfx.c               # Nordic UART 驱动
│   └── ...
├── spi/
│   ├── spi.h                     # SPI 公共 API
│   ├── spi.c                     # SPI 通用实现
│   ├── spi_stm32.c               # STM32 SPI 驱动
│   └── ...
└── i2c/
    ├── i2c.h                     # I2C 公共 API
    ├── i2c.c                     # I2C 通用实现
    ├── i2c_stm32.c               # STM32 I2C 驱动
    └── ...
```

### 2.2 驱动 API 结构

Zephyr 使用 API 结构分离：

```c
/* UART 驱动 API 结构 */
struct uart_driver_api {
    int (*poll_in)(const struct device *dev, unsigned char *c);
    int (*poll_out)(const struct device *dev, unsigned char c);
    int (*err_check)(const struct device *dev);
    int (*configure)(const struct device *dev, 
                     const struct uart_config *cfg);
    int (*config_get)(const struct device *dev, 
                      struct uart_config *cfg);
};

/* UART 驱动实现 */
static const struct uart_driver_api uart_stm32_api = {
    .poll_in = uart_stm32_poll_in,
    .poll_out = uart_stm32_poll_out,
    .err_check = uart_stm32_err_check,
    .configure = uart_stm32_configure,
    .config_get = uart_stm32_config_get,
};
```

## 3. 设备树集成

### 3.1 设备树定义

```dts
/* 设备树源文件 (.dts) */
/ {
    soc {
        serial@40002800 {
            compatible = "st,stm32-uart";
            reg = <0x40002800 0x400>;
            interrupts = <38 1 0>;
            status = "okay";
            current-speed = <115200>;
        };
    };
};
```

### 3.2 设备树绑定

```yaml
# drivers/serial/stm32/uart.yaml
description: STMicroelectronics STM32 Universal Asynchronous Receiver/Transmitter
compatible: "st,stm32-uart"

include: [base.yaml, serial.yaml, interrupt-controller.yaml]

properties:
  reg:
    type: array
    required: true
  interrupts:
    type: array
    required: true
  clocks:
    type: phandle-array
    required: true
  current-speed:
    type: int
    required: false
    default: 115200
```

## 4. 电源管理集成

```c
/* 电源管理回调 */
static int device_pm_action(const struct device *dev,
                           enum pm_device_action action)
{
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        /* 设备暂停 */
        break;
    case PM_DEVICE_ACTION_RESUME:
        /* 设备恢复 */
        break;
    case PM_DEVICE_ACTION_TURN_OFF:
        /* 设备关闭 */
        break;
    case PM_DEVICE_ACTION_TURN_ON:
        /* 设备开启 */
        break;
    default:
        return -ENOTSUP;
    }
    
    return 0;
}

/* 在设备注册时包含 PM 控制 */
DEVICE_DEFINE(node_id,
              init_func,
              device_pm_action,  /* PM 控制函数 */
              data,
              config,
              level,
              priority,
              api);
```

## 5. 传感器框架

### 5.1 传感器 API

```c
/* 传感器类型 */
enum sensor_channel {
    SENSOR_CHAN_ACCEL_X,
    SENSOR_CHAN_ACCEL_Y,
    SENSOR_CHAN_ACCEL_Z,
    SENSOR_CHAN_ACCEL_XYZ,
    SENSOR_CHAN_GYRO_X,
    SENSOR_CHAN_GYRO_Y,
    SENSOR_CHAN_GYRO_Z,
    SENSOR_CHAN_GYRO_XYZ,
    // ... 更多传感器类型
};

/* 传感器值结构 */
struct sensor_value {
    int32_t val1;                   /* 整数部分 */
    int32_t val2;                   /* 小数部分 (百万分之一) */
};

/* 传感器驱动 API */
struct sensor_driver_api {
    int (*sample_fetch)(const struct device *dev, enum sensor_channel chan);
    int (*channel_get)(const struct device *dev,
                       enum sensor_channel chan,
                       struct sensor_value *val);
};
```

## 6. Zephyr 架构特点总结

### 6.1 优势

1. **编译时配置验证**: 通过设备树和 Kconfig 提供编译时验证
2. **API 结构分离**: 驱动实现与 API 结构分离，便于维护
3. **严格的代码质量**: 统一的代码风格和质量要求
4. **完善的电源管理**: 统一的 PM 框架
5. **模块化组织**: 按功能分类组织驱动代码

### 6.2 缺点

1. **学习曲线陡峭**: 设备树概念复杂
2. **静态注册灵活性差**: 不支持运行时动态注册
3. **构建时间较长**: 编译时验证增加了构建时间

### 6.3 与 RT-Thread 对比

| 特性 | Zephyr | RT-Thread | XinYi 建议 |
|------|--------|-----------|------------|
| **注册机制** | 静态 (编译时) | 动态 (运行时) | 混合模式 |
| **配置方式** | 设备树 + Kconfig | 代码内配置 | Kconfig + 简化设备树 |
| **API 风格** | API 结构分离 | 统一设备结构 | 统一结构 + API 分离 |
| **易用性** | 低 | 高 | 中 (平衡) |
| **验证时机** | 编译时 | 运行时 | 编译+运行时 |
