#!/usr/bin/env python3
"""Host contract for the Pandora FOTA multi-reset evidence validator."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_fota_boot_contract import analyze_capture  # noqa: E402


COMMIT = "0123456789abcdef0123456789abcdef01234567"


def valid_capture() -> bytes:
    return (
        f"PANDORA STM32L475VE XINYI SMOKE OK\r\nFIRMWARE_COMMIT {COMMIT}\r\n"
        "FOTA_METADATA_INITIALIZED slot=0 version=1\r\n"
        "FOTA_BOOT_HANDOFF_COMMITTED slot=1 version=2\r\n"
        f"PANDORA STM32L475VE XINYI SMOKE OK\r\nFIRMWARE_COMMIT {COMMIT}\r\n"
        "FOTA_BOOT_ATTEMPT_COMMITTED count=1\r\n"
        f"PANDORA STM32L475VE XINYI SMOKE OK\r\nFIRMWARE_COMMIT {COMMIT}\r\n"
        "FOTA_BOOT_CONFIRM_COMMITTED slot=1 version=2\r\n"
        "FOTA_BOOT_CONTRACT_OK active_slot=1 version=2 min_version=2\r\n"
        "FOTA_METADATA_FLASH_OK\r\n"
    ).encode("ascii")


def main() -> int:
    result = analyze_capture(valid_capture(), COMMIT)
    assert result["status"] == "FOTA_BOOT_CONTRACT_REVIEW_CANDIDATE"
    assert result["boot_count"] == 3
    assert result["failures"] == []

    out_of_order = valid_capture().replace(
        b"FOTA_BOOT_HANDOFF_COMMITTED slot=1 version=2",
        b"FOTA_BOOT_ATTEMPT_COMMITTED count=1",
        1,
    )
    result = analyze_capture(out_of_order, COMMIT)
    assert result["status"] == "FOTA_BOOT_CONTRACT_VALIDATION_FAILED"
    assert result["failures"]

    wrong_identity = valid_capture().replace(COMMIT.encode(), b"f" * 40, 1)
    result = analyze_capture(wrong_identity, COMMIT)
    assert result["status"] == "FOTA_BOOT_CONTRACT_VALIDATION_FAILED"
    assert "boot identity chain mismatch" in result["failures"]

    error_capture = valid_capture() + b"FOTA_BOOT_CONTRACT_ERROR\r\n"
    result = analyze_capture(error_capture, COMMIT)
    assert result["status"] == "FOTA_BOOT_CONTRACT_VALIDATION_FAILED"
    assert "runtime error marker present" in result["failures"]
    return 0


if __name__ == "__main__":
    raise SystemExit(main())