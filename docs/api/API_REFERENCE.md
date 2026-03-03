# XinYi API 参考文档

**生成时间**: 2026-03-01  
**版本**: 2.0

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
int xy_os_kernel_init(void);
```

初始化 OS 内核。

**返回值**:
- `XY_OS_OK`: 成功
- `XY_OS_ERROR`: 失败

---

#### xy_os_kernel_start

```c
int xy_os_kernel_start(void);
```

启动 OS 内核调度器。

---

#### xy_os_thread_create

```c
int xy_os_thread_create(
    xy_os_thread_t *thread,
    const char *name,
    xy_os_thread_func_t func,
    void *arg,
    uint8_t priority,
    uint8_t *stack,
    uint32_t stack_size
);
```

创建线程/任务。

**参数**:
- `thread`: 线程句柄
- `name`: 线程名称
- `func`: 线程函数
- `arg`: 线程参数
- `priority`: 优先级
- `stack`: 栈内存
- `stack_size`: 栈大小

---

## HAL <a name="hal"></a>

### GPIO API

#### xy_hal_gpio_init

```c
xy_hal_error_t xy_hal_gpio_init(
    void *port,
    uint16_t pin,
    const xy_hal_gpio_config_t *config
);
```

初始化 GPIO 引脚。

---

## 传感器驱动 <a name="sensors"></a>

### SHT30

#### xy_sht30_init

```c
int xy_sht30_init(xy_sht30_t *dev, void *i2c_handle, uint8_t addr);
```

初始化 SHT30 传感器。

---

#### xy_sht30_read

```c
int xy_sht30_read(xy_sht30_t *dev);
```

读取温湿度数据。

**输出**:
- `dev->temperature`: 温度 (0.01°C)
- `dev->humidity`: 湿度 (0.01%RH)

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
