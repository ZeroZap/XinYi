#!/usr/bin/env python3
"""Guard that openCH OSAL board images drive USART1 through the public HAL.

The OSAL smoke images were written during the FreeRTOS port bring-up and each
one opens USART1 with vendor calls (RCC_APB2PeriphClockCmd, GPIO_Init,
USART_Init, USART_SendData). XinYi requires application and component code to
use xy_hal_*, keeping vendor HAL inside platform backends and explicit
startup/clock/IRQ boundaries, so the images are migrated one at a time.

MIGRATED files must use the HAL and must not touch vendor USART/GPIO/RCC.
DEFERRED files are the documented backlog: they must still be vendor-driven, so
an entry cannot silently stay listed after somebody migrates it without updating
this guard. Any osal_freertos_*_main.c that appears in neither set fails, which
stops a new image from starting life outside the HAL.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards/opench_ch32v307"

MIGRATED = {
    "osal_freertos_sdk_main.c",
    "osal_freertos_isr_stress_main.c",
    "osal_freertos_timer_flags_main.c",
    "osal_freertos_oneshot_timer_main.c",
    "osal_freertos_queue_lifecycle_main.c",
    "osal_freertos_semaphore_lifecycle_main.c",
    "osal_freertos_mutex_lifecycle_main.c",
    "osal_freertos_mempool_lifecycle_main.c",
    "osal_freertos_event_flags_lifecycle_main.c",
    "osal_freertos_thread_lifecycle_main.c",
    "osal_freertos_primitives_main.c",
    "osal_freertos_stall_probe_main.c",
}

# The backlog is empty: every OSAL board image drives USART1 through the HAL now.
# Keep the set so a future image can be parked here with a reason instead of
# quietly bypassing the HAL.
DEFERRED: set[str] = set()

# UART and GPIO on the marker path must go through the HAL in every image.
VENDOR_PATTERNS = (
    r"\bGPIO_Init\b",
    r"\bUSART_Init\b",
    r"\bUSART_Cmd\b",
    r"\bUSART_SendData\b",
    r"\bUSART_GetFlagStatus\b",
    r"\bUSART_ReceiveData\b",
)

# TIM2 plus its NVIC line is the ISR stimulus for the stress images. xy_hal_timer
# Empty on purpose: xy_hal_timer_enable_irq() now claims the NVIC line as well as
# the peripheral interrupt, which was the only reason the sdk and isr_stress
# images kept vendor TIM2 setup. An entry here has to still show real timer setup
# so the allowance cannot rot into a stale exception, and nothing needs one now.
TIMER_BOUNDARY = {}
TIMER_PATTERNS = (
    r"\bRCC_APB[12]PeriphClockCmd\b",
    r"\bTIM_TimeBaseInit\b",
    r"\bTIM_ITConfig\b",
    r"\bNVIC_Init\b",
)


def main() -> int:
    errors: list[str] = []
    images = sorted(path.name for path in BOARD.glob("osal_freertos_*_main.c"))

    # xy_hal_delay_* reprograms SysTick->CMP/CTLR, so calling it under an RTOS
    # stops the kernel tick outright. OSAL images wait with xy_os_delay instead.
    for name in images:
        source = (BOARD / name).read_text(encoding="utf-8")
        if re.search(r"\bxy_hal_delay_(ms|us)\b", source):
            errors.append(
                f"{name} calls xy_hal_delay_*, which reprograms the SysTick the kernel owns: "
                "use xy_os_delay from an OSAL image"
            )

    overlap = MIGRATED & DEFERRED
    if overlap:
        errors.append(f"files listed as both migrated and deferred: {sorted(overlap)}")

    unclassified = [name for name in images if name not in MIGRATED and name not in DEFERRED]
    for name in unclassified:
        errors.append(
            f"{name} is neither migrated nor deferred: a new OSAL board image must either "
            "use xy_hal_uart_*/xy_hal_gpio_* or be listed in DEFERRED with a reason"
        )

    for name in sorted(MIGRATED):
        path = BOARD / name
        if not path.is_file():
            errors.append(f"migrated image is missing: {name}")
            continue
        source = path.read_text(encoding="utf-8")
        if "#include \"xy_hal_uart.h\"" not in source:
            errors.append(f"{name} must include xy_hal_uart.h")
        if "xy_hal_uart_init(" not in source:
            errors.append(f"{name} must open USART1 through xy_hal_uart_init")
        if "xy_hal_uart_send(" not in source:
            errors.append(f"{name} must write markers through xy_hal_uart_send")
        if "xy_hal_gpio_init(" not in source:
            errors.append(f"{name} must configure PA9/PA10 through xy_hal_gpio_init")
        # The UART on this board drops and reorders bytes, so a marker only
        # counts as evidence while it is still in RAM. Every image has to publish
        # that record from mark() or its verdict becomes unreadable.
        for required in ("xy_image_steps", "xy_image_failed", "xy_image_last"):
            if required not in source:
                errors.append(
                    f"{name} does not publish {required}, so the sweep cannot judge it "
                    "without trusting the virtual COM port"
                )

        if "xy_os_event_flags_wait(" in source and "EVENT_FLAGS_ERROR" not in source:
            errors.append(
                f"{name} waits on event flags but never names the 0x80000000 sentinel "
                "xy_os_event_flags_wait returns, so a timeout assertion cannot be correct"
            )
        for pattern in VENDOR_PATTERNS:
            if re.search(pattern, source):
                errors.append(
                    f"{name} still calls vendor {pattern} outside a platform backend"
                )
        if name not in TIMER_BOUNDARY:
            for pattern in TIMER_PATTERNS:
                if re.search(pattern, source):
                    errors.append(
                        f"{name} configures a timer/IRQ itself ({pattern}) without being listed "
                        "in TIMER_BOUNDARY with a reason"
                    )

    for name, reason in sorted(TIMER_BOUNDARY.items()):
        path = BOARD / name
        if not path.is_file():
            errors.append(f"timer-boundary image is missing: {name}")
            continue
        source = path.read_text(encoding="utf-8")
        if "TIM_TimeBaseInit(" not in source:
            errors.append(
                f"{name} no longer sets up TIM2 itself, so its TIMER_BOUNDARY entry "
                f"({reason}) is stale"
            )

    for name in sorted(DEFERRED):
        path = BOARD / name
        if not path.is_file():
            errors.append(f"deferred image is missing: {name}")
            continue
        source = path.read_text(encoding="utf-8")
        if "xy_hal_uart_init(" in source:
            errors.append(
                f"{name} already uses xy_hal_uart_init: move it from DEFERRED to MIGRATED "
                "instead of leaving a stale deferral"
            )

    if errors:
        print("opench_osal_hal_usage failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "opench_osal_hal_usage_ok "
        f"migrated={len(MIGRATED)} deferred={len(DEFERRED)} images={len(images)}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
