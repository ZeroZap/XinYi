#!/usr/bin/env python3
"""Generate and independently validate the bounded Pandora MCU SBOM record.

The default mode is policy-only. Generation requires an explicit acknowledgement and
an already-built Pandora RTOS directory; it never upgrades release or legal status.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
POLICY = ROOT / "docs/validation/pandora-release-sbom-policy.json"
SCOPE = ROOT / "docs/validation/pandora-release-scope.json"
CUBE_RELATIVE = Path("MCU/ST/STM32L4/STM32CubeL4")
TARGET = "pandora_stm32l475_rtos"
ARTIFACT = f"{TARGET}.bin"
OUTPUT = "pandora_stm32l475_rtos.bin.cdx.json"
ACK = "--i-understand-mcu-sbom-generation"


def args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generate", action="store_true")
    parser.add_argument("--verify", action="store_true")
    parser.add_argument(ACK, action="store_true")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--output", type=Path)
    return parser.parse_args()


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def output(command: list[str], cwd: Path) -> str:
    return subprocess.run(command, cwd=cwd, check=True, capture_output=True, text=True).stdout.strip()


def load_policy() -> dict[str, object]:
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    if policy.get("status") != "GENERATOR_IMPLEMENTED_REVIEW_PENDING":
        raise SystemExit("Pandora SBOM policy must record the implemented generator")
    if policy.get("format") != "CycloneDX JSON 1.6" or policy.get("approval") != "REVIEW_PENDING":
        raise SystemExit("Pandora SBOM policy format or approval mismatch")
    return policy


def validate_scope() -> None:
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    if scope.get("status") != "FROZEN_PRE_RC_NO_GO" or scope.get("release_decision") != "NO-GO":
        raise SystemExit("Pandora scope must remain frozen/no-go")
    inputs = scope.get("release_inputs")
    artifacts = inputs.get("mcu_release_artifacts") if isinstance(inputs, dict) else None
    if not isinstance(artifacts, list) or len(artifacts) != 1 or artifacts[0].get("target") != TARGET:
        raise SystemExit("Pandora scope must select the frozen RTOS artifact")


def source_from_object(map_dir: Path, token: str) -> Path | None:
    match = re.search(r"CMakeFiles/[^/]+\.dir/(.+\.c)\.obj", token)
    if not match:
        return None
    relative = match.group(1).replace("/__", "/..")
    candidate = (map_dir / relative).resolve()
    if not candidate.is_file():
        candidate = (ROOT / relative.lstrip("../")).resolve()
    if candidate.is_file() and ROOT in candidate.parents:
        return candidate
    return None


def first_party_sources(build_dir: Path, map_file: Path) -> list[str]:
    found: set[str] = set()
    for line in map_file.read_text(encoding="utf-8", errors="replace").splitlines():
        for token in re.findall(r"[^\s()]+\.c\.obj", line):
            source = source_from_object(map_file.parent, token)
            if source is not None:
                found.add(source.relative_to(ROOT).as_posix())
    if not found:
        raise SystemExit("link map contains no resolvable first-party C source inventory")
    return sorted(found)


def vendor_inventory(build_dir: Path) -> list[dict[str, object]]:
    build_dir = build_dir.resolve()
    archives = [
        build_dir / "third_party/libfreertos_kernel.a",
        build_dir / "components/clib/xy_clib/libxy_xy_clib.a",
    ]
    inventory: list[dict[str, object]] = []
    for archive in archives:
        if not archive.is_file():
            raise SystemExit(f"linked vendor/owned archive is missing: {archive}")
        members = output(["ar", "t", str(archive)], ROOT).splitlines()
        inventory.append({
            "archive": archive.relative_to(ROOT).as_posix() if ROOT in archive.parents else str(archive),
            "sha256": sha256(archive),
            "members": [member for member in members if member.endswith((".o", ".obj"))],
        })
    return inventory


def generate(build_dir: Path) -> dict[str, object]:
    load_policy()
    validate_scope()
    artifact = build_dir / "boards/pandora_stm32l475" / ARTIFACT
    map_file = build_dir / "boards/pandora_stm32l475" / f"{TARGET}.map"
    if not artifact.is_file() or not map_file.is_file():
        raise SystemExit("Pandora build directory must contain the RTOS BIN and link map")
    source_archive = subprocess.run(["git", "archive", "--format=tar", "HEAD"], cwd=ROOT,
                                    check=True, capture_output=True).stdout
    source_commit = output(["git", "rev-parse", "HEAD"], ROOT)
    artifact_hash = sha256(artifact)
    first_party = first_party_sources(build_dir, map_file)
    components = []
    for path in first_party:
        source_scope = "stm32cubel4-vendor" if path.startswith(f"{CUBE_RELATIVE.as_posix()}/") else "xinyi-first-party"
        components.append({
            "type": "file", "bom-ref": f"file:{path}", "name": path,
            "version": "source-commit-bound",
            "hashes": [{"alg": "SHA-256", "content": sha256(ROOT / path)}],
            "properties": [{"name": "xinyi:source-scope", "value": source_scope}],
        })
    vendor = vendor_inventory(build_dir)
    return {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "serialNumber": f"urn:uuid:{uuid.uuid5(uuid.NAMESPACE_URL, source_commit + artifact_hash)}",
        "version": 1,
        "metadata": {"component": {"type": "application", "name": ARTIFACT,
                                     "version": source_commit}},
        "components": components,
        "dependencies": [{"ref": ARTIFACT, "dependsOn": [f"file:{path}" for path in first_party]}],
        "properties": [
            {"name": "xinyi:source-commit", "value": source_commit},
            {"name": "xinyi:source-archive-sha256", "value": hashlib.sha256(source_archive).hexdigest()},
            {"name": "xinyi:artifact-sha256", "value": artifact_hash},
            {"name": "xinyi:artifact-size", "value": str(artifact.stat().st_size)},
            {"name": "xinyi:link-map-sha256", "value": sha256(map_file)},
            {"name": "xinyi:vendor-archives", "value": json.dumps(vendor, sort_keys=True)},
            {"name": "xinyi:approval", "value": "REVIEW_PENDING"},
            {"name": "xinyi:evidence-boundary", "value": "MCU compile/input inventory only; no legal, security, runtime, RC, or R1 claim"},
        ],
    }


def validate(record: dict[str, object]) -> None:
    if record.get("bomFormat") != "CycloneDX" or record.get("specVersion") != "1.6":
        raise SystemExit("generated record is not CycloneDX JSON 1.6")
    properties = {item.get("name"): item.get("value") for item in record.get("properties", [])
                  if isinstance(item, dict)}
    if len(str(properties.get("xinyi:source-commit", ""))) != 40:
        raise SystemExit("source commit binding is invalid")
    if properties.get("xinyi:approval") != "REVIEW_PENDING":
        raise SystemExit("SBOM approval boundary must remain review-pending")
    components = record.get("components")
    dependencies = record.get("dependencies")
    if not isinstance(components, list) or not components or not isinstance(dependencies, list) or not dependencies:
        raise SystemExit("SBOM source inventory is empty")


def verify_file(path: Path) -> None:
    try:
        record = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit(f"SBOM output is not valid JSON: {exc}") from exc
    if not isinstance(record, dict):
        raise SystemExit("SBOM output root must be an object")
    validate(record)
    properties = {item.get("name"): item.get("value") for item in record.get("properties", [])
                  if isinstance(item, dict)}
    artifact_hash = str(properties.get("xinyi:artifact-sha256", ""))
    if not re.fullmatch(r"[0-9a-f]{64}", artifact_hash):
        raise SystemExit("SBOM artifact SHA-256 binding is invalid")
    components = record.get("components")
    print(f"pandora_release_sbom_verify_ok output={path} components={len(components) if isinstance(components, list) else 0} status=REVIEW_PENDING release_scope=blocked")


def main() -> int:
    parsed = args()
    load_policy()
    validate_scope()
    if parsed.verify:
        if parsed.generate or parsed.i_understand_mcu_sbom_generation or parsed.build_dir:
            raise SystemExit("--verify cannot be combined with generation options")
        if parsed.output is None:
            raise SystemExit("--verify requires --output")
        verify_file(parsed.output)
        return 0
    if not parsed.generate:
        if parsed.i_understand_mcu_sbom_generation or parsed.build_dir or parsed.output:
            raise SystemExit("generation options require --generate")
        print("pandora_release_sbom_plan_ok format=CycloneDX-1.6 status=GENERATION_PENDING release_scope=blocked")
        return 0
    if not parsed.i_understand_mcu_sbom_generation:
        raise SystemExit(f"--generate requires {ACK}")
    if parsed.build_dir is None or parsed.output is None:
        raise SystemExit("--generate requires --build-dir and --output")
    record = generate(parsed.build_dir)
    validate(record)
    parsed.output.parent.mkdir(parents=True, exist_ok=True)
    parsed.output.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    components = record.get("components")
    print(f"pandora_release_sbom_generated output={parsed.output} components={len(components) if isinstance(components, list) else 0} status=REVIEW_PENDING release_scope=blocked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
