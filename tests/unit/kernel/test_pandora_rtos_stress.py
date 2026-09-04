#!/usr/bin/env python3
"""Host contracts for the bounded Pandora OSAL/FreeRTOS stress capture."""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_rtos_stress import analyze_capture  # noqa: E402


COMMIT = "0123456789abcdef0123456789abcdef01234567"
CYCLE = [
    "OSAL_MUTEX_FAST",
    "OSAL_TASK_FAST",
    "OSAL_QUEUE_SEND",
    "OSAL_EVENT_SET",
    "OSAL_SEM_TAKE",
    "OSAL_EVENT_WAIT",
    "OSAL_QUEUE_RECV",
    "OSAL_MUTEX_SLOW",
    "OSAL_TASK_SLOW",
]


def make_capture(cycles: int, isr_wakes: int) -> bytes:
    lines = [
        "PANDORA STM32L475VE XINYI OSAL FREERTOS READY",
        f"FIRMWARE_COMMIT {COMMIT}",
        "OSAL_RESOURCE_EXHAUSTED",
        "OSAL_BLOCKING_TIMEOUT_OK",
        "OSAL_RESOURCE_RECOVERED",
        "OSAL_LIFECYCLE_REINIT",
    ]
    for index in range(cycles):
        lines.extend(CYCLE)
        if index < isr_wakes:
            lines.append("OSAL_ISR_TAKE")
            lines.append("OSAL_TIM6_IRQ_TAKE")
    return ("\r\n".join(lines) + "\r\n").encode("ascii")


class StressCaptureContract(unittest.TestCase):
    def test_accepts_identity_ordered_cycles_and_thresholds(self) -> None:
        result = analyze_capture(make_capture(120, 60), COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_REVIEW_CANDIDATE")
        self.assertEqual(result["ordered_pipeline_cycles"], 120)
        self.assertEqual(result["isr_take_count"], 60)
        self.assertEqual(result["tim6_irq_take_count"], 60)
        self.assertEqual(result["error_markers"], {})

    def test_rejects_too_few_pipeline_cycles(self) -> None:
        result = analyze_capture(make_capture(99, 60), COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("pipeline cycles 99 < 100", result["failures"])

    def test_rejects_too_few_peripheral_irq_wakes(self) -> None:
        payload = make_capture(120, 60).replace(b"OSAL_TIM6_IRQ_TAKE\r\n", b"", 11)
        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("TIM6 IRQ wakes 49 < 50", result["failures"])

    def test_rejects_error_marker_and_wrong_identity(self) -> None:
        payload = make_capture(120, 60).replace(
            f"FIRMWARE_COMMIT {COMMIT}".encode("ascii"),
            b"FIRMWARE_COMMIT ffffffffffffffffffffffffffffffffffffffff",
        )
        payload += b"OSAL_QUEUE_MISMATCH\r\n"

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("firmware identity mismatch", result["failures"])
        self.assertEqual(result["error_markers"], {"OSAL_QUEUE_MISMATCH": 1})

    def test_rejects_reordered_pipeline(self) -> None:
        payload = make_capture(120, 60).replace(
            b"OSAL_QUEUE_SEND\r\nOSAL_EVENT_SET",
            b"OSAL_EVENT_SET\r\nOSAL_QUEUE_SEND",
            1,
        )

        result = analyze_capture(payload, COMMIT, 120, 120, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("pipeline cycles 119 < 120", result["failures"])


if __name__ == "__main__":
    unittest.main()
