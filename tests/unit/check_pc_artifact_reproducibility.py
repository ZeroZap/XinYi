#!/usr/bin/env python3
"""Rebuild one PC root artifact twice from the committed source archive."""

from __future__ import annotations

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


def extract(archive: bytes, destination: Path) -> None:
    destination.mkdir()
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:") as tar:
        tar.extractall(destination, filter="data")


def build_artifact(archive: bytes, root: Path, name: str) -> tuple[str, int]:
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
    return hashlib.sha256(payload).hexdigest(), len(payload)


def main() -> int:
    load_environment_manifest()
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
    with tempfile.TemporaryDirectory(prefix="xinyi-pc-artifact-") as temporary:
        temporary_root = Path(temporary)
        first_hash, first_size = build_artifact(archive, temporary_root, "first")
        second_hash, second_size = build_artifact(archive, temporary_root, "second")

    if (first_hash, first_size) != (second_hash, second_size):
        raise SystemExit(
            "PC artifact is not reproducible: "
            f"first={first_hash}/{first_size} second={second_hash}/{second_size}"
        )

    print(
        "pc_artifact_reproducibility_ok source=git-archive-head "
        f"target={TARGET} artifact={ARTIFACT} sha256={first_hash} size={first_size} "
        f"identity={json.dumps(identity, sort_keys=True, separators=(',', ':'))} "
        "evidence=pc-static-library-only release_scope=blocked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
