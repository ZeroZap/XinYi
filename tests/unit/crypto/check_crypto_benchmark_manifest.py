#!/usr/bin/env python3
"""Validate the XinYi crypto benchmark manifest policy.

This is intentionally a policy smoke, not a benchmark runner. It keeps the first
benchmark-harness slice free of timing, speed thresholds, hardware claims, and
security/provenance approval language.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]
MANIFEST_PATH = ROOT / "tests" / "unit" / "crypto" / "crypto_benchmark_manifest.json"
PROPOSAL_PATH = ROOT / "docs" / "design" / "xinyi-crypto-benchmark-harness-proposal-2026-08-14.md"
REVIEW_MANIFEST_PATH = ROOT / "components" / "crypto" / "crypto_review_manifest.json"
UNIT_CMAKE_PATH = ROOT / "tests" / "unit" / "CMakeLists.txt"

REQUIRED_NO_CLAIMS = {
    "no security approval",
    "no provenance approval",
    "no MCU timing claim",
    "no hardware validation",
    "no side-channel claim",
    "no production enablement",
}

REQUIRED_RESULT_METADATA = {
    "algorithm_id",
    "source_ownership",
    "input_sizes",
    "iterations",
    "warmup_iterations",
    "test_key_or_seed_policy",
    "correctness_gate",
    "compiler",
    "compile_flags",
    "hal_platform",
    "commit_hash",
    "dirty_state",
}

REQUIRED_PLATFORM_TIERS = {
    "host_manifest_smoke",
    "host_micro_benchmark_opt_in",
    "stm32u5_compile_probe",
    "mcu_cycle_record",
}

REQUIRED_ALGORITHM_GROUPS = {
    "crc_base64_hex",
    "hash_hmac_blake2",
    "cipher_aead_lwc",
    "random_csprng",
    "placeholder_or_focused_only",
}

FORBIDDEN_APPROVAL_WORDS = {
    "security-approved",
    "provenance-approved",
    "hardware-passed",
    "production-ready",
    "constant-time-proven",
}

FORBIDDEN_INPUTS = {
    "production_key",
    "secure_boot_signing_key",
    "product_identity_material",
    "customer_secret",
    "certificate_private_key",
}


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def as_dict(value: Any, field: str, errors: list[str]) -> dict[str, Any]:
    require(isinstance(value, dict), f"{field} must be an object", errors)
    return value if isinstance(value, dict) else {}


def as_list(value: Any, field: str, errors: list[str]) -> list[Any]:
    require(isinstance(value, list), f"{field} must be a list", errors)
    return value if isinstance(value, list) else []


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def validate() -> list[str]:
    errors: list[str] = []
    manifest = load_json(MANIFEST_PATH)
    review_manifest = load_json(REVIEW_MANIFEST_PATH)
    proposal_text = PROPOSAL_PATH.read_text(encoding="utf-8")
    unit_cmake_text = UNIT_CMAKE_PATH.read_text(encoding="utf-8")
    manifest_text = MANIFEST_PATH.read_text(encoding="utf-8")

    require(manifest.get("schema_version") == 1, "schema_version must be 1", errors)
    require(
        manifest.get("status") == "policy-smoke-only-no-timing",
        "status must stay policy-smoke-only-no-timing until a separate timing slice lands",
        errors,
    )
    require(manifest.get("component") == "components/crypto", "component must be components/crypto", errors)
    require(
        manifest.get("proposal") == "docs/design/xinyi-crypto-benchmark-harness-proposal-2026-08-14.md",
        "manifest must link the benchmark harness proposal",
        errors,
    )
    require(
        "crypto_benchmark_manifest" in unit_cmake_text,
        "crypto_benchmark_manifest CTest must remain registered",
        errors,
    )

    policy = as_dict(manifest.get("policy"), "policy", errors)
    no_claims = set(as_list(policy.get("no_claims"), "policy.no_claims", errors))
    require(REQUIRED_NO_CLAIMS <= no_claims, "policy.no_claims is missing required no-claim boundaries", errors)

    metadata = set(as_list(policy.get("required_result_metadata"), "policy.required_result_metadata", errors))
    require(
        REQUIRED_RESULT_METADATA <= metadata,
        "policy.required_result_metadata must include reproducibility fields before timing exists",
        errors,
    )

    forbidden_inputs = set(as_list(policy.get("forbidden_inputs"), "policy.forbidden_inputs", errors))
    require(
        FORBIDDEN_INPUTS <= forbidden_inputs,
        "policy.forbidden_inputs must ban production/customer/identity keys",
        errors,
    )

    platform_tiers = as_list(manifest.get("platform_tiers"), "platform_tiers", errors)
    tier_ids = {tier.get("id") for tier in platform_tiers if isinstance(tier, dict)}
    require(REQUIRED_PLATFORM_TIERS == tier_ids, "platform_tiers must keep the four evidence tiers", errors)
    for tier in platform_tiers:
        if not isinstance(tier, dict):
            continue
        text = " ".join(str(tier.get(key, "")) for key in ("evidence", "allowed_claim"))
        if tier.get("id") == "host_manifest_smoke":
            require("no timing is executed" in text, "host_manifest_smoke must explicitly avoid timing", errors)
        if tier.get("id") == "host_micro_benchmark_opt_in":
            require("not MCU performance" in text, "host timing tier must not claim MCU performance", errors)
        if tier.get("id") == "stm32u5_compile_probe":
            require("not timing or hardware pass" in text, "STM32U5 compile tier must not claim hardware pass", errors)

    algorithm_groups = as_list(manifest.get("algorithm_groups"), "algorithm_groups", errors)
    group_ids = {group.get("id") for group in algorithm_groups if isinstance(group, dict)}
    require(REQUIRED_ALGORITHM_GROUPS == group_ids, "algorithm_groups must keep the expected bounded groups", errors)
    for group in algorithm_groups:
        if not isinstance(group, dict):
            continue
        tests = as_list(group.get("contract_tests"), f"algorithm_groups[{group.get('id')}].contract_tests", errors)
        require(len(tests) > 0, f"algorithm group {group.get('id')} must list at least one correctness gate", errors)
        sizes = as_list(group.get("input_sizes"), f"algorithm_groups[{group.get('id')}].input_sizes", errors)
        require(all(isinstance(size, int) and size >= 0 for size in sizes), f"algorithm group {group.get('id')} input_sizes must be non-negative integers", errors)
        input_policy = str(group.get("benchmark_input_policy", ""))
        require(
            "production" in input_policy or "secret" in input_policy or "policy boundary" in input_policy,
            f"algorithm group {group.get('id')} must state benchmark input/key policy",
            errors,
        )

    review_algorithms = review_manifest.get("algorithms", [])
    require(
        isinstance(review_algorithms, list) and len(review_algorithms) > 0,
        "crypto review manifest must still list algorithms",
        errors,
    )
    require(
        review_manifest.get("status") == "contract-guarded",
        "crypto review manifest must not be upgraded by benchmark policy smoke",
        errors,
    )
    for algorithm in review_algorithms if isinstance(review_algorithms, list) else []:
        if not isinstance(algorithm, dict):
            continue
        require(
            algorithm.get("provenance_status") == "review-pending",
            f"{algorithm.get('id')} provenance_status must remain review-pending",
            errors,
        )

    for word in FORBIDDEN_APPROVAL_WORDS:
        require(word not in manifest_text, f"benchmark manifest must not contain approval phrase: {word}", errors)

    for phrase in (
        "不计时",
        "默认 `make test-unit` 不因机器性能波动失败",
        "避免把 host CTest 或未校准 timing 当作 MCU 性能结论",
        "不使用真实密钥",
    ):
        require(phrase in proposal_text, f"benchmark proposal must preserve no-claim phrase: {phrase}", errors)

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("crypto_benchmark_manifest failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "crypto_benchmark_manifest_ok status=policy-smoke-only-no-timing "
        f"groups={len(REQUIRED_ALGORITHM_GROUPS)} tiers={len(REQUIRED_PLATFORM_TIERS)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
