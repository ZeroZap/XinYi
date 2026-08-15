#!/usr/bin/env python3
"""Host-only XinYi crypto benchmark harness skeleton.

This helper is deliberately conservative: the default path prints a reproducible
benchmark plan and executes no timing loops. A future timing slice must opt in
explicitly and preserve the metadata/no-claim boundary guarded by
``crypto_benchmark_manifest``.
"""

from __future__ import annotations

import argparse
import json
import platform
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]
MANIFEST_PATH = ROOT / "tests" / "unit" / "crypto" / "crypto_benchmark_manifest.json"

REQUIRED_METADATA_DEFAULTS = {
    "compiler": "pending-until-opt-in-run",
    "compile_flags": "pending-until-opt-in-run",
    "hal_platform": "PC-host-plan-only",
    "commit_hash": "pending-until-opt-in-run",
    "dirty_state": "pending-until-opt-in-run",
    "iterations": 0,
    "warmup_iterations": 0,
    "correctness_gate": "run listed contract_tests before any timing result is accepted",
}

HOST_TIMING_ALLOWED_STATUS = "host-timing-recorded-pc-only-no-mcu-claim"
HOST_TIMING_MIN_ITERATIONS = 1
HOST_TIMING_MAX_ITERATIONS = 1000
HOST_TIMING_MAX_INPUT_SIZE = 4096

FORBIDDEN_APPROVAL_CLAIMS = {
    "security-approved",
    "provenance-approved",
    "hardware-passed",
    "constant-time-proven",
    "production-ready",
}


def load_manifest() -> dict[str, Any]:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def git_value(args: list[str]) -> str:
    try:
        return subprocess.check_output(args, cwd=ROOT, text=True, stderr=subprocess.DEVNULL).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "unavailable"


def git_dirty_state() -> str:
    status = git_value(["git", "status", "--short"])
    return "clean" if status == "" else status


def host_compiler() -> str:
    for compiler in ("cc", "gcc", "clang"):
        value = git_value([compiler, "--version"])
        if value != "unavailable":
            return value.splitlines()[0]
    return "unavailable"


def synthetic_workload(algorithm_id: str, size: int, iteration: int) -> int:
    """Small deterministic host-only workload used to validate timing plumbing.

    It is intentionally not a crypto implementation or throughput benchmark. The
    returned accumulator prevents the measured loop from being optimized away if
    this helper is later ported, while keeping default unit tests independent of
    machine speed thresholds.
    """

    state = (len(algorithm_id) * 131 + size + iteration) & 0xFFFFFFFF
    for offset in range(size):
        state = ((state << 5) ^ (state >> 2) ^ offset ^ size) & 0xFFFFFFFF
    return state


def validated_input_sizes(group: dict[str, Any]) -> list[int]:
    sizes = group.get("input_sizes")
    if not isinstance(sizes, list) or not sizes:
        raise ValueError(f"algorithm group {group.get('id')} input_sizes must be a non-empty list")
    if not all(
        type(size) is int and 0 <= size <= HOST_TIMING_MAX_INPUT_SIZE
        for size in sizes
    ):
        raise ValueError(
            f"algorithm group {group.get('id')} input_sizes must be integers within "
            f"0..{HOST_TIMING_MAX_INPUT_SIZE}"
        )
    return sizes


def validated_iterations(iterations: int) -> int:
    if (
        type(iterations) is not int
        or iterations < HOST_TIMING_MIN_ITERATIONS
        or iterations > HOST_TIMING_MAX_ITERATIONS
    ):
        raise ValueError(
            "iterations must be an integer within "
            f"{HOST_TIMING_MIN_ITERATIONS}..{HOST_TIMING_MAX_ITERATIONS}"
        )
    return iterations


