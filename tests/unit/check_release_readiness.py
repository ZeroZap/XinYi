#!/usr/bin/env python3
"""Guard the fail-closed XinYi release-readiness checklist."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CHECKLIST = ROOT / "docs" / "release" / "release-checklist.md"
EVIDENCE = ROOT / "docs" / "validation" / "component-evidence-matrix.md"
ARTIFACT_MANIFEST = ROOT / "docs" / "validation" / "pc-release-artifact-manifest.json"
SIGNING_POLICY = ROOT / "docs" / "validation" / "release-signing-policy.json"
SBOM_POLICY = ROOT / "docs" / "validation" / "pc-release-sbom-policy.json"


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
                    require(archive.get("files") == [
                                "libxy_device.a",
                                "libxy_device.a.sha256",
                                "libxy_device.a.sig",
                                "pc-artifact-signing-public.pem",
                            ],
                            "PC release artifact CI archive files mismatch", errors)
                    require(archive.get("checksum_verification") == "independent-ci-step",
                            "PC release artifact checksum verification mismatch", errors)
                    require(archive.get("signature") == {
                                "algorithm": "Ed25519",
                                "key_scope": "ephemeral-ci-gate-only",
                                "verification": "independent-ci-step",
                            },
                            "PC release artifact signature boundary mismatch", errors)
                    require(archive.get("retention_days") == 14,
                            "PC release artifact CI archive retention mismatch", errors)
            boundary = str(manifest.get("evidence_boundary", ""))
            for phrase in ("not a complete release artifact set", "hardware", "security", "R1"):
                require(phrase in boundary,
                        f"PC release artifact evidence boundary missing phrase: {phrase}", errors)

    require(SIGNING_POLICY.is_file(), "release signing policy is missing", errors)
    if SIGNING_POLICY.is_file():
        try:
            policy = json.loads(SIGNING_POLICY.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"release signing policy is invalid JSON: {exc}")
        else:
            required = {
                "schema_version",
                "status",
                "algorithm",
                "release_identity",
                "key_custody",
                "publication",
                "approval_requirements",
                "evidence_boundary",
            }
            missing = sorted(required - policy.keys())
            require(not missing, f"release signing policy missing fields: {missing}", errors)
            require(policy.get("status") == "DESIGN_RECORDED_NO_RELEASE_KEY",
                    "release signing policy must remain fail-closed until a release key exists", errors)
            require(policy.get("algorithm") == "Ed25519",
                    "release signing policy algorithm mismatch", errors)
            require(policy.get("release_identity") == "UNASSIGNED",
                    "release signing identity must remain unassigned", errors)
            require(policy.get("key_custody") == "NOT_ESTABLISHED",
                    "release signing key custody must remain unestablished", errors)
            require(policy.get("publication") == "BLOCKED",
                    "signed release publication must remain blocked", errors)
            approvals = policy.get("approval_requirements")
            require(isinstance(approvals, list) and approvals == [
                        "named release owner",
                        "documented key custodian",
                        "recovery and revocation procedure",
                        "independent signature verification record",
                    ],
                    "release signing approval requirements mismatch", errors)
            boundary = str(policy.get("evidence_boundary", ""))
            for phrase in ("ephemeral CI key", "not a release identity", "R1 remains blocked"):
                require(phrase in boundary,
                        f"release signing policy evidence boundary missing phrase: {phrase}", errors)

    require(SBOM_POLICY.is_file(), "PC release SBOM policy is missing", errors)
    if SBOM_POLICY.is_file():
        try:
            policy = json.loads(SBOM_POLICY.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"PC release SBOM policy is invalid JSON: {exc}")
        else:
            required = {
                "schema_version",
                "status",
                "format",
                "artifact_scope",
                "required_inputs",
                "approval",
                "evidence_boundary",
            }
            missing = sorted(required - policy.keys())
            require(not missing, f"PC release SBOM policy missing fields: {missing}", errors)
            require(policy.get("status") == "DESIGN_RECORDED_GENERATION_BLOCKED",
                    "PC release SBOM policy must remain fail-closed before generation exists", errors)
            require(policy.get("format") == "CycloneDX JSON 1.6",
                    "PC release SBOM format mismatch", errors)
            require(policy.get("artifact_scope") == {
                        "platform": "PC",
                        "target": "xy_device",
                        "artifact": "libxy_device.a",
                        "selection": "reproducibility-gate-only",
                    },
                    "PC release SBOM artifact scope mismatch", errors)
            require(policy.get("required_inputs") == [
                        "committed source SHA and source-archive SHA-256",
                        "exact artifact SHA-256",
                        "artifact build tool identity",
                        "tracked direct source inputs for xy_device",
                        "resolved vendored and submodule dependency identity",
                        "license identifiers and evidence references",
                    ],
                    "PC release SBOM required inputs mismatch", errors)
            require(policy.get("approval") == "REVIEW_PENDING",
                    "PC release SBOM approval must remain pending", errors)
            boundary = str(policy.get("evidence_boundary", ""))
            for phrase in ("does not generate an SBOM", "license approval", "R1 remains blocked"):
                require(phrase in boundary,
                        f"PC release SBOM policy evidence boundary missing phrase: {phrase}", errors)

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
