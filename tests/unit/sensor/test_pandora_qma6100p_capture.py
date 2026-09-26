#!/usr/bin/env python3
"""Fail-closed contracts for Pandora QMA6100P runtime captures."""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_qma6100p_capture import analyze_capture  # noqa: E402

COMMIT = "1" * 40
HEADER = "\n".join(
    (
        "PANDORA QMA6100P I2C2 PROBE",
        f"FIRMWARE_COMMIT {COMMIT}",
        "QMA6100P_ADDR=0x12 CHIP_ID=0x90",
        "QMA6100P_IRQ_MAP INT1=PC6 INT2=PD15 ACTIVE=HIGH",
    )
)


def sample(index: int, *, status: str = "00", int1_edges: int = 0) -> str:
    x = 1000 + index
    y = 1400 + index
    z = -1600 - index
    scale = lambda value: (abs(value) * 1000 // 4096) * (-1 if value < 0 else 1)
    return (
        f"QMA6100P_SAMPLE n={index} raw={x},{y},{z} "
        f"mg={scale(x)},{scale(y)},{scale(z)} "
        f"status=0x{status} int1_level=0 int2_level=0 "
        f"int1_edges={int1_edges} int2_edges=0"
    )


def valid_capture() -> bytes:
    lines = [HEADER]
    lines.extend(sample(index) for index in range(40))
    lines.append("QMA6100P_PROBE_DONE")
    return ("\r\n".join(lines) + "\r\n").encode()


class PandoraQma6100pCaptureContract(unittest.TestCase):
    def test_accepts_identity_bound_static_basic_chain(self) -> None:
        result = analyze_capture(valid_capture(), COMMIT)

        self.assertEqual(result["status"], "B1_QMA6100P_BASIC_CHAIN_PASS")
        self.assertEqual(result["sample_count"], 40)
        self.assertEqual(result["unique_raw_samples"], 40)
        self.assertEqual(result["address"], "0x12")
        self.assertEqual(result["chip_id"], "0x90")
        self.assertEqual(result["interrupt_evidence"], "NOT_OBSERVED")
        self.assertEqual(result["failures"], [])

    def test_rejects_wrong_identity_or_device(self) -> None:
        for payload in (
            valid_capture().replace(COMMIT.encode(), ("2" * 40).encode()),
            valid_capture().replace(b"CHIP_ID=0x90", b"CHIP_ID=0xFF"),
            valid_capture().replace(b"ADDR=0x12", b"ADDR=0x14"),
        ):
            result = analyze_capture(payload, COMMIT)
            self.assertEqual(result["status"], "QMA6100P_VALIDATION_FAILED")
            self.assertTrue(result["failures"])

    def test_rejects_missing_malformed_duplicate_or_out_of_order_samples(self) -> None:
        payload = valid_capture().replace(sample(20).encode(), sample(19).encode())
        result = analyze_capture(payload, COMMIT)
        self.assertEqual(result["status"], "QMA6100P_VALIDATION_FAILED")
        self.assertTrue(any("sequence" in failure for failure in result["failures"]))

        malformed = valid_capture().replace(b"raw=1010,1410,-1610", b"raw=bad")
        result = analyze_capture(malformed, COMMIT)
        self.assertEqual(result["status"], "QMA6100P_VALIDATION_FAILED")
        self.assertTrue(any("malformed" in failure for failure in result["failures"]))

    def test_rejects_error_marker_and_out_of_range_acceleration(self) -> None:
        payload = valid_capture().replace(b"QMA6100P_PROBE_DONE", b"QMA6100P_RAW_ERROR\r\nQMA6100P_PROBE_DONE")
        result = analyze_capture(payload, COMMIT)
        self.assertEqual(result["status"], "QMA6100P_VALIDATION_FAILED")
        self.assertTrue(any("error marker" in failure for failure in result["failures"]))

        out_of_range = valid_capture().replace(b"mg=244,341,-390", b"mg=9000,341,-390")
        result = analyze_capture(out_of_range, COMMIT)
        self.assertEqual(result["status"], "QMA6100P_VALIDATION_FAILED")
        self.assertTrue(any("configured range" in failure for failure in result["failures"]))


if __name__ == "__main__":
    unittest.main()