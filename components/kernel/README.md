# XinYi Kernel Components - 内核组件

**版本**: 1.0.0  
**日期**: 2026-03-18  
**状态**: 🟡 完善中

---

## 📋 概述

Kernel 层提供操作系统抽象层 (OSAL) 和基础服务，支持多 RTOS 后端。

---

## 🏗️ 组件结构

```
kernel/
├── osal/           # 操作系统抽象层
│   ├── backend/    # 多后端支持
│   │   ├── freertos/
│   │   ├── rtthread/
│   │   ├── rtx5/
│   │   └── baremetal/
│   ├── inc/        # 头文件
│   └── src/        # 源文件
├── misc/           # 杂项服务
│   └── xy_sysmon.c # 系统监控
└── service/        # 内核服务
    └── ...
```

---

## 🔧 OSAL 支持的后端

| 后端 | 状态 | 说明 |
|------|------|------|
| **Bare-metal** | ✅ | 基础内核控制、tick/delay、软件定时器，以及主 CMSIS-like API 下的 mutex/semaphore/event flags/message queue/memory pool 单线程契约；thread creation 仍为 stub |
| **FreeRTOS** | ✅ | 最常用的 RTOS |
| **RT-Thread** | ✅ | 国产 RTOS |
| **CMSIS-RTX5** | ✅ | ARM 官方 RTOS |

---

## 📁 核心 API

Kernel OSAL 的主公共接口是 `components/kernel/osal/xy_os.h`，采用 CMSIS-RTOS2 风格命名；
`components/kernel/osal/inc/xy_os.h` 只是兼容 include 路径的 forward header。`inc/xy_os_sys.h`
提供 IRQ/critical/cache/watchdog/diagnostics 等 XinYi 系统扩展，不属于 CMSIS-RTOS2 核心 OSAL。

### 线程管理
```c
xy_os_thread_id_t task = xy_os_thread_new(entry_func, arg, NULL);
xy_os_thread_terminate(task);
xy_os_delay(100);  // 延迟 100 个 kernel tick
```

### 信号量
```c
xy_os_semaphore_id_t sem = xy_os_semaphore_new(1, 0, NULL);
xy_os_semaphore_acquire(sem, 1000);
xy_os_semaphore_release(sem);
```

### 互斥锁
```c
xy_os_mutex_id_t mutex = xy_os_mutex_new(NULL);
xy_os_mutex_acquire(mutex, 1000);
xy_os_mutex_release(mutex);
```

### 消息队列
```c
xy_os_msgqueue_id_t queue = xy_os_msgqueue_new(10, sizeof(msg_t), NULL);
xy_os_msgqueue_put(queue, &msg, 0, 100);
xy_os_msgqueue_get(queue, &msg, NULL, 100);
```

---

## 🔨 构建配置

### CMake
```cmake
# 启用 OSAL
-DCONFIG_KERNEL_OSAL=y

# 选择后端
-DOSAL_BACKEND=freertos
-DOSAL_BACKEND=rtthread
-DOSAL_BACKEND=cmsis_rtx
-DOSAL_BACKEND=baremetal
```

### Kconfig
```kconfig
config KERNEL_OSAL
    bool "Enable OSAL"
    default y

config OSAL_BACKEND_FREERTOS
    bool "FreeRTOS Backend"
    depends on KERNEL_OSAL
```

---

## 📊 完成度

| 模块 | 完成度 | 状态 |
|------|--------|------|
| **OSAL** | 98% | ✅ |
| **Misc** | 80% | 🟡 |
| **Service** | 60% | 🟡 |

**总体**: 85% 🟡

---

## 🚀 使用示例

### Bare-metal 模式
```c
#include "xy_os.h"

int main(void) {
    xy_os_kernel_init();

    // 裸机后端可使用 tick/delay/timer，以及单线程同步/队列/内存池契约；
    // thread creation 仍按 stub 处理。
    xy_os_delay(100);

    return 0;
}
```

### FreeRTOS 模式
```c
#include "xy_os.h"

static void blink_entry(void *arg) {
    (void)arg;
    while (1) {
        xy_gpio_toggle(LED_PIN);
        xy_os_delay(100);
    }
}

int main(void) {
    xy_os_kernel_init();
    xy_os_thread_new(blink_entry, NULL, NULL);
    xy_os_kernel_start();
    return 0;
}
```

---

## 📝 待完成任务

- [ ] 继续对齐 OSAL 与 CMSIS-RTOS2 能力面，补齐 backend compile/contract 测试
- [ ] 完善 Kconfig 聚合与后端命名文档
- [ ] 添加更多单元测试
- [ ] 补充文档示例

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
