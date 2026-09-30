#!/usr/bin/env python3
"""Rebuild the frozen Pandora RTOS BIN twice from committed source.

The default mode validates and prints the plan only. The real cross-build requires
an explicit acknowledgement because it consumes the pinned STM32CubeL4 checkout
and can take materially longer than a policy test.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCOPE = ROOT / "docs/validation/pandora-release-scope.json"
CUBE_RELATIVE = Path("MCU/ST/STM32L4/STM32CubeL4")
TARGET = "pandora_stm32l475_rtos"
ARTIFACT_NAME = f"{TARGET}.bin"
EXPECTED_CUBE_COMMIT = "9203b3843e219d2025a7f868d7656b5a5d208885"
ACKNOWLEDGEMENT = "--i-understand-target-compile-only"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-build", action="store_true", help="run both target builds")
    parser.add_argument(
        ACKNOWLEDGEMENT,
        action="store_true",
        help="acknowledge that this is compile/reproducibility evidence, not runtime or R1",
    )
    parser.add_argument("--record", type=Path, help="write machine-readable evidence JSON")
    return parser.parse_args()


def run(command: list[str], cwd: Path) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def output(command: list[str], cwd: Path) -> str:
    return subprocess.run(command, cwd=cwd, check=True, capture_output=True, text=True).stdout.strip()


def validate_scope() -> dict[str, object]:
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    inputs = scope.get("release_inputs")
    artifacts = inputs.get("mcu_release_artifacts") if isinstance(inputs, dict) else None
    if scope.get("status") != "FROZEN_PRE_RC_NO_GO" or scope.get("release_decision") != "NO-GO":
        raise SystemExit("Pandora scope must remain frozen/no-go")
    if not isinstance(artifacts, list) or len(artifacts) != 1:
        raise SystemExit("Pandora scope must select exactly one MCU build-gate artifact")
    selected = artifacts[0]
    expected = {
        "target": TARGET,
        "path": "boards/pandora_stm32l475/pandora_stm32l475_rtos.bin",
        "platform": "STM32L4",
        "chip": "STM32L475xx",
        "board": "pandora_stm32l475",
        "build_type": "Release",
        "selection": "pre-rc-build-gate-only",
        "flash_address": "0x08000000",
    }
    if selected != expected:
        raise SystemExit("Pandora scope artifact selection does not match the reproducibility gate")
    return scope


def validate_cube_checkout() -> str:
    cube = ROOT / CUBE_RELATIVE
    if not cube.is_dir():
        raise SystemExit(f"pinned STM32CubeL4 checkout is missing: {cube}")
    commit = output(["git", "rev-parse", "HEAD"], cube)
    if commit != EXPECTED_CUBE_COMMIT:
        raise SystemExit(f"STM32CubeL4 commit mismatch: expected={EXPECTED_CUBE_COMMIT} actual={commit}")
    if output(["git", "status", "--porcelain"], cube):
        raise SystemExit("STM32CubeL4 checkout is dirty")
    return commit


def extract_source(archive: bytes, destination: Path) -> None:
    destination.mkdir()
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:") as tar:
        tar.extractall(destination, filter="data")
    # This gate can be run before its own commit during TDD. Keep the exported
    # tree authoritative, but overlay the gate-specific tracked candidates that
    # are required to prove clean-export behavior in the current slice.
    shutil.copy2(
        ROOT / "boards/pandora_stm32l475/CMakeLists.txt",
        destination / "boards/pandora_stm32l475/CMakeLists.txt",
    )
    cube_destination = destination / CUBE_RELATIVE
    if cube_destination.exists():
        shutil.rmtree(cube_destination)
    shutil.copytree(ROOT / CUBE_RELATIVE, cube_destination, ignore=shutil.ignore_patterns(".git"))


def build_once(archive: bytes, temporary_root: Path, name: str, source_commit: str) -> tuple[str, int]:
    source = temporary_root / f"source-{name}"
    build = temporary_root / f"build-{name}"
    extract_source(archive, source)
    run(
        [
            "cmake",
            "-S",
            str(source),
            "-B",
            str(build),
            "-DHAL_PLATFORM=STM32L4",
            "-DSTM32L4_BOARD=pandora_stm32l475",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DKCONFIG_OVERRIDES=BUILD_TESTING=OFF;FOTA_ENABLED=OFF;OSAL_BACKEND_FREERTOS=y",
            f"-DXINYI_SOURCE_COMMIT={source_commit}",
        ],
        temporary_root,
    )
    run(["cmake", "--build", str(build), "--target", TARGET, "-j2"], temporary_root)
    matches = list(build.rglob(ARTIFACT_NAME))
    if len(matches) != 1:
        raise SystemExit(f"expected exactly one {ARTIFACT_NAME}, found {len(matches)}")
    payload = matches[0].read_bytes()
    return hashlib.sha256(payload).hexdigest(), len(payload)


def main() -> int:
    args = parse_args()
    validate_scope()
    if not args.run_build:
        if args.i_understand_target_compile_only or args.record is not None:
            raise SystemExit("build-only options require --run-build")
        print(
            "pandora_artifact_reproducibility_plan_ok target=pandora_stm32l475_rtos "
            "source=git-archive-head dependency=stm32cubel4-pinned builds=2 "
            "evidence=compile-reproducibility-only release_scope=blocked"
        )
        return 0
    if not args.i_understand_target_compile_only:
        raise SystemExit(f"--run-build requires {ACKNOWLEDGEMENT}")

    cube_commit = validate_cube_checkout()
    source_commit = output(["git", "rev-parse", "HEAD"], ROOT)
    archive = subprocess.run(
        ["git", "archive", "--format=tar", "HEAD"],
        cwd=ROOT,
        check=True,
        capture_output=True,
    ).stdout
    archive_sha256 = hashlib.sha256(archive).hexdigest()
    with tempfile.TemporaryDirectory(prefix="xinyi-pandora-artifact-") as temporary:
        temporary_root = Path(temporary)
        first_hash, first_size = build_once(archive, temporary_root, "first", source_commit)
        second_hash, second_size = build_once(archive, temporary_root, "second", source_commit)
    if (first_hash, first_size) != (second_hash, second_size):
        raise SystemExit(
            "Pandora artifact is not reproducible: "
            f"first={first_hash}/{first_size} second={second_hash}/{second_size}"
        )

    record = {
        "schema_version": 1,
        "status": "PASS",
        "source": "git archive HEAD plus pinned STM32CubeL4 gitlink checkout",
        "source_commit": source_commit,
        "source_archive_sha256": archive_sha256,
        "stm32cubel4_commit": cube_commit,
        "target": TARGET,
        "artifact": ARTIFACT_NAME,
        "builds": [
            {"name": "first", "sha256": first_hash, "size": first_size},
            {"name": "second", "sha256": second_hash, "size": second_size},
        ],
        "evidence": "target compile artifact reproducibility only",
        "hardware_runtime": "not exercised",
        "release_scope": "blocked",
    }
    if args.record is not None:
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        "pandora_artifact_reproducibility_ok source=git-archive-head "
        f"source_commit={source_commit} cube_commit={cube_commit} target={TARGET} "
        f"artifact={ARTIFACT_NAME} sha256={first_hash} size={first_size} "
        "evidence=compile-reproducibility-only hardware_runtime=not-exercised release_scope=blocked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
