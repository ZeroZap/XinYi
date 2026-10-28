#!/usr/bin/env python3
"""Fail-closed contracts for the Pandora ICM20608 capture validator."""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_icm20608_capture import analyze_capture  # noqa: E402

COMMIT = "1" * 40
HEADER = "\r\n".join(
    (
        "PANDORA ICM20608 SENSOR READY",
        f"FIRMWARE_COMMIT {COMMIT}",
        "I2C3_PINS SCL=PC0 SDA=PC1",
        "PANDORA_I2C3_INIT_DONE",
        "I2C3_ACK=0x68",
        "PANDORA_I2C3_SCAN_DONE count=1",
        "ICM20608_WHO_AM_I_RAW=174",
    )
)
REGS = "ICM20608_REGS_HEX=01,00,08,08,04,04,01"
RAW = "ICM20608_RAW14_HEX=FF34FFDE20200D18005DFFECFFCE"


def sample(accel=(24, 5, 1001), gyro=(1, 0, 0), index: int = 0) -> str:
    return (
        f"ICM20608 ACCEL_mg={accel[0]},{accel[1]},{accel[2]} "
        f"GYRO_dps={gyro[0]},{gyro[1]},{gyro[2]}"
    )


def build(samples=None, whoami=174, header=HEADER) -> bytes:
    rows = [header]
    for index, (accel, gyro) in enumerate(samples if samples is not None else [((24, 5, 1001), (1, 0, 0))] * 5):
        rows.append(REGS)
        rows.append(RAW)
        rows.append(sample(accel, gyro, index))
    return ("\r\n".join(rows) + "\r\n").encode()


def build_with_whoami(value: int) -> bytes:
    return build(header=HEADER.replace("ICM20608_WHO_AM_I_RAW=174", f"ICM20608_WHO_AM_I_RAW={value}"))


class PandoraIcm20608CaptureContract(unittest.TestCase):
    def test_accepts_ordered_identity_and_static_samples(self) -> None:
        result = analyze_capture(build(), COMMIT)

        self.assertEqual(result["status"], "ICM20608_B1_REVIEW_CANDIDATE", result["failures"])
        self.assertEqual(result["samples"], 5)
        self.assertEqual(result["whoami"], [hex(0xAE)])

    def test_rejects_wrong_who_am_i(self) -> None:
        result = analyze_capture(build_with_whoami(0x68), COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("WHO_AM_I" in failure for failure in result["failures"]))

    def test_rejects_out_of_order_header(self) -> None:
        reversed_header = "\r\n".join(reversed(HEADER.split("\r\n")))
        result = analyze_capture(build(header=reversed_header), COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("out-of-order" in failure for failure in result["failures"]))

    def test_rejects_error_marker_after_evidence(self) -> None:
        payload = build() + b"PANDORA_ICM20608_WHOAMI_MISMATCH\r\n"
        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("error marker" in failure for failure in result["failures"]))

    def test_rejects_dead_accelerometer(self) -> None:
        result = analyze_capture(build(samples=[((0, 0, 0), (0, 0, 0))] * 5), COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("accel magnitude" in failure for failure in result["failures"]))

    def test_rejects_too_few_samples(self) -> None:
        result = analyze_capture(build(samples=[((24, 5, 1001), (1, 0, 0))]), COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("need at least" in failure for failure in result["failures"]))

    def test_rejects_malformed_sample_line(self) -> None:
        payload = build() + b"ICM20608 ACCEL_mg=1,2 GYRO_dps=1,0,0\r\n"
        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("malformed" in failure for failure in result["failures"]))

    def test_rejects_register_block_reading_all_ones(self) -> None:
        payload = build().replace(b"01,00,08,08,04,04,01", b"FF,FF,FF,FF,FF,FF,FF")
        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("0xFF" in failure for failure in result["failures"]))

    def test_rejects_wrong_commit(self) -> None:
        result = analyze_capture(build(), "2" * 40)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("FIRMWARE_COMMIT" in failure for failure in result["failures"]))

    def test_rejects_gyro_out_of_range(self) -> None:
        result = analyze_capture(build(samples=[((24, 5, 1001), (3000, 0, 0))] * 5), COMMIT)

        self.assertEqual(result["status"], "CONTENT_MISMATCH")
        self.assertTrue(any("gyro" in failure for failure in result["failures"]))


if __name__ == "__main__":
    unittest.main()
