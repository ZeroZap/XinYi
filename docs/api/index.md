# API 参考

XinYi 嵌入式框架的 API 文档索引。

---

## 📖 导航

- [按组件浏览](#按组件浏览)
- [按功能浏览](#按功能浏览)
- [全局宏](#全局宏)
- [错误码](#错误码)
- [术语表](#术语表)

---

## 按组件浏览

### OSAL (OS 抽象层)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_os_kernel_init()` | 初始化内核 | `xy_os.h` |
| `xy_os_kernel_start()` | 启动内核 | `xy_os.h` |
| `xy_os_delay()` | 延时 | `xy_os.h` |
| `xy_os_thread_create()` | 创建任务 | `xy_os.h` |
| `xy_os_timer_sw_create()` | 创建定时器 | `xy_os_timer_sw.h` |

[详细 API](../components/osal/api-reference.md)

---

### HAL (硬件抽象层)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_hal_gpio_init()` | 初始化 GPIO | `xy_hal_gpio.h` |
| `xy_hal_uart_init()` | 初始化 UART | `xy_hal_uart.h` |
| `xy_hal_spi_init()` | 初始化 SPI | `xy_hal_spi.h` |
| `xy_hal_i2c_init()` | 初始化 I2C | `xy_hal_i2c.h` |
| `xy_hal_timer_init()` | 初始化 Timer | `xy_hal_timer.h` |

[详细 API](../components/hal/api-reference.md)

---

### Crypto (密码学)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_aes_init()` | 初始化 AES | `xy_tiny_crypto.h` |
| `xy_md5_hash()` | MD5 哈希 | `xy_tiny_crypto.h` |
| `xy_sha256_hash()` | SHA256 哈希 | `xy_tiny_crypto.h` |
| `xy_hmac_sha256()` | HMAC-SHA256 | `xy_tiny_crypto.h` |
| `xy_base64_encode()` | Base64 编码 | `xy_tiny_crypto.h` |
| `xy_crc32()` | CRC32 校验 | `xy_tiny_crypto.h` |

[详细 API](../components/crypto/api-reference.md)

---

### CLib (C 库)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_filter_median()` | 中值滤波 | `xy_filter.h` |
| `xy_bubble_sort()` | 冒泡排序 | `xy_sort.h` |
| `xy_quick_sort()` | 快速排序 | `xy_sort.h` |
| `xy_memcpy()` | 内存复制 | `xy_string.h` |
| `xy_memset()` | 内存设置 | `xy_string.h` |

[详细 API](../components/clib/api-reference.md)

---

### DM (数据管理)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_tlv_encode_uint8()` | TLV 编码 | `xy_tlv.h` |
| `xy_tlv_decode_uint8()` | TLV 解码 | `xy_tlv.h` |
| `xy_tlv_iterator_init()` | 迭代器初始化 | `xy_tlv.h` |
| `xy_tlv_find_by_type()` | 按类型查找 | `xy_tlv.h` |

[详细 API](../components/dm/api-reference.md)

---

### NET (网络协议)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_iso7816_init()` | 初始化 ISO7816 | `xy_iso7816.h` |
| `xy_iso7816_apdu_send()` | 发送 APDU | `xy_iso7816.h` |
| `mb_slave_init()` | 初始化 Modbus | `mb_slave.h` |
| `mb_slave_process()` | 处理 Modbus | `mb_slave.h` |

[详细 API](../components/net/api-reference.md)

---

### Sensor (传感器)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_sensor_init()` | 初始化传感器 | `sensor_core.h` |
| `xy_sensor_read()` | 读取传感器 | `sensor_core.h` |
| `xy_sensor_calibrate()` | 校准传感器 | `sensor_calibration.h` |

[详细 API](../components/sensor/api-reference.md)

---

### IPC (进程间通信)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_pipe_init()` | 初始化管道 | `xy_pipe.h` |
| `xy_pipe_write()` | 写入管道 | `xy_pipe.h` |
| `xy_pipe_read()` | 读取管道 | `xy_pipe.h` |
| `xy_observer_init()` | 初始化观察者 | `xy_observer.h` |
| `xy_subject_notify()` | 通知观察者 | `xy_observer.h` |

[详细 API](../components/ipc/api-reference.md)

---

### PM (电源管理)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_charger_init()` | 初始化充电器 | `xy_charger.h` |
| `xy_charger_start()` | 开始充电 | `xy_charger.h` |
| `xy_fuel_gauge_init()` | 初始化电量计 | `xy_fuel_gauge.h` |
| `xy_fuel_gauge_get_soc()` | 获取 SOC | `xy_fuel_gauge.h` |

[详细 API](../components/pm/api-reference.md)

---

### PID (控制算法)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_pid_init()` | 初始化 PID | `xy_pid.h` |
| `xy_pid_compute()` | PID 计算 | `xy_pid.h` |
| `xy_pid_reset()` | 重置 PID | `xy_pid.h` |

[详细 API](../components/pid/api-reference.md)

---

### ADDC (ADC/DAC)

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `xy_adc_init()` | 初始化 ADC | `xy_adc.h` |
| `xy_adc_sample()` | ADC 采样 | `xy_adc.h` |
| `xy_dac_init()` | 初始化 DAC | `xy_adc.h` |
| `xy_dac_set_voltage()` | 设置 DAC 电压 | `xy_adc.h` |

[详细 API](../components/addc/api-reference.md)

---

## 按功能浏览

### 内存操作

| 函数 | 说明 |
|------|------|
| `xy_memcpy()` | 内存复制 |
| `xy_memset()` | 内存设置 |
| `xy_memcmp()` | 内存比较 |

### 字符串操作

| 函数 | 说明 |
|------|------|
| `xy_strlen()` | 字符串长度 |
| `xy_strcpy()` | 字符串复制 |
| `xy_strcat()` | 字符串连接 |
| `xy_strcmp()` | 字符串比较 |

### 数学运算

| 函数 | 说明 |
|------|------|
| `XY_MIN()` | 最小值 |
| `XY_MAX()` | 最大值 |
| `XY_CLAMP()` | 限幅 |
| `XY_SWAP()` | 交换 |

### 位操作

| 函数 | 说明 |
|------|------|
| `XY_BIT()` | 位掩码 |
| `BIT_SET()` | 置位 |
| `BIT_CLEAR()` | 清零 |
| `BIT_IS_SET()` | 检查位 |

---

## 全局宏

### 版本宏

```c
#define XY_VERSION_MAJOR    1
#define XY_VERSION_MINOR    0
#define XY_VERSION_PATCH    0
```

### 错误码

```c
#define XY_OK               0
#define XY_ERROR            (-1)
#define XY_INVALID_PARAM    (-2)
#define XY_BUFFER_TOO_SMALL (-3)
```

### 工具宏

```c
#define XY_MIN(x, y)        ((x) < (y) ? (x) : (y))
#define XY_MAX(x, y)        ((x) > (y) ? (x) : (y))
#define XY_CLAMP(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))
#define XY_SWAP(a, b)       do { typeof(a) _t = (a); (a) = (b); (b) = _t; } while (0)
#define XY_BIT(pos)         (1UL << (pos))
#define XY_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
```

---

## 错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `XY_OK` | 0 | 成功 |
| `XY_ERROR` | -1 | 通用错误 |
| `XY_INVALID_PARAM` | -2 | 无效参数 |
| `XY_BUFFER_TOO_SMALL` | -3 | 缓冲区太小 |
| `XY_NOT_FOUND` | -4 | 未找到 |
| `XY_TIMEOUT` | -5 | 超时 |
| `XY_NO_MEM` | -6 | 内存不足 |
| `XY_NOT_INIT` | -7 | 未初始化 |
| `XY_ALREADY_INIT` | -8 | 已初始化 |
| `XY_NOT_SUPPORTED` | -9 | 不支持 |

---

## 术语表

| 术语 | 说明 |
|------|------|
| **OSAL** | OS Abstraction Layer，操作系统抽象层 |
| **HAL** | Hardware Abstraction Layer，硬件抽象层 |
| **RTOS** | Real-Time Operating System，实时操作系统 |
| **GPIO** | General Purpose I/O，通用输入输出 |
| **UART** | Universal Asynchronous Receiver-Transmitter，通用异步收发传输器 |
| **SPI** | Serial Peripheral Interface，串行外设接口 |
| **I2C** | Inter-Integrated Circuit，集成电路总线 |
| **ADC** | Analog-to-Digital Converter，模数转换器 |
| **DAC** | Digital-to-Analog Converter，数模转换器 |
| **DMA** | Direct Memory Access，直接内存访问 |
| **PWM** | Pulse Width Modulation，脉冲宽度调制 |
| **RTC** | Real-Time Clock，实时时钟 |
| **TLV** | Tag-Length-Value，标签 - 长度 - 值 |
| **CRC** | Cyclic Redundancy Check，循环冗余校验 |
| **AES** | Advanced Encryption Standard，高级加密标准 |
| **HMAC** | Hash-based Message Authentication Code，基于哈希的消息认证码 |
| **SOC** | State of Charge，充电状态 |
| **SOH** | State of Health，健康状态 |
| **FOTA** | Firmware Over-The-Air，固件无线升级 |
| **GUI** | Graphical User Interface，图形用户界面 |

---

## 📞 获取帮助

- 📚 [组件文档](../components/index.md)
- ❓ [常见问题](../about/faq.md)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