def validated_algorithm_groups(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    groups = manifest.get("algorithm_groups")
    if not isinstance(groups, list) or not groups:
        raise ValueError("algorithm_groups must be a non-empty list")
    if not all(isinstance(group, dict) for group in groups):
        raise ValueError("algorithm_groups entries must be objects")
    for index, group in enumerate(groups):
        for field in ("id", "source_ownership", "benchmark_input_policy"):
            value = group.get(field)
            if not isinstance(value, str) or not value.strip():
                raise ValueError(f"algorithm_groups[{index}].{field} must be a non-empty string")
        contract_tests = group.get("contract_tests")
        if (
            not isinstance(contract_tests, list)
            or not contract_tests
            or not all(isinstance(test, str) and test.strip() for test in contract_tests)
        ):
            raise ValueError(
                f"algorithm_groups[{index}].contract_tests must be a non-empty list "
                "of non-empty strings"
            )
    return groups


def validated_manifest_metadata(manifest: dict[str, Any]) -> dict[str, Any]:
    metadata: dict[str, Any] = {}
    for field in ("component", "proposal", "benchmark_record_template"):
        value = manifest.get(field)
        if not isinstance(value, str) or not value.strip():
            raise ValueError(f"{field} must be a non-empty string")
        metadata[field] = value

    policy = manifest.get("policy")
    if not isinstance(policy, dict):
        raise ValueError("policy must be an object")
    no_claims = policy.get("no_claims")
    if (
        not isinstance(no_claims, list)
        or not no_claims
        or not all(isinstance(claim, str) and claim.strip() for claim in no_claims)
    ):
        raise ValueError("policy.no_claims must be a non-empty list of non-empty strings")
    metadata["no_claims"] = no_claims
    return metadata


def build_timing_record(manifest: dict[str, Any], iterations: int) -> dict[str, Any]:
    iterations = validated_iterations(iterations)
    metadata = validated_manifest_metadata(manifest)
    groups = []
    for group in validated_algorithm_groups(manifest):
        sizes = validated_input_sizes(group)
        samples = []
        correctness_accumulator = 0
        for size in sizes:
            sample_values = []
            for iteration in range(iterations):
                started = time.perf_counter_ns()
                correctness_accumulator ^= synthetic_workload(str(group.get("id")), size, iteration)
                elapsed_ns = time.perf_counter_ns() - started
                sample_values.append(elapsed_ns)
            samples.append(
                {
                    "input_size": size,
                    "samples_ns": sample_values,
                    "min_ns": min(sample_values),
                    "median_ns": statistics.median(sample_values),
                    "max_ns": max(sample_values),
                }
            )
        groups.append(
            {
                "algorithm_id": group.get("id"),
                "source_ownership": group.get("source_ownership"),
                "input_sizes": sizes,
                "iterations": iterations,
                "warmup_iterations": 0,
                "test_key_or_seed_policy": group.get("benchmark_input_policy"),
                "correctness_gate": "synthetic workload plumbing only; run listed contract_tests before accepting algorithm timing",
                "compiler": host_compiler(),
                "compile_flags": "python-host-synthetic-workload-no-compiled-crypto",
                "hal_platform": "PC-host-timing-only-not-MCU-performance",
                "commit_hash": git_value(["git", "rev-parse", "--short", "HEAD"]),
                "dirty_state": git_dirty_state(),
                "contract_tests": group.get("contract_tests", []),
                "samples": samples,
                "correctness_accumulator": correctness_accumulator,
            }
        )

    return {
        "status": HOST_TIMING_ALLOWED_STATUS,
        "component": metadata["component"],
        "manifest": str(MANIFEST_PATH.relative_to(ROOT)),
        "proposal": metadata["proposal"],
        "record_template": metadata["benchmark_record_template"],
        "host": {
            "python": sys.version.split()[0],
            "system": platform.system(),
            "machine": platform.machine(),
            "timer": "time.perf_counter_ns",
            "commit_context": git_value(["git", "rev-parse", "--short", "HEAD"]),
            "dirty_context": git_dirty_state(),
        },
        "no_claims": metadata["no_claims"],
        "evidence_boundary": "opt-in PC host timing plumbing only; not MCU timing, hardware validation, security approval, provenance approval, constant-time proof, or production enablement",
        "groups": groups,
    }


def validate_record_metadata(record: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    for field in ("component", "proposal", "record_template"):
        value = record.get(field)
        if not isinstance(value, str) or not value.strip():
            errors.append(f"{field} must be a non-empty string")
    no_claims = record.get("no_claims")
    if (
        not isinstance(no_claims, list)
        or not no_claims
        or not all(isinstance(claim, str) and claim.strip() for claim in no_claims)
    ):
        errors.append("no_claims must be a non-empty list of non-empty strings")
    return errors


def validate_timing_record(record: dict[str, Any], iterations: int) -> list[str]:
    errors = validate_record_metadata(record)
    if record.get("status") != HOST_TIMING_ALLOWED_STATUS:
        errors.append("timing record must stay PC-only/no-MCU-claim")
    if (
        type(iterations) is not int
        or iterations < HOST_TIMING_MIN_ITERATIONS
        or iterations > HOST_TIMING_MAX_ITERATIONS
    ):
        errors.append("iterations must stay inside the bounded host-smoke range")
    record_text = json.dumps(record, sort_keys=True)
    for claim in FORBIDDEN_APPROVAL_CLAIMS:
        if claim in record_text:
            errors.append(f"timing record must not contain approval phrase: {claim}")
    if "not MCU timing" not in record.get("evidence_boundary", ""):
        errors.append("timing record must explicitly reject MCU timing claims")

    groups = record.get("groups")
    if not isinstance(groups, list) or not groups:
        errors.append("timing record groups must be a non-empty list")
        return errors

    for group_index, group in enumerate(groups):
        if not isinstance(group, dict):
            errors.append(f"groups[{group_index}] must be an object")
            continue
        group_name = group.get("algorithm_id")
        for field in ("algorithm_id", "source_ownership", "test_key_or_seed_policy"):
            value = group.get(field)
            if not isinstance(value, str) or not value.strip():
                errors.append(f"groups[{group_index}].{field} must be a non-empty string")
        contract_tests = group.get("contract_tests")
        if (
            not isinstance(contract_tests, list)
            or not contract_tests
            or not all(isinstance(test, str) and test.strip() for test in contract_tests)
        ):
            errors.append(f"group {group_name} must list non-empty correctness gates")
        if group.get("iterations") != iterations:
            errors.append(f"group {group_name} must record requested iterations")
        if group.get("warmup_iterations") != 0:
            errors.append(f"group {group_name} must record zero warmup iterations")

        input_sizes = group.get("input_sizes")
        if not isinstance(input_sizes, list) or not input_sizes or not all(
            type(size) is int and 0 <= size <= HOST_TIMING_MAX_INPUT_SIZE for size in input_sizes
        ):
            errors.append(f"group {group_name} input_sizes must stay within the host timing bound")
            input_sizes = []

        samples = group.get("samples")
        if not isinstance(samples, list) or not samples:
            errors.append(f"group {group_name} must contain host timing samples")
            continue
        if input_sizes and len(samples) != len(input_sizes):
            errors.append(f"group {group_name} must contain one sample record per input size")
        for sample_index, sample in enumerate(samples):
            if not isinstance(sample, dict):
                errors.append(f"group {group_name} samples[{sample_index}] must be an object")
                continue
            sample_values = sample.get("samples_ns")
            valid_sample_values = (
                isinstance(sample_values, list)
                and len(sample_values) == iterations
                and all(type(value) is int and value >= 0 for value in sample_values)
            )
            if not valid_sample_values:
                errors.append(
                    f"group {group_name} samples[{sample_index}].samples_ns must contain "
                    "the requested number of non-negative integer samples"
                )
            input_size = sample.get("input_size")
            if type(input_size) is not int or input_size not in input_sizes:
                errors.append(f"group {group_name} samples[{sample_index}].input_size is invalid")
            for field in ("min_ns", "median_ns", "max_ns"):
                value = sample.get(field)
                if not isinstance(value, (int, float)) or isinstance(value, bool) or value < 0:
                    errors.append(
                        f"group {group_name} samples[{sample_index}].{field} must be non-negative numeric data"
                    )
            if valid_sample_values and isinstance(sample_values, list):
                validated_samples = [int(value) for value in sample_values]
                expected_summaries = {
                    "min_ns": min(validated_samples),
                    "median_ns": statistics.median(validated_samples),
                    "max_ns": max(validated_samples),
                }
                for field, expected in expected_summaries.items():
                    if sample.get(field) != expected:
                        errors.append(
                            f"group {group_name} samples[{sample_index}].{field} must match samples_ns"
                        )
    return errors


def build_plan(manifest: dict[str, Any]) -> dict[str, Any]:
    metadata = validated_manifest_metadata(manifest)
    groups = []
    for group in validated_algorithm_groups(manifest):
        groups.append(
            {
                "algorithm_id": group.get("id"),
                "source_ownership": group.get("source_ownership"),
                "input_sizes": validated_input_sizes(group),
                "test_key_or_seed_policy": group.get("benchmark_input_policy"),
                "contract_tests": group.get("contract_tests", []),
                **REQUIRED_METADATA_DEFAULTS,
            }
        )

    return {
        "status": "host-plan-only-no-timing",
        "component": metadata["component"],
        "manifest": str(MANIFEST_PATH.relative_to(ROOT)),
        "proposal": metadata["proposal"],
        "record_template": metadata["benchmark_record_template"],
        "host": {
            "python": sys.version.split()[0],
            "system": platform.system(),
            "machine": platform.machine(),
            "commit_context": git_value(["git", "rev-parse", "--short", "HEAD"]),
            "dirty_context": git_value(["git", "status", "--short"]),
        },
        "no_claims": metadata["no_claims"],
        "groups": groups,
    }


def validate_plan(plan: dict[str, Any]) -> list[str]:
    errors = validate_record_metadata(plan)
    if plan.get("status") != "host-plan-only-no-timing":
        errors.append("plan status must remain host-plan-only-no-timing")

    groups = plan.get("groups")
    if not isinstance(groups, list) or not groups:
        errors.append("plan groups must be a non-empty list")
        return errors

    for group_index, group in enumerate(groups):
        if not isinstance(group, dict):
            errors.append(f"groups[{group_index}] must be an object")
            continue
        group_name = group.get("algorithm_id")
        for field in ("algorithm_id", "source_ownership", "test_key_or_seed_policy"):
            value = group.get(field)
            if not isinstance(value, str) or not value.strip():
                errors.append(f"groups[{group_index}].{field} must be a non-empty string")
        for field in (
            "correctness_gate",
            "compiler",
            "compile_flags",
            "hal_platform",
            "commit_hash",
            "dirty_state",
        ):
            value = group.get(field)
            if not isinstance(value, str) or not value.strip():
                errors.append(f"group {group_name} {field} must be a non-empty string")
        for field in ("iterations", "warmup_iterations"):
            if type(group.get(field)) is not int or group[field] != 0:
                errors.append(f"group {group_name} {field} must be integer zero in plan-only mode")
        input_sizes = group.get("input_sizes")
        if not isinstance(input_sizes, list) or not input_sizes or not all(
            type(size) is int and 0 <= size <= HOST_TIMING_MAX_INPUT_SIZE for size in input_sizes
        ):
            errors.append(f"group {group_name} input_sizes must stay within the host timing bound")
        if group.get("iterations") != 0 or group.get("warmup_iterations") != 0:
            errors.append(f"group {group_name} must not run timing in plan-only mode")
        contract_tests = group.get("contract_tests")
        if (
            not isinstance(contract_tests, list)
            or not contract_tests
            or not all(isinstance(test, str) and test.strip() for test in contract_tests)
        ):
            errors.append(f"group {group_name} contract_tests must list non-empty correctness gates")
    forbidden_claims = {"security-approved", "provenance-approved", "hardware-passed", "constant-time-proven"}
    plan_text = json.dumps(plan, sort_keys=True)
    for claim in forbidden_claims:
        if claim in plan_text:
            errors.append(f"plan must not contain approval phrase: {claim}")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="XinYi crypto host benchmark harness skeleton")
    parser.add_argument(
        "--plan-only",
        action="store_true",
        help="emit and validate a host-only benchmark plan without timing (default)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="print the full JSON plan instead of the compact success marker",
    )
    parser.add_argument(
        "--run-timing",
        action="store_true",
        help="run bounded opt-in PC host timing smoke; never an MCU/security/hardware claim",
    )
    parser.add_argument(
        "--i-understand-host-only-timing",
        action="store_true",
        help="required acknowledgement that --run-timing is PC-only and does not validate MCU/security claims",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=3,
        help="bounded host timing smoke iterations per input size when --run-timing is enabled",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    manifest = load_manifest()

    if args.run_timing:
        if not args.i_understand_host_only_timing:
            print(
                "crypto_benchmark_host refused: --run-timing requires "
                "--i-understand-host-only-timing and remains PC-only/no-MCU-claim",
                file=sys.stderr,
            )
            return 2
        if args.iterations < HOST_TIMING_MIN_ITERATIONS or args.iterations > HOST_TIMING_MAX_ITERATIONS:
            print(
                "crypto_benchmark_host refused: --iterations must stay within "
                f"{HOST_TIMING_MIN_ITERATIONS}..{HOST_TIMING_MAX_ITERATIONS} for bounded PC-only smoke",
                file=sys.stderr,
            )
            return 2
        record = build_timing_record(manifest, args.iterations)
        errors = validate_timing_record(record, args.iterations)
        if errors:
            print("crypto_benchmark_host timing record failed:", file=sys.stderr)
            for error in errors:
                print(f"- {error}", file=sys.stderr)
            return 1
        if args.json:
            print(json.dumps(record, ensure_ascii=False, indent=2, sort_keys=True))
        else:
            print(
                "crypto_benchmark_host_timing_ok "
                f"status={HOST_TIMING_ALLOWED_STATUS} groups={len(record['groups'])} "
                f"iterations={args.iterations} claim=pc-only-no-mcu-security-hardware"
            )
        return 0

    plan = build_plan(manifest)
    errors = validate_plan(plan)
    if errors:
        print("crypto_benchmark_host plan failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    if args.json:
        print(json.dumps(plan, ensure_ascii=False, indent=2, sort_keys=True))
    else:
        print(
            "crypto_benchmark_host_plan_ok status=host-plan-only-no-timing "
            f"groups={len(plan['groups'])} timing=disabled"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
