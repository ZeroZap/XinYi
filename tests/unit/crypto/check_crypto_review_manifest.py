#!/usr/bin/env python3
"""Validate the XinYi crypto review manifest.

This is a policy smoke guard: it keeps the component at contract-guarded / review-pending
unless a real review record is wired into the manifest. It intentionally does not perform
cryptographic security review.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]
MANIFEST_PATH = ROOT / "components" / "crypto" / "crypto_review_manifest.json"
UNIT_CMAKE_PATH = ROOT / "tests" / "unit" / "CMakeLists.txt"

ALLOWED_TOP_STATUS = {"contract-guarded"}
ALLOWED_REVIEW_PENDING = {"review-pending"}
ALLOWED_DUPLICATE_POLICIES = {
    "source-map-pending",
    "single-active-source",
    "focused-test-only-until-root-ownership-decided",
    "focused-test-only-upstream-material",
}
REQUIRED_ALGORITHM_IDS = {
    "crc",
    "base64_hex",
    "random_csprng",
    "md5_sha256_hmac",
    "aes_sm3_sm4_chacha20",
    "sm2",
    "lwc_ascon_tinyjambu_photon_beetle",
    "curve25519_generic",
    "curve25519_cortex_m0",
}
REQUIRED_UNREVIEWED_ROOT_SOURCE_IDS = {
    "blake2_root_copy",
    "ecdsa_root_copy",
}
ALLOWED_UNREVIEWED_ROOT_SOURCE_STATUS = {"root-source-unreviewed"}
APPROVED_STATUSES = {
    "provenance-reviewed",
    "security-reviewed-limited",
    "security-rejected",
    "hardware-validated",
}


def _rel_exists(rel_path: str) -> bool:
    return (ROOT / rel_path).exists()


def _read_policy_document(rel_path: str) -> str:
    path = ROOT / rel_path
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def _require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def _registered_crypto_ctests() -> set[str]:
    """Return crypto-related CTest names declared in tests/unit/CMakeLists.txt."""
    cmake_text = UNIT_CMAKE_PATH.read_text(encoding="utf-8")
    names = set(re.findall(r"xy_add_unit_test\(\s*\S+\s+(crypto_[A-Za-z0-9_]+)\s+UNITY", cmake_text))
    names.update(re.findall(r"add_test\(\s*NAME\s+(crypto_[A-Za-z0-9_]+)", cmake_text))
    return names


def validate_manifest(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []

    _require(data.get("schema_version") == 1, "schema_version must be 1", errors)
    _require(data.get("component") == "components/crypto", "component must be components/crypto", errors)
    _require(data.get("status") in ALLOWED_TOP_STATUS, "top-level status must remain contract-guarded", errors)

    policy = data.get("policy")
    source_map_text = ""
    _require(isinstance(policy, dict), "policy must be an object", errors)
    if isinstance(policy, dict):
        for key in (
            "plan",
            "source_ownership_map",
            "record_template",
            "default_component_enablement",
            "approval_guard",
        ):
            value = policy.get(key)
            _require(isinstance(value, str) and len(value) > 0, f"policy.{key} is required", errors)
        for key in ("plan", "source_ownership_map", "record_template"):
            if isinstance(policy.get(key), str):
                _require(_rel_exists(policy[key]), f"policy.{key} path does not exist: {policy[key]}", errors)
        if isinstance(policy.get("source_ownership_map"), str):
            source_map_text = _read_policy_document(policy["source_ownership_map"])
        _require(
            "default-off" in policy.get("default_component_enablement", ""),
            "policy.default_component_enablement must preserve default-off wording",
            errors,
        )

    algorithms = data.get("algorithms")
    _require(isinstance(algorithms, list) and len(algorithms) > 0, "algorithms must be a non-empty list", errors)
    if not isinstance(algorithms, list):
        return errors

    seen_ids: set[str] = set()
    registered_crypto_ctests = _registered_crypto_ctests()
    for index, algorithm in enumerate(algorithms):
        prefix = f"algorithms[{index}]"
        _require(isinstance(algorithm, dict), f"{prefix} must be an object", errors)
        if not isinstance(algorithm, dict):
            continue

        algorithm_id = algorithm.get("id")
        _require(isinstance(algorithm_id, str) and len(algorithm_id) > 0, f"{prefix}.id is required", errors)
        if isinstance(algorithm_id, str):
            _require(algorithm_id not in seen_ids, f"duplicate algorithm id: {algorithm_id}", errors)
            seen_ids.add(algorithm_id)
            prefix = f"algorithm {algorithm_id}"

        for key in ("description", "allowed_usage", "duplicate_source_policy"):
            value = algorithm.get(key)
            _require(isinstance(value, str) and len(value) > 0, f"{prefix}.{key} is required", errors)

        _require(
            algorithm.get("duplicate_source_policy") in ALLOWED_DUPLICATE_POLICIES,
            f"{prefix}.duplicate_source_policy is not recognized",
            errors,
        )

        for key in ("contract_tests", "runtime_sources", "focused_test_sources"):
            _require(isinstance(algorithm.get(key), list), f"{prefix}.{key} must be a list", errors)
        _require(bool(algorithm.get("contract_tests")), f"{prefix}.contract_tests must not be empty", errors)
        _require(bool(algorithm.get("focused_test_sources")), f"{prefix}.focused_test_sources must not be empty", errors)
        contract_tests = algorithm.get("contract_tests")
        if isinstance(contract_tests, list):
            for ctest_name in contract_tests:
                _require(isinstance(ctest_name, str) and len(ctest_name) > 0, f"{prefix}.contract_tests entries must be strings", errors)
                if isinstance(ctest_name, str) and len(ctest_name) > 0:
                    _require(
                        ctest_name in registered_crypto_ctests,
                        f"{prefix}.contract_tests references unregistered CTest: {ctest_name}",
                        errors,
                    )

        for source_key in ("runtime_sources", "focused_test_sources"):
            sources = algorithm.get(source_key)
            if isinstance(sources, list):
                for rel_source in sources:
                    _require(isinstance(rel_source, str) and len(rel_source) > 0, f"{prefix}.{source_key} entries must be strings", errors)
                    if isinstance(rel_source, str) and len(rel_source) > 0:
                        _require(_rel_exists(rel_source), f"{prefix}.{source_key} path does not exist: {rel_source}", errors)
                        _require(
                            rel_source in source_map_text,
                            f"{prefix}.{source_key} is missing from source ownership map: {rel_source}",
                            errors,
                        )

        for status_key in ("provenance_status", "security_status"):
            status = algorithm.get(status_key)
            _require(isinstance(status, str) and len(status) > 0, f"{prefix}.{status_key} is required", errors)
            review_record = algorithm.get("review_record")
            if status in APPROVED_STATUSES:
                _require(
                    isinstance(review_record, str) and len(review_record) > 0 and _rel_exists(review_record),
                    f"{prefix}.{status_key}={status} requires an existing review_record",
                    errors,
                )
            else:
                _require(status in ALLOWED_REVIEW_PENDING, f"{prefix}.{status_key} must remain review-pending", errors)

        if algorithm.get("review_record") is not None:
            review_record = algorithm.get("review_record")
            _require(isinstance(review_record, str) and len(review_record) > 0, f"{prefix}.review_record must be null or path", errors)
            if isinstance(review_record, str) and len(review_record) > 0:
                _require(_rel_exists(review_record), f"{prefix}.review_record path does not exist: {review_record}", errors)

    missing = REQUIRED_ALGORITHM_IDS - seen_ids
    extra = seen_ids - REQUIRED_ALGORITHM_IDS
    _require(not missing, f"missing required algorithm ids: {sorted(missing)}", errors)
    _require(not extra, f"unexpected algorithm ids: {sorted(extra)}", errors)

    unreviewed_root_sources = data.get("unreviewed_root_sources")
    _require(
        isinstance(unreviewed_root_sources, list),
        "unreviewed_root_sources must be a list of root aggregate copies that lack algorithm review",
        errors,
    )
    if isinstance(unreviewed_root_sources, list):
        seen_unreviewed_ids: set[str] = set()
        for index, entry in enumerate(unreviewed_root_sources):
            prefix = f"unreviewed_root_sources[{index}]"
            _require(isinstance(entry, dict), f"{prefix} must be an object", errors)
            if not isinstance(entry, dict):
                continue

            entry_id = entry.get("id")
            _require(isinstance(entry_id, str) and len(entry_id) > 0, f"{prefix}.id is required", errors)
            if isinstance(entry_id, str):
                _require(entry_id not in seen_unreviewed_ids, f"duplicate unreviewed root source id: {entry_id}", errors)
                seen_unreviewed_ids.add(entry_id)
                prefix = f"unreviewed root source {entry_id}"

            for key in ("source", "mapped_in", "status", "reason"):
                value = entry.get(key)
                _require(isinstance(value, str) and len(value) > 0, f"{prefix}.{key} is required", errors)

            source = entry.get("source")
            if isinstance(source, str) and len(source) > 0:
                _require(_rel_exists(source), f"{prefix}.source path does not exist: {source}", errors)
                _require(source in source_map_text, f"{prefix}.source is missing from source ownership map: {source}", errors)

            mapped_in = entry.get("mapped_in")
            if isinstance(mapped_in, str) and len(mapped_in) > 0:
                _require(_rel_exists(mapped_in), f"{prefix}.mapped_in path does not exist: {mapped_in}", errors)

            _require(
                entry.get("status") in ALLOWED_UNREVIEWED_ROOT_SOURCE_STATUS,
                f"{prefix}.status must remain root-source-unreviewed",
                errors,
            )

        missing_unreviewed = REQUIRED_UNREVIEWED_ROOT_SOURCE_IDS - seen_unreviewed_ids
        extra_unreviewed = seen_unreviewed_ids - REQUIRED_UNREVIEWED_ROOT_SOURCE_IDS
        _require(
            not missing_unreviewed,
            f"missing required unreviewed root source ids: {sorted(missing_unreviewed)}",
            errors,
        )
        _require(
            not extra_unreviewed,
            f"unexpected unreviewed root source ids: {sorted(extra_unreviewed)}",
            errors,
        )

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate crypto review manifest policy guard")
    parser.add_argument("--manifest", default=str(MANIFEST_PATH), help="manifest path")
    args = parser.parse_args()

    manifest_path = Path(args.manifest)
    data = json.loads(manifest_path.read_text(encoding="utf-8"))
    errors = validate_manifest(data)
    if errors:
        print("crypto review manifest validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "crypto_review_manifest_ok "
        f"algorithms={len(data['algorithms'])} "
        f"unreviewed_root_sources={len(data.get('unreviewed_root_sources', []))} "
        f"status={data['status']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
