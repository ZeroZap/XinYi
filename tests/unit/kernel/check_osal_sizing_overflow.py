#!/usr/bin/env python3
"""A buffer sized by a product has to refuse the product that overflows.

This is the second time the same mistake appeared. xy_os_msgqueue_new and xy_os_mempool_new both
computed `count * size` into a uint32_t and compared the result against the small embedded buffer:
65536 x 65536 wraps to zero, 0x10000000 x 16 wraps to zero, both looked small enough, and both built
the object on a few hundred bytes of internal storage while reporting a capacity that needed gigabytes.
The queue then memcpy'd past its buffer; the pool walked a free-list out of it and the host test
segfaulted. Repairing the queue alone left the sibling broken, which is the argument for a rule.

The requirement is narrow on purpose: in those two constructors, refuse the product before computing it
- a division is the portable way to ask - or hand the dimensions to a creator the vendor already
guards. Anything else (a shift, a bit test, a comment claiming it cannot happen) is not a check.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import braced_body, read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKENDS = ROOT / "components" / "kernel" / "osal" / "backend"

DEFINITION = re.compile(r"^[A-Za-z_][\w \t\*]*\b(\w+)\s*\([^;{]*\)\s*$", re.M)
TARGETS = ("xy_os_msgqueue_new", "xy_os_mempool_new")
#: Asking "would this product overflow" by dividing is the portable form; SIZE_MAX and UINT_MAX are the
#: same question asked of a different width.
OVERFLOW_GUARD = re.compile(r"(?:UINT32_MAX|SIZE_MAX|UINT_MAX)\s*/")

#: Constructors that hand the dimensions to a creator which refuses the overflowing product itself.
PINNED = {
    ("freertos", "xy_os_msgqueue_new"):
        "delegates to xQueueCreate, which returns NULL when uxQueueLength * uxItemSize overflows "
        "size_t - the check happens where the allocation does",
    ("cmsis_rtx", "xy_os_msgqueue_new"): "compiled by no target in this build",
    ("cmsis_rtx", "xy_os_mempool_new"): "compiled by no target in this build",
    ("rtthread", "xy_os_msgqueue_new"): "compiled by no target in this build",
    ("rtthread", "xy_os_mempool_new"): "compiled by no target in this build",
    ("rtx5", "xy_os_msgqueue_new"): "orphaned backend: no OSAL_BACKEND branch names this directory",
    ("rtx5", "xy_os_mempool_new"): "orphaned backend: no OSAL_BACKEND branch names this directory",
}


def bodies(source: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for match in DEFINITION.finditer(source):
        body = braced_body(source[match.end():])
        if body:
            out.setdefault(match.group(1), body)
    return out


def with_helpers(name: str, all_bodies: dict[str, str], depth: int = 1, seen: set[str] | None = None) -> str:
    seen = seen or set()
    if name in seen or name not in all_bodies:
        return ""
    seen.add(name)
    body = all_bodies[name]
    text = body
    if depth > 0:
        for helper in sorted(set(re.findall(r"\b(\w+)\s*\(", body))):
            if helper != name and helper in all_bodies:
                text += "\n" + with_helpers(helper, all_bodies, depth - 1, seen)
    return text


def findings(backend_dir: Path = BACKENDS, pinned: dict[tuple[str, str], str] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    problems: list[str] = []
    checked = 0
    unpinned_files = 0
    for path in sorted(backend_dir.glob("*/xy_os_*.c")):
        name = path.parent.name
        all_bodies = bodies(read_code(path))
        # Count the file itself, not only files that contained a target: a backend with none is exactly
        # the case where "no findings" and "nothing was looked at" look the same.
        if not any((name, target) in pinned for target in TARGETS):
            unpinned_files += 1
        for target in TARGETS:
            if target not in all_bodies:
                continue
            if (name, target) in pinned:
                continue
            checked += 1
            if not OVERFLOW_GUARD.search(with_helpers(target, all_bodies)):
                problems.append(
                    f"{name}:{target}: sizes storage with a product and never refuses the product that "
                    f"overflows, so the result can wrap small enough to pass a size test while the "
                    f"object reports a capacity it cannot hold")
    if unpinned_files and checked == 0:
        problems.append("no sizing constructor was checked - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("osal_sizing_overflow failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    checked = sum(1 for path in sorted(BACKENDS.glob("*/xy_os_*.c"))
                  for target in TARGETS
                  if target in bodies(read_code(path))
                  and (path.parent.name, target) not in PINNED)
    print(f"osal_sizing_overflow ok ({checked} sizing constructors refuse an overflowing product; "
          f"pinned: {len(PINNED)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
