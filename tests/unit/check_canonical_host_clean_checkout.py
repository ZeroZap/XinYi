#!/usr/bin/env python3
"""Build and run the canonical Host suite from the committed source archive."""

from __future__ import annotations

import io
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPOSITORY_STATE_TESTS = (
    "supply_chain_inventory|release_input_inventory|release_input_clean_checkout|"
    "canonical_host_clean_checkout"
)


def run(command: list[str], cwd: Path) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def main() -> int:
    archive = subprocess.run(
        ["git", "archive", "--format=tar", "HEAD"],
        cwd=ROOT,
        check=True,
        capture_output=True,
    ).stdout
    with tempfile.TemporaryDirectory(prefix="xinyi-canonical-host-") as temporary:
        exported = Path(temporary) / "source"
        exported.mkdir()
        with tarfile.open(fileobj=io.BytesIO(archive), mode="r:") as tar:
            tar.extractall(exported, filter="data")

        build = Path(temporary) / "build"
        run(["cmake", "-S", str(exported / "tests" / "unit"), "-B", str(build)], exported)
        run(["cmake", "--build", str(build), "-j2"], exported)
        run(
            [
                "ctest",
                "--test-dir",
                str(build),
                "--output-on-failure",
                "-E",
                f"^({REPOSITORY_STATE_TESTS})$",
            ],
            exported,
        )

    print(
        "canonical_host_clean_checkout_ok source=git-archive-head "
        "repo_state_tests=excluded evidence=host-only release_scope=blocked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())