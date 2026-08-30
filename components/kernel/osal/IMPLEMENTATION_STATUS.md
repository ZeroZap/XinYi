# XY OSAL Implementation Status

## Evidence-bounded implementation inventory

Overall status: `runtime-pending`. This file inventories adapter source coverage, not completed
runtime qualification. The default bare-metal backend is Host-guarded; FreeRTOS has a bounded
STM32U5 Cortex-M33 source/static-library compile gate; RT-Thread has no current XinYi target or
runtime gate.

### 1. Bare-metal Backend
**File**: `baremetal/xy_os_baremetal.c` (149 lines)
**Status**: Host-guarded default; thread creation remains a stub
**Features**:
- Kernel init/start/lock/unlock
- Tick count via `xy_tick_get()`
- Busy-wait delays
- All sync primitives return stubs

**Use Case**: Simple embedded apps, bootloaders, minimal firmware

---

### 2. FreeRTOS Backend
**File**: `freertos/xy_os_freertos.c` (383 lines)
**Status**: source/static-library compile-guarded; runtime/ISR/concurrency/hardware pending
**Features**:
- Full task management
- Mutex (standard + recursive)
- Semaphore (binary + counting)
- Event groups
- Message queues
- Software timers
- Task notifications (for thread flags)

**Priority**: Direct mapping (XY 0-56 → FreeRTOS 0-configMAX_PRIORITIES)
**Dependencies**: FreeRTOS 10.x headers
**Use Case**: Standard embedded RTOS applications

---

### 3. RT-Thread Backend
**File**: `rt-thread/xy_os_rtthread.c` (397 lines)
**Status**: source candidate; not selected and not target/runtime guarded in Sprint 5
**Features**:
- Full thread management
- Mutex with priority inheritance
- Semaphore
- Event flags
- Message queues
- Memory pools (native support)
- Software timers

**Priority**: Inverted mapping (XY 56 → RT 0)
**Dependencies**: RT-Thread 4.x headers
**Use Case**: Complex embedded applications with full RTOS features

---

## Feature Matrix

| Feature | Bare-metal | FreeRTOS | RT-Thread |
|---------|-----------|----------|-----------|
| **Kernel Control** | ✅ | ✅ | ✅ |
| Tick count | ✅ | ✅ | ✅ |
| Scheduler lock | ✅ (counter) | ✅ (suspend) | ✅ (critical) |
| **Thread Management** | ❌ | ✅ | ✅ |
| Create/delete | ❌ | ✅ | ✅ |
| Priority control | ❌ | ✅ | ✅ |
| Suspend/resume | ❌ | ✅ | ✅ |
| Thread flags | ❌ | ✅ (notify) | ✅ (event) |
| **Synchronization** | | | |
| Mutex | ❌ | ✅ | ✅ |
| Semaphore | ❌ | ✅ | ✅ |
| Event flags | ❌ | ✅ | ✅ |
| **Communication** | | | |
| Message queue | ❌ | ✅ | ✅ |
| Memory pool | ❌ | ⚠️ (stub) | ✅ |
| **Timers** | ❌ | ✅ | ✅ |
| **Delay** | ✅ (busy) | ✅ (sleep) | ✅ (sleep) |

Legend:
- ✅ Adapter source path present; not runtime qualification
- ⚠️ Partial/stub
- ❌ Not supported

---

## Build Configuration

### Select Backend

**Option 1: Makefile**
```makefile
# Choose one:
OSAL_BACKEND = baremetal
# OSAL_BACKEND = freertos
# OSAL_BACKEND = rt-thread

SRC_FILES += components/osal/$(OSAL_BACKEND)/xy_os_$(OSAL_BACKEND).c
```

