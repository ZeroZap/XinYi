#!/usr/bin/env python3
"""Guard the include paths of the non-default OSAL backends.

gcc silently ignores an -I directory that does not exist, so a wrong backend include
path does not fail the configure step or even the compile of an unrelated target -
the backend simply compiles to an empty object. That is how `third_party/rt-thread/
include` and `third_party/cmsis-rtx/Include` went unnoticed: both name directories
that are not in the tree. This checks the paths against the filesystem and requires
each non-default branch to fail closed when its headers are absent.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_hash_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
CMAKE = ROOT / "components" / "kernel" / "osal" / "CMakeLists.txt"

# Each backend must resolve to a header that really exists in this checkout.
HEADERS = {
    "rtthread": "third_party/rt-thread/rt-thread/include/rtthread.h",
    "cmsis_rtx": "third_party/CMSIS-RTX/CMSIS-RTX/Include/rtx_os.h",
}

# Paths that were wrong: the top-level rt-thread directory has no include/, and the
# CMSIS-RTX tree is upper case and nested.
OBSOLETE = (
    "third_party/rt-thread/include",
    "third_party/cmsis-rtx/Include",
)


def backend_block(text: str, backend: str) -> str:
    """Return the elseif() branch body for one backend."""
    marker = f'elseif(OSAL_BACKEND STREQUAL "{backend}")'
    start = text.find(marker)
    if start < 0:
        return ""
    end = text.find("elseif(", start + len(marker))
    stop = text.find("else()", start + len(marker))
    for candidate in (end, stop, len(text)):
        if candidate > 0:
            end = candidate if end < 0 else min(end, candidate)
    return text[start:end]


def main() -> int:
    errors: list[str] = []
    text = read_hash_code(CMAKE)

    for backend, rel_header in HEADERS.items():
        block = backend_block(text, backend)
        if not block:
            errors.append(f"{backend}: branch is missing from {CMAKE.name}")
            continue
        if not (ROOT / rel_header).is_file():
            errors.append(f"{backend}: vendored header {rel_header} is not in the tree")
        for piece in Path(rel_header).parts[:-1]:
            if f"{piece}" not in block:
                errors.append(
                    f"{backend}: include path in CMake does not mention '{piece}', so it does "
                    f"not point at {rel_header}"
                )
                break
        if "FATAL_ERROR" not in block:
            errors.append(
                f"{backend}: selecting a backend whose headers are missing has to fail at "
                "configure time, not compile to an empty object"
            )

    for stale in OBSOLETE:
        if stale in text:
            errors.append(f"stale include path is back: {stale}")

    if errors:
        print("osal_backend_paths failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"osal_backend_paths_ok backends={len(HEADERS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
