#!/usr/bin/env python3
"""A pool free has two ways to refuse, and they mean different things.

"this pointer was never mine" and "this block is already free" are not the same event: the first is a
caller passing the wrong address, the second is a caller releasing something twice. FreeRTOS reports
PARAMETER and RESOURCE for them, and the openCH firmware self-check keeps the two apart with a separate
marker for each. The bare-metal backend reported PARAMETER for both - which is how a test can pin a
defect as a contract and still look right.

The rule is narrow: the function must be able to say both things.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import braced_body, read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKENDS = ROOT / "components" / "kernel" / "osal" / "backend"

ENTRY = "xy_os_mempool_free"
DEFINITION = re.compile(r"xy_os_status_t\s+" + ENTRY + r"\s*\([^;{]*\)\s*$", re.M)

#: Backends that cannot report both, with the reason.
PINNED = {
    "cmsis_rtx":
        "compiled by no target in this build; it maps every osMemoryPoolFree failure to XY_OS_ERROR, so "
        "CMSIS's own osErrorParameter and osErrorResource are flattened away before they reach a caller",
    "rtx5":
        "orphaned backend (no OSAL_BACKEND branch names it); same flattening of the osMemoryPoolFree "
        "status into XY_OS_ERROR",
    "rtthread":
        "compiled by no target in this build, so an edit could not be verified; its free path reports "
        "XY_OS_ERROR_PARAMETER for a block that is already on the free list, which is the same flattening "
        "the bare-metal backend had",
}


def documents(source: str) -> bool:
    return bool(DEFINITION.search(source))


def body_of(source: str) -> str:
    match = DEFINITION.search(source)
    if not match:
        return ""
    body = braced_body(source[match.end():])
    return body or ""


def findings(backend_dir: Path = BACKENDS, pinned: dict[str, str] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    problems: list[str] = []
    checked = 0
    implemented = 0
    for path in sorted(backend_dir.glob("*/xy_os_*.c")):
        name = path.parent.name
        source = read_code(path)
        if not documents(source):
            continue
        implemented += 1
        if name in pinned:
            continue
        checked += 1
        body = body_of(source)
        if not body:
            problems.append(f"{name}:{ENTRY}: the body could not be read - the scan is broken here")
            continue
        for code, meaning in (("XY_OS_ERROR_PARAMETER", "a pointer this pool never handed out"),
                              ("XY_OS_ERROR_RESOURCE", "a block that is already free")):
            if code not in body:
                problems.append(
                    f"{name}:{ENTRY}: never reports {code}, so {meaning} is indistinguishable from the "
                    f"other refusal - a caller cannot see which mistake it made")
    if implemented == 0:
        problems.append("no backend implements a pool free - the walk is broken")
    elif checked == 0:
        problems.append("every implementation is pinned - nothing was actually checked")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("osal_mempool_free_contract failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    implemented = sum(1 for path in sorted(BACKENDS.glob("*/xy_os_*.c"))
                      if documents(read_code(path)))
    print(f"osal_mempool_free_contract ok ({implemented - len(PINNED)} pool frees report both refusals; "
          f"pinned: {len(PINNED)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
