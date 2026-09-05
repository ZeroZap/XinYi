#!/usr/bin/env python3
"""Guard the Pandora STM32L475 OSAL/FreeRTOS runtime image wiring."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"
CMAKE = BOARD / "CMakeLists.txt"
MAIN = BOARD / "rtos_main.c"
HANDLERS = BOARD / "rtos_handlers.c"
STRESS_VALIDATOR = BOARD / "validate_rtos_stress.py"
L4_DMA = ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_dma.c"


def main() -> int:
    errors: list[str] = []
    cmake = CMAKE.read_text(encoding="utf-8")

    for path, label in (
        (MAIN, "runtime main"),
        (HANDLERS, "runtime handlers"),
        (STRESS_VALIDATOR, "stress validator"),
    ):
        if not path.is_file():
            errors.append(f"Pandora OSAL/FreeRTOS {label} is missing")

    for token in (
        "pandora_stm32l475_rtos",
        "rtos_main.c",
        "rtos_handlers.c",
        "xy_osal",
        "freertos_kernel",
        "components/hal/stm32/stm32l4/xy_hal_dma.c",
        "stm32l4xx_hal_dma.c",
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
            "xy_os_mutex_new",
            "xy_os_mutex_acquire",
            "xy_os_mutex_release",
            "xy_os_mempool_new",
            "xy_os_mempool_alloc",
            "xy_os_mempool_free",
            "xy_os_mempool_delete",
            "xy_os_msgqueue_delete",
            "xy_broker_init",
            "xy_broker_register_server",
            "xy_broker_send_msg",
            "xy_broker_process_msgs",
            "xy_device_register",
            "xy_device_find",
            "xy_stdio_printf_init",
            "xy_pm_tick_get",
            "OSAL_IPC_SEND",
            "OSAL_IPC_DELIVER",
            "OSAL_TRACE_DELIVER",
            "OSAL_DEVICE_LOOKUP",
            "OSAL_IPC_ERROR",
            "OSAL_IPC_SATURATED",
            "OSAL_IPC_RECOVERED",
            "OSAL_IPC_SATURATION_ERROR",
            "OSAL_TRACE_ERROR",
            "OSAL_PM_TICK",
            "OSAL_PM_ERROR",
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
            "OSAL_MUTEX_FAST",
            "OSAL_MUTEX_SLOW",
            "OSAL_MUTEX_TIMEOUT",
            "OSAL_MUTEX_MISMATCH",
            "OSAL_ISR_TAKE",
            "OSAL_ISR_TIMEOUT",
            "OSAL_TIM6_IRQ_TAKE",
            "OSAL_TIM6_IRQ_TIMEOUT",
            "OSAL_TIM6_IRQ_TIMEOUT_EXPECTED",
            "OSAL_TIM6_IRQ_RECOVERED",
            "OSAL_TIM6_IRQ_RECOVERY_ERROR",
            "HAL_TIM_Base_Stop_IT",
            "OSAL_RESOURCE_EXHAUSTED",
            "OSAL_RESOURCE_RECOVERED",
            "OSAL_LIFECYCLE_REINIT",
            "OSAL_RESOURCE_ERROR",
            "xy_os_kernel_get_tick_count",
            "XY_OS_ERROR_TIMEOUT",
            "BLOCKING_TIMEOUT_TICKS 100U",
            "BLOCKING_TIMEOUT_TOLERANCE_TICKS 20U",
            "timeout_elapsed < BLOCKING_TIMEOUT_TICKS",
            "timeout_elapsed > BLOCKING_TIMEOUT_TICKS + BLOCKING_TIMEOUT_TOLERANCE_TICKS",
            "OSAL_BLOCKING_TIMEOUT_OK",
            "OSAL_BLOCKING_TIMEOUT_ERROR",
            "OSAL_STRESS_READY",
            "xy_hal_dma_init",
            "xy_hal_dma_start",
            "xy_hal_dma_register_callback",
            "xy_hal_dma_deinit",
            "__HAL_RCC_DMA1_CLK_ENABLE",
            "DMA1_Channel1_IRQn",
            "PANDORA_DMA_IRQ_CALLBACK_OK",
            "PANDORA_DMA_MEM2MEM_OK",
            "PANDORA_DMA_INIT_ERROR",
            "PANDORA_DMA_START_ERROR",
            "PANDORA_DMA_IRQ_ERROR",
            "PANDORA_DMA_COMPARE_ERROR",
            "PANDORA_DMA_DEINIT_ERROR",
        ):
            if token not in main_source:
                errors.append(f"runtime image must preserve token: {token}")
        for forbidden in ("xTaskCreate(", "vTaskStartScheduler(", "vTaskDelay("):
            if forbidden in main_source:
                errors.append(f"board application must use OSAL, not direct FreeRTOS API: {forbidden}")
        if main_source.count("uint32_t primask = __get_PRIMASK();") != 1 or main_source.count(
            "if (primask == 0U)"
        ) != 1:
            errors.append("Pandora runtime must serialize concurrent UART evidence output")

    if not L4_DMA.is_file():
        errors.append("dedicated STM32L4 DMA wrapper is missing")
    else:
        dma_source = L4_DMA.read_text(encoding="utf-8")
        for token in (
            "STM32L4",
            "HAL_DMA_Init",
            "HAL_DMA_Start",
            "HAL_DMA_Start_IT",
            "HAL_DMA_PollForTransfer",
            "HAL_DMA_DeInit",
            "DMA_PDATAALIGN_WORD",
            "DMA_MDATAALIGN_WORD",
            "XY_HAL_ERROR_INVALID_PARAM",
            "XY_HAL_ERROR_NOT_INIT",
            "XY_HAL_DMA_EVENT_COMPLETE",
        ):
            if token not in dma_source:
                errors.append(f"STM32L4 DMA wrapper must preserve token: {token}")

    if STRESS_VALIDATOR.is_file():
        validator_source = STRESS_VALIDATOR.read_text(encoding="utf-8")
        for token in (
            "STRESS_REVIEW_CANDIDATE",
            "STRESS_VALIDATION_FAILED",
            "min_pipeline_cycles",
            "min_isr_wakes",
            "runtime error markers present",
            "firmware identity mismatch",
        ):
            if token not in validator_source:
                errors.append(f"stress validator must preserve token: {token}")

    if HANDLERS.is_file():
        handler_source = HANDLERS.read_text(encoding="utf-8")
        for token in (
            "vPortSVCHandler",
            "xPortPendSVHandler",
            "xPortSysTickHandler",
            "HAL_IncTick",
            "xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED",
            "xy_os_semaphore_release_from_isr",
            "TIM6_DAC_IRQHandler",
            "HAL_TIM_IRQHandler",
            "DMA1_Channel1_IRQHandler",
            "HAL_DMA_IRQHandler",
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
