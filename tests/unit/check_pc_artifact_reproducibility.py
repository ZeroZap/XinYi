#!/usr/bin/env python3
"""Rebuild one PC root artifact twice from the committed source archive."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import platform
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TARGET = "xy_device"
ARTIFACT = Path("components/device/libxy_device.a")
ENVIRONMENT_MANIFEST = ROOT / "docs/validation/pc-release-build-environment.json"
ARTIFACT_MANIFEST = ROOT / "docs/validation/pc-release-artifact-manifest.json"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--record", type=Path, help="write machine-readable evidence JSON")
    parser.add_argument(
        "--artifact-dir",
        type=Path,
        help="write the verified gate artifact and SHA-256 checksum to this directory",
    )
    parser.add_argument(
        "--verify-artifact-dir",
        type=Path,
        help="independently verify an archived artifact against its SHA-256 file",
    )
    parser.add_argument(
        "--sign-artifact-dir",
        type=Path,
        help="sign the archived artifact with an ephemeral Ed25519 CI gate key",
    )
    return parser.parse_args()


def run(command: list[str], cwd: Path) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def first_line(command: list[str]) -> str:
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    return result.stdout.splitlines()[0]


def load_environment_manifest() -> dict[str, object]:
    manifest = json.loads(ENVIRONMENT_MANIFEST.read_text(encoding="utf-8"))
    required = {
        "schema_version",
        "status",
        "platform",
        "architecture",
        "build_type",
        "hal_platform",
        "target",
        "artifact",
        "ci_runner",
        "required_tools",
        "evidence_boundary",
    }
    missing = sorted(required - manifest.keys())
    if missing:
        raise SystemExit(f"PC release build environment manifest missing fields: {missing}")
    if manifest["status"] != "PINNED_IDENTITY_RECORDED":
        raise SystemExit("PC release build environment manifest status is not fail-closed")
    expected = {
        "platform": "PC",
        "architecture": "x86_64",
        "build_type": "Release",
        "hal_platform": "PC",
        "target": TARGET,
        "artifact": str(ARTIFACT),
        "ci_runner": "ubuntu-24.04",
        "required_tools": ["cmake", "cc", "ar", "python3"],
    }
    for key, value in expected.items():
        if manifest[key] != value:
            raise SystemExit(
                f"PC release build environment manifest mismatch: {key}={manifest[key]!r}, "
                f"expected {value!r}"
            )
    boundary = str(manifest["evidence_boundary"])
    for phrase in ("not a container digest", "not R1", "PC static library only"):
        if phrase not in boundary:
            raise SystemExit(f"PC release build environment boundary missing phrase: {phrase}")
    return manifest


def load_artifact_manifest() -> dict[str, object]:
    manifest = json.loads(ARTIFACT_MANIFEST.read_text(encoding="utf-8"))
    artifacts = manifest.get("artifacts")
    if manifest.get("status") != "SELECTED_PC_ARTIFACT_SET_RECORDED":
        raise SystemExit("PC release artifact manifest status is not fail-closed")
    if not isinstance(artifacts, list) or len(artifacts) != 1:
        raise SystemExit("PC release artifact manifest must select exactly one artifact")
    selected = artifacts[0]
    if selected.get("target") != TARGET or selected.get("path") != str(ARTIFACT):
        raise SystemExit("PC release artifact manifest does not match the reproducibility target")
    if selected.get("selection") != "reproducibility-gate-only":
        raise SystemExit("PC release artifact manifest selection must remain gate-only")
    return manifest


def extract(archive: bytes, destination: Path) -> None:
    destination.mkdir()
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:") as tar:
        tar.extractall(destination, filter="data")


def build_artifact(archive: bytes, root: Path, name: str) -> tuple[str, int, bytes]:
    source = root / f"source-{name}"
    build = root / f"build-{name}"
    extract(archive, source)
    run(
        [
            "cmake",
            "-S",
            str(source),
            "-B",
            str(build),
            "-DHAL_PLATFORM=PC",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DKCONFIG_OVERRIDES=BUILD_TESTING=OFF;COMPONENT_DEVICE=ON",
        ],
        root,
    )
    run(["cmake", "--build", str(build), "--target", TARGET, "-j2"], root)
    artifact = build / ARTIFACT
    payload = artifact.read_bytes()
    return hashlib.sha256(payload).hexdigest(), len(payload), payload


def verify_archived_artifact(artifact_dir: Path) -> None:
    artifact = artifact_dir / ARTIFACT.name
    checksum = artifact_dir / f"{ARTIFACT.name}.sha256"
    if not artifact.is_file() or not checksum.is_file():
        raise SystemExit("archived artifact or SHA-256 file is missing")

    fields = checksum.read_text(encoding="utf-8").strip().split()
    if len(fields) != 2 or fields[1] != ARTIFACT.name:
        raise SystemExit("archived SHA-256 file has an invalid format or artifact name")
    expected = fields[0]
    if len(expected) != 64 or any(character not in "0123456789abcdef" for character in expected):
        raise SystemExit("archived SHA-256 value is invalid")

    actual = hashlib.sha256(artifact.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f"archived artifact SHA-256 mismatch: expected={expected} actual={actual}")
    print(
        f"pc_artifact_checksum_ok artifact={artifact} sha256={actual} "
        "verification=independent release_scope=blocked"
    )


def sign_archived_artifact(artifact_dir: Path) -> None:
    artifact = artifact_dir / ARTIFACT.name
    signature = artifact_dir / f"{ARTIFACT.name}.sig"
    public_key = artifact_dir / "pc-artifact-signing-public.pem"
    if not artifact.is_file():
        raise SystemExit("archived artifact is missing")
    with tempfile.TemporaryDirectory(prefix="xinyi-pc-artifact-signing-") as temporary:
        private_key = Path(temporary) / "private.pem"
        run(["openssl", "genpkey", "-algorithm", "ED25519", "-out", str(private_key)], ROOT)
        run(
            ["openssl", "pkey", "-in", str(private_key), "-pubout", "-out", str(public_key)],
            ROOT,
        )
        run(
            [
                "openssl", "pkeyutl", "-sign", "-rawin", "-inkey", str(private_key),
                "-in", str(artifact), "-out", str(signature),
            ],
            ROOT,
        )
    print(
        f"pc_artifact_signature_created artifact={artifact} signature={signature} "
        "algorithm=Ed25519 key_scope=ephemeral-ci-gate-only release_scope=blocked"
    )


def verify_archived_signature(artifact_dir: Path) -> None:
    artifact = artifact_dir / ARTIFACT.name
    signature = artifact_dir / f"{ARTIFACT.name}.sig"
    public_key = artifact_dir / "pc-artifact-signing-public.pem"
    if not artifact.is_file() or not signature.is_file() or not public_key.is_file():
        raise SystemExit("archived artifact, signature, or public key is missing")
    run(
        [
            "openssl", "pkeyutl", "-verify", "-rawin", "-pubin", "-inkey",
            str(public_key), "-in", str(artifact), "-sigfile", str(signature),
        ],
        ROOT,
    )
    print(
        f"pc_artifact_signature_ok artifact={artifact} signature={signature} "
        "algorithm=Ed25519 key_scope=ephemeral-ci-gate-only release_scope=blocked"
    )


def main() -> int:
    args = parse_args()
    if args.sign_artifact_dir is not None:
        if args.record is not None or args.artifact_dir is not None or args.verify_artifact_dir is not None:
            raise SystemExit("--sign-artifact-dir cannot be combined with other modes")
        sign_archived_artifact(args.sign_artifact_dir)
        verify_archived_signature(args.sign_artifact_dir)
        return 0
    if args.verify_artifact_dir is not None:
        if args.record is not None or args.artifact_dir is not None:
            raise SystemExit("--verify-artifact-dir cannot be combined with build output options")
        verify_archived_artifact(args.verify_artifact_dir)
        verify_archived_signature(args.verify_artifact_dir)
        return 0

    manifest = load_environment_manifest()
    artifact_manifest = load_artifact_manifest()
    identity = {
        "system": platform.system(),
        "machine": platform.machine(),
        "cmake": first_line(["cmake", "--version"]),
        "cc": first_line(["cc", "--version"]),
        "ar": first_line(["ar", "--version"]),
        "python": first_line(["python3", "--version"]),
    }
    if identity["system"] != "Linux" or identity["machine"] != "x86_64":
        raise SystemExit(f"unsupported PC artifact build host: {identity}")
    archive = subprocess.run(
        ["git", "archive", "--format=tar", "HEAD"],
        cwd=ROOT,
        check=True,
        capture_output=True,
    ).stdout
    source_sha = first_line(["git", "-C", str(ROOT), "rev-parse", "HEAD"])
    source_archive_sha256 = hashlib.sha256(archive).hexdigest()
    with tempfile.TemporaryDirectory(prefix="xinyi-pc-artifact-") as temporary:
        temporary_root = Path(temporary)
        first_hash, first_size, first_payload = build_artifact(archive, temporary_root, "first")
        second_hash, second_size, _ = build_artifact(archive, temporary_root, "second")

    if (first_hash, first_size) != (second_hash, second_size):
        raise SystemExit(
            "PC artifact is not reproducible: "
            f"first={first_hash}/{first_size} second={second_hash}/{second_size}"
        )

    artifact_output = None
    checksum_output = None
    if args.artifact_dir is not None:
        args.artifact_dir.mkdir(parents=True, exist_ok=True)
        artifact_output = args.artifact_dir / ARTIFACT.name
        checksum_output = args.artifact_dir / f"{ARTIFACT.name}.sha256"
        artifact_output.write_bytes(first_payload)
        checksum_output.write_text(f"{first_hash}  {ARTIFACT.name}\n", encoding="utf-8")

    record = {
        "schema_version": 1,
        "status": "PASS",
        "source": "git archive HEAD",
        "source_commit": source_sha,
        "source_archive_sha256": source_archive_sha256,
        "target": TARGET,
        "artifact": str(ARTIFACT),
        "builds": [
            {"name": "first", "sha256": first_hash, "size": first_size},
            {"name": "second", "sha256": second_hash, "size": second_size},
        ],
        "identity": identity,
        "environment_manifest_schema_version": manifest["schema_version"],
        "artifact_manifest_schema_version": artifact_manifest["schema_version"],
        "artifact_set_status": artifact_manifest["status"],
        "archived_artifact": str(artifact_output) if artifact_output is not None else None,
        "archived_checksum": str(checksum_output) if checksum_output is not None else None,
        "evidence": "PC static library only",
        "release_scope": "blocked",
    }
    if args.record is not None:
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(
        "pc_artifact_reproducibility_ok source=git-archive-head "
        f"target={TARGET} artifact={ARTIFACT} sha256={first_hash} size={first_size} "
        f"archived={artifact_output is not None} "
        f"identity={json.dumps(identity, sort_keys=True, separators=(',', ':'))} "
        "evidence=pc-static-library-only release_scope=blocked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
