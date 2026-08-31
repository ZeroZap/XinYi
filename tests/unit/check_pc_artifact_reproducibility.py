#!/usr/bin/env python3
"""Rebuild one PC root artifact twice from the committed source archive."""

from __future__ import annotations

import hashlib
import io
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TARGET = "xy_device"
ARTIFACT = Path("components/device/libxy_device.a")


def run(command: list[str], cwd: Path) -> None:
    subprocess.run(command, cwd=cwd, check=True)


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
        "evidence=pc-static-library-only release_scope=blocked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
