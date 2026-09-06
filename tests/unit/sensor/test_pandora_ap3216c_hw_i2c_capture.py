#!/usr/bin/env python3
"""Fail-closed contracts for Pandora AP3216C hardware-I2C3 captures."""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_ap3216c_hw_i2c_capture import analyze_capture  # noqa: E402

COMMIT = "1" * 40
HEADER = "\r\n".join(
    (
        "PANDORA AP3216C SENSOR READY",
        f"FIRMWARE_COMMIT {COMMIT}",
        "AP3216C_BUS=HW_I2C3 SCL=PC0 SDA=PC1 ADDR=0x1E",
        "AP3216C_HW_I2C_READY",
        "AP3216C_NACK_OBSERVED ADDR=0x7F",
        "AP3216C_NACK_RECOVERED ADDR=0x1E",
        "AP3216C_CONFIG=0x03 MODE=ALS_PS",
        "AP3216C_DIAG INT=0x00 ALS_CONF=0x00 PS_CONF=0x05 PS_LED=0x13",
    )
)


def sample(index: int) -> str:
    return (
        f"AP3216C_INT=0x00 RAW_HEX={index:012X} "
        f"ALS_lux={index} PS_raw={index % 16} IR_raw={index % 512}"
    )


class PandoraAp3216cHwI2cCaptureContract(unittest.TestCase):
    def test_accepts_ordered_identity_and_bounded_samples(self) -> None:
        payload = ((HEADER + "\r\n" + "\r\n".join(sample(i) for i in range(10))).encode() +
                   b"\r\nPAND\xffPANDORA AP3216C SENSOR READY\r\n")

        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "AP3216C_HW_I2C3_B1")
        self.assertEqual(result["sample_count"], 10)
        self.assertEqual(result["non_ascii_byte_count"], 1)
        self.assertEqual(result["failures"], [])

    def test_rejects_wrong_bus_or_error_marker(self) -> None:
        payload = (HEADER.replace("HW_I2C3", "SOFT_I2C3") + "\n" +
                   "\n".join(sample(i) for i in range(10)) +
                   "\nPANDORA_AP3216C_SAMPLE_ERROR\n").encode()

        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "AP3216C_HW_I2C3_VALIDATION_FAILED")
        self.assertTrue(any("HW_I2C3" in failure for failure in result["failures"]))
        self.assertTrue(any("error marker" in failure for failure in result["failures"]))

    def test_rejects_missing_or_out_of_order_nack_recovery(self) -> None:
        samples = "\n".join(sample(i) for i in range(10))
        missing = HEADER.replace("AP3216C_NACK_OBSERVED ADDR=0x7F\r\n", "")
        out_of_order = HEADER.replace(
            "AP3216C_NACK_OBSERVED ADDR=0x7F\r\nAP3216C_NACK_RECOVERED ADDR=0x1E",
            "AP3216C_NACK_RECOVERED ADDR=0x1E\r\nAP3216C_NACK_OBSERVED ADDR=0x7F",
        )

        for payload in (missing, out_of_order):
            result = analyze_capture((payload + "\n" + samples).encode(), COMMIT)
            self.assertEqual(result["status"], "AP3216C_HW_I2C3_VALIDATION_FAILED")
            self.assertTrue(any("NACK" in failure for failure in result["failures"]))

    def test_rejects_short_malformed_or_out_of_range_samples(self) -> None:
        payload = (HEADER + "\n" + "\n".join(sample(i) for i in range(8)) +
                   "\nAP3216C_INT=bad\n" +
                   "AP3216C_INT=0x00 RAW_HEX=000000000000 ALS_lux=0 PS_raw=0 IR_raw=1024\n").encode()

        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "AP3216C_HW_I2C3_VALIDATION_FAILED")
        self.assertIn("malformed sample count 1", result["failures"])
        self.assertIn("sample outside AP3216C public range", result["failures"])
        self.assertIn("sample count 9 < 10", result["failures"])


if __name__ == "__main__":
    unittest.main()
