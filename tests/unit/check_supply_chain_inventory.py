#!/usr/bin/env python3
"""Guard XinYi's source dependency inventory without claiming release SBOM approval."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs" / "validation" / "source-dependency-inventory.json"
LEGACY_LICENSE_LIST = ROOT / "components" / "third_party" / "LICENSE清单.md"

REQUIRED_TOP_LEVEL = {
    "schema_version",
    "status",
    "scope",
    "no_claim",
    "vendored_dependencies",
    "submodules",
}
ALLOWED_REVIEW_STATUS = {"recorded", "review-pending", "license-text-missing"}


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def gitlinks() -> dict[str, str]:
    result = subprocess.run(
        ["git", "ls-files", "--stage"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    links: dict[str, str] = {}
    for line in result.stdout.splitlines():
        metadata, path = line.split("\t", 1)
        mode, sha, _stage = metadata.split()
        if mode == "160000":
            links[path] = sha
    return links


def validate() -> list[str]:
    errors: list[str] = []
    require(MANIFEST.is_file(), "source dependency inventory is missing", errors)
    if not MANIFEST.is_file():
        return errors

    try:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"source dependency inventory is unreadable: {exc}"]

    require(isinstance(manifest, dict), "inventory root must be an object", errors)
    if not isinstance(manifest, dict):
        return errors

    require(REQUIRED_TOP_LEVEL <= manifest.keys(), "inventory is missing required fields", errors)
    require(manifest.get("schema_version") == 1, "unsupported inventory schema", errors)
    require(manifest.get("status") == "REVIEW_PENDING", "inventory must remain REVIEW_PENDING", errors)
    require(
        manifest.get("scope") == "tracked source inputs; not a release artifact SBOM",
        "inventory scope must not imply release-artifact coverage",
        errors,
    )
    no_claim = manifest.get("no_claim")
    require(
        isinstance(no_claim, str) and "does not satisfy the release SBOM or license-review gate" in no_claim,
        "inventory must preserve the no-release-claim boundary",
        errors,
    )

    vendored = manifest.get("vendored_dependencies")
    require(isinstance(vendored, list) and bool(vendored), "vendored dependency list is empty", errors)
    if isinstance(vendored, list):
        paths: set[str] = set()
        for index, entry in enumerate(vendored):
            prefix = f"vendored_dependencies[{index}]"
            require(isinstance(entry, dict), f"{prefix} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            for key in ("name", "path", "version", "license", "license_evidence", "review_status"):
                require(isinstance(entry.get(key), str) and bool(entry[key]), f"{prefix}.{key} is required", errors)
            path = entry.get("path")
            if isinstance(path, str):
                require(path not in paths, f"duplicate vendored path: {path}", errors)
                paths.add(path)
                require((ROOT / path).exists(), f"vendored path does not exist: {path}", errors)
            evidence = entry.get("license_evidence")
            if isinstance(evidence, str):
                require((ROOT / evidence).is_file(), f"license evidence does not exist: {evidence}", errors)
            require(entry.get("review_status") in ALLOWED_REVIEW_STATUS,
                    f"{prefix}.review_status is invalid", errors)

    indexed_links = gitlinks()
    entries = manifest.get("submodules")
    require(isinstance(entries, list), "submodules must be a list", errors)
    manifest_links: dict[str, str] = {}
    if isinstance(entries, list):
        for index, entry in enumerate(entries):
            prefix = f"submodules[{index}]"
            require(isinstance(entry, dict), f"{prefix} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            for key in ("path", "url", "gitlink", "license_status"):
                require(isinstance(entry.get(key), str) and bool(entry[key]), f"{prefix}.{key} is required", errors)
            path = entry.get("path")
            sha = entry.get("gitlink")
            if isinstance(path, str) and isinstance(sha, str):
                require(path not in manifest_links, f"duplicate submodule path: {path}", errors)
                manifest_links[path] = sha
            require(entry.get("license_status") in ALLOWED_REVIEW_STATUS,
                    f"{prefix}.license_status is invalid", errors)
    require(manifest_links == indexed_links,
            "manifest submodule paths/gitlinks do not match the Git index", errors)

    legacy = LEGACY_LICENSE_LIST.read_text(encoding="utf-8")
    require("SUPERSEDED" in legacy and "source-dependency-inventory.json" in legacy,
            "legacy license list must be marked SUPERSEDED and point to the inventory", errors)
    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("supply_chain_inventory failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("supply_chain_inventory_ok status=REVIEW_PENDING sbom=blocked license_review=pending")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())