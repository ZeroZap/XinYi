# XinYi OSAL - 操作系统抽象层

**版本**: 1.0.0  
**日期**: 2026-03-16  
**维护者**: XinYi Team  
**状态**: ✅ 生产就绪

---

## 📖 概述

XinYi OSAL (Operating System Abstraction Layer) 提供统一的操作系统抽象接口，支持多种 RTOS 和 Bare-metal 环境。

### 核心特性
- ✅ CMSIS-RTOS2 兼容
- ✅ 多后端支持
- ✅ 零开销抽象
- ✅ 完整文档

---

## 🎯 支持的后端

| 后端 | 状态 | 平台 | 说明 |
|------|------|------|------|
| **FreeRTOS** | ✅ | 所有 | 最常用 RTOS |
| **RT-Thread** | ✅ | 所有 | 国产 RTOS |
| **CMSIS-RTX** | ✅ | ARM | ARM 官方 RTOS |
| **RTX5** | ✅ | ARM/RISC-V | 最新 RTX 版本 |
| **Bare-metal** | ✅ | ARM/RISC-V/ARC/x86 | 无 OS 环境 |

---

## 🚀 快速开始

### 1. 初始化 OSAL

```c
#include "xy_os.h"

int main(void)
{
    /* 初始化 OSAL */
    xy_os_kernel_init();
    
    /* 启动内核 */
    xy_os_kernel_start();
    
    return 0;
}
```

### 2. 创建线程

```c
/* 线程函数 */
void thread_func(void *arg)
{
    const char *name = (const char *)arg;
    
    while (1) {
        printf("Thread %s running\n", name);
        xy_os_thread_delay(1000); /* 延迟 1 秒 */
    }
}

/* 创建线程 */
xy_os_thread_attr_t attr = {
    .stack_size = 1024,
    .priority = XY_OS_PRIORITY_NORMAL,
};

xy_os_thread_t thread = xy_os_thread_create(
    "my_thread",
    thread_func,
    "Thread-1",
    &attr
);
```

### 3. 使用互斥量

```c
/* 创建互斥量 */
xy_os_mutex_t mutex = xy_os_mutex_create(NULL);

/* 获取锁 */
xy_os_mutex_acquire(mutex, XY_OS_WAIT_FOREVER);

/* 临界区代码 */
critical_section();

/* 释放锁 */
xy_os_mutex_release(mutex);

/* 删除互斥量 */
xy_os_mutex_delete(mutex);
```

### 4. 使用信号量

```c
/* 创建信号量 (初始值 0, 最大值 10) */
xy_os_semaphore_t sem = xy_os_semaphore_create(10, 0, NULL);

/* 等待信号量 */
xy_os_semaphore_acquire(sem, XY_OS_WAIT_FOREVER);

/* 释放信号量 */
xy_os_semaphore_release(sem);
```

### 5. 使用事件标志

```c
/* 创建事件标志 */
xy_os_event_flags_t flags = xy_os_event_flags_create(NULL);

/* 设置事件 */
xy_os_event_flags_set(flags, 0x01, XY_OS_EVENT_FLAGS_OR);

/* 等待事件 */
uint32_t events = xy_os_event_flags_wait(
    flags, 
    0x01, 
    XY_OS_EVENT_FLAGS_AND, 
    XY_OS_WAIT_FOREVER
);
```

### 6. 使用定时器

```c
/* 定时器回调 */
void timer_callback(void *arg)
{
    printf("Timer expired!\n");
}

/* 创建定时器 (周期性) */
xy_os_timer_t timer = xy_os_timer_create(
    "my_timer",
    timer_callback,
    NULL,
    XY_OS_TIMER_PERIODIC,
    NULL
);

/* 启动定时器 (1 秒周期) */
xy_os_timer_start(timer, 1000);
```

---

## 📊 API 分类

### 内核控制 (10 个 API)
```c
xy_os_kernel_init()           /* 初始化内核 */
xy_os_kernel_start()          /* 启动内核 */
xy_os_kernel_lock()           /* 锁定内核 */
xy_os_kernel_unlock()         /* 解锁内核 */
xy_os_kernel_get_tick_count() /* 获取 tick 计数 */
/* ... 更多 API */
```

