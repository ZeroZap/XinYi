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

COMPILE_PROBE_PATH = Path(__file__).with_name("crypto_benchmark_stm32u5_compile_probe.py")
COMPILE_PROBE_SPEC = importlib.util.spec_from_file_location(
    "crypto_benchmark_stm32u5_compile_probe", COMPILE_PROBE_PATH
)
assert COMPILE_PROBE_SPEC is not None and COMPILE_PROBE_SPEC.loader is not None
COMPILE_PROBE = importlib.util.module_from_spec(COMPILE_PROBE_SPEC)
COMPILE_PROBE_SPEC.loader.exec_module(COMPILE_PROBE)


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

    def test_record_builder_rejects_missing_component(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["component"]

        with self.assertRaisesRegex(ValueError, "component"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_plan_builder_rejects_empty_proposal(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["proposal"] = ""

        with self.assertRaisesRegex(ValueError, "proposal"):
            BENCHMARK_HOST.build_plan(manifest)

    def test_record_builder_rejects_missing_record_template(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["benchmark_record_template"]

        with self.assertRaisesRegex(ValueError, "benchmark_record_template"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_plan_builder_rejects_empty_no_claims(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["policy"]["no_claims"] = []

        with self.assertRaisesRegex(ValueError, "policy.no_claims"):
            BENCHMARK_HOST.build_plan(manifest)

    def test_record_builder_rejects_non_string_no_claim(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["policy"]["no_claims"][0] = True

        with self.assertRaisesRegex(ValueError, "policy.no_claims"):
            BENCHMARK_HOST.build_timing_record(manifest, 1)

    def test_timing_record_validator_rejects_missing_identity_metadata(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)
        del record["proposal"]

        errors = BENCHMARK_HOST.validate_timing_record(record, 1)

        self.assertTrue(any("proposal" in error for error in errors))

    def test_timing_record_validator_rejects_empty_no_claims(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)
        record["no_claims"] = []

        errors = BENCHMARK_HOST.validate_timing_record(record, 1)

        self.assertTrue(any("no_claims" in error for error in errors))

    def test_timing_record_validator_rejects_non_string_no_claim(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)
        record["no_claims"] = [True]

        errors = BENCHMARK_HOST.validate_timing_record(record, 1)

        self.assertTrue(any("no_claims" in error for error in errors))

    def test_timing_record_validator_rejects_missing_groups(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)
        del record["groups"]

        errors = BENCHMARK_HOST.validate_timing_record(record, 1)

        self.assertTrue(any("groups" in error for error in errors))

    def test_timing_record_validator_rejects_non_object_group(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)
        record["groups"][0] = "not-an-object"

        errors = BENCHMARK_HOST.validate_timing_record(record, 1)

        self.assertTrue(any("groups[0]" in error for error in errors))

    def test_timing_record_validator_rejects_missing_group_identity(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)
        del record["groups"][0]["source_ownership"]

        errors = BENCHMARK_HOST.validate_timing_record(record, 1)

        self.assertTrue(any("source_ownership" in error for error in errors))

    def test_timing_record_validator_rejects_malformed_samples(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)
        record["groups"][0]["samples"][0]["samples_ns"] = [True]

        errors = BENCHMARK_HOST.validate_timing_record(record, 1)

        self.assertTrue(any("samples_ns" in error for error in errors))

    def test_timing_record_validator_rejects_inconsistent_sample_summary(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 2)
        record["groups"][0]["samples"][0]["min_ns"] += 1

        errors = BENCHMARK_HOST.validate_timing_record(record, 2)

        self.assertTrue(any("min_ns" in error for error in errors))

    def test_timing_record_validator_rejects_boolean_iterations(self) -> None:
        record = BENCHMARK_HOST.build_timing_record(copy.deepcopy(self.manifest), 1)

        errors = BENCHMARK_HOST.validate_timing_record(record, True)

        self.assertTrue(any("iterations" in error for error in errors))

    def test_plan_validator_rejects_missing_identity_metadata(self) -> None:
        plan = BENCHMARK_HOST.build_plan(copy.deepcopy(self.manifest))
        del plan["record_template"]

        errors = BENCHMARK_HOST.validate_plan(plan)

        self.assertTrue(any("record_template" in error for error in errors))

    def test_plan_validator_rejects_non_object_group(self) -> None:
        plan = BENCHMARK_HOST.build_plan(copy.deepcopy(self.manifest))
        plan["groups"][0] = "not-an-object"

        errors = BENCHMARK_HOST.validate_plan(plan)

        self.assertTrue(any("groups[0]" in error for error in errors))

    def test_plan_validator_rejects_boolean_input_size(self) -> None:
        plan = BENCHMARK_HOST.build_plan(copy.deepcopy(self.manifest))
        plan["groups"][0]["input_sizes"] = [True]

        errors = BENCHMARK_HOST.validate_plan(plan)

        self.assertTrue(any("input_sizes" in error for error in errors))

    def test_plan_validator_rejects_empty_contract_tests(self) -> None:
        plan = BENCHMARK_HOST.build_plan(copy.deepcopy(self.manifest))
        plan["groups"][0]["contract_tests"] = []

        errors = BENCHMARK_HOST.validate_plan(plan)

        self.assertTrue(any("contract_tests" in error for error in errors))

    def test_plan_validator_rejects_empty_reproducibility_metadata(self) -> None:
        plan = BENCHMARK_HOST.build_plan(copy.deepcopy(self.manifest))
        plan["groups"][0]["compiler"] = ""

        errors = BENCHMARK_HOST.validate_plan(plan)

        self.assertTrue(any("compiler" in error for error in errors))

    def test_plan_validator_rejects_boolean_zero_iteration_metadata(self) -> None:
        plan = BENCHMARK_HOST.build_plan(copy.deepcopy(self.manifest))
        plan["groups"][0]["iterations"] = False

        errors = BENCHMARK_HOST.validate_plan(plan)

        self.assertTrue(any("iterations" in error for error in errors))

    def test_policy_checker_rejects_empty_input_sizes(self) -> None:
        self.assertFalse(BENCHMARK_CHECKER.valid_host_input_sizes([]))

    def test_policy_checker_rejects_boolean_input_sizes(self) -> None:
        self.assertFalse(BENCHMARK_CHECKER.valid_host_input_sizes([True]))

    def test_compile_probe_plan_rejects_missing_component(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["component"]

        with self.assertRaisesRegex(ValueError, "component"):
            COMPILE_PROBE.build_plan(manifest)

    def test_compile_probe_plan_rejects_empty_proposal(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["proposal"] = ""

        with self.assertRaisesRegex(ValueError, "proposal"):
            COMPILE_PROBE.build_plan(manifest)

    def test_compile_probe_plan_rejects_missing_record_template(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        del manifest["benchmark_record_template"]

        with self.assertRaisesRegex(ValueError, "benchmark_record_template"):
            COMPILE_PROBE.build_plan(manifest)

    def test_compile_probe_plan_rejects_empty_no_claims(self) -> None:
        manifest = copy.deepcopy(self.manifest)
        manifest["policy"]["no_claims"] = []

        with self.assertRaisesRegex(ValueError, "policy.no_claims"):
            COMPILE_PROBE.build_plan(manifest)

    def test_compile_probe_plan_validator_rejects_missing_identity_metadata(self) -> None:
        plan = COMPILE_PROBE.build_plan(copy.deepcopy(self.manifest))
        del plan["proposal"]

        errors = COMPILE_PROBE.validate_plan(plan)

        self.assertTrue(any("proposal" in error for error in errors))

    def test_compile_probe_plan_validator_rejects_empty_no_claims(self) -> None:
        plan = COMPILE_PROBE.build_plan(copy.deepcopy(self.manifest))
        plan["no_claims"] = []

        errors = COMPILE_PROBE.validate_plan(plan)

        self.assertTrue(any("no_claims" in error for error in errors))

    def test_compile_probe_record_validator_rejects_missing_record_template(self) -> None:
        record = {
            "status": COMPILE_PROBE.COMPILE_ONLY_STATUS,
            "component": self.manifest["component"],
            "proposal": self.manifest["proposal"],
            "no_claims": self.manifest["policy"]["no_claims"],
            "evidence_boundary": "not benchmark timing",
        }

        errors = COMPILE_PROBE.validate_compile_record(record)

        self.assertTrue(any("record_template" in error for error in errors))

    def test_compile_probe_record_validator_rejects_non_string_no_claim(self) -> None:
        record = {
            "status": COMPILE_PROBE.COMPILE_ONLY_STATUS,
            "component": self.manifest["component"],
            "proposal": self.manifest["proposal"],
            "record_template": self.manifest["benchmark_record_template"],
            "no_claims": [True],
            "evidence_boundary": "not benchmark timing",
        }

        errors = COMPILE_PROBE.validate_compile_record(record)

        self.assertTrue(any("no_claims" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
