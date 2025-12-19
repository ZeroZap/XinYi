# XY OS Abstraction Layer (OSAL)

## Overview

The XY OSAL provides a unified RTOS interface based on CMSIS-RTOS2 API design patterns, allowing applications to run on different operating systems without modification.

## Supported Backends

### 1. Bare-metal (`baremetal/`)
- **Target**: Systems without an RTOS
- **Features**: 
  - Minimal kernel control (init, start, lock/unlock)
  - Delay functions using `xy_tick`
  - All threading/synchronization primitives return stubs/errors
- **Use Case**: Simple embedded applications, bootloaders, minimal firmware

### 2. RT-Thread (`rt-thread/`)
- **Target**: RT-Thread RTOS
- **Features**:
  - Full thread management
  - Complete synchronization primitives (mutex, semaphore, event, message queue)
  - Timer support
  - Memory pool support
- **Use Case**: Complex embedded applications requiring multitasking

### 3. FreeRTOS (`freertos/`)
- **Target**: FreeRTOS
- **Features**:
  - Full task management
  - Complete synchronization primitives (mutex, semaphore, event groups, queues)
  - Software timer support
  - Memory pool emulation
- **Use Case**: Standard embedded RTOS applications

## Directory Structure

```
osal/
├── xy_os.h              # OSAL public interface
├── README.md            # This file
├── baremetal/
│   ├── xy_os_baremetal.c
│   └── README.md
├── rt-thread/
│   ├── xy_os_rtthread.c
│   └── README.md
└── freertos/
    ├── xy_os_freertos.c
    └── README.md
```

## Integration Guide

### Step 1: Choose Backend

Select one backend based on your system:
- No RTOS → Use `baremetal`
- RT-Thread → Use `rt-thread`
- FreeRTOS → Use `freertos`

### Step 2: Add to Build System

Add the corresponding `.c` file to your build:

```makefile
# For bare-metal
SRC_FILES += components/osal/baremetal/xy_os_baremetal.c

# For RT-Thread
SRC_FILES += components/osal/rt-thread/xy_os_rtthread.c

# For FreeRTOS
SRC_FILES += components/osal/freertos/xy_os_freertos.c
```

### Step 3: Include Header

```c
#include "components/osal/xy_os.h"
```

### Step 4: Initialize

```c
// Initialize kernel
xy_os_kernel_init();

// Start kernel (for RTOS backends)
xy_os_kernel_start();
```

## API Documentation

See `xy_os.h` for complete API documentation. All functions follow CMSIS-RTOS2 conventions.

### Key Functions

**Kernel Control:**
- `xy_os_kernel_init()` - Initialize RTOS
- `xy_os_kernel_start()` - Start scheduler
- `xy_os_kernel_get_tick_count()` - Get system tick
- `xy_os_delay(ticks)` - Delay execution

**Thread Management:**
- `xy_os_thread_new()` - Create thread
- `xy_os_thread_terminate()` - Terminate thread
- `xy_os_thread_yield()` - Yield execution

**Synchronization:**
- `xy_os_mutex_new()` / `acquire()` / `release()`
- `xy_os_semaphore_new()` / `acquire()` / `release()`
- `xy_os_event_flags_new()` / `set()` / `wait()`
- `xy_os_msgqueue_new()` / `put()` / `get()`

## Example Usage

### Creating a Thread (RTOS only)

```c
void my_thread_func(void *arg)
{
    while (1) {
        // Do work
        xy_os_delay(1000); // Delay 1 second
    }
}

xy_os_thread_attr_t attr = {
    .name = "MyThread",
    .stack_size = 1024,
    .priority = XY_OS_PRIORITY_NORMAL
};

xy_os_thread_id_t tid = xy_os_thread_new(my_thread_func, NULL, &attr);
```

### Using Mutex

```c
xy_os_mutex_id_t mutex = xy_os_mutex_new(NULL);

// Acquire
if (xy_os_mutex_acquire(mutex, XY_OS_WAIT_FOREVER) == XY_OS_OK) {
    // Critical section
    xy_os_mutex_release(mutex);
}
```

### Using Semaphore

```c
xy_os_semaphore_id_t sem = xy_os_semaphore_new(1, 0, NULL);

// Producer
xy_os_semaphore_release(sem);

// Consumer
xy_os_semaphore_acquire(sem, XY_OS_WAIT_FOREVER);
```

## Backend Comparison

| Feature | Bare-metal | RT-Thread | FreeRTOS |
|---------|-----------|-----------|----------|
| Thread creation | ❌ | ✅ | ✅ |
| Mutex | ❌ | ✅ | ✅ |
| Semaphore | ❌ | ✅ | ✅ |
| Event flags | ❌ | ✅ | ✅ |
| Message queue | ❌ | ✅ | ✅ |
| Memory pool | ❌ | ✅ | ✅ (emulated) |
| Timer | ❌ | ✅ | ✅ |
| Delay | ✅ (busy-wait) | ✅ (sleep) | ✅ (sleep) |
| Kernel lock | ✅ (counter) | ✅ (scheduler) | ✅ (scheduler) |
| Priority inversion | N/A | ✅ | ✅ |
| ISR safety | ✅ | ✅ | ✅ |

## Porting Guidelines

When porting to a new RTOS:

1. Create new directory under `osal/`
2. Implement all functions in `xy_os.h`
3. Map XY OSAL priorities to native RTOS priorities
4. Handle timeout conversion (ticks)
5. Map error codes correctly
6. Test all synchronization primitives
7. Document backend-specific limitations

## Testing

Each backend should be tested with:
- Thread creation and termination
- Mutex lock/unlock
- Semaphore acquire/release
- Event flag set/wait
- Message queue send/receive
- Timeout handling
- Priority scheduling
- ISR context detection

## Notes

- All timeouts are in system ticks
- `XY_OS_WAIT_FOREVER` = infinite wait
- `XY_OS_NO_WAIT` = non-blocking
- Thread priorities: 0-56 (higher = higher priority)
- Bare-metal backend provides minimal functionality for compatibility

## Version History

- **v1.0.0** (2025-10-27): Initial release with bare-metal, RT-Thread, and FreeRTOS backends

## License

Same as XinYi project
