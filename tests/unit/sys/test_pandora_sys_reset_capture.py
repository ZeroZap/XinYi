#!/usr/bin/env python3
"""Fail-closed contracts for the Pandora SYS reset-cause capture validator.

The point of these cases is that the validator must be able to reject a capture
that prints the firmware's own conclusion. Before this slice it accepted any
`SYS_RESET_KIND <word>` and never looked at the register or the decoded mask, so
a mislabelled brownout boot or a drifted decoder would both have passed.
"""

from pathlib import Path
import json
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

import validate_sys_reset_request_capture as val  # noqa: E402

COMMIT = "2" * 40
SCRIPT = ROOT / "boards" / "pandora_stm32l475" / "validate_sys_reset_request_capture.py"
ARCHIVED_OLD_CAPTURE = (
    ROOT
    / "docs"
    / "validation"
    / "evidence"
    / "pandora-stm32l475"
    / "2026-10-27-sys-reset-request"
    / "uart-external-pin.txt"
)

# Real masks measured on this board: an internal reset also asserts PINRSTF.
PIN_RESET_CSR = "0x04000600"
PIN_RESET_FLAGS = 0x1
SOFTWARE_RESET_CSR = "0x14000600"
SOFTWARE_RESET_FLAGS = 0x5
WATCHDOG_RESET_CSR = "0x24000600"
WATCHDOG_RESET_FLAGS = 0x9


def record(csr: str, flags: int, kind: str, request: int) -> str:
    return "\r\n".join(
        (
            f"SYS_RESET_CSR {csr}",
            f"SYS_RESET_FLAGS 0x{flags:08X}",
            "SYS_CHIP_ID 001B002E3647501320313556",
            "SYS_MAC_ADDR 2E001B001350",
            f"SYS_RESET_REQUEST {request}",
            f"SYS_RESET_KIND {kind}",
            "",
        )
    )


def chain(
    first_reason: tuple[str, int, str] = (PIN_RESET_CSR, PIN_RESET_FLAGS, "EXTERNAL_PIN"),
    first_request: int = 0,
    software_request: int = 1,
    watchdog_request: int = 0,
    commit: str = COMMIT,
) -> bytes:
    text = "\r\n".join(
        (
            record(*first_reason, first_request),
            record(SOFTWARE_RESET_CSR, SOFTWARE_RESET_FLAGS, "SOFTWARE", software_request),
            record(WATCHDOG_RESET_CSR, WATCHDOG_RESET_FLAGS, "WATCHDOG", watchdog_request),
            f"FIRMWARE_COMMIT {commit}",
            "PANDORA STM32L475VE XINYI SMOKE OK",
            "",
        )
    )
    return text.encode("ascii")


class ResetCauseDecoderContract(unittest.TestCase):
    def test_decoder_mirrors_the_register_bits(self) -> None:
        self.assertEqual(val.decode_csr(0x04000600), 0x1)
        self.assertEqual(val.decode_csr(0x14000600), 0x5)
        self.assertEqual(val.decode_csr(0x24000600), 0x9)
        self.assertEqual(val.decode_csr(0x20000600), 0x8)  # WWDG alone still means watchdog
        self.assertEqual(val.decode_csr(0x00000600), 0x0)  # no cause bit set at all

    def test_a_brownout_is_never_attributed_to_power_on(self) -> None:
        """The shipped inline chain reported BROWNOUT|PIN as POWER_ON."""
        self.assertEqual(val.decode_csr(0x0C000600), 0x3)
        self.assertEqual(val.attribute(0x3), "BROWNOUT")
        self.assertEqual(val.attribute(0x2), "BROWNOUT")

    def test_attribution_priority_is_pinned(self) -> None:
        self.assertEqual(val.attribute(0x0), "POWER_ON")
        self.assertEqual(val.attribute(0x1), "EXTERNAL_PIN")
        self.assertEqual(val.attribute(0x5), "SOFTWARE")
        self.assertEqual(val.attribute(0x9), "WATCHDOG")
        self.assertEqual(val.attribute(0x11), "LOW_POWER")
        self.assertEqual(val.attribute(0x13), "BROWNOUT")
        self.assertEqual(val.attribute(0x0A), "WATCHDOG")
        self.assertEqual(val.attribute(0x20), "UNKNOWN")


