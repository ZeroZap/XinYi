#!/usr/bin/env python3
"""Fail-closed contracts for Pandora AP3216C bounded stimulus captures."""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_ap3216c_capture import analyze_capture  # noqa: E402


def sample(ir: int, als: int = 10, ps: int = 0, raw: str | None = None) -> str:
    if raw is None:
        raw = f"{ir & 0xff:02X}{(ir >> 8) & 0x03:02X}1F00{ps & 0xff:02X}00"
    return (
        f"AP3216C_INT=0x00 RAW_HEX={raw} ALS_lux={als} "
        f"PS_raw={ps} IR_raw={ir}"
    )


class PandoraAp3216cCaptureContract(unittest.TestCase):
    def test_accepts_bounded_ir_stimulus_response(self) -> None:
        lines = [sample(index % 4, raw=f"{index:012X}") for index in range(50)]
        lines += [sample(257 + index % 3, raw=f"{index + 50:012X}") for index in range(50)]

        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode())

        self.assertEqual(result["status"], "AP3216C_BOUNDED_IR_RESPONSE_B1")
        self.assertEqual(result["sample_count"], 100)
        self.assertEqual(result["first_window_ir_median"], 1.0)
        self.assertEqual(result["max_later_window_ir_median"], 258.0)
        self.assertEqual(result["failures"], [])

    def test_rejects_malformed_or_error_capture(self) -> None:
        payload = (sample(2) + "\r\nERROR read failed\r\nmalformed\r\n").encode()
        result = analyze_capture(payload)

        self.assertEqual(result["status"], "AP3216C_CAPTURE_VALIDATION_FAILED")
        self.assertIn("error marker present", result["failures"])
        self.assertIn("unrecognized non-empty line present", result["failures"])

    def test_rejects_static_or_short_capture(self) -> None:
        payload = ("\n".join(sample(2) for _ in range(99)) + "\n").encode()
        result = analyze_capture(payload)

        self.assertEqual(result["status"], "AP3216C_CAPTURE_VALIDATION_FAILED")
        self.assertIn("sample count 99 < 100", result["failures"])
        self.assertIn("unique line count 1 < 50", result["failures"])
        self.assertIn("IR median response delta 0.0 < 128.0", result["failures"])


if __name__ == "__main__":
    unittest.main()