**Option 2: CMake**
```cmake
set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend selection")

if(OSAL_BACKEND STREQUAL "freertos")
    target_sources(${PROJECT_NAME} PRIVATE
        components/osal/freertos/xy_os_freertos.c
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE freertos_kernel)
elseif(OSAL_BACKEND STREQUAL "rt-thread")
    target_sources(${PROJECT_NAME} PRIVATE
        components/osal/rt-thread/xy_os_rtthread.c
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE rtthread)
else()
    target_sources(${PROJECT_NAME} PRIVATE
        components/osal/baremetal/xy_os_baremetal.c
    )
endif()
```

---

## Compilation Notes

### Current Status
- ✅ **Bare-metal**: Compiles successfully (no external dependencies)
- ⚠️ **FreeRTOS**: STM32U5 source/static-library compile gate passes; runtime remains pending
- ⚠️ **RT-Thread**: Source candidate; no current XinYi STM32U5 target/runtime gate

### Header Issues (xy_os.h)
The current `xy_os.h` has a type issue:
- Uses `xy_u8_t` (doesn't exist in `xy_typedef.h`)
- Should use `uint8_t` or include `xy_typedef.h` properly

**Fix needed in xy_os.h**:
```c
// Option 1: Use standard types
xy_os_status_t xy_os_msgqueue_put(..., uint8_t msg_prio, ...);

// Option 2: Include typedef
#include "../xy_clib/xy_typedef.h"
xy_os_status_t xy_os_msgqueue_put(..., xy_uint8_t msg_prio, ...);
```

---

## Testing Checklist

For each backend:

- [ ] Kernel init and start
- [ ] Tick count incrementing
- [ ] Delay function accuracy
- [ ] Thread create/delete (RTOS only)
- [ ] Mutex lock/unlock
- [ ] Semaphore acquire/release
- [ ] Event flags set/wait
- [ ] Message queue send/receive
- [ ] Timer start/stop (RTOS only)
- [ ] Priority scheduling (RTOS only)
- [ ] ISR safety

---

## Documentation

| Document | Status |
|----------|--------|
| Main README | ✅ Complete |
| Implementation Guide | ✅ Complete (572 lines) |
| Bare-metal README | Present; Host contract documented |
| FreeRTOS README | Present; `compile-guarded-runtime-pending` documented |
| RT-Thread README | Present; source-candidate boundary documented |
| API Reference | ✅ (in xy_os.h) |

---

## Migration Path

### From Bare-metal to RTOS

1. Replace backend file in build system
2. Add RTOS initialization before `xy_os_kernel_start()`
3. Convert main loop to RTOS task
4. Add threading/synchronization as needed
5. Revalidate application behavior; a shared API does not guarantee identical backend semantics.

### Between RTOSes

1. Change backend file
2. Adjust build configuration for new RTOS
3. Revalidate application behavior, startup/link integration, timeouts, ISR use, and shutdown.

---

## Known Limitations

### FreeRTOS
- Memory pool: Not natively supported (stub implementation)
- Thread join: Not supported
- Thread enumeration: Not implemented

### RT-Thread
- Thread enumeration: Not implemented
- Thread join: Not supported

### Bare-metal
- Only kernel control and delays supported
- All threading/sync features unavailable

---

## Future Enhancements

### Potential Additions
- [ ] Zephyr RTOS backend
- [ ] Azure RTOS (ThreadX) backend
- [ ] POSIX threads backend (Linux/macOS)
- [ ] Memory pool implementation for FreeRTOS
- [ ] Thread enumeration for all backends
- [ ] Performance benchmarks

### Bare-metal Enhancements
- [ ] Software timer list
- [ ] Basic cooperative scheduler
- [ ] Simple semaphore using interrupt disable

---

## Version History

**v1.0.0** (2025-10-27)
- Initial release
- Bare-metal backend complete
- FreeRTOS backend complete
- RT-Thread backend complete
- Comprehensive documentation

---

## Contact & Support

For issues or questions:
1. Check implementation guide
2. Review backend-specific README
3. Examine `xy_os.h` API documentation
4. Test with minimal example

**Project**: XinYi (xy)
**Module**: OSAL (Operating System Abstraction Layer)
**License**: Same as XinYi project
