#!/usr/bin/env python3
"""Guard the Pandora STM32L475 OSAL/FreeRTOS runtime image wiring."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"
CMAKE = BOARD / "CMakeLists.txt"
MAIN = BOARD / "rtos_main.c"
HANDLERS = BOARD / "rtos_handlers.c"


def main() -> int:
    errors: list[str] = []
    cmake = CMAKE.read_text(encoding="utf-8")

    for path, label in ((MAIN, "runtime main"), (HANDLERS, "runtime handlers")):
        if not path.is_file():
            errors.append(f"Pandora OSAL/FreeRTOS {label} is missing")

    for token in (
        "pandora_stm32l475_rtos",
        "rtos_main.c",
        "rtos_handlers.c",
        "xy_osal",
        "freertos_kernel",
    ):
        if token not in cmake:
            errors.append(f"Pandora CMake must wire token: {token}")

    if MAIN.is_file():
        main_source = MAIN.read_text(encoding="utf-8")
        for token in (
            "xy_os_thread_new",
            "xy_os_kernel_start",
            "xy_os_delay",
            "xy_os_semaphore_new",
            "xy_os_semaphore_acquire",
            "xy_os_semaphore_release",
            "xy_os_msgqueue_new",
            "xy_os_msgqueue_put",
            "xy_os_msgqueue_get",
            "xy_os_event_flags_new",
            "xy_os_event_flags_set",
            "xy_os_event_flags_wait",
            "PANDORA STM32L475VE XINYI OSAL FREERTOS READY",
            "OSAL_TASK_FAST",
            "OSAL_TASK_SLOW",
            "OSAL_SEM_TAKE",
            "OSAL_QUEUE_SEND",
            "OSAL_QUEUE_RECV",
            "OSAL_QUEUE_MISMATCH",
            "OSAL_EVENT_SET",
            "OSAL_EVENT_WAIT",
            "OSAL_EVENT_MISMATCH",
        ):
            if token not in main_source:
                errors.append(f"runtime image must preserve token: {token}")
        for forbidden in ("xTaskCreate(", "vTaskStartScheduler(", "vTaskDelay("):
            if forbidden in main_source:
                errors.append(f"board application must use OSAL, not direct FreeRTOS API: {forbidden}")

    if HANDLERS.is_file():
        handler_source = HANDLERS.read_text(encoding="utf-8")
        for token in (
            "vPortSVCHandler",
            "xPortPendSVHandler",
            "xPortSysTickHandler",
            "HAL_IncTick",
            "xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED",
        ):
            if token not in handler_source:
                errors.append(f"runtime handlers must preserve token: {token}")

    if errors:
        print("pandora_freertos_runtime failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("pandora_freertos_runtime_ok evidence=linkable-runtime-candidate hardware=pending")
    return 0


if __name__ == "__main__":
    sys.exit(main())
