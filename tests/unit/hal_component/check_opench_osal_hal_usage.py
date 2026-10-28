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
    "osal_freertos_primitives_main.c",
    "osal_freertos_stall_probe_main.c",
}

DEFERRED = {
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
}

VENDOR_PATTERNS = (
    r"\bRCC_APB[12]PeriphClockCmd\b",
    r"\bGPIO_Init\b",
    r"\bUSART_Init\b",
    r"\bUSART_Cmd\b",
    r"\bUSART_SendData\b",
    r"\bUSART_GetFlagStatus\b",
    r"\bUSART_ReceiveData\b",
)


def main() -> int:
    errors: list[str] = []
    images = sorted(path.name for path in BOARD.glob("osal_freertos_*_main.c"))

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
        if "EVENT_FLAGS_ERROR" not in source:
            errors.append(
                f"{name} must assert event-flag timeouts against the 0x80000000 sentinel "
                "xy_os_event_flags_wait returns, not against 0"
            )
        for pattern in VENDOR_PATTERNS:
            if re.search(pattern, source):
                errors.append(
                    f"{name} still calls vendor {pattern} outside a platform backend"
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
