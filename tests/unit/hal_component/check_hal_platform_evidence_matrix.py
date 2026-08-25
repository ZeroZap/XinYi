#!/usr/bin/env python3
"""Guard the fail-closed HAL platform evidence matrix."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
MATRIX = ROOT / "docs" / "validation" / "hal-platform-evidence-matrix.md"

PLATFORMS = ("STM32U5", "STM32F4", "STM32L4", "WCH CH32V30x", "HC32L021", "PC simulation")
PERIPHERALS = ("GPIO", "UART", "I2C", "SPI", "Timer", "I2S", "DMA")
BOUNDARIES = (
    "源文件存在只表示 implementation source present",
    "不能互相升级",
    "BOARD_PENDING",
    "BLOCKED_NO_HARDWARE",
    "STM32L4 复用 STM32F4 wrapper",
    "只有最后一项完成并保留原始日志/capture 后",
)


def validate(text: str) -> list[str]:
    errors: list[str] = []
    header = "| Platform | " + " | ".join(PERIPHERALS) + " | Host | Compile/QEMU | Board |"
    if header not in text:
        errors.append("canonical peripheral/evidence header is missing")
    for platform in PLATFORMS:
        if f"| {platform} |" not in text:
            errors.append(f"platform row is missing: {platform}")
    for phrase in BOUNDARIES:
        if phrase not in text:
            errors.append(f"evidence boundary is missing: {phrase}")
    if "| STM32U5 |" in text and "| `BOARD_PENDING` |" not in text:
        errors.append("STM32U5 must remain board-pending without real evidence")
    return errors


def main() -> int:
    if not MATRIX.is_file():
        print(f"hal_platform_evidence_matrix failed: missing {MATRIX.relative_to(ROOT)}")
        return 1
    errors = validate(MATRIX.read_text(encoding="utf-8"))
    if errors:
        print("hal_platform_evidence_matrix failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(
        "hal_platform_evidence_matrix_ok "
        f"platforms={len(PLATFORMS)} peripherals={len(PERIPHERALS)} "
        "board_evidence=BOARD_PENDING"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
