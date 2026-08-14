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

CHECKER_PATH = Path(__file__).with_name("check_crypto_benchmark_manifest.py")
CHECKER_SPEC = importlib.util.spec_from_file_location("check_crypto_benchmark_manifest", CHECKER_PATH)
assert CHECKER_SPEC is not None and CHECKER_SPEC.loader is not None
BENCHMARK_CHECKER = importlib.util.module_from_spec(CHECKER_SPEC)
CHECKER_SPEC.loader.exec_module(BENCHMARK_CHECKER)


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

    def test_record_builder_rejects_zero_iterations(self) -> None:
        with self.assertRaisesRegex(ValueError, "iterations"):
            BENCHMARK_HOST.build_timing_record(self.manifest, 0)

    def test_record_builder_rejects_iterations_above_host_bound(self) -> None:
        with self.assertRaisesRegex(ValueError, "iterations"):
            BENCHMARK_HOST.build_timing_record(
                self.manifest, BENCHMARK_HOST.HOST_TIMING_MAX_ITERATIONS + 1
            )

    def test_record_builder_rejects_boolean_iterations(self) -> None:
        with self.assertRaisesRegex(ValueError, "iterations"):
            BENCHMARK_HOST.build_timing_record(self.manifest, True)

    def test_record_builder_rejects_missing_algorithm_groups(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["algorithm_groups"]

        with self.assertRaisesRegex(ValueError, "algorithm_groups"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_record_builder_rejects_empty_algorithm_groups(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"] = []

        with self.assertRaisesRegex(ValueError, "algorithm_groups"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_record_builder_rejects_non_object_algorithm_group(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0] = "not-an-object"

        with self.assertRaisesRegex(ValueError, "algorithm_groups"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_plan_builder_rejects_missing_algorithm_groups(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["algorithm_groups"]

        with self.assertRaisesRegex(ValueError, "algorithm_groups"):
            BENCHMARK_HOST.build_plan(manifest)

    def test_plan_builder_rejects_non_object_algorithm_group(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0] = "not-an-object"

        with self.assertRaisesRegex(ValueError, "algorithm_groups"):
            BENCHMARK_HOST.build_plan(manifest)

    def test_plan_builder_rejects_invalid_input_sizes(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0]["input_sizes"] = [True]

        with self.assertRaisesRegex(ValueError, "input_sizes"):
            BENCHMARK_HOST.build_plan(manifest)

    def test_record_builder_rejects_missing_group_id(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["algorithm_groups"][0]["id"]

        with self.assertRaisesRegex(ValueError, "id"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_plan_builder_rejects_empty_source_ownership(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0]["source_ownership"] = ""

        with self.assertRaisesRegex(ValueError, "source_ownership"):
            BENCHMARK_HOST.build_plan(manifest)

    def test_record_builder_rejects_empty_contract_tests(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["algorithm_groups"][0]["contract_tests"] = []

        with self.assertRaisesRegex(ValueError, "contract_tests"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_plan_builder_rejects_missing_benchmark_input_policy(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["algorithm_groups"][0]["benchmark_input_policy"]

        with self.assertRaisesRegex(ValueError, "benchmark_input_policy"):
            BENCHMARK_HOST.build_plan(manifest)

    def test_policy_checker_rejects_empty_input_sizes(self) -> None:
        self.assertFalse(BENCHMARK_CHECKER.valid_host_input_sizes([]))

    def test_policy_checker_rejects_boolean_input_sizes(self) -> None:
        self.assertFalse(BENCHMARK_CHECKER.valid_host_input_sizes([True]))


if __name__ == "__main__":
    unittest.main()
