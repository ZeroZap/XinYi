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
import subprocess
import sys
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


def load_manifest() -> dict[str, Any]:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def git_value(args: list[str]) -> str:
    try:
        return subprocess.check_output(args, cwd=ROOT, text=True, stderr=subprocess.DEVNULL).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "unavailable"


def build_plan(manifest: dict[str, Any]) -> dict[str, Any]:
    groups = []
    for group in manifest.get("algorithm_groups", []):
        if not isinstance(group, dict):
            continue
        groups.append(
            {
                "algorithm_id": group.get("id"),
                "source_ownership": group.get("source_ownership"),
                "input_sizes": group.get("input_sizes", []),
                "test_key_or_seed_policy": group.get("benchmark_input_policy"),
                "contract_tests": group.get("contract_tests", []),
                **REQUIRED_METADATA_DEFAULTS,
            }
        )

    return {
        "status": "host-plan-only-no-timing",
        "component": manifest.get("component"),
        "manifest": str(MANIFEST_PATH.relative_to(ROOT)),
        "proposal": manifest.get("proposal"),
        "record_template": manifest.get("benchmark_record_template"),
        "host": {
            "python": sys.version.split()[0],
            "system": platform.system(),
            "machine": platform.machine(),
            "commit_context": git_value(["git", "rev-parse", "--short", "HEAD"]),
            "dirty_context": git_value(["git", "status", "--short"]),
        },
        "no_claims": manifest.get("policy", {}).get("no_claims", []),
        "groups": groups,
    }


def validate_plan(plan: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if plan.get("status") != "host-plan-only-no-timing":
        errors.append("plan status must remain host-plan-only-no-timing")
    if "groups" not in plan or not plan["groups"]:
        errors.append("plan must include algorithm groups from the manifest")
    for group in plan.get("groups", []):
        for field in (
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
        ):
            if field not in group:
                errors.append(f"group {group.get('algorithm_id')} missing metadata field: {field}")
        if group.get("iterations") != 0 or group.get("warmup_iterations") != 0:
            errors.append(f"group {group.get('algorithm_id')} must not run timing in plan-only mode")
        if not group.get("contract_tests"):
            errors.append(f"group {group.get('algorithm_id')} must list correctness gates")
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
        help="reserved for a future explicit opt-in timing slice; currently rejected",
    )
    parser.add_argument(
        "--i-understand-host-only-timing",
        action="store_true",
        help="future safety acknowledgement for host timing; no effect until timing is implemented",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.run_timing:
        print(
            "crypto_benchmark_host refused: timing is not implemented in this policy slice; "
            "add a separate opt-in host-timing record before running benchmarks",
            file=sys.stderr,
        )
        return 2

    manifest = load_manifest()
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
