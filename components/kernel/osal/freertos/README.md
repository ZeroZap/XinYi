# XY OSAL FreeRTOS Backend

## Overview
FreeRTOS adapter source for the XinYi OSAL. The STM32U5 reference configuration currently has
Arm Cortex-M33 source/static-library compile evidence only; scheduler, ISR, concurrency and board
runtime validation remain pending.

## Features
- Source mappings: kernel/tasks, mutex, semaphore, event groups, message queue and software timers
- Known limitations: thread join/enumeration and queue message-size reporting are unsupported;
  runtime semantics and resource exhaustion are not yet validated

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
Version 1.0.0 | `compile-guarded-runtime-pending` | Written for pinned FreeRTOS 10.4.6

See `docs/validation/reference-rtos-decision.md` and `BACKEND_COMPARISON.md` for the authoritative
evidence boundary. This README does not claim runtime, performance, safety or hardware approval.
