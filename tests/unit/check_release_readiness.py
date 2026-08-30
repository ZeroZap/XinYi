#!/usr/bin/env python3
"""Guard the fail-closed XinYi release-readiness checklist."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CHECKLIST = ROOT / "docs" / "release" / "release-checklist.md"
EVIDENCE = ROOT / "docs" / "validation" / "component-evidence-matrix.md"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate() -> list[str]:
    errors: list[str] = []
    require(CHECKLIST.is_file(), "release checklist is missing", errors)
    if not CHECKLIST.is_file():
        return errors

    checklist = CHECKLIST.read_text(encoding="utf-8")
    evidence = EVIDENCE.read_text(encoding="utf-8")

    for token in (
        "**Status:** `BLOCKED`",
        "**Release decision:** `NO-GO`",
        "Canonical Host suite",
        "Supported target compile matrix",
        "Reference-board HIL",
        "Security review and Secure FOTA provider",
        "SBOM and third-party license review",
        "Reproducible release artifacts",
        "Checksums and signatures",
        "Known Limitations",
        "Clean-checkout rebuild",
        "Rollback and recovery evidence",
        "Host/PC/QEMU/compile-only evidence cannot satisfy Board, Security, Performance, or Release qualification",
    ):
        require(token in checklist, f"release checklist is missing fail-closed requirement: {token}", errors)

    require("- [x] Release Candidate approved" not in checklist,
            "release checklist must not claim Release Candidate approval", errors)
    require("- [x] R1 release qualified" not in checklist,
            "release checklist must not claim R1 qualification", errors)
    require("docs/release/release-checklist.md" in evidence,
            "component evidence matrix must index the release checklist", errors)

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("release_readiness failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("release_readiness_ok status=BLOCKED decision=NO-GO r1=pending")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