class ResetCauseCaptureContract(unittest.TestCase):
    def test_accepts_a_chain_whose_three_numbers_agree(self) -> None:
        ok, result = val.validate(chain(), COMMIT)

        self.assertTrue(ok, result["problems"])
        self.assertEqual(len(result["observed_chain"]), 3)
        self.assertEqual(result["observed_chain"][1]["flags"], SOFTWARE_RESET_FLAGS)
        self.assertEqual(result["observed_chain"][1]["csr"], "14000600")

    def test_rejects_a_decoded_mask_that_does_not_match_the_register(self) -> None:
        payload = chain().replace(b"SYS_RESET_FLAGS 0x00000009", b"SYS_RESET_FLAGS 0x00000005")

        ok, result = val.validate(payload, COMMIT)

        self.assertFalse(ok)
        self.assertTrue(
            any("does not match CSR" in problem for problem in result["problems"]),
            result["problems"],
        )

    def test_rejects_a_kind_name_the_mask_does_not_support(self) -> None:
        payload = chain().replace(b"SYS_RESET_KIND WATCHDOG", b"SYS_RESET_KIND POWER_ON")

        ok, result = val.validate(payload, COMMIT)

        self.assertFalse(ok)
        self.assertTrue(
            any("does not match 0x00000009" in problem for problem in result["problems"]),
            result["problems"],
        )

    def test_rejects_a_capture_that_hides_the_register(self) -> None:
        payload = chain().replace(b"SYS_RESET_CSR 0x04000600", b"SYS_RESET_CSR UNAVAILABLE")

        ok, result = val.validate(payload, COMMIT)

        self.assertFalse(ok)
        self.assertTrue(
            any("does not publish the reset register" in problem for problem in result["problems"]),
            result["problems"],
        )

    def test_rejects_the_previous_self_graded_capture_format(self) -> None:
        """The archived 2026-10-27 log printed the decoded mask under the register's
        name and never printed the mask separately; that cannot be re-checked."""
        payload = ARCHIVED_OLD_CAPTURE.read_bytes()

        ok, result = val.validate(payload, "0c6d3be0aaa5ad685f63f97382b410f4b2cb8efa")

        self.assertFalse(ok)
        self.assertTrue(result["problems"])

    def test_rejects_a_capture_bound_to_another_commit(self) -> None:
        ok, result = val.validate(chain(), "3" * 40)

        self.assertFalse(ok)
        self.assertTrue(any("FIRMWARE_COMMIT" in problem for problem in result["problems"]))

    def test_rejects_a_chain_that_reports_a_stale_request(self) -> None:
        ok, result = val.validate(chain(watchdog_request=1), COMMIT)

        self.assertFalse(ok)
        self.assertTrue(any("reset chain" in problem for problem in result["problems"]))

    def test_cli_reports_pass_and_fail_with_an_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            capture = Path(directory) / "uart.txt"
            output = Path(directory) / "contract.json"
            capture.write_bytes(chain())

            passed = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--capture",
                    str(capture),
                    "--firmware-commit",
                    COMMIT,
                    "--output",
                    str(output),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(passed.returncode, 0, passed.stdout + passed.stderr)
            self.assertIn("PASS", passed.stdout)
            artifact = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(artifact["status"], "PASS")

            capture.write_bytes(chain().replace(b"SYS_RESET_KIND SOFTWARE", b"SYS_RESET_KIND WATCHDOG"))
            failed = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--capture",
                    str(capture),
                    "--firmware-commit",
                    COMMIT,
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(failed.returncode, 1, failed.stdout + failed.stderr)
            self.assertIn("FAIL", failed.stdout)


if __name__ == "__main__":
    unittest.main()
