#!/usr/bin/env python3
"""Guard the pending/real-evidence boundary of the Crypto MCU cycle record template."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TEMPLATE_PATH = (
    ROOT
    / "docs"
    / "validation"
    / "xinyi-crypto-benchmark-record-template-2026-08-14.md"
)

REQUIRED_MCU_FIELDS = (
    "Board / revision",
    "MCU / clock tree",
    "Cache / interrupt state",
    "Cycle counter or timer source",
    "Counter width / overflow handling",
    "Warm-up count",
    "Sample count",
    "Raw sample values",
    "UART/SWO/log artifact",
    "bytes/s or cycles/byte",
    "Known measurement limitations",
)

REQUIRED_PENDING_GUARDS = (
    "Current result: `pending`",
    "Pending-record rule:",
    "all MCU cycle fields must remain `pending`",
    "must not contain numeric cycle, latency, throughput, or sample values",
    "Only a real board run may change the result to `mcu-cycle-recorded`",
    "Compile-only, host timing, synthetic workload, estimated, or copied historical values are invalid substitutions",
)


def validate_template(text: str) -> list[str]:
    errors: list[str] = []
    for field in REQUIRED_MCU_FIELDS:
        if f"| {field} | pending |" not in text:
            errors.append(f"MCU cycle field must exist and remain pending in the template: {field}")
    for phrase in REQUIRED_PENDING_GUARDS:
        if phrase not in text:
            errors.append(f"MCU cycle pending guard must remain explicit: {phrase}")
    return errors


def main() -> int:
    errors = validate_template(TEMPLATE_PATH.read_text(encoding="utf-8"))
    if errors:
        print("crypto_mcu_cycle_record_template failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "crypto_mcu_cycle_record_template_ok "
        f"fields={len(REQUIRED_MCU_FIELDS)} status=pending real_board_required=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
