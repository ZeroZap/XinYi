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
    "OSAL_IPC_SEND",
    "OSAL_PM_TICK",
    "OSAL_SEM_TAKE",
    "OSAL_EVENT_WAIT",
    "OSAL_QUEUE_RECV",
    "OSAL_DEVICE_LOOKUP",
    "[I] OSAL_TRACE_DELIVER",
    "OSAL_IPC_DELIVER",
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
        "OSAL_TIM6_IRQ_TIMEOUT_EXPECTED",
        "OSAL_TIM6_IRQ_RECOVERED",
        "OSAL_IPC_SATURATED",
        "OSAL_IPC_RECOVERED",
        "OSAL_MULTI_PRODUCER_OK",
        "OSAL_MULTI_CONSUMER_DISTRIBUTED",
        "PANDORA_DMA_MEM2MEM_OK",
        "PANDORA_DMA_IRQ_CALLBACK_OK",
        "PANDORA_DMA_STOP_RECOVERY_OK",
    ]
    lines.extend(["OSAL_MULTI_CONSUMER_0_TAKE"] * 8)
    lines.extend(["OSAL_MULTI_CONSUMER_1_TAKE"] * 8)
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
        self.assertEqual(result["multi_consumer_take_counts"], [8, 8])
        self.assertEqual(result["error_markers"], {})

    def test_rejects_too_few_pipeline_cycles(self) -> None:
        result = analyze_capture(make_capture(99, 60), COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("pipeline cycles 99 < 100", result["failures"])

    def test_rejects_missing_or_reordered_ipc_saturation_recovery(self) -> None:
        payload = make_capture(120, 60).replace(b"OSAL_IPC_RECOVERED\r\n", b"")
        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("IPC saturation/recovery marker count mismatch", result["failures"])

        payload = make_capture(120, 60).replace(
            b"OSAL_IPC_SATURATED\r\nOSAL_IPC_RECOVERED",
            b"OSAL_IPC_RECOVERED\r\nOSAL_IPC_SATURATED",
        )
        result = analyze_capture(payload, COMMIT, 120, 100, 50)
        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("IPC saturation/recovery markers are not ordered", result["failures"])

    def test_rejects_too_few_peripheral_irq_wakes(self) -> None:
        payload = make_capture(120, 60).replace(b"OSAL_TIM6_IRQ_TAKE\r\n", b"", 11)
        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("TIM6 IRQ wakes 49 < 50", result["failures"])

    def test_rejects_missing_or_reordered_peripheral_irq_recovery(self) -> None:
        payload = make_capture(120, 60).replace(b"OSAL_TIM6_IRQ_RECOVERED\r\n", b"")
        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("TIM6 IRQ recovery marker count mismatch", result["failures"])

        payload = make_capture(120, 60).replace(
            b"OSAL_TIM6_IRQ_TIMEOUT_EXPECTED\r\nOSAL_TIM6_IRQ_RECOVERED",
            b"OSAL_TIM6_IRQ_RECOVERED\r\nOSAL_TIM6_IRQ_TIMEOUT_EXPECTED",
        )
        result = analyze_capture(payload, COMMIT, 120, 100, 50)
        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("TIM6 IRQ recovery markers are not ordered", result["failures"])

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

    def test_rejects_missing_cross_component_delivery_marker(self) -> None:
        payload = make_capture(120, 60).replace(b"OSAL_DEVICE_LOOKUP\r\n", b"", 1)

        result = analyze_capture(payload, COMMIT, 120, 120, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("pipeline cycles 119 < 120", result["failures"])

    def test_rejects_missing_multi_producer_completion(self) -> None:
        payload = make_capture(120, 60).replace(b"OSAL_MULTI_PRODUCER_OK\r\n", b"")

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("OSAL_MULTI_PRODUCER_OK count 0 != 1", result["failures"])

    def test_rejects_missing_multi_consumer_distribution(self) -> None:
        payload = make_capture(120, 60).replace(
            b"OSAL_MULTI_CONSUMER_DISTRIBUTED\r\n", b""
        )

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("OSAL_MULTI_CONSUMER_DISTRIBUTED count 0 != 1", result["failures"])

    def test_rejects_missing_consumer_payload_count(self) -> None:
        payload = make_capture(120, 60).replace(b"OSAL_MULTI_CONSUMER_1_TAKE\r\n", b"", 1)

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("multi-consumer take total 15 != 16", result["failures"])

    def test_rejects_single_consumer_monopoly(self) -> None:
        payload = make_capture(120, 60).replace(
            b"OSAL_MULTI_CONSUMER_1_TAKE", b"OSAL_MULTI_CONSUMER_0_TAKE"
        )

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("both multi-consumers must receive payloads", result["failures"])

    def test_rejects_missing_dma_mem2mem_evidence(self) -> None:
        payload = make_capture(120, 60).replace(b"PANDORA_DMA_MEM2MEM_OK\r\n", b"")

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("PANDORA_DMA_MEM2MEM_OK count 0 != 1", result["failures"])

    def test_rejects_missing_dma_irq_callback_evidence(self) -> None:
        payload = make_capture(120, 60).replace(b"PANDORA_DMA_IRQ_CALLBACK_OK\r\n", b"")

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("PANDORA_DMA_IRQ_CALLBACK_OK count 0 != 1", result["failures"])

    def test_rejects_missing_dma_stop_recovery_evidence(self) -> None:
        payload = make_capture(120, 60).replace(b"PANDORA_DMA_STOP_RECOVERY_OK\r\n", b"")

        result = analyze_capture(payload, COMMIT, 120, 100, 50)

        self.assertEqual(result["status"], "STRESS_VALIDATION_FAILED")
        self.assertIn("PANDORA_DMA_STOP_RECOVERY_OK count 0 != 1", result["failures"])


if __name__ == "__main__":
    unittest.main()