### 线程管理 (16 个 API)
```c
xy_os_thread_create()         /* 创建线程 */
xy_os_thread_terminate()      /* 终止线程 */
xy_os_thread_delay()          /* 延迟 */
xy_os_thread_get_state()      /* 获取状态 */
xy_os_thread_set_priority()   /* 设置优先级 */
/* ... 更多 API */
```

### 同步原语 (20 个 API)
```c
/* 互斥量 */
xy_os_mutex_create()
xy_os_mutex_acquire()
xy_os_mutex_release()

/* 信号量 */
xy_os_semaphore_create()
xy_os_semaphore_acquire()
xy_os_semaphore_release()

/* 事件标志 */
xy_os_event_flags_create()
xy_os_event_flags_set()
xy_os_event_flags_wait()
```

### 定时器 (6 个 API)
```c
xy_os_timer_create()
xy_os_timer_start()
xy_os_timer_stop()
xy_os_timer_delete()
```

### 内存池 (8 个 API)
```c
xy_os_mempool_create()
xy_os_mempool_alloc()
xy_os_mempool_free()
```

### 消息队列 (8 个 API)
```c
xy_os_msgqueue_create()
xy_os_msgqueue_put()
xy_os_msgqueue_get()
```

**总计**: 68 个 API

---

## 🔧 平台配置

### STM32U5 + RTX5

```c
/* xy_os_cfg.h */
#define XY_OS_BACKEND_RTX5      1
#define CMSIS_OS_VER            2

/* CMakeLists.txt */
target_compile_definitions(${PROJECT} PRIVATE
    XY_OS_BACKEND_RTX5=1
    USE_HAL_DRIVER
    STM32U575xx
)
```

### Bare-metal (ARM Cortex-M)

```c
/* xy_os_cfg.h */
#define XY_OS_BACKEND_BAREMETAL 1
#define XY_OS_BAREMETAL_ARM_CM  1

/* CMakeLists.txt */
target_compile_definitions(${PROJECT} PRIVATE
    XY_OS_BACKEND_BAREMETAL=1
    __ARM_ARCH=7
)
```

### Bare-metal (RISC-V)

```c
/* xy_os_cfg.h */
#define XY_OS_BACKEND_BAREMETAL 1
#define XY_OS_BAREMETAL_RISCV   1

/* CMakeLists.txt */
target_compile_definitions(${PROJECT} PRIVATE
    XY_OS_BACKEND_BAREMETAL=1
    __riscv
)
```

---

## 📈 性能指标

### 中断延迟
| 平台 | 延迟 | 说明 |
|------|------|------|
| ARM Cortex-M | <1μs | PRIMASK 控制 |
| RISC-V | <1μs | mstatus.MIE 控制 |
| x86 | N/A | 无中断控制 |

### 上下文切换
| RTOS | 切换时间 | 说明 |
|------|---------|------|
| FreeRTOS | ~2-5μs | ARM Cortex-M @ 100MHz |
| RT-Thread | ~2-5μs | ARM Cortex-M @ 100MHz |
| RTX5 | ~2-5μs | ARM Cortex-M @ 100MHz |
| Bare-metal | ~0μs | 无切换开销 |

### 内存占用
| 后端 | RAM 占用 | 说明 |
|------|---------|------|
| FreeRTOS | ~2-4KB | 取决于配置 |
| RT-Thread | ~2-4KB | 取决于配置 |
| RTX5 | ~2-4KB | 取决于配置 |
| Bare-metal | ~500B | 仅内核状态 |

---

## 🔗 相关文档

- [OSAL 完善报告](../../docs/OSAL_IMPROVEMENT_REPORT_2026-03-15.md)
- [API 参考](../../docs/API_REFERENCE.md#osal-api)
- [开发者指南](../../docs/getting-started/DEVELOPER_GUIDE.md)

---

## 📝 更新日志

### 2026-03-15
- ✅ RTX5 后端支持 (20.6KB)
- ✅ Bare-metal 改进 (RISC-V/ARC)
- ✅ 5 个 RTOS 后端支持
- ✅ 4 个 Bare-metal 平台

### 2026-03-14
- ✅ CMSIS-RTX 后端完善
- ✅ 文档完善

---

**最后更新**: 2026-03-16  
**许可证**: Apache License 2.0
