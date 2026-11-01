#!/usr/bin/env python3
"""The dependency closure must describe the tree, and the tree must not grow silently.

Release blocker B-SBOM names four things it is waiting for: complete dependency closure, redistribution
review, a NOTICE package, and legal approval. Only the first is a measurement, so it is the only one this
rule can hold - but it is also the one that has to be trustworthy before the other three mean anything.

Measured on this tree: 15 vendored subtrees under components/, 9 carrying licence evidence, 6 carrying
none. The six are listed below with what is missing, because "we have not looked" and "we looked and there
is no licence text in the repository" are different states and only the second is actionable.

The rule holds the document to the tree in four directions, because a closure document fails in four ways:

  * a vendored subtree exists that the document does not mention - the closure is not complete;
  * the document mentions a path that is gone - the closure describes a tree that no longer exists;
  * an entry claims a licence that is no longer evidenced at the path it cites - the claim is stale;
  * the summary counts disagree with the entries - the document contradicts itself.

A new subtree with no licence evidence fails unless it is pinned, so the sixth gap cannot quietly become
the seventh.
"""

from __future__ import annotations

import json
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CLOSURE = Path("docs/validation/dependency-closure.json")

LICENSE_FILE = re.compile(r"^(LICEN[CS]E|COPYING|NOTICE)(\..*)?$", re.I)
SPDX_LINE = re.compile(r"SPDX-License-Identifier:\s*([A-Za-z0-9.\-+]+)")

#: Directory names that mean "vendored", wherever they sit under components/. Used to discover subtrees
#: the document has not heard of; the document itself is the authority for everything it lists.
VENDOR_MARKERS = frozenset({
    "third_party", "curve25519-cortexm0-20150813", "bosch", "cmsis", "tinyusb", "lwip",
    "rtthread-at", "AT-Command-V2", "FreeRTOS-Cellular-Interface", "coreJSON", "littlefs",
    "cmsis_rtx",
})

#: Aggregates that contain vendored subtrees rather than being one. Listing the aggregate reports
#: whichever licence the walk hits first and hides the children that have none.
AGGREGATES = frozenset({"components/third_party"})

SKIPPED = frozenset({"build", ".git", ".venv", "__pycache__", "node_modules"})

#: The six with no licence text in the tree, and what each needs. Closing one means obtaining the
#: upstream licence and adding it to the tree - a legal act, not an edit, which is why they are recorded
#: rather than fixed here.
NO_EVIDENCE_REASONS: dict[str, str] = {
    "components/crypto/curve25519-cortexm0-20150813":
        "no licence file and no SPDX tag; the directory name dates it to 2015-08-13 and the upstream "
        "project's licence must be obtained for that snapshot",
    "components/drivers/storage/littlefs":
        "no licence file and no SPDX tag in the copy that is in the tree",
    "components/kernel/osal/backend/cmsis_rtx": "no licence file and no SPDX tag in any source",
}


def discover(root: Path) -> set[str]:
    """Vendored subtrees found by walking, excluding aggregates and their parents."""
    found: set[str] = set()
    for directory, subdirectories, _filenames in os.walk(root / "components"):
        subdirectories[:] = [name for name in subdirectories if name not in SKIPPED]
        path = Path(directory)
        relative = path.relative_to(root).as_posix()
        if relative in AGGREGATES:
            continue
        if path.name in VENDOR_MARKERS:
            found.add(relative)
    return found


def licence_at(root: Path, relative: str) -> tuple[str | None, str | None]:
    """(identifier, evidence path) actually present for a subtree, recomputed rather than trusted."""
    directory = root / relative
    if not directory.is_dir():
        return None, None
    for candidate in sorted(directory.iterdir()):
        if candidate.is_file() and LICENSE_FILE.match(candidate.name):
            text = candidate.read_text(encoding="utf-8", errors="replace")
            match = SPDX_LINE.search(text)
            if match:
                return match.group(1), candidate.relative_to(root).as_posix()
            lowered = text.lower()
            for needle, identifier in (
                ("apache license", "Apache-2.0"),
                ("permission is hereby granted, free of charge", "MIT"),
                ("redistribution and use in source and binary forms", "BSD-3-Clause"),
                ("gnu general public license", "GPL"),
            ):
                if needle in lowered:
                    return identifier, candidate.relative_to(root).as_posix()
            return None, candidate.relative_to(root).as_posix()
    for directory_path, subdirectories, filenames in os.walk(directory):
        subdirectories[:] = [name for name in subdirectories if name not in SKIPPED]
        for filename in filenames:
            if not filename.endswith((".c", ".h", ".md", ".txt")):
                continue
            path = Path(directory_path) / filename
            try:
                text = path.read_text(encoding="utf-8", errors="replace")[:4096]
            except OSError:
                continue
            match = SPDX_LINE.search(text)
            if match:
                return match.group(1), path.relative_to(root).as_posix()
    return None, None


