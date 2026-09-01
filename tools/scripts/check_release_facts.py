#!/usr/bin/env python3
"""Validate XinYi's canonical version and release metadata."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

VERSION_RE = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")


def fail(message: str) -> None:
    raise ValueError(message)


def read_text(path: Path) -> str:
    if not path.is_file():
        fail(f"required release fact is missing: {path}")
    return path.read_text(encoding="utf-8")


def validate_release_authorization(repo: Path) -> None:
    checklist = read_text(repo / "docs/release/release-checklist.md")
    evidence = read_text(repo / "docs/validation/component-evidence-matrix.md")

    if "**Status:** `READY`" not in checklist:
        fail("release publication is blocked: checklist status is not READY")
    if "**Release decision:** `GO`" not in checklist:
        fail("release publication is blocked: checklist decision is not GO")
    if re.search(r"^- \[ \] ", checklist, flags=re.MULTILINE):
        fail("release publication is blocked: checklist contains incomplete gates")
    if "**R1 status:** `QUALIFIED`" not in evidence:
        fail("release publication is blocked: R1 evidence is not recorded")


def validate(repo: Path, tag: str | None = None, require_release_authorization: bool = False) -> str:
    version = read_text(repo / "VERSION").strip()
    match = VERSION_RE.fullmatch(version)
    if match is None:
        fail(f"VERSION must be strict MAJOR.MINOR.PATCH, got: {version!r}")
    assert match is not None

    major, minor, patch = match.groups()
    cmake = read_text(repo / "CMakeLists.txt")
    if "file(READ \"${CMAKE_SOURCE_DIR}/VERSION\" XY_FRAMEWORK_VERSION)" not in cmake:
        fail("root CMake must read the canonical VERSION file")
    if "project(XY_Framework VERSION ${XY_FRAMEWORK_VERSION}" not in cmake:
        fail("root CMake project version must come from XY_FRAMEWORK_VERSION")

    kconfig = read_text(repo / "Kconfig")
    expected_kconfig = f'default "{version}"'
    if expected_kconfig not in kconfig:
        fail(f"Kconfig version mirror does not match VERSION ({version})")

    header = read_text(repo / "components/xy_version.h")
    expected_macros = (
        f"#define XY_VERSION_MAJOR       {major}",
        f"#define XY_VERSION_MINOR       {minor}",
        f"#define XY_VERSION_PATCH       {patch}",
        f'#define XY_VERSION_STRING      "{version}"',
    )
    for expected in expected_macros:
        if expected not in header:
            fail(f"public version header does not match VERSION: missing {expected}")

    changelog = read_text(repo / "docs/release/CHANGELOG.md")
    if "## [Unreleased]" not in changelog:
        fail("release changelog must keep an [Unreleased] section")
    if f"## [{version}]" not in changelog:
        fail(f"release changelog has no [{version}] entry")

    read_text(repo / "docs/release/RELEASE_PROCESS.md")
    read_text(repo / "docs/release/known-limitations.md")

    if tag is not None:
        if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag):
            fail(f"release tag must use vMAJOR.MINOR.PATCH, got: {tag!r}")
        if tag != f"v{version}":
            fail(f"tag {tag} does not match canonical version v{version}")

    if require_release_authorization:
        validate_release_authorization(repo)

    return version


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--tag", help="tag to validate, normally github.ref_name")
    parser.add_argument(
        "--require-release-authorization",
        action="store_true",
        help="fail unless the release checklist is complete and explicitly authorized",
    )
    args = parser.parse_args()

    try:
        version = validate(args.repo.resolve(), args.tag, args.require_release_authorization)
    except ValueError as exc:
        print(f"release facts: FAIL: {exc}", file=sys.stderr)
        return 1

    tag_status = f", tag={args.tag}" if args.tag else ""
    print(f"release facts: OK: version={version}{tag_status}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
