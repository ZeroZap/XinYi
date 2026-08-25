#!/usr/bin/env python3
"""Guard the fail-closed boundary of the STM32U5 HAL/HIL record."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
RECORD_PATH = ROOT / "docs" / "validation" / "xinyi-stm32u5-hal-hardware-validation-record.md"

REQUIRED_FIELDS = (
    "Git SHA",
    "Board / revision / MCU",
    "Debugger / programmer",
    "Logic analyzer / oscilloscope",
    "Toolchain / build command",
    "Raw log / capture directory",
    "GPIO pins",
    "UART instance / pins",
    "I2C instance / pins / pull-ups",
    "SPI instance / pins / chip select",
    "DMA controller / channel / request",
)

REQUIRED_SCENARIOS = (
    "HAL-01",
    "HAL-02",
    "HAL-03",
    "HAL-04",
    "HAL-05",
    "HAL-06",
    "HAL-07",
    "HAL-08",
    "HAL-09",
)

REQUIRED_GUARDS = (
    "Current result: `BLOCKED_NO_HARDWARE`",
    "Host, QEMU, and compile-only results cannot be promoted to board, timing, or production evidence.",
    "Only a real STM32U5 board run with raw logs and captures may select a `BOARD_` result.",
    "NACK",
    "timeout",
    "bus reset / re-init",
    "IRQ callback",
    "DMA",
)


def validate_record(text: str) -> list[str]:
    errors: list[str] = []
    for field in REQUIRED_FIELDS:
        if f"| {field} | pending |" not in text:
            errors.append(f"required HAL record field must exist and remain pending: {field}")
    for scenario in REQUIRED_SCENARIOS:
        if f"| {scenario} |" not in text:
            errors.append(f"required HAL scenario is missing: {scenario}")
    for phrase in REQUIRED_GUARDS:
        if phrase not in text:
            errors.append(f"HAL evidence boundary must remain explicit: {phrase}")
    return errors


def main() -> int:
    if not RECORD_PATH.is_file():
        print(f"stm32u5_hal_hardware_record failed: missing {RECORD_PATH.relative_to(ROOT)}")
        return 1

    errors = validate_record(RECORD_PATH.read_text(encoding="utf-8"))
    if errors:
        print("stm32u5_hal_hardware_record failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "stm32u5_hal_hardware_record_ok "
        f"fields={len(REQUIRED_FIELDS)} scenarios={len(REQUIRED_SCENARIOS)} "
        "status=BLOCKED_NO_HARDWARE real_board_required=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
