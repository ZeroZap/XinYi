#!/usr/bin/env python3
"""Guard FreeRTOS OSAL thread-flag semantics."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/freertos/xy_os_freertos.c"
PUBLIC_API = ROOT / "components/kernel/osal/xy_os.h"


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    public_api = PUBLIC_API.read_text(encoding="utf-8")
    errors: list[str] = []

    if "return ulTaskNotifyValueClear(NULL, flags);" not in source:
        errors.append(
            "xy_os_thread_flags_clear must atomically return the previous notification "
            "value while clearing only the requested bits"
        )

    required_set_contract = (
        "xTaskNotifyAndQuery(h, flags, eSetBits, &previous) != pdPASS",
        "return previous | flags;",
    )
    for required in required_set_contract:
        if required not in source:
            errors.append(
                "xy_os_thread_flags_set must propagate notification failure and return "
                f"the complete post-set flag value; missing: {required}"
            )

    required_get_contract = (
        "if (xTaskNotifyAndQuery(xTaskGetCurrentTaskHandle(), 0, eNoAction, &flags) != pdPASS)",
        "return 0x80000000;",
    )
    for required in required_get_contract:
        if required not in source:
            errors.append(
                "xy_os_thread_flags_get must fail closed when the FreeRTOS notification "
                f"query is rejected; missing: {required}"
            )

    for forbidden in (
        "xTaskNotifyStateClear(NULL);",
        "xTaskNotify(xTaskGetCurrentTaskHandle(), ~flags, eSetValueWithOverwrite);",
    ):
        if forbidden in source:
            errors.append(f"thread flag clear retains invalid FreeRTOS sequence: {forbidden}")

    for required in (
        "pdstatus_to_xy_wait",
        "return XY_OS_ERROR_TIMEOUT;",
        "pdstatus_to_xy_wait(xSemaphoreTake(m, ticks))",
        "pdstatus_to_xy_wait(xSemaphoreTake(s, ticks))",
        "pdstatus_to_xy_wait(xQueueSendToBack(q, msg_ptr, ticks))",
        "pdstatus_to_xy_wait(xQueueReceive(q, msg_ptr, ticks))",
    ):
        if required not in source:
            errors.append(
                "FreeRTOS blocking synchronization operations must map an expired wait to "
                f"XY_OS_ERROR_TIMEOUT; missing: {required}"
            )

    if "xy_os_semaphore_release_from_isr" not in public_api:
        errors.append("OSAL public API must expose an explicit ISR-safe semaphore release")
    for token in (
        "xy_os_semaphore_release_from_isr",
        "xSemaphoreGiveFromISR",
        "portYIELD_FROM_ISR",
    ):
        if token not in source:
            errors.append(f"FreeRTOS ISR-safe semaphore release must preserve token: {token}")

    if errors:
        print("freertos_osal_thread_flags failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("freertos_osal_thread_flags_ok clear=atomic-selective previous=value")
    return 0


if __name__ == "__main__":
    sys.exit(main())
