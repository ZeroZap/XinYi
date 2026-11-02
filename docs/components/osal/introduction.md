# OSAL 组件 - OS 抽象层

**状态**: Host-guarded / FreeRTOS `runtime-pending`（部分 Pandora bounded runtime） | **版本**: 1.0

> 当前证据边界：Pandora STM32L475VE 是正式 reference board；FreeRTOS/CM4F 已取得 bounded
> scheduler、同步、ISR→task、资源恢复、2P/2C、120 秒与 12-cycle stress、跨组件并发及 shallow
> sleep/wakeup 实板证据。STM32U5/M33 仅保留 enhancement compile compatibility；RT-Thread 与
> CMSIS-RTX 仅为 source candidate。多小时耐久、性能和完整产品资格仍为 `runtime-pending`。
> SSD1306 deferred，不是 OSAL 或 Sprint 1–4 基础验收依赖。

---

## 📖 简介

XinYi OS 抽象层（OSAL）提供统一的操作系统接口，支持多种 RTOS 和裸机环境。

### 核心特性

- **统一 API** - 提供跨后端接口；各后端支持度必须按证据分别判断
- **编译时选择** - 不引入运行时 backend dispatch；尚无性能/“零开销”测量
- **当前基线** - Bare-metal Host contract；FreeRTOS compile-guarded-runtime-pending

### 支持的 RTOS

| RTOS | 许可证 | 状态 | 适用场景 |
|------|--------|------|---------|
| Bare-metal | - | Host-guarded | Host contract；真实并发/实板 pending |
| FreeRTOS | MIT | Pandora bounded runtime / runtime-pending | Sprint 5 reference；Pandora 已有受限运行证据，U5 仅 compile compatibility |
| RT-Thread | Apache-2.0 | source candidate | 本 Sprint 未选择，无 target/runtime gate |
| CMSIS-RTX | Apache-2.0 | source candidate | 无 target/runtime gate |

---

## 🚀 快速开始

### 1. 配置 RTOS 后端

FreeRTOS reference backend 使用 root Kconfig/CMake opt-in；默认仍为 bare-metal。Pandora 是 runtime
基线；以下 STM32U5 命令只保留 enhancement compile compatibility：

```bash
KCONFIG_OVERRIDES=OSAL_BACKEND_FREERTOS=ON make HAL_PLATFORM=STM32U5 configure
```

该命令只证明 U5 root-selected source/static-library compile；不可当作 runnable firmware，也不阻塞
Pandora-first 的基础验收。

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

static xy_os_thread_id_t led_thread;
static uint8_t led_stack[512];

static void led_task(void *argument) {
    (void)argument;

    while (1) {
        // 切换 LED
        led_toggle();

        // 延时 1 秒（ticks）
        xy_os_delay(1000);
    }
}

