#!/usr/bin/env python3
"""Guard the release-facing examples/projects inventory without claiming support."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs" / "validation" / "release-input-inventory.json"
ALLOWED_STATUS = {
    "host-guarded",
    "compile-only",
    "candidate-unverified",
    "historical-unverified",
}


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def tracked_children(parent: str) -> set[str]:
    result = subprocess.run(
        ["git", "ls-tree", "--name-only", "HEAD", f"{parent}/"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return {line.strip() for line in result.stdout.splitlines() if line.strip()}


def validate() -> list[str]:
    errors: list[str] = []
    require(MANIFEST.is_file(), "release input inventory is missing", errors)
    if not MANIFEST.is_file():
        return errors

    try:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"release input inventory is unreadable: {exc}"]

    require(isinstance(manifest, dict), "inventory root must be an object", errors)
    if not isinstance(manifest, dict):
        return errors

    require(manifest.get("schema_version") == 1, "unsupported inventory schema", errors)
    require(manifest.get("status") == "REVIEW_PENDING", "inventory must remain REVIEW_PENDING", errors)
    no_claim = manifest.get("no_claim")
    require(
        isinstance(no_claim, str)
        and "does not make an entry release-supported" in no_claim
        and "hardware" in no_claim,
        "inventory must preserve release/hardware no-claim boundaries",
        errors,
    )

    entries = manifest.get("entries")
    require(isinstance(entries, list) and bool(entries), "inventory entries must be non-empty", errors)
    inventoried: set[str] = set()
    if isinstance(entries, list):
        for index, entry in enumerate(entries):
            prefix = f"entries[{index}]"
            require(isinstance(entry, dict), f"{prefix} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            for key in ("path", "kind", "status", "evidence", "release_scope"):
                require(isinstance(entry.get(key), str) and bool(entry[key]), f"{prefix}.{key} is required", errors)
            path = entry.get("path")
            if isinstance(path, str):
                require(path not in inventoried, f"duplicate inventory path: {path}", errors)
                inventoried.add(path)
                require((ROOT / path).exists(), f"inventory path does not exist: {path}", errors)
                require(path.startswith(("examples/", "projects/")), f"out-of-scope path: {path}", errors)
            require(entry.get("kind") in {"example", "project"}, f"{prefix}.kind is invalid", errors)
            require(entry.get("status") in ALLOWED_STATUS, f"{prefix}.status is invalid", errors)
            require(entry.get("release_scope") == "excluded-pending-review",
                    f"{prefix} must remain excluded-pending-review", errors)
            if entry.get("status") == "host-guarded":
                require("CTest" in entry.get("evidence", ""),
                        f"{prefix} host-guarded status requires CTest evidence", errors)

    expected = tracked_children("examples") | tracked_children("projects")
    require(inventoried == expected, "inventory paths do not match tracked top-level examples/projects", errors)
    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("release_input_inventory failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("release_input_inventory_ok status=REVIEW_PENDING release_scope=excluded-pending-review")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
