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
│   ├── xy_sysmon.c # 系统监控
│   └── xy_autotask.c # 空闲触发的 AutoTask 调度器
└── service/        # 内核服务
    └── bootreason_check/ # 启动原因检查服务
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

## Host 验证入口

当前主线 host 单元测试由仓库根目录的 `make test-unit` 驱动，Kernel 相关 active CTest 包括：

| CTest 名称 | 覆盖范围 |
| --- | --- |
| `osal_baremetal` | Bare-metal OSAL mutex/semaphore/event flags/message queue/memory pool/tick/delay/software timer 单线程契约。 |
| `kernel_autotask` | `xy_autotask` 初始化、手动/空闲触发、暂停/恢复、tick wraparound、回调与统计契约。 |
| `kernel_sysmon` | `xy_sysmon` portable host contract：init/stats/getter、zero-heap guard、任务列表打印 stub 与 alarm registration 日志契约；不代表真实 RTOS task/heap telemetry。 |
| `bootreason_check` | bootreason kernel service 的 portable guard/override 路径。 |

常用验证命令：

```bash
make test-unit
cd build/tests/unit && ctest -R '^(osal_baremetal|kernel_autotask|kernel_sysmon|bootreason_check)$' --output-on-failure
```

这些测试只证明 host/portable contract；RTOS backend 的真实线程调度、ISR 语义、低功耗唤醒和板级 bootreason 来源仍需要目标平台验证。

---

## 📊 完成度

| 模块 | 完成度 | 状态 |
|------|--------|------|
| **OSAL** | 98% | ✅ Host-guarded / backend 实证持续补齐 |
| **Misc** | 90% | 🟢 `xy_sysmon` + `xy_autotask` 主线可发现，SysMon/AutoTask 均有 host CTest 护栏 |
| **Service** | 65% | 🟡 bootreason check 有 host CTest，更多板级服务按需求推进 |

**总体**: 90% 🟡

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

- [x] `xy_sysmon` portable stats/getter/print/alarm stub contract 已由 `kernel_sysmon` host CTest 守护；后续只按真实监控指标/平台失败补最小回归。
- [ ] 在有目标平台证据时补 RTOS backend 线程调度、ISR/event flags 与低功耗唤醒验证记录。
- [ ] bootreason check 的板级来源仍需由项目/BSP 记录真实 RTC/backup-register/复位源证据。
- [ ] Kernel 当前 host guard 状态与后续边界见 `docs/design/xinyi-kernel-host-guard-status-sync-2026-08-08.md`。
- [ ] Kconfig 聚合与后端命名如需继续整理，先写 proposal，再做小步 build-gated 迁移。

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