int main(void) {
    xy_os_thread_attr_t attr = {0};

    xy_os_kernel_init();

    // 创建任务：名称、优先级、栈都由属性结构描述
    attr.name = "LED";
    attr.priority = XY_OS_PRIORITY_NORMAL;
    attr.stack_mem = led_stack;
    attr.stack_size = sizeof(led_stack);
    led_thread = xy_os_thread_new(led_task, NULL, &attr);
    if (led_thread == NULL) {
        return -1;
    }

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
| `xy_os_thread_new()` | 创建任务（`xy_os_thread_attr_t`） | 任务句柄，失败为 `NULL` |
| `xy_os_thread_get_id()` | 当前任务句柄 | 任务句柄 |
| `xy_os_thread_get_state()` | 任务状态 | `xy_os_thread_state_t` |
| `xy_os_thread_suspend()` | 挂起任务 | `XY_OS_OK` 成功 |
| `xy_os_thread_resume()` | 恢复任务 | `XY_OS_OK` 成功 |
| `xy_os_thread_yield()` | 让出 CPU | `XY_OS_OK` 成功 |
| `xy_os_thread_join()` | 等待任务退出 | `XY_OS_OK` 成功 |
| `xy_os_thread_detach()` | 分离任务（不 join） | `XY_OS_OK` 成功 |

### 延时和 Tick

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `xy_os_delay(ticks)` | 延时指定 tick 数 | `XY_OS_OK` 成功 |
| `xy_os_delay_until(tick)` | 延时到指定 Tick | `XY_OS_OK` 成功 |
| `xy_os_tick_get()` | 获取当前 Tick | Tick 值 |
| `xy_os_tick_freq()` | 获取 Tick 频率 | Hz |

### 软件定时器

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `xy_os_timer_new()` | 创建定时器（`XY_OS_TIMER_ONCE` / `XY_OS_TIMER_PERIODIC`） | 定时器句柄，失败为 `NULL` |
| `xy_os_timer_start()` | 启动/重启定时器 | `XY_OS_OK` 成功 |
| `xy_os_timer_stop()` | 停止定时器 | `XY_OS_OK` 成功 |
| `xy_os_timer_delete()` | 删除定时器 | `XY_OS_OK` 成功 |

### 信号量

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `xy_os_semaphore_new()` | 创建信号量（最大计数 + 初始计数） | 句柄，失败为 `NULL` |
| `xy_os_semaphore_acquire()` | 取一个令牌（`XY_OS_WAIT_FOREVER` 阻塞） | `XY_OS_OK` 成功 |
| `xy_os_semaphore_release()` | 释放一个令牌 | `XY_OS_OK` 成功 |
| `xy_os_semaphore_release_from_isr()` | 中断内释放令牌 | `XY_OS_OK` 成功 |

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
        xy_log_i("Tick: %u\n", (unsigned)xy_os_tick_get());
        xy_os_delay(1000);  // 1 秒周期
    }
}

void app_main(void) {
    xy_os_thread_attr_t attr = {0};

    xy_os_kernel_init();

    attr.name = "Periodic";
    attr.priority = XY_OS_PRIORITY_NORMAL;
    xy_os_thread_new(periodic_task, NULL, &attr);

    xy_os_kernel_start();
}
```

### 示例 2: 软件定时器

```c
#include "xy_os.h"
#include "xy_log.h"

static void timer_callback(void *argument) {
    (void)argument;
    xy_log_i("Timer expired!\n");
}

void app_main(void) {
    xy_os_timer_id_t timer;

    xy_os_kernel_init();

    // 创建定时器，再按 tick 启动（1000 ticks 周期）
    timer = xy_os_timer_new(timer_callback, XY_OS_TIMER_PERIODIC, NULL, NULL);
    if (timer == NULL) {
        return;
    }
    xy_os_timer_start(timer, 1000);

    xy_os_kernel_start();
}
```

### 示例 3: 多任务同步

```c
#include "xy_os.h"

static xy_os_semaphore_id_t sem;

static void producer_task(void *argument) {
    (void)argument;

    while (1) {
        // 生产数据
        produce_data();

        // 释放一个令牌
        xy_os_semaphore_release(sem);

        xy_os_delay(100);
    }
}

static void consumer_task(void *argument) {
    (void)argument;

    while (1) {
        // 等待令牌（永不到期）
        xy_os_semaphore_acquire(sem, XY_OS_WAIT_FOREVER);

        // 消费数据
        consume_data();
    }
}

void app_main(void) {
    xy_os_thread_attr_t prod_attr = {0}, cons_attr = {0};

    xy_os_kernel_init();

    // 创建信号量：最大计数 1，初始 0
    sem = xy_os_semaphore_new(1, 0, NULL);

    prod_attr.name = "Producer";
    prod_attr.priority = XY_OS_PRIORITY_NORMAL;
    cons_attr.name = "Consumer";
    cons_attr.priority = XY_OS_PRIORITY_NORMAL;

    xy_os_thread_new(producer_task, NULL, &prod_attr);
    xy_os_thread_new(consumer_task, NULL, &cons_attr);

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

- [RTOS 选择指南](../../hardware/rtos_selection_guide.md)
- [OSAL API 参考](../../api/API_REFERENCE.md)
- [移植到新平台](../../hardware/porting.md)
- [OSAL 组件源码约定](../../../components/kernel/osal/README.md)

---

## 📞 获取帮助

- 📚 [API 文档](../../api/API_REFERENCE.md)
- ❓ [常见问题](../../about/faq.md)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