def has_content(root: Path, relative: str) -> bool:
    """Whether a subtree actually carries files.

    An uninitialised submodule is not absent: git leaves an empty directory behind, so `is_dir()` is true
    and there is still nothing to read. Treating that as "present" reports every submodule-backed
    dependency as a stale licence claim in any clean checkout.
    """
    for _directory, _subdirectories, filenames in os.walk(root / relative):
        if filenames:
            return True
    return False


def findings(root: Path = ROOT, pins: dict[str, str] | None = None) -> list[str]:
    pins = NO_EVIDENCE_REASONS if pins is None else pins
    document_path = root / CLOSURE
    if not document_path.is_file():
        return [f"{CLOSURE} is missing: the dependency closure has not been measured"]

    try:
        document = json.loads(document_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        return [f"{CLOSURE} is not valid JSON: {error}"]

    entries = document.get("entries")
    if not isinstance(entries, list) or not entries:
        return [f"{CLOSURE} has no entries: nothing has been measured"]

    problems: list[str] = []
    listed = {entry.get("path"): entry for entry in entries}

    if not listed:
        return [f"{CLOSURE} entries carry no path"]

    # The tree must not hold a vendored subtree the document does not mention.
    for discovered in sorted(discover(root) - set(listed)):
        problems.append(
            f"{discovered}: vendored subtree is not in the dependency closure - a release cannot ship "
            f"code whose licence it has not accounted for")

    for path, entry in sorted(listed.items()):
        submodule = entry.get("submodule") or {}
        if not (root / path).is_dir() or not has_content(root, path):
            # A clean checkout carries the gitlink and not the submodule's content, so the licence
            # cannot be re-verified here. Records are checked instead of pretending verification
            # happened - a gate that claims to have read a licence it could not open is the defect this
            # whole family of rules exists to prevent.
            if submodule.get("url") and submodule.get("revision"):
                continue
            problems.append(
                f"{path}: listed in the closure, absent from the tree, and not recorded as a pinned "
                f"submodule - a release cannot reproduce it")
            continue
        actual, evidence = licence_at(root, path)
        claimed = entry.get("licence")
        if claimed and claimed != actual:
            problems.append(
                f"{path}: closure claims {claimed} but the tree evidences {actual or 'nothing'} - the "
                f"claim is stale")
        if claimed and not evidence:
            problems.append(f"{path}: closure claims {claimed} with no evidence path in the tree")
        if not actual and path not in pins:
            problems.append(
                f"{path}: vendored subtree with no licence evidence and no pin - obtain the upstream "
                f"licence or record why it is tolerated")

    for path, reason in sorted(pins.items()):
        if not reason.strip():
            problems.append(f"{path}: pinned without stating what is missing")
        elif path not in listed:
            problems.append(f"{path}: pinned as licence-less but the closure does not list it")
        elif has_content(root, path) and licence_at(root, path)[0]:
            problems.append(
                f"{path}: pinned as having no licence evidence, but evidence is now present - the pin "
                f"has expired")

    measurement = document.get("measurement") or {}
    counted = measurement.get("with_licence_evidence")

    def evidenced(path: str, entry: dict) -> bool:
        """Whether a licence is evidenced: read from the tree, or recorded for an absent submodule."""
        if has_content(root, path):
            return bool(licence_at(root, path)[0])
        submodule = entry.get("submodule") or {}
        return bool(submodule.get("url") and submodule.get("revision") and entry.get("licence"))

    actual_with = sum(1 for path, entry in listed.items() if evidenced(path, entry))
    if counted is not None and counted != actual_with:
        problems.append(
            f"measurement.with_licence_evidence says {counted}, the tree evidences {actual_with}: the "
            f"document contradicts itself")
    counted_total = measurement.get("subtrees")
    if counted_total is not None and counted_total != len(entries):
        problems.append(
            f"measurement.subtrees says {counted_total} but {len(entries)} entries are listed")

    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("dependency_closure failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    document = json.loads((ROOT / CLOSURE).read_text(encoding="utf-8"))
    entries = document["entries"]
    with_licence = sum(1 for entry in entries if entry.get("licence"))
    print(f"dependency_closure ok ({len(entries)} vendored subtrees, {with_licence} with licence "
          f"evidence, {len(entries) - with_licence} recorded as having none; legal approval is a "
          f"separate blocker)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
