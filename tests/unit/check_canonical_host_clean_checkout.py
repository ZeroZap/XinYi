#!/usr/bin/env python3
"""Build and run the canonical Host suite from the committed source archive.

`git archive HEAD` does not descend into submodules, so the archive of this repository is *not*
the tree a `git clone --recursive` gives: the ten pinned vendor SDKs (`MCU/**`,
`components/third_party/**`) are absent. That was invisible until a test started reading them:
`doc_api_symbols` checks `#include "stm32l4xx_hal.h"` against the headers actually present, so in
the partial archive a correct HAL porting note became a finding, and the clean-checkout gate went
red for a tree that is fine.

Copying them is not an option - `MCU/` alone is 3.2 GiB - so each submodule worktree is linked in
after being checked against the commit HEAD records. A submodule that is uninitialised, dirty or
on a different commit fails the run instead of silently producing a different tree than the one
this repository pins.
"""

from __future__ import annotations

import io
import os
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPOSITORY_STATE_TESTS = (
    "supply_chain_inventory|release_input_inventory|release_input_clean_checkout|"
    "canonical_host_clean_checkout|pc_artifact_reproducibility|"
    "pandora_artifact_reproducibility_.*"
)


def run(command: list[str], cwd: Path) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def gitlinks(root: Path) -> list[tuple[str, str]]:
    """(path, commit) for every submodule this HEAD records."""
    listing = subprocess.run(["git", "ls-tree", "-r", "HEAD"], cwd=root, check=True,
                             capture_output=True, text=True).stdout
    found: list[tuple[str, str]] = []
    for line in listing.splitlines():
        meta, _, path = line.partition("\t")
        mode, kind, commit = meta.split()
        if mode == "160000" and kind == "commit":
            found.append((path, commit))
    return found


def link_submodules(root: Path, exported: Path) -> int:
    """Place each pinned submodule's headers in the export, after proving the worktree is pinned.

    Two things are deliberate here. The proof matters more than the copy: without the commit and
    cleanliness checks this harness would happily run against a checkout whose vendor SDK is at
    some other commit, which is the failure mode the gate exists to rule out. And only headers are
    materialised, as hardlinks (falling back to symlinks), because that is the whole of what the
    Host suite reads from a vendor tree - `doc_api_symbols` resolves `#include` names against it -
    while `MCU/` is 3.2 GiB of sources nothing in the Host build compiles. Whole-directory symlinks
    were tried first and do not work: `Path.rglob()` does not descend into a symlinked directory,
    so the export looked exactly like the archive it came from.
    """
    modules = gitlinks(root)
    for path, commit in modules:
        worktree = root / path
        if not (worktree / ".git").exists():
            raise SystemExit(f"clean checkout: submodule {path} is not initialised "
                             f"(run `git submodule update --init --recursive`)")
        head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=worktree, check=True,
                              capture_output=True, text=True).stdout.strip()
        if head != commit:
            raise SystemExit(f"clean checkout: submodule {path} is at {head}, "
                             f"HEAD records {commit}")
        dirty = subprocess.run(["git", "status", "--porcelain"], cwd=worktree, check=True,
                               capture_output=True, text=True).stdout.strip()
        if dirty:
            raise SystemExit(f"clean checkout: submodule {path} has local changes, so the "
                             f"archive would not be the pinned source:\n{dirty}")
        mirror_tree(worktree, exported / path)
    return len(modules)


def mirror_tree(source: Path, destination: Path) -> int:
    """Recreate `source` under `destination` without copying file contents.

    Everything is mirrored, not just headers: the Host suite reads more than include paths from a
    vendor tree - `dependency_closure` matches each submodule's declared licence against the
    licence files and SPDX banners inside it - and a partial mirror turned that check red for a
    tree whose licences are fine. A hardlink costs an inode rather than 3.2 GiB of I/O, and a file
    whose link fails is copied instead of being silently dropped.
    """
    written = 0
    for directory, _, files in os.walk(source):
        relative = Path(directory).relative_to(source)
        (destination / relative).mkdir(parents=True, exist_ok=True)
        for name in files:
            original = Path(directory) / name
            target = destination / relative / name
            try:
                os.link(original, target)
            except OSError:
                try:
                    os.symlink(original, target)
                except OSError:
                    shutil.copy2(original, target)
            written += 1
    return written


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

        linked = link_submodules(ROOT, exported)

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
        f"canonical_host_clean_checkout_ok source=git-archive-head submodules={linked} "
        "repo_state_tests=excluded evidence=host-only release_scope=blocked"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
