#!/usr/bin/env python3
"""Guard the fail-closed XinYi release-readiness checklist."""

from __future__ import annotations

import hashlib
import json
import re
from collections import Counter
from pathlib import Path

from check_pc_artifact_reproducibility import DIRECT_SOURCES

ROOT = Path(__file__).resolve().parents[2]
CHECKLIST = ROOT / "docs" / "release" / "release-checklist.md"
EVIDENCE = ROOT / "docs" / "validation" / "component-evidence-matrix.md"
ARTIFACT_MANIFEST = ROOT / "docs" / "validation" / "pc-release-artifact-manifest.json"
SIGNING_POLICY = ROOT / "docs" / "validation" / "release-signing-policy.json"
SBOM_POLICY = ROOT / "docs" / "validation" / "pc-release-sbom-policy.json"
LICENSE_REVIEW = ROOT / "docs" / "validation" / "pc-release-license-review.json"
NOTICE_REVIEW = ROOT / "docs" / "validation" / "pc-release-notice-review.json"
RELEASE_WORKFLOW = ROOT / ".github" / "workflows" / "release.yml"
UNIT_WORKFLOW = ROOT / ".github" / "workflows" / "unit-tests.yml"
RELEASE_CHECKOUT_ACTION = "actions/checkout@11d5960a326750d5838078e36cf38b85af677262"
UNIT_UPLOAD_ACTION = "actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02"
WORKFLOW_ACTIONS = {
    "actions/checkout@11d5960a326750d5838078e36cf38b85af677262": 5,
    "actions/setup-python@a26af69be951a213d495a4c3e4e4022e16d87065": 3,
    "actions/configure-pages@1f0c5cde4bc74cd7e1254d0cb4de8d49e9068c7d": 1,
    "actions/upload-pages-artifact@56afc609e74202658d3ffba0e8f6dda462b719fa": 1,
    "actions/deploy-pages@d6db90164ac5ed86f2b6aed7e0febac5b3c0c03e": 1,
    "actions/github-script@f28e40c7f34bde8b3046d885e986cb6290c5673b": 2,
    "actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02": 4,
}


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate() -> list[str]:
    errors: list[str] = []
    require(CHECKLIST.is_file(), "release checklist is missing", errors)
    if not CHECKLIST.is_file():
        return errors

    action_uses: Counter[str] = Counter()
    for workflow_path in sorted((ROOT / ".github" / "workflows").glob("*.yml")):
        workflow_text = workflow_path.read_text(encoding="utf-8")
        action_uses.update(re.findall(r"^\s*uses:\s*([^\s#]+)", workflow_text, re.MULTILINE))
    require(action_uses == Counter(WORKFLOW_ACTIONS),
            f"workflow actions must match the reviewed immutable set: {dict(action_uses)}", errors)

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
    require("Generation is still `BLOCKED`" not in checklist,
            "release checklist must not claim bounded SBOM generation is blocked", errors)
    require("now generates and independently validates one bounded CycloneDX JSON 1.6 SBOM" in checklist,
            "release checklist must record the bounded generated SBOM without upgrading approval", errors)
    known_limitations = (ROOT / "docs" / "release" / "known-limitations.md").read_text(
        encoding="utf-8"
    )
    known_limitations_normalized = " ".join(known_limitations.split()).lower()
    require("no sbom, reproducible release artifact, signed checksum set" not in
            known_limitations_normalized,
            "Known Limitations must not claim bounded release evidence is absent", errors)
    for token in (
        "one bounded pc static-library artifact",
        "cyclonedx json 1.6 sbom",
        "ephemeral ci-gate ed25519 signature",
        "legal_review_pending",
        "not a complete release artifact set",
    ):
        require(token in known_limitations_normalized,
                f"Known Limitations is missing bounded release evidence boundary: {token}", errors)
    require("docs/release/release-checklist.md" in evidence,
            "component evidence matrix must index the release checklist", errors)
    require(RELEASE_WORKFLOW.is_file(), "release workflow is missing", errors)
    if RELEASE_WORKFLOW.is_file():
        workflow = RELEASE_WORKFLOW.read_text(encoding="utf-8")
        require("runs-on: ubuntu-24.04" in workflow,
                "release workflow must use the canonical pinned Ubuntu runner", errors)
        require("runs-on: ubuntu-latest" not in workflow,
                "release workflow must not use the drifting ubuntu-latest runner", errors)
        require("--require-release-authorization" in workflow,
                "release workflow must require explicit publication authorization", errors)
        require(RELEASE_CHECKOUT_ACTION in workflow,
                "release workflow must pin actions/checkout to the reviewed commit", errors)
        require("uses: actions/checkout@v" not in workflow,
                "release workflow must not use a movable checkout version tag", errors)
    require(UNIT_WORKFLOW.is_file(), "canonical unit workflow is missing", errors)
    if UNIT_WORKFLOW.is_file():
        unit_workflow = UNIT_WORKFLOW.read_text(encoding="utf-8")
        require(RELEASE_CHECKOUT_ACTION in unit_workflow,
                "canonical unit workflow must pin actions/checkout to the reviewed commit", errors)
        require(unit_workflow.count(UNIT_UPLOAD_ACTION) == 4,
                "canonical unit workflow must pin every upload-artifact use to the reviewed commit",
                errors)
        require("uses: actions/checkout@v" not in unit_workflow,
                "canonical unit workflow must not use a movable checkout version tag", errors)
        require("uses: actions/upload-artifact@v" not in unit_workflow,
                "canonical unit workflow must not use a movable upload-artifact version tag", errors)
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
                                "libxy_device.a.cdx.json",
                                "libxy_device.a.sig",
                                "pc-artifact-signing-public.pem",
                                "LICENSE",
                                "pc-release-license-review.json",
                                "pc-release-notice-review.json",
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
            require(policy.get("status") == "GENERATED_REVIEW_PENDING",
                    "PC release SBOM policy must record generated/review-pending status", errors)
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
            require(policy.get("generated_output") == "libxy_device.a.cdx.json",
                    "PC release SBOM output mismatch", errors)
            for phrase in ("independently validates", "not license approval", "R1 remains blocked"):
                require(phrase in boundary,
                        f"PC release SBOM policy evidence boundary missing phrase: {phrase}", errors)

    require(LICENSE_REVIEW.is_file(), "PC bounded artifact license review is missing", errors)
    if LICENSE_REVIEW.is_file():
        try:
            review = json.loads(LICENSE_REVIEW.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"PC bounded artifact license review is invalid JSON: {exc}")
        else:
            require(review.get("status") == "TECHNICAL_REVIEW_RECORDED_LEGAL_PENDING",
                    "PC bounded artifact license review status must remain legal-pending", errors)
            require(review.get("artifact") == "libxy_device.a",
                    "PC bounded artifact license review artifact mismatch", errors)
            require(review.get("approval") == "LEGAL_REVIEW_PENDING",
                    "PC bounded artifact license review must not claim legal approval", errors)
            sources = review.get("sources")
            require(sources == DIRECT_SOURCES,
                    "PC bounded artifact license review source inventory mismatch", errors)
            require(review.get("artifact_selection") == "reproducibility-gate-only",
                    "PC bounded artifact license review selection mismatch", errors)
            require(review.get("declared_license") == "Apache-2.0",
                    "PC bounded artifact declared license mismatch", errors)
            license_evidence = review.get("license_evidence")
            require(isinstance(license_evidence, dict),
                    "PC bounded artifact license evidence is missing", errors)
            if isinstance(license_evidence, dict):
                license_path = ROOT / str(license_evidence.get("path", ""))
                require(license_path == ROOT / "LICENSE" and license_path.is_file(),
                        "PC bounded artifact license evidence path mismatch", errors)
                if license_path.is_file():
                    actual_license_sha256 = hashlib.sha256(license_path.read_bytes()).hexdigest()
                    require(license_evidence.get("sha256") == actual_license_sha256,
                            "PC bounded artifact license evidence hash mismatch", errors)
            boundary = str(review.get("evidence_boundary", ""))
            for phrase in ("technical evidence review", "not legal advice", "R1 remains blocked"):
                require(phrase in boundary,
                        f"PC bounded artifact license boundary missing phrase: {phrase}", errors)

    require(NOTICE_REVIEW.is_file(), "PC bounded artifact NOTICE review is missing", errors)
    if NOTICE_REVIEW.is_file():
        try:
            notice = json.loads(NOTICE_REVIEW.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"PC bounded artifact NOTICE review is invalid JSON: {exc}")
        else:
            require(notice.get("status") == "BOUNDED_NOTICE_NOT_REQUIRED_LEGAL_PENDING",
                    "PC bounded artifact NOTICE review status mismatch", errors)
            require(notice.get("artifact") == "libxy_device.a",
                    "PC bounded artifact NOTICE review artifact mismatch", errors)
            require(notice.get("approval") == "LEGAL_REVIEW_PENDING",
                    "PC bounded artifact NOTICE review must not claim legal approval", errors)
            require(notice.get("notice_output") is None,
                    "PC bounded artifact NOTICE review must not invent a NOTICE file", errors)
            require(notice.get("artifact_selection") == "reproducibility-gate-only",
                    "PC bounded artifact NOTICE review selection mismatch", errors)
            scope = notice.get("scope")
            require(isinstance(scope, dict),
                    "PC bounded artifact NOTICE review scope is missing", errors)
            if isinstance(scope, dict):
                require(scope.get("direct_source_count") == len(DIRECT_SOURCES),
                        "PC bounded artifact NOTICE source count mismatch", errors)
                require(scope.get("source_inventory") ==
                        "docs/validation/pc-release-license-review.json",
                        "PC bounded artifact NOTICE source inventory mismatch", errors)
            notice_evidence = notice.get("evidence")
            require(isinstance(notice_evidence, dict),
                    "PC bounded artifact NOTICE evidence is missing", errors)
            if isinstance(notice_evidence, dict):
                require(notice_evidence.get("repository_license") == "LICENSE",
                        "PC bounded artifact NOTICE license path mismatch", errors)
                actual_license_sha256 = hashlib.sha256((ROOT / "LICENSE").read_bytes()).hexdigest()
                require(notice_evidence.get("repository_license_sha256") == actual_license_sha256,
                        "PC bounded artifact NOTICE license hash mismatch", errors)
            boundary = str(notice.get("evidence_boundary", ""))
            for phrase in ("bounded artifact", "not legal advice", "complete release scope", "R1 remains blocked"):
                require(phrase in boundary,
                        f"PC bounded artifact NOTICE boundary missing phrase: {phrase}", errors)

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
