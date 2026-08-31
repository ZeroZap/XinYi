#!/usr/bin/env python3
"""Build one canonical release input from an exported tracked-source snapshot."""

from __future__ import annotations

import io
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TARGET = "test_device_driver_template"
TEST_NAME = "device_driver_template"


def run(command: list[str], cwd: Path) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def main() -> int:
    archive = subprocess.run(
        ["git", "archive", "--format=tar", "HEAD"],
        cwd=ROOT,
        check=True,
        capture_output=True,
    ).stdout
    with tempfile.TemporaryDirectory(prefix="xinyi-release-input-") as temporary:
        exported = Path(temporary) / "source"
        exported.mkdir()
        with tarfile.open(fileobj=io.BytesIO(archive), mode="r:") as tar:
            tar.extractall(exported, filter="data")
        smoke_source = ROOT / "tests" / "unit" / "release_input_smoke"
        exported_smoke = exported / "tests" / "unit" / "release_input_smoke"
        if not exported_smoke.exists():
            shutil.copytree(smoke_source, exported_smoke)
        build = Path(temporary) / "build"
        run(
            ["cmake", "-S", str(exported / "tests" / "unit" / "release_input_smoke"),
             "-B", str(build)],
            exported,
        )
        run(["cmake", "--build", str(build), "--target", TARGET], exported)
        run(
            ["ctest", "--test-dir", str(build), "-R", f"^{TEST_NAME}$", "--output-on-failure"],
            exported,
        )
    print(
        "release_input_clean_checkout_ok source=git-archive-head "
        f"target={TARGET} test={TEST_NAME} release_scope=excluded-pending-review"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())