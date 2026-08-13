#!/usr/bin/env python3
"""Guard Curve25519/Ed25519 root ownership policy.

This is a policy smoke test only. It verifies that Curve25519/Ed25519
remain focused-test-only until a separate root-runtime ownership slice lands,
and that the docs/manifest keep the evidence boundary explicit.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]
CRYPTO_CMAKE = ROOT / "components" / "crypto" / "CMakeLists.txt"
MANIFEST = ROOT / "components" / "crypto" / "crypto_review_manifest.json"
SOURCE_MAP = ROOT / "docs" / "design" / "xinyi-crypto-source-ownership-map-2026-08-12.md"
PROPOSAL = ROOT / "docs" / "design" / "xinyi-crypto-curve25519-root-ownership-proposal-2026-08-14.md"
UNIT_CMAKE = ROOT / "tests" / "unit" / "CMakeLists.txt"

CURVE_RUNTIME_SOURCES = {
    "components/crypto/xy_25519/xy_25519.c",
    "components/crypto/xy_25519/xy_25519_m0.c",
    "components/crypto/xy_25519/fe25519_m0.c",
}


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def registered_crypto_ctests() -> set[str]:
    text = UNIT_CMAKE.read_text(encoding="utf-8")
    names = set(re.findall(r"xy_add_unit_test\(\s*\S+\s+(crypto_[A-Za-z0-9_]+)\s+UNITY", text))
    names.update(re.findall(r"add_test\(\s*NAME\s+(crypto_[A-Za-z0-9_]+)", text))
    return names


def find_algorithm(data: dict[str, Any], algorithm_id: str) -> dict[str, Any] | None:
    for algorithm in data.get("algorithms", []):
        if isinstance(algorithm, dict) and algorithm.get("id") == algorithm_id:
            return algorithm
    return None


def validate() -> list[str]:
    errors: list[str] = []
    cmake_text = CRYPTO_CMAKE.read_text(encoding="utf-8")
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    source_map_text = SOURCE_MAP.read_text(encoding="utf-8")
    proposal_text = PROPOSAL.read_text(encoding="utf-8")
    ctests = registered_crypto_ctests()

    require("crypto_curve25519_root_policy" in ctests,
            "crypto_curve25519_root_policy CTest must remain registered", errors)

    # Root aggregate target must not silently append Curve25519/Ed25519 sources.
    for rel_source in CURVE_RUNTIME_SOURCES:
        cmake_source_literal = "${CMAKE_CURRENT_SOURCE_DIR}/" + rel_source.removeprefix("components/crypto/")
        require(cmake_source_literal not in cmake_text,
                f"root xy_tiny_crypto target must not append focused-only source: {cmake_source_literal}",
                errors)

    generic = find_algorithm(manifest, "curve25519_generic")
    m0 = find_algorithm(manifest, "curve25519_cortex_m0")
    require(generic is not None, "manifest must keep curve25519_generic entry", errors)
    require(m0 is not None, "manifest must keep curve25519_cortex_m0 entry", errors)

    if isinstance(generic, dict):
        require(generic.get("runtime_sources") == [],
                "curve25519_generic.runtime_sources must remain empty until root ownership is approved",
                errors)
        require(generic.get("duplicate_source_policy") == "focused-test-only-until-root-ownership-decided",
                "curve25519_generic duplicate policy must remain focused-test-only", errors)
        require("crypto_25519" in generic.get("contract_tests", []),
                "curve25519_generic must keep crypto_25519 focused contract guard", errors)
        allowed_usage = generic.get("allowed_usage", "")
        for phrase in ("focused-test-only ownership", "prohibited for production security-sensitive"):
            require(phrase in allowed_usage,
                    f"curve25519_generic.allowed_usage must preserve warning phrase: {phrase}",
                    errors)

    if isinstance(m0, dict):
        require(m0.get("runtime_sources") == [],
                "curve25519_cortex_m0.runtime_sources must remain empty until root ownership is approved",
                errors)
        require(m0.get("duplicate_source_policy") == "focused-test-only-upstream-material",
                "curve25519_cortex_m0 duplicate policy must remain focused-test-only upstream-material",
                errors)
        require("crypto_25519_m0" in m0.get("contract_tests", []),
                "curve25519_cortex_m0 must keep crypto_25519_m0 focused contract guard", errors)

    for phrase in (
        "Focused-test-only until root ownership is intentionally decided",
        "`crypto_25519` | `components/crypto/xy_25519/xy_25519.c` | Focused-test-only until root ownership is intentionally decided.",
        "`crypto_25519_m0` | `components/crypto/xy_25519/xy_25519_m0.c`, `components/crypto/xy_25519/fe25519_m0.c` | Focused-test-only/upstream-material boundary.",
        "not broad duplicate-source reconciliation or security validation",
    ):
        require(phrase in source_map_text,
                f"source ownership map must preserve Curve25519 boundary phrase: {phrase}",
                errors)

    for phrase in (
        "Use **Option A** until a real root consumer or product decision exists.",
        "Add a `crypto_curve25519_root_policy` smoke",
        "do not link `xy_25519.c` into the root target in the same slice",
        "No claim that host CTest output proves production key exchange",
    ):
        require(phrase in proposal_text,
                f"Curve25519 proposal must preserve policy phrase: {phrase}",
                errors)

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("crypto_curve25519_root_policy failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("crypto_curve25519_root_policy_ok root_runtime_sources=0 focused_contracts=2 status=focused-test-only")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
