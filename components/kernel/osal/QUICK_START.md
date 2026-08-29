# XY OSAL Quick Start Guide

> Evidence boundary: `runtime-pending`. The default bare-metal backend is Host-guarded. The
> FreeRTOS reference backend currently has only a source/static-library STM32U5 compile gate;
> RT-Thread has no XinYi STM32U5 compile/runtime gate. The RTOS snippets below are integration
> examples, not proof that the scheduler, ISR paths, concurrency, or hardware have run.

## 1. Choose Your Backend

| Backend | When to Use |
|---------|-------------|
| **Bare-metal** | No RTOS needed, simple app, minimal code |
| **FreeRTOS** | Sprint 5 reference backend; compile-guarded, runtime pending |
| **RT-Thread** | Source candidate; not selected for the current Sprint |

## 2. Add to Build

### Makefile
```makefile
SRC_FILES += components/osal/baremetal/xy_os_baremetal.c
INCLUDE_DIRS += components/osal
```

### CMake
```cmake
target_sources(${PROJECT_NAME} PRIVATE
    components/osal/baremetal/xy_os_baremetal.c
)
target_include_directories(${PROJECT_NAME} PUBLIC
    components/osal
)
```

## 3. Basic Usage

```c
#include "components/osal/xy_os.h"

int main(void) {
    // Initialize
    xy_os_kernel_init();
    xy_os_kernel_start();

    while (1) {
        // Main loop
        xy_os_delay(1000);  // 1 second delay
    }
}
```

## 4. With RTOS (FreeRTOS/RT-Thread)

```c
#include "components/osal/xy_os.h"

void task1(void *arg) {
    while (1) {
        // Task work
        xy_os_delay(500);
    }
}

int main(void) {
    xy_os_kernel_init();

    // Create task
    xy_os_thread_attr_t attr = {
        .name = "Task1",
        .stack_size = 1024,
        .priority = XY_OS_PRIORITY_NORMAL
    };
    xy_os_thread_new(task1, NULL, &attr);

    // Start scheduler
    xy_os_kernel_start();

    // Never returns
    while (1);
}
```

## 5. Using Synchronization

### Mutex
```c
xy_os_mutex_id_t mutex = xy_os_mutex_new(NULL);

xy_os_mutex_acquire(mutex, XY_OS_WAIT_FOREVER);
// Critical section
xy_os_mutex_release(mutex);
```

### Semaphore
```c
xy_os_semaphore_id_t sem = xy_os_semaphore_new(1, 0, NULL);

// Producer
xy_os_semaphore_release(sem);

// Consumer
xy_os_semaphore_acquire(sem, XY_OS_WAIT_FOREVER);
```

### Message Queue
```c
xy_os_msgqueue_id_t queue = xy_os_msgqueue_new(10, sizeof(uint32_t), NULL);

// Send
uint32_t msg = 42;
xy_os_msgqueue_put(queue, &msg, 0, 100);

// Receive
uint32_t rx_msg;
xy_os_msgqueue_get(queue, &rx_msg, NULL, XY_OS_WAIT_FOREVER);
```

## That's It

The public API is intended to reduce application changes between backends, but backend selection
also requires the matching Kconfig/CMake integration, kernel, config, port, startup/link ownership,
and runtime validation. Do not treat a source-file switch as a qualified migration.

See [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) for complete API reference.
