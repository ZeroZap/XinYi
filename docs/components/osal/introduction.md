# OSAL 组件 - OS 抽象层

**状态**: ✅ 完善 | **测试**: 17 用例 | **版本**: 1.0

---

## 📖 简介

XinYi OS 抽象层（OSAL）提供统一的操作系统接口，支持多种 RTOS 和裸机环境。

### 核心特性

- ✅ **多 RTOS 支持** - FreeRTOS、RT-Thread、CMSIS-RTX、Bare-metal
- ✅ **统一 API** - 一套接口适配所有后端
- ✅ **零开销** - 编译时选择后端，无运行时开销
- ✅ **完整功能** - 内核管理、定时器、Tick、信号量等

### 支持的 RTOS

| RTOS | 许可证 | 状态 | 适用场景 |
|------|--------|------|---------|
| Bare-metal | - | ✅ 完善 | 简单应用 |
| FreeRTOS | MIT | ✅ 完善 | 通用嵌入式 |
| RT-Thread | Apache-2.0 | ✅ 完善 | 物联网应用 |
| CMSIS-RTX | Apache-2.0 | ✅ 完善 | ARM 生态 |

---

## 🚀 快速开始

### 1. 配置 RTOS 后端

使用 Kconfig 配置：

```bash
make menuconfig
# 选择 Kernel -> OSAL -> Backend
```

或修改 `xy_os_cfg.h`：

```c
// 选择后端
#define XY_OS_BACKEND XY_OS_BACKEND_FREERTOS
// #define XY_OS_BACKEND XY_OS_BACKEND_RTTHREAD
// #define XY_OS_BACKEND XY_OS_BACKEND_BAREMETAL
```

### 2. 初始化内核

```c
#include "xy_os.h"

int main(void) {
    xy_os_status_t status;
    
    // 初始化内核
    status = xy_os_kernel_init();
    if (status != XY_OS_OK) {
        // 错误处理
    }
    
    // 启动内核
    xy_os_kernel_start();
    
    return 0;
}
```

### 3. 创建任务

```c
#include "xy_os.h"

static xy_os_thread_t led_thread;
static uint8_t led_stack[512];

static void led_task(void *arg) {
    (void)arg;
    
    while (1) {
        // 切换 LED
        led_toggle();
        
        // 延时 1 秒
        xy_os_delay(1000);
    }
}

int main(void) {
    xy_os_kernel_init();
    
    // 创建任务
    xy_os_thread_create(
        &led_thread,
        "LED",
        led_task,
        NULL,
        5,              // 优先级
        led_stack,
        sizeof(led_stack)
    );
    
    xy_os_kernel_start();
    return 0;
}
```

---

## 📋 API 参考

### 内核管理

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `xy_os_kernel_init()` | 初始化内核 | `XY_OS_OK` 成功 |
| `xy_os_kernel_start()` | 启动内核 | `XY_OS_OK` 成功 |
| `xy_os_kernel_get_state()` | 获取内核状态 | 内核状态 |
| `xy_os_kernel_get_tick_count()` | 获取 Tick 计数 | Tick 值 |

### 任务管理

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `xy_os_thread_create()` | 创建任务 | `XY_OS_OK` 成功 |
| `xy_os_thread_delete()` | 删除任务 | `XY_OS_OK` 成功 |
| `xy_os_thread_suspend()` | 挂起任务 | `XY_OS_OK` 成功 |
| `xy_os_thread_resume()` | 恢复任务 | `XY_OS_OK` 成功 |

### 延时和 Tick

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `xy_os_delay(ms)` | 延时（毫秒） | - |
| `xy_os_delay_until(tick)` | 延时到指定 Tick | - |
| `xy_os_tick_get()` | 获取当前 Tick | Tick 值 |
| `xy_os_tick_set(tick)` | 设置 Tick | - |

### 软件定时器

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `xy_os_timer_sw_create()` | 创建定时器 | 定时器句柄 |
| `xy_os_timer_sw_start()` | 启动定时器 | `XY_OS_OK` 成功 |
| `xy_os_timer_sw_stop()` | 停止定时器 | `XY_OS_OK` 成功 |
| `xy_os_timer_sw_delete()` | 删除定时器 | `XY_OS_OK` 成功 |

