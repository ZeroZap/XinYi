#!/usr/bin/env python3
"""Contract tests for the host-only crypto benchmark record builder."""

from __future__ import annotations

import copy
import importlib.util
import unittest
from pathlib import Path

SCRIPT_PATH = Path(__file__).with_name("crypto_benchmark_host.py")
SPEC = importlib.util.spec_from_file_location("crypto_benchmark_host", SCRIPT_PATH)
assert SPEC is not None and SPEC.loader is not None
BENCHMARK_HOST = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BENCHMARK_HOST)


class CryptoBenchmarkHostTests(unittest.TestCase):
    def setUp(self) -> None:
        self.manifest = BENCHMARK_HOST.load_manifest()

    def test_record_builder_rejects_negative_manifest_input_size(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0]["input_sizes"] = [-1]

        with self.assertRaisesRegex(ValueError, "input_sizes"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_record_builder_rejects_manifest_input_above_host_bound(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0]["input_sizes"] = [
            BENCHMARK_HOST.HOST_TIMING_MAX_INPUT_SIZE + 1
        ]

        with self.assertRaisesRegex(ValueError, "input_sizes"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_record_builder_rejects_boolean_manifest_input_size(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0]["input_sizes"] = [True]

        with self.assertRaisesRegex(ValueError, "input_sizes"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)


if __name__ == "__main__":
    unittest.main()
