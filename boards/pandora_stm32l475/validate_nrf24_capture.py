#!/usr/bin/env python3
"""Validate an identity-bound Pandora NRF24L01 SPI probe capture."""

import argparse
import json
from pathlib import Path
import re


COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")
DETECTED_RE = re.compile(
    r"NRF24_DETECTED status=0x([0-9A-F]{2}) config=0x([0-9A-F]{2}) "
    r"en_aa=0x([0-9A-F]{2}) setup_aw=0x([0-9A-F]{2}) rf_ch=0x([0-9A-F]{2}) "
    r"rf_setup=0x([0-9A-F]{2}) fifo=0x([0-9A-F]{2}) irq=(LOW|HIGH)"
)
TX_ACK_RE = re.compile(r"NRF24_TX_ACK_OK retries=([0-9A-F]{2}) payload=PANDORA_NRF24_TEST")


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    if COMMIT_RE.fullmatch(firmware_commit) is None:
        raise ValueError("firmware commit must be an exact lowercase 40-character Git SHA")

    text = payload.decode("ascii", errors="replace")
    required = [
        "PANDORA NRF24L01 SPI2 PROBE",
        f"FIRMWARE_COMMIT {firmware_commit}",
        "NRF24_DETECTED ",
        "NRF24_TX_ACK_OK ",
        "NRF24_PROBE_DONE",
    ]
    positions = [text.find(marker) for marker in required]
    errors = [marker for marker, position in zip(required, positions) if position < 0]
    if not errors and positions != sorted(positions):
        errors.append("marker ordering")
    if "NRF24_NOT_DETECTED" in text:
        errors.append("not-detected marker")
    if "NRF24_TX_NO_ACK" in text:
        errors.append("no-ack marker")

    detected = DETECTED_RE.search(text)
    registers = None
    if detected is None:
        errors.append("complete detected register snapshot")
    else:
        values = [int(value, 16) for value in detected.groups()[:-1]]
        registers = dict(
            zip(("status", "config", "en_aa", "setup_aw", "rf_ch", "rf_setup", "fifo"), values)
        )
        if registers["status"] == 0xFF:
            errors.append("floating-bus status")
        if registers["setup_aw"] & 0xFC or registers["setup_aw"] & 0x03 == 0:
            errors.append("invalid address-width register")

    tx_ack = TX_ACK_RE.search(text)
    tx_retransmits = None
    if tx_ack is None:
        errors.append("complete acknowledged-transmit marker")
    else:
        tx_retransmits = int(tx_ack.group(1), 16)
        if tx_retransmits > 15:
            errors.append("invalid retransmit count")

    return {
        "status": "B1_NRF24_ACKNOWLEDGED_TX_PASS" if not errors else "FAILED",
        "firmware_commit": firmware_commit,
        "captured_bytes": len(payload),
        "registers": registers,
        "tx_retransmits": tx_retransmits,
        "errors": errors,
        "claim_boundary": (
            "SPI register access plus one acknowledged fixed-payload PTX transaction; "
            "peer identity and payload receipt are not independently observed, with no RX, "
            "IRQ transition, range, throughput, recovery, or endurance claim"
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    try:
        result = analyze_capture(args.capture.read_bytes(), args.firmware_commit)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "B1_NRF24_ACKNOWLEDGED_TX_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())