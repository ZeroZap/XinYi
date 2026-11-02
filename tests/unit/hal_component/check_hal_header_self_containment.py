#!/usr/bin/env python3
"""Every HAL header must compile alone. Most of them did not, and the umbrella is what hid it.

`xy_hal_dac.h` declared `xy_hal_dac_write_dma(..., size_t count)` and never included `<stddef.h>`.
That went unnoticed for as long as it did because nothing ever compiled the header on its own: a
consumer reached `<stddef.h>` through `<stdio.h>` or through another HAL header first, so the defect
only existed in the include order nobody used. Completing `xy_hal.h` changed that order and turned it
into a hard error in the very first test target:

    components/hal/inc/xy_hal_dac.h:169:60: error: unknown type name 'size_t'

The same shape was present in four more headers (`adc`, `can`, `i2s`, `sys`): 19 uses of `size_t`
across five headers, with no include that owns the type. A header that depends on ambient includes is
a header whose contract silently changes with the include list above it.

So this compiles each header of `components/hal/inc/` alone, in a throwaway translation unit that
includes nothing else, with `-Wall -Wextra -Werror -fsyntax-only`. A missing standard include, a typo
in a type name, or a dependency on a neighbour header is a failure that names the header and quotes
the compiler's first error.

This is a compile-level claim only: it says a header can be read by a compiler on its own, nothing
about what any backend implements. It also cannot see a missing include that a sibling supplies
transitively - the peripheral headers include `xy_hal.h` by design, so `xy_hal_dac.h` would still
compile if only its own `<stddef.h>` went away while `xy_hal_adc.h` kept its. What it does catch is
the failure that actually happened: the one include the whole family leans on disappearing, which
takes seventeen headers down at once.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HAL_INC = Path("components/hal/inc")
PROBE = '#include "{header}"\n\nint main(void) {{ return 0; }}\n'


def headers(root: Path) -> list[Path]:
    directory = root / HAL_INC
    if not directory.is_dir():
        return []
    return sorted(path for path in directory.iterdir() if path.is_file() and path.suffix == ".h")


def compile_alone(cc: str, include: Path, header: Path, workdir: Path) -> str | None:
    """None when `header` compiles alone, otherwise the compiler's first error line."""
    probe = workdir / f"probe_{header.stem}.c"
    probe.write_text(PROBE.format(header=header.name))
    result = subprocess.run(
        [cc, "-std=c99", "-Wall", "-Wextra", "-Werror", "-fsyntax-only", f"-I{include}", str(probe)],
        capture_output=True, text=True)
    if result.returncode == 0:
        return None
    for line in (result.stderr or result.stdout).splitlines():
        if "error:" in line:
            return line.strip()
    return f"{cc} failed with exit {result.returncode} and no error line"


def findings(root: Path | None = None, cc: str = "cc") -> list[str]:
    tree = ROOT if root is None else root
    include = tree / HAL_INC
    if not include.is_dir():
        return [f"{HAL_INC.as_posix()}: the header directory is not there"]
    candidates = headers(tree)
    if not candidates:
        return [f"{HAL_INC.as_posix()}: no header was found, so nothing was compiled"]
    problems: list[str] = []
    with tempfile.TemporaryDirectory() as directory:
        workdir = Path(directory)
        for header in candidates:
            error = compile_alone(cc, include, header, workdir)
            if error is not None:
                problems.append(f"{HAL_INC.as_posix()}/{header.name}: does not compile on its own - "
                                f"{error}")
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    parser.add_argument("--cc", default="cc", help="C compiler to use (CMake passes its own)")
    parser.add_argument("--root", default=None, help="repository root (default: this checkout)")
    args = parser.parse_args(argv)
    tree = Path(args.root).resolve() if args.root else ROOT
    problems = findings(tree, args.cc)
    if problems:
        for problem in problems:
            print(problem)
        return 1
    print(f"hal_header_self_containment ok ({len(headers(tree))} header(s) compiled alone with "
          f"-Wall -Wextra -Werror, {args.cc})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
