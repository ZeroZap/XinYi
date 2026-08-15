#!/usr/bin/env python3
"""Opt-in STM32U5 compile probe for the XinYi crypto benchmark harness.

Default execution is plan-only so `make test-unit` remains a PC/unit gate and does
not require the ARM toolchain. The real compile path requires an explicit
acknowledgement and records compile-only/no-timing/no-hardware boundaries.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]
MANIFEST_PATH = ROOT / "tests" / "unit" / "crypto" / "crypto_benchmark_manifest.json"
BUILD_DIR = ROOT / "build" / "crypto_benchmark_stm32u5_probe"
TARGET = "xy_tiny_crypto"

COMPILE_ONLY_STATUS = "target-compile-only-no-timing-no-hardware-claim"
FORBIDDEN_CLAIMS = {
    "security-approved",
    "provenance-approved",
    "hardware-passed",
    "constant-time-proven",
    "production-ready",
    "mcu-cycle-recorded",
}


def load_manifest() -> dict[str, Any]:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


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


def command_strings() -> dict[str, str]:
    configure = (
        f"cmake -S {ROOT} -B {BUILD_DIR} -DHAL_PLATFORM=STM32U5 "
        "-DCMAKE_BUILD_TYPE=Release -DKCONFIG_OVERRIDES=BUILD_TESTING=OFF"
    )
    build = f"cmake --build {BUILD_DIR} --target {TARGET} -j$(nproc)"
    return {"configure": configure, "build": build}


def build_plan(manifest: dict[str, Any]) -> dict[str, Any]:
    metadata = validated_manifest_metadata(manifest)
    return {
        "status": "stm32u5-compile-probe-plan-only-no-build",
        "component": metadata["component"],
        "manifest": str(MANIFEST_PATH.relative_to(ROOT)),
        "proposal": metadata["proposal"],
        "record_template": metadata["benchmark_record_template"],
        "no_claims": metadata["no_claims"],
        "hal_platform": "STM32U5",
        "target": TARGET,
        "build_dir": str(BUILD_DIR.relative_to(ROOT)),
        "commands": command_strings(),
        "required_acknowledgement": "--run-compile --i-understand-target-compile-only",
        "evidence_boundary": (
            "plan only by default; opt-in target compile proves only STM32U5 compile reachability, "
            "not timing, hardware validation, security approval, provenance approval, side-channel proof, "
            "or production enablement"
        ),
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


def validate_plan(plan: dict[str, Any]) -> list[str]:
    errors = validate_record_metadata(plan)
    if plan.get("status") != "stm32u5-compile-probe-plan-only-no-build":
        errors.append("plan status must stay plan-only/no-build")
    if plan.get("hal_platform") != "STM32U5":
        errors.append("plan must target STM32U5")
    if plan.get("target") != TARGET:
        errors.append(f"plan target must be {TARGET}")
    command_text = " ".join(plan.get("commands", {}).values())
    for token in ("HAL_PLATFORM=STM32U5", "BUILD_TESTING=OFF", TARGET):
        if token not in command_text:
            errors.append(f"compile plan command missing {token}")
    boundary = str(plan.get("evidence_boundary", ""))
    for phrase in ("not timing", "hardware validation", "security approval", "provenance approval"):
        if phrase not in boundary:
            errors.append(f"evidence boundary missing: {phrase}")
    for claim in FORBIDDEN_CLAIMS:
        if claim in json.dumps(plan, sort_keys=True):
            errors.append(f"plan must not contain approval/result phrase: {claim}")
    return errors


def run_command(args: list[str]) -> dict[str, Any]:
    completed = subprocess.run(
        args,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return {
        "command": " ".join(args),
        "exit_code": completed.returncode,
        "output_tail": completed.stdout.splitlines()[-40:],
    }


def run_compile_probe(manifest: dict[str, Any]) -> dict[str, Any]:
    metadata = validated_manifest_metadata(manifest)
    configure_result = run_command([
        "cmake",
        "-S",
        str(ROOT),
        "-B",
        str(BUILD_DIR),
        "-DHAL_PLATFORM=STM32U5",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DKCONFIG_OVERRIDES=BUILD_TESTING=OFF",
    ])
    build_result: dict[str, Any] | None = None
    if configure_result["exit_code"] == 0:
        build_result = run_command(["cmake", "--build", str(BUILD_DIR), "--target", TARGET, "-j"])

    ok = configure_result["exit_code"] == 0 and build_result is not None and build_result["exit_code"] == 0
    return {
        "status": COMPILE_ONLY_STATUS if ok else "target-compile-probe-failed",
        "component": metadata["component"],
        "manifest": str(MANIFEST_PATH.relative_to(ROOT)),
        "proposal": metadata["proposal"],
        "record_template": metadata["benchmark_record_template"],
        "no_claims": metadata["no_claims"],
        "hal_platform": "STM32U5",
        "target": TARGET,
        "build_dir": str(BUILD_DIR.relative_to(ROOT)),
        "evidence_boundary": (
            "target compile output only; not benchmark timing, not MCU cycle measurement, not hardware validation, "
            "not security approval, not provenance approval, not side-channel proof, not production enablement"
        ),
        "configure": configure_result,
        "build": build_result,
    }


def validate_command_result(value: Any, field: str) -> list[str]:
    errors: list[str] = []
    if not isinstance(value, dict):
        return [f"{field} must be an object"]
    command = value.get("command")
    if not isinstance(command, str) or not command.strip():
        errors.append(f"{field}.command must be a non-empty string")
    if type(value.get("exit_code")) is not int or value["exit_code"] != 0:
        errors.append(f"{field}.exit_code must be integer zero")
    output_tail = value.get("output_tail")
    if not isinstance(output_tail, list) or not all(isinstance(line, str) for line in output_tail):
        errors.append(f"{field}.output_tail must be a list of strings")
    return errors


def validate_compile_record(record: dict[str, Any]) -> list[str]:
    errors = validate_record_metadata(record)
    if record.get("status") != COMPILE_ONLY_STATUS:
        errors.append("compile probe did not complete successfully")
    if record.get("hal_platform") != "STM32U5":
        errors.append("compile record must target STM32U5")
    if record.get("target") != TARGET:
        errors.append(f"compile record target must be {TARGET}")
    build_dir = record.get("build_dir")
    if not isinstance(build_dir, str) or not build_dir.strip():
        errors.append("compile record build_dir must be a non-empty string")
    errors.extend(validate_command_result(record.get("configure"), "configure"))
    errors.extend(validate_command_result(record.get("build"), "build"))
    boundary = str(record.get("evidence_boundary", ""))
    for phrase in (
        "not benchmark timing",
        "not hardware validation",
        "not security approval",
        "not provenance approval",
    ):
        if phrase not in boundary:
            errors.append(f"compile record evidence boundary missing: {phrase}")
    for claim in FORBIDDEN_CLAIMS:
        if claim in json.dumps(record, sort_keys=True):
            errors.append(f"compile record must not contain approval/result phrase: {claim}")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="XinYi crypto STM32U5 compile probe")
    parser.add_argument("--plan-only", action="store_true", help="validate and print a compile-probe plan without building")
    parser.add_argument("--json", action="store_true", help="print full JSON plan/record")
    parser.add_argument("--run-compile", action="store_true", help="run the opt-in STM32U5 compile probe")
    parser.add_argument(
        "--i-understand-target-compile-only",
        action="store_true",
        help="required acknowledgement that this proves compile-only, not timing/security/hardware",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    manifest = load_manifest()

    if args.run_compile:
        if not args.i_understand_target_compile_only:
            print(
                "crypto_benchmark_stm32u5_compile_probe refused: --run-compile requires "
                "--i-understand-target-compile-only and remains compile-only/no-timing/no-hardware-claim",
                file=sys.stderr,
            )
            return 2
        record = run_compile_probe(manifest)
        errors = validate_compile_record(record)
        if errors:
            print("crypto_benchmark_stm32u5_compile_probe failed:", file=sys.stderr)
            for error in errors:
                print(f"- {error}", file=sys.stderr)
            if args.json:
                print(json.dumps(record, ensure_ascii=False, indent=2, sort_keys=True))
            return 1
        if args.json:
            print(json.dumps(record, ensure_ascii=False, indent=2, sort_keys=True))
        else:
            print(
                "crypto_benchmark_stm32u5_compile_probe_ok "
                f"status={COMPILE_ONLY_STATUS} target={TARGET} claim=compile-only-no-timing-hardware-security"
            )
        return 0

    plan = build_plan(manifest)
    errors = validate_plan(plan)
    if errors:
        print("crypto_benchmark_stm32u5_compile_probe plan failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    if args.json:
        print(json.dumps(plan, ensure_ascii=False, indent=2, sort_keys=True))
    else:
        print(
            "crypto_benchmark_stm32u5_compile_probe_plan_ok "
            f"status={plan['status']} target={TARGET} build=disabled-by-default"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
