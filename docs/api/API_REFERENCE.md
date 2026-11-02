# XinYi API 参考文档

**生成时间**: 2026-03-01  
**版本**: 2.1

> 本页是索引，不是全量清单：只列每个组件最主要的入口，完整接口以 `components/<组件>/inc/` 下的头文件为准。
>
> 页内每个 C 代码块都由 CTest `doc_samples_compile` 对着它所描述的标头真实编译（`-fsyntax-only -Wall -Wextra -Werror`）。原型与标头不一致会直接失败，所以这里的返回类型、参数个数与参数类型与代码一致；**语义**（行为、所有权、错误码）仍由各组件文档负责，编译器判不了。

---

## 📚 目录

1. [OSAL - OS 抽象层](#osal)
2. [HAL - 硬件抽象层](#hal)
3. [传感器驱动](#sensors)
4. [数据管理](#dm)
5. [网络协议](#net)
6. [PID 控制](#pid)
7. [FOTA](#fota)
8. [GUI](#gui)

---

## OSAL <a name="osal"></a>

### 核心 API

#### xy_os_kernel_init

```c
xy_os_status_t xy_os_kernel_init(void);
```

初始化 OS 内核。

**返回值**:
- `XY_OS_OK`: 成功
- `XY_OS_ERROR`: 失败

---

#### xy_os_kernel_start

```c
xy_os_status_t xy_os_kernel_start(void);
```

启动 OS 内核调度器。

---

#### xy_os_thread_new

```c
xy_os_thread_id_t xy_os_thread_new(
    xy_os_thread_func_t func,
    void *argument,
    const xy_os_thread_attr_t *attr
);
```

创建线程/任务，返回线程 ID，失败返回 `NULL`。`attr` 传 `NULL` 表示使用默认属性；名称、优先级、栈内存都在 `xy_os_thread_attr_t` 里。

**没有** `xy_os_thread_create()`，也没有 `xy_os_thread_t`：线程句柄类型是 `xy_os_thread_id_t`，创建入口是 `xy_os_thread_new()`。把名称、优先级、栈内存逐个当参数传的写法是旧草案，标头里从来没有过。

**参数**:
- `func`: 线程函数（`void (*)(void *)`）
- `argument`: 传给线程函数的参数
- `attr`: 线程属性（`NULL` = 默认）

---

## HAL <a name="hal"></a>

### GPIO API

#### xy_hal_gpio_init

```c
xy_hal_error_t xy_hal_gpio_init(
    xy_hal_gpio_port_t port,
    uint8_t pin,
    const xy_hal_gpio_config_t *config
);
```

初始化 GPIO 引脚。`pin` 是 `uint8_t`（0–15）；`xy_hal_gpio_port_t` 在嵌入式平台是不透明指针，在 PC 仿真层同样按端口句柄传递。

---

## 传感器驱动 <a name="sensors"></a>

### SHT30

#### xy_sht30_init_addr / xy_sht30_init

```c
int xy_sht30_init_addr(xy_sht30_t *sht, void *i2c_handle, uint16_t i2c_addr);
```

初始化 SHT30 传感器并指定 I2C 地址。`xy_sht30_init(sht, i2c_handle)` 是 `xy_sht30.h` 里的 `static inline` 包装，固定用默认地址 `0x44`；**不存在**接受地址参数的 `xy_sht30_init()`，要换地址就得调 `_addr` 版本。

---

#### xy_sht30_read

```c
int xy_sht30_read(xy_sht30_t *sht);
```

读取温湿度数据。

**输出**:
- `sht->temperature`: 温度 (0.01°C)
- `sht->humidity`: 湿度 (0.01%RH)

---

### MPU6050

#### xy_mpu6050_read_accel

```c
int xy_mpu6050_read_accel(
    xy_mpu6050_t *dev,
    float *ax, float *ay, float *az
);
```

读取加速度数据 (单位：g)。

---

## PID 控制 <a name="pid"></a>

#### xy_pid_init

```c
int xy_pid_init(xy_pid_t *pid, const xy_pid_config_t *config);
```

初始化 PID 控制器。

---

#### xy_pid_compute

```c
int xy_pid_compute(
    xy_pid_t *pid,
    float input,
    float *output
);
```

计算 PID 输出。

---

## FOTA <a name="fota"></a>

#### xy_fota_init

```c
int xy_fota_init(xy_fota_t *fota, const xy_fota_config_t *config);
```

初始化 FOTA 系统。

---

#### xy_fota_download_chunk

```c
int xy_fota_download_chunk(
    xy_fota_t *fota,
    const uint8_t *data,
    uint32_t size
);
```

下载固件数据块。

---

## GUI <a name="gui"></a>

#### xy_oled_ssd1306_init

```c
int xy_oled_ssd1306_init(
    xy_oled_ssd1306_t *dev,
    void *i2c_handle
);
```

初始化 OLED 显示屏。

---

#### xy_font_draw_string

```c
int xy_font_draw_string(
    const xy_font_t *font,
    const char *text,
    int16_t x, int16_t y,
    uint16_t color,
    void *framebuffer,
    uint16_t fb_width, uint16_t fb_height
);
```

绘制字符串。

---

**文档维护者**: XinYi Team  
**许可证**: Apache License 2.0
