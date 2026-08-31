#!/usr/bin/env python3
"""Guard the fail-closed XinYi release-readiness checklist."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CHECKLIST = ROOT / "docs" / "release" / "release-checklist.md"
EVIDENCE = ROOT / "docs" / "validation" / "component-evidence-matrix.md"
ARTIFACT_MANIFEST = ROOT / "docs" / "validation" / "pc-release-artifact-manifest.json"


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
    require(ARTIFACT_MANIFEST.is_file(), "PC release artifact manifest is missing", errors)
    if ARTIFACT_MANIFEST.is_file():
        try:
            manifest = json.loads(ARTIFACT_MANIFEST.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"PC release artifact manifest is invalid JSON: {exc}")
        else:
            required = {
                "schema_version",
                "status",
                "platform",
                "build_type",
                "source",
                "artifacts",
                "excluded_scope",
                "evidence_boundary",
            }
            missing = sorted(required - manifest.keys())
            require(not missing, f"PC release artifact manifest missing fields: {missing}", errors)
            require(manifest.get("status") == "SELECTED_PC_ARTIFACT_SET_RECORDED",
                    "PC release artifact manifest status is not fail-closed", errors)
            require(manifest.get("platform") == "PC" and manifest.get("build_type") == "Release",
                    "PC release artifact manifest platform/build type mismatch", errors)
            require(manifest.get("source") == "git archive HEAD",
                    "PC release artifact manifest source must be git archive HEAD", errors)
            artifacts = manifest.get("artifacts")
            require(isinstance(artifacts, list) and len(artifacts) == 1,
                    "PC release artifact manifest must select exactly one bounded artifact", errors)
            if isinstance(artifacts, list) and len(artifacts) == 1:
                artifact = artifacts[0]
                require(artifact.get("target") == "xy_device",
                        "PC release artifact manifest target must be xy_device", errors)
                require(artifact.get("path") == "components/device/libxy_device.a",
                        "PC release artifact manifest path mismatch", errors)
                require(artifact.get("selection") == "reproducibility-gate-only",
                        "PC release artifact selection must remain gate-only", errors)
                archive = artifact.get("ci_archive")
                require(isinstance(archive, dict),
                        "PC release artifact manifest must define bounded CI archival", errors)
                if isinstance(archive, dict):
                    require(archive.get("artifact_name") == "pc-release-gate-artifact",
                            "PC release artifact CI archive name mismatch", errors)
                    require(archive.get("files") == ["libxy_device.a", "libxy_device.a.sha256"],
                            "PC release artifact CI archive files mismatch", errors)
                    require(archive.get("retention_days") == 14,
                            "PC release artifact CI archive retention mismatch", errors)
            boundary = str(manifest.get("evidence_boundary", ""))
            for phrase in ("not a complete release artifact set", "hardware", "security", "R1"):
                require(phrase in boundary,
                        f"PC release artifact evidence boundary missing phrase: {phrase}", errors)

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
