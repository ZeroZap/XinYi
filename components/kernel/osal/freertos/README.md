# XY OSAL FreeRTOS Backend

## Overview
FreeRTOS adapter for the XinYi OSAL. Pandora STM32L475VE/CM4F has bounded board runtime evidence for
scheduler/delay, task synchronization, SysTick/TIM6 ISR→task, resource recovery, 2P/2C,
120-second stress, PM shallow sleep and IPC/Device/Trace integration. STM32U5/M33 remains
enhancement compile compatibility only.

## Features
- Source mappings: kernel/tasks, mutex, semaphore, event groups, message queue and software timers
- Known limitations: thread join/enumeration and queue message-size reporting are unsupported;
  multi-hour endurance, performance, power, shutdown and complete product qualification remain
  pending

## Priority Mapping
Direct mapping: XY 0 (lowest) → FreeRTOS 0, capped at `configMAX_PRIORITIES - 1`

## FreeRTOSConfig.h Requirements
```c
#define configUSE_MUTEXES                1
#define configUSE_RECURSIVE_MUTEXES      1
#define configUSE_COUNTING_SEMAPHORES    1
#define configUSE_TIMERS                 1
#define configUSE_TASK_NOTIFICATIONS     1
#define configUSE_EVENT_GROUPS           1
```

## Usage Example
```c
xy_os_thread_attr_t attr = {
    .name = "Task",
    .stack_size = 512 * sizeof(StackType_t),
    .priority = XY_OS_PRIORITY_NORMAL
};
xy_os_thread_id_t task = xy_os_thread_new(my_task_func, NULL, &attr);
```

## Status
Version 1.0.0 | `compile-guarded-runtime-pending` | Pandora bounded board runtime | Written for
pinned FreeRTOS 10.4.6

See `docs/validation/reference-rtos-decision.md` and `BACKEND_COMPARISON.md` for the authoritative
evidence boundary. This README does not claim runtime, performance, safety or hardware approval.