---

## 🔧 配置选项

### 编译时配置

```c
// xy_os_cfg.h

// Tick 频率 (Hz)
#define XY_OS_TICK_FREQ 1000

// 最大任务数
#define XY_OS_MAX_THREADS 32

// 软件定时器支持
#define XY_OS_USE_SW_TIMER 1

// 软件定时器最大数量
#define XY_OS_SW_TIMER_MAX_NUM 16
```

### Kconfig 配置

```kconfig
config XY_OS_TICK_FREQ
    int "Tick frequency"
    default 1000

config XY_OS_MAX_THREADS
    int "Maximum number of threads"
    default 32

config XY_OS_USE_SW_TIMER
    bool "Enable software timers"
    default y
```

---

## 📝 使用示例

### 示例 1: 周期性任务

```c
#include "xy_os.h"
#include "xy_log.h"

static void periodic_task(void *arg) {
    (void)arg;
    
    xy_log_i("Periodic task started\n");
    
    while (1) {
        xy_log_i("Tick: %lu\n", xy_os_tick_get());
        xy_os_delay(1000);  // 1 秒周期
    }
}

void app_main(void) {
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

### 示例 2: 软件定时器

```c
#include "xy_os.h"
#include "xy_log.h"

static void timer_callback(void *arg) {
    (void)arg;
    xy_log_i("Timer expired!\n");
}

void app_main(void) {
    xy_os_kernel_init();
    
    // 创建定时器
    xy_os_timer_sw_t timer;
    timer = xy_os_timer_sw_create(
        "MyTimer",
        timer_callback,
        NULL,
        1000,   // 1 秒周期
        XY_OS_TIMER_PERIODIC
    );
    
    // 启动定时器
    xy_os_timer_sw_start(timer);
    
    xy_os_kernel_start();
}
```

### 示例 3: 多任务同步

```c
#include "xy_os.h"

static xy_os_sem_t sem;

static void producer_task(void *arg) {
    (void)arg;
    
    while (1) {
        // 生产数据
        produce_data();
        
        // 发送信号量
        xy_os_sem_post(&sem);
        
        xy_os_delay(100);
    }
}

static void consumer_task(void *arg) {
    (void)arg;
    
    while (1) {
        // 等待信号量
        xy_os_sem_wait(&sem, XY_OS_WAIT_FOREVER);
        
        // 消费数据
        consume_data();
    }
}

void app_main(void) {
    xy_os_kernel_init();
    
    // 创建信号量
    xy_os_sem_create(&sem, 0);
    
    // 创建任务
    xy_os_thread_t prod_thread, cons_thread;
    static uint8_t prod_stack[512], cons_stack[512];
    
    xy_os_thread_create(&prod_thread, "Producer", producer_task, NULL, 5, prod_stack, sizeof(prod_stack));
    xy_os_thread_create(&cons_thread, "Consumer", consumer_task, NULL, 5, cons_stack, sizeof(cons_stack));
    
    xy_os_kernel_start();
}
```

---

## 🧪 测试用例

OSAL bare-metal 单元测试已并入统一 PC 测试套件：

| CTest 名称 | 目标 | 说明 |
|----------|------|------|
| `osal_baremetal` | `test_osal` | kernel/thread/timer/mutex/semaphore/event flags/msgqueue/mempool |

运行测试：

```bash
make test-unit

# 或只运行 OSAL focused 测试
cmake --build build/tests/unit --target test_osal -j"$(nproc)"
ctest --test-dir build/tests/unit -R '^osal_baremetal$' --output-on-failure
```

---

## 📚 相关文档

- [RTOS 选择指南](../docs/rtos_selection_guide.md)
- [API 参考](api-reference.md)
- [示例代码](examples.md)
- [移植指南](porting.md)

---

## 📞 获取帮助

- 📚 [API 文档](api-reference.md)
- ❓ [常见问题](../about/faq.md)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
