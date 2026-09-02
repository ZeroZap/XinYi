#!/usr/bin/env python3
"""Guard FreeRTOS OSAL thread-flag semantics."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
BACKEND = ROOT / "components/kernel/osal/backend/freertos/xy_os_freertos.c"


def main() -> int:
    source = BACKEND.read_text(encoding="utf-8")
    errors: list[str] = []

    if "return ulTaskNotifyValueClear(NULL, flags);" not in source:
        errors.append(
            "xy_os_thread_flags_clear must atomically return the previous notification "
            "value while clearing only the requested bits"
        )

    for forbidden in (
        "xTaskNotifyStateClear(NULL);",
        "xTaskNotify(xTaskGetCurrentTaskHandle(), ~flags, eSetValueWithOverwrite);",
    ):
        if forbidden in source:
            errors.append(f"thread flag clear retains invalid FreeRTOS sequence: {forbidden}")

    if errors:
        print("freertos_osal_thread_flags failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("freertos_osal_thread_flags_ok clear=atomic-selective previous=value")
    return 0


if __name__ == "__main__":
    sys.exit(main())
