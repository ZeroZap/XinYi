#!/usr/bin/env python3
"""Some OSAL entries have more than one way to refuse, and the difference matters to the caller.

"a pointer that was never mine" and "a block that is already free" are not the same event, and neither is
"this mutex is held" versus "this handle is stale". The FreeRTOS backend distinguishes them and the openCH
firmware self-checks assert the distinctions on hardware; the bare-metal backend got away with flattening
them because nothing on the host looked. A flattened refusal is how a real defect - deleting a mutex
somebody holds - can sit behind a green suite.

The rule is narrow on purpose: the function must be able to say each thing the contract requires of it.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import braced_body, read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKENDS = ROOT / "components" / "kernel" / "osal" / "backend"

#: entry -> the refusals it must be able to express, each with the condition it stands for.
REQUIREMENTS: dict[str, list[tuple[str, str]]] = {
    "xy_os_mempool_free": [
        ("XY_OS_ERROR_PARAMETER", "a pointer this pool never handed out"),
        ("XY_OS_ERROR_RESOURCE", "a block that is already on the free list"),
    ],
    "xy_os_mutex_delete": [
        ("XY_OS_ERROR_RESOURCE", "a mutex that is still held"),
    ],
}

DEFINITION = re.compile(r"xy_os_status_t\s+(xy_os_\w+)\s*\([^;{]*\)\s*$", re.M)

#: (backend, entry) -> why it cannot express these, and what would have to change.
PINNED = {
    ("cmsis_rtx", "xy_os_mempool_free"):
        "compiled by no target in this build; it maps every osMemoryPoolFree failure to XY_OS_ERROR, so "
        "CMSIS's own osErrorParameter and osErrorResource are flattened away before they reach a caller",
    ("cmsis_rtx", "xy_os_mutex_delete"):
        "compiled by no target in this build; it calls osMutexDelete without looking at the owner, so "
        "deleting a held mutex is accepted",
    ("rtx5", "xy_os_mempool_free"):
        "orphaned backend (no OSAL_BACKEND branch names it); same flattening of the osMemoryPoolFree "
        "status into XY_OS_ERROR",
    ("rtx5", "xy_os_mutex_delete"):
        "orphaned backend (no OSAL_BACKEND branch names it); deletes without checking whether it is held",
    ("rtthread", "xy_os_mempool_free"):
        "compiled by no target in this build, so an edit could not be verified; its free path reports "
        "XY_OS_ERROR_PARAMETER for a block that is already on the free list, which is the same flattening "
        "the bare-metal backend had",
}


def definitions(source: str) -> dict[str, str]:
    """entry -> the body of that function, for the entries this guard knows about."""
    found: dict[str, str] = {}
    for match in DEFINITION.finditer(source):
        entry = match.group(1)
        if entry not in REQUIREMENTS:
            continue
        body = braced_body(source[match.end():])
        found[entry] = body or ""
    return found


def findings(backend_dir: Path = BACKENDS, pinned: dict[tuple[str, str], str] | None = None,
             requirements: dict[str, list[tuple[str, str]]] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    requirements = REQUIREMENTS if requirements is None else requirements
    problems: list[str] = []
    checked = 0
    found_any = 0
    for path in sorted(backend_dir.glob("*/xy_os_*.c")):
        name = path.parent.name
        for entry, body in definitions(read_code(path)).items():
            found_any += 1
            if (name, entry) in pinned:
                continue
            checked += 1
            if not body:
                problems.append(f"{name}:{entry}: the body could not be read - the scan is broken here")
                continue
            for code, meaning in requirements[entry]:
                if code not in body:
                    problems.append(
                        f"{name}:{entry}: never reports {code}, so {meaning} is indistinguishable from "
                        f"the other refusal - a caller cannot see which mistake it made")
    if found_any == 0:
        problems.append("no backend implements any of the watched entries - the walk is broken")
    elif checked == 0:
        problems.append("every implementation is pinned - nothing was actually checked")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("osal_refusal_coverage failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    checked = 0
    for path in sorted(BACKENDS.glob("*/xy_os_*.c")):
        for entry in definitions(read_code(path)):
            if (path.parent.name, entry) not in PINNED:
                checked += 1
    print(f"osal_refusal_coverage ok ({checked} implementations can express every refusal their contract "
          f"requires; pinned: {len(PINNED)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
