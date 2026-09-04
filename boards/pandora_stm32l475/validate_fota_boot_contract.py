#!/usr/bin/env python3
"""Validate a Pandora FOTA metadata handoff/attempt/confirm UART capture."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re

BANNER = "PANDORA STM32L475VE XINYI SMOKE OK"
IDENTITY_PREFIX = "FOTA_FIRMWARE_COMMIT "
INITIALIZED = "FOTA_METADATA_INITIALIZED slot=0 version=1"
HANDOFF = "FOTA_BOOT_HANDOFF_COMMITTED slot=1 version=2"
ATTEMPT = "FOTA_BOOT_ATTEMPT_COMMITTED count=1"
CONFIRM = "FOTA_BOOT_CONFIRM_COMMITTED slot=1 version=2"
CONTRACT_OK = "FOTA_BOOT_CONTRACT_OK active_slot=1 version=2 min_version=2"
ANTI_ROLLBACK = "FOTA_ANTI_ROLLBACK_REJECTED version=1 floor=2"
ROLLBACK_HANDOFF = "FOTA_ROLLBACK_HANDOFF_COMMITTED slot=0 version=3"
ROLLBACK_ATTEMPT = "FOTA_ROLLBACK_ATTEMPT_COMMITTED count=1"
AUTOMATIC_ROLLBACK = "FOTA_AUTOMATIC_ROLLBACK_COMMITTED active_slot=1 version=2"
METADATA_OK = "FOTA_METADATA_FLASH_OK"
ERROR_MARKER = "FOTA_BOOT_CONTRACT_ERROR"


def validate_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    firmware_commit = validate_commit(firmware_commit)
    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    identity = IDENTITY_PREFIX + firmware_commit
    failures: list[str] = []

    identity_positions = [index for index, line in enumerate(lines) if line == identity]
    if len(identity_positions) != 5:
        failures.append("boot identity chain mismatch")

    expected = (
        INITIALIZED,
        HANDOFF,
        ATTEMPT,
        CONFIRM,
        CONTRACT_OK,
        ANTI_ROLLBACK,
        ROLLBACK_HANDOFF,
        ROLLBACK_ATTEMPT,
        AUTOMATIC_ROLLBACK,
        METADATA_OK,
    )
    positions = []
    for marker in expected:
        count = lines.count(marker)
        if count != 1:
            failures.append(f"{marker} count {count} != 1")
        positions.append(lines.index(marker) if marker in lines else -1)
    if -1 not in positions and positions != sorted(positions):
        failures.append("FOTA transition markers are not ordered")

    if len(identity_positions) == 5 and -1 not in positions:
        (
            initialized_pos,
            handoff_pos,
            attempt_pos,
            confirm_pos,
            contract_pos,
            anti_rollback_pos,
            rollback_handoff_pos,
            rollback_attempt_pos,
            automatic_rollback_pos,
            metadata_pos,
        ) = positions
        if not (
            identity_positions[0] < initialized_pos < handoff_pos < identity_positions[1]
            and identity_positions[1] < attempt_pos < identity_positions[2]
            and identity_positions[2]
            < confirm_pos
            < contract_pos
            < anti_rollback_pos
            < rollback_handoff_pos
            < identity_positions[3]
            and identity_positions[3] < rollback_attempt_pos < identity_positions[4]
            and identity_positions[4] < automatic_rollback_pos < metadata_pos
        ):
            failures.append("FOTA transitions do not belong to five ordered boots")

    if ERROR_MARKER in lines:
        failures.append("runtime error marker present")

    return {
        "status": (
            "FOTA_BOOT_CONTRACT_REVIEW_CANDIDATE"
            if not failures
            else "FOTA_BOOT_CONTRACT_VALIDATION_FAILED"
        ),
        "firmware_commit": firmware_commit,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "boot_count": len(identity_positions),
        "failures": failures,
        "scope": (
            "Pandora metadata handoff/attempt/confirm persistence across software resets, "
            "anti-rollback rejection, and bounded automatic rollback metadata recovery"
        ),
        "not_evidence_for": [
            "candidate image execution",
            "bootloader vector handoff",
            "power-loss recovery",
            "secure FOTA",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--firmware-commit", required=True)
    args = parser.parse_args()

    result = analyze_capture(args.input.read_bytes(), args.firmware_commit)
    result["recorded_at_utc"] = datetime.now(timezone.utc).isoformat()
    result["source_device"] = "/dev/serial/by-id/usb-wch.cn_WCH-Link_B49C8F0639CE-if01"
    args.metadata.parent.mkdir(parents=True, exist_ok=True)
    args.metadata.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"{result['status']} boots={result['boot_count']} bytes={result['capture_bytes']}"
    )
    for failure in result["failures"]:
        print(f"- {failure}")
    return 0 if result["status"] == "FOTA_BOOT_CONTRACT_REVIEW_CANDIDATE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
