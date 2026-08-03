# 示例代码

XinYi 嵌入式框架的示例代码和演示项目。

---

## 📖 导航

- [Hello World](#hello-world)
- [组件示例](#组件示例)
- [综合项目](#综合项目)
- [外部资源](#外部资源)

---

## Hello World

### 最小示例

```c
/**
 * @file hello_world.c
 * @brief XinYi Hello World
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"

int main(void) {
    xy_log_i("Hello, XinYi!\n");
    return 0;
}
```

**编译**:
```bash
gcc -I../components/trace/xy_log/inc hello_world.c -o hello_world
```

---

### 带 OS 的 Hello World

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"
#include "xy_os.h"

static void blink_task(void *arg) {
    (void)arg;
    
    while (1) {
        xy_log_i("Blink!\n");
        xy_os_delay(1000);
    }
}

int main(void) {
    xy_os_kernel_init();
    
    xy_os_thread_t thread;
    static uint8_t stack[512];
    
    xy_os_thread_create(
        &thread,
        "Blink",
        blink_task,
        NULL,
        5,
        stack,
        sizeof(stack)
    );
    
    xy_os_kernel_start();
    return 0;
}
```

---

## 组件示例

### OSAL 示例

#### 创建周期性任务

```c
#include "xy_os.h"
#include "xy_log.h"

static void periodic_task(void *arg) {
    (void)arg;
    
    while (1) {
        xy_log_i("Tick: %lu\n", xy_os_tick_get());
        xy_os_delay(1000);
    }
}

int main(void) {
    xy_os_kernel_init();
    
    xy_os_thread_t thread;
    static uint8_t stack[512];
    
    xy_os_thread_create(
        &thread,
        "Periodic",
        periodic_task,
        NULL,
        5,
        stack,
        sizeof(stack)
    );
    
    xy_os_kernel_start();
}
```

#### 软件定时器

```c
#include "xy_os.h"
#include "xy_log.h"

static void timer_callback(void *arg) {
    (void)arg;
    xy_log_i("Timer expired!\n");
}

int main(void) {
    xy_os_kernel_init();
    
    xy_os_timer_sw_t timer;
    timer = xy_os_timer_sw_create(
        "MyTimer",
        timer_callback,
        NULL,
        1000,  // 1 秒
        XY_OS_TIMER_PERIODIC
    );
    
    xy_os_timer_sw_start(timer);
    xy_os_kernel_start();
}
```

---

### Crypto 示例

#### AES 加密解密

```c
#include "xy_tiny_crypto.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    const uint8_t key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    const uint8_t plaintext[16] = "Hello, AES!     ";
    uint8_t ciphertext[16];
    uint8_t decrypted[16];
    
    xy_aes_ctx_t aes_ctx;
    xy_aes_init(&aes_ctx, key, XY_AES_KEY_SIZE_128);
    
    xy_aes_encrypt_block(&aes_ctx, plaintext, ciphertext);
    xy_aes_decrypt_block(&aes_ctx, ciphertext, decrypted);
    
    printf("AES 加密/解密成功！\n");
    return 0;
}
```

#### MD5 哈希

```c
#include "xy_tiny_crypto.h"
#include <stdio.h>

int main(void) {
    const char *data = "Hello, World!";
    uint8_t digest[XY_MD5_DIGEST_SIZE];
    
    xy_md5_hash((const uint8_t *)data, strlen(data), digest);
    
    printf("MD5: ");
    for (int i = 0; i < XY_MD5_DIGEST_SIZE; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");
    
    return 0;
}
```

#### Base64 编码

```c
#include "xy_tiny_crypto.h"
#include <stdio.h>

int main(void) {
    const char *data = "Hello, Base64!";
    char encoded[64];
    
    xy_base64_encode((const uint8_t *)data, strlen(data), encoded, sizeof(encoded));
    
    printf("Base64: %s\n", encoded);
    return 0;
}
```

---

### HAL 示例

#### GPIO 控制 LED

```c
#include "xy_hal_gpio.h"

int main(void) {
    xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_LOW,
    };
    
    xy_hal_gpio_init(GPIOA, 5, &config);
    
    while (1) {
        xy_hal_gpio_toggle(GPIOA, 5);
        xy_hal_delay_ms(1000);
    }
    
    return 0;
}
```

#### UART 发送数据

```c
#include "xy_hal_uart.h"

int main(void) {
    xy_hal_uart_config_t config = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
    };
    
    xy_hal_uart_init(UART1, &config);
    
    const char *msg = "Hello, UART!\r\n";
    xy_hal_uart_send(UART1, (const uint8_t *)msg, strlen(msg), 1000);
    
    return 0;
}
```

#### I2C 读取传感器

```c
#include "xy_hal_i2c.h"

int main(void) {
    xy_hal_i2c_config_t config = {
        .speed = XY_HAL_I2C_SPEED_STANDARD,
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
    };
    
    xy_hal_i2c_init(I2C1, &config);
    
    uint8_t data[2];
    xy_hal_i2c_mem_read(I2C1, 0x68, 0x00, data, 2, 1000);
    
    return 0;
}
```

---

### CLib 示例

#### 滤波算法

```c
#include "xy_filter.h"
#include <stdio.h>

int main(void) {
    xy_median_filter_t filter;
    uint16_t buffer[5];
    
    xy_filter_median_init(&filter, buffer, 5);
    
    // 带噪声的输入
    printf("Filtered: %d\n", xy_filter_median(&filter, 100));
    printf("Filtered: %d\n", xy_filter_median(&filter, 102));
    printf("Filtered: %d\n", xy_filter_median(&filter, 500));  // 噪声
    printf("Filtered: %d\n", xy_filter_median(&filter, 101));
    
    return 0;
}
```

#### 排序算法

```c
#include "xy_sort.h"
#include <stdio.h>

int main(void) {
    uint16_t arr[] = {64, 34, 25, 12, 22, 11, 90, 5};
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    
    xy_quick_sort(arr, len);
    
    printf("Sorted: ");
    for (int i = 0; i < len; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    
    return 0;
}
```

---

### IPC 示例

#### 管道通信

```c
#include "xy_pipe.h"
#include <stdio.h>

int main(void) {
    uint8_t buffer[256];
    xy_pipe_t pipe;
    
    xy_pipe_init(&pipe, "test", buffer, sizeof(buffer));
    
    // 写入数据
    const char *msg = "Hello, Pipe!";
    xy_pipe_write(&pipe, (const uint8_t *)msg, strlen(msg));
    
    // 读取数据
    char read_buf[64];
    int len = xy_pipe_read(&pipe, (uint8_t *)read_buf, sizeof(read_buf));
    read_buf[len] = '\0';
    
    printf("Read: %s\n", read_buf);
    
    xy_pipe_deinit(&pipe);
    return 0;
}
```

#### 观察者模式

```c
#include "xy_observer.h"
#include <stdio.h>

static void my_callback(xy_subject_t *subject, const void *data, void *user_data) {
    printf("Notification received: %s\n", (const char *)data);
}

int main(void) {
    xy_subject_t subject;
    xy_observer_t observer;
    
    xy_subject_init(&subject, "MySubject");
    xy_observer_init(&observer, "MyObserver", my_callback, NULL);
    
    xy_subject_attach(&subject, &observer);
    xy_subject_notify(&subject, "Hello, Observer!");
    
    xy_subject_deinit(&subject);
    return 0;
}
```

---

### PID 示例

#### 温度控制

当前 PID API 使用 `xy_pid_config_t` 配置结构和 `float` 输入/输出；示例中的
`xy_os_tick_get()` 由 OSAL/平台层提供，控制循环需要先切换到自动模式。

```c
#include "xy_pid.h"
#include <stdio.h>

int main(void) {
    xy_pid_t pid;
    float output = 0.0F;

    xy_pid_config_t config = {
        .kp = 2.0F,
        .ki = 0.5F,
        .kd = 1.0F,
        .output_min = 0.0F,
        .output_max = 100.0F,
        .integral_min = 0.0F,
        .integral_max = 100.0F,
        .derivative_filter = 0.1F,
    };

    if (xy_pid_init(&pid, &config) != XY_PID_OK) {
        return 1;
    }

    xy_pid_set_setpoint(&pid, 100.0F);  // 目标温度 100°C
    xy_pid_set_mode(&pid, XY_PID_MODE_AUTO);

    // 模拟控制循环
    for (int i = 0; i < 10; i++) {
        float current_temp = 25.0F + (float)i * 5.0F;
        xy_pid_compute(&pid, current_temp, &output);

        printf("Setpoint: %.2f, Current: %.2f, Output: %.2f\n",
               pid.setpoint, current_temp, output);
    }

    return 0;
}
```

---

## 综合项目

### 项目示例

| 项目 | 说明 | 位置 |
|------|------|------|
| Power Bank | 电池管理系统 | `projects/Bank/` |
| Soldering Iron | 温度控制烙铁 | `projects/Soldering Iron/` |
| USB Bridge | USB 转 SPI/I2C/UART | `projects/USBBridge/` |
| Smart Card Bridge | ISO7816 SIM 卡接口 | `projects/SmartCard_USB_Bridge/` |
| LCR Meter | 电感/电容/电阻测量 | `projects/LCRMeter/` |

---

## 外部资源

- [GitHub 仓库](https://github.com/ZeroZap/XinYi)
- [组件文档](../components/index.md)
- [API 参考](../api/index.md)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
