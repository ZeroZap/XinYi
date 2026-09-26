#!/usr/bin/env python3
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_nrf24_capture import analyze_capture  # noqa: E402


COMMIT = "0123456789abcdef0123456789abcdef01234567"


def capture(
    detected: str,
    tx: str = "NRF24_TX_ACK_OK retries=01 payload=PANDORA_NRF24_TEST",
    rx: str = "",
) -> bytes:
    return (
        "PANDORA NRF24L01 SPI2 PROBE\r\n"
        f"FIRMWARE_COMMIT {COMMIT}\r\n"
        f"{detected}\r\n"
        f"{tx}\r\n"
        "NRF24_PROBE_DONE\r\n"
        f"{rx}"
    ).encode("ascii")


good = analyze_capture(
    capture(
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x03 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH"
    ),
    COMMIT,
)
assert good["status"] == "B1_NRF24_ACKNOWLEDGED_TX_PASS"
assert good["registers"]["rf_ch"] == 2
assert good["tx_retransmits"] == 1
assert "peer identity" in good["claim_boundary"]
assert good["rx_outcome"] == "NOT_ATTEMPTED"

rx_timeout = analyze_capture(
    capture(
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x03 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH",
        rx="NRF24_RX_READY payload_width=32 timeout_ms=30000\r\nNRF24_RX_TIMEOUT\r\n",
    ),
    COMMIT,
)
assert rx_timeout["status"] == "B1_NRF24_ACKNOWLEDGED_TX_PASS"
assert rx_timeout["rx_outcome"] == "TIMEOUT_NO_PAYLOAD"
assert rx_timeout["rx_payload_hex"] is None
assert "no receive payload observed" in rx_timeout["claim_boundary"]

rx_payload = analyze_capture(
    capture(
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x03 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH",
        rx="NRF24_RX_READY payload_width=32 timeout_ms=30000\r\n"
        "NRF24_RX_OK length=04 payload_hex=50494E47\r\n",
    ),
    COMMIT,
)
assert rx_payload["status"] == "B1_NRF24_RX_PAYLOAD_PASS"
assert rx_payload["rx_outcome"] == "PAYLOAD_RECEIVED"
assert rx_payload["rx_payload_hex"] == "50494E47"

assert analyze_capture(
    capture(
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x03 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH",
        rx="NRF24_RX_READY payload_width=32 timeout_ms=30000\r\n"
        "NRF24_RX_OK length=04 payload_hex=5049\r\n",
    ),
    COMMIT,
)["status"] == "FAILED"

assert analyze_capture(
    capture(
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x03 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH",
        rx="NRF24_RX_READY payload_width=32 timeout_ms=30000\r\nNRF24_RX_ERROR error=8\r\n",
    ),
    COMMIT,
)["status"] == "FAILED"

assert analyze_capture(
    capture(
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x03 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH",
        "NRF24_TX_NO_ACK error=8 retries=0F",
    ),
    COMMIT,
)["status"] == "FAILED"

assert analyze_capture(
    capture("NRF24_NOT_DETECTED error=8 irq=HIGH"), COMMIT
)["status"] == "FAILED"
assert analyze_capture(
    capture(
        "NRF24_DETECTED status=0xFF config=0xFF en_aa=0xFF setup_aw=0x03 "
        "rf_ch=0x7F rf_setup=0xFF fifo=0xFF irq=HIGH"
    ),
    COMMIT,
)["status"] == "FAILED"
assert analyze_capture(
    capture(
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x00 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH"
    ),
    COMMIT,
)["status"] == "FAILED"
assert analyze_capture(
    (
        f"FIRMWARE_COMMIT {COMMIT}\n"
        "PANDORA NRF24L01 SPI2 PROBE\n"
        "NRF24_DETECTED status=0x0E config=0x08 en_aa=0x3F setup_aw=0x03 "
        "rf_ch=0x02 rf_setup=0x0E fifo=0x11 irq=HIGH\n"
        "NRF24_PROBE_DONE\n"
    ).encode("ascii"),
    COMMIT,
)["status"] == "FAILED"