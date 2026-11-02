#!/usr/bin/env python3
"""A reference page's prototype is a claim, and the compiler is the only thing that can check it.

`docs/api/API_REFERENCE.md` is the page a reader opens to learn a signature, and until now nothing
had ever compiled it. Measured against the headers it documents, **five of its thirteen prototypes
did not match**:

- `xy_sht30_init()` was written with three parameters - `(dev, i2c_handle, addr)` - and the function
  takes two; the address-taking entry is `xy_sht30_init_addr()`. A reader following this page wrote
  a call that does not exist;
- `xy_hal_gpio_init()` was written with a `uint16_t pin`, where the header takes `uint8_t`;
- `xy_os_thread_create()` and `xy_os_thread_t` named a function and a type no header declares; the
  header's `xy_os_thread_new()` takes an `xy_os_thread_attr_t` and returns an `xy_os_thread_id_t`.

Two more: `xy_os_kernel_init()` / `xy_os_kernel_start()` were documented as returning `int` where
they return `xy_os_status_t` - a warning under `-Wenum-int-mismatch`, an error under `-Werror`.

None of that is visible to a name sweep, which is why this defect survived next door to a guard
that reports `ok` about the same directory: `xy_sht30_init` *is* a declared symbol. Existence and
signature are different claims, and only the second one needs a compiler.

So this compiles the documents. Each fenced C block is appended to a translation unit that first
includes the headers the page is about, with a `#line` directive before every block so a diagnostic
names the page and the page's own line rather than a temporary file. The compiler is asked for
`-fsyntax-only -Wall -Wextra -Werror`: a prototype that disagrees with its header is a
`conflicting types` error, a type the headers do not define is an unknown type name, and a page that
was fine yesterday stays fine without a build.

Boundaries, stated rather than hidden:

- **Presence is not this guard's job.** A bare `int xy_nothing(void);` declares a function that
  exists nowhere and compiles happily - the symbol sweep is what catches that, and the two rules are
  complementary on purpose rather than one of them being redundant.
- **What a prototype can assert is what is judged**: return type, arity, parameter types. Behaviour,
  ownership, sentinel values and threading rules stay prose; a signature that compiles can still
  mislead about semantics, and this file does not claim otherwise.
- A document with no fenced C block is reported rather than skipped, because an extractor that finds
  nothing reports the same result as a page that compiles cleanly.
- A compiler that cannot be run is reported too. A missing toolchain must not read as a pass.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from typing import NamedTuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import fenced_blocks  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
#: Fenced languages that make a compile claim.
CODE_LANGUAGES = ("c", "cpp", "c++")
#: Warnings are errors: `-Wenum-int-mismatch` is what reports a documented `int` against a header's
#: `xy_os_status_t`, and it is only a warning by default. A documentation defect that scrolls past in
#: a warning list is not a gate.
COMPILE_ARGS = ("-std=c99", "-Wall", "-Wextra", "-Werror", "-fsyntax-only")


class Sample(NamedTuple):
    """One document that makes a compile claim, plus what a translation unit needs to judge it."""

    doc: str
    #: Headers the page is *about*. Included before every block so the page's own prototypes are
    #: redeclarations checked against the real ones instead of free-standing declarations.
    includes: tuple[str, ...]
    #: Include roots, relative to the repository root, in the order the page's intended headers need.
    roots: tuple[str, ...]


SAMPLES: tuple[Sample, ...] = (
    Sample(
        doc="docs/api/API_REFERENCE.md",
        includes=(
            "xy_os.h",
            "xy_hal_gpio.h",
            "xy_sht30.h",
            "xy_mpu6050.h",
            "xy_pid.h",
            "xy_fota.h",
            "xy_oled_ssd1306.h",
            "xy_font.h",
        ),
        roots=(
            "components/kernel/osal",
            "components/kernel/osal/inc",
            "components/kernel/misc/inc",
            "components/hal/inc",
            "components/clib/xy_clib/inc",
            "components/clib/xy_clib",
            "components/common/inc",
            "components/pid/inc",
            "components/fota/inc",
            "components/sensor/inc",
            "components/gui/inc",
            "components/device/inc",
            "components/drivers/sensor/temperature/sht30",
            "components/drivers/sensor/motion/mpu6050",
        ),
    ),
)


def code_blocks(path: Path) -> list[tuple[int, str]]:
    """(first line number, text) for every fenced C/C++ block in the document."""
    text = path.read_text(encoding="utf-8", errors="replace")
    blocks: list[tuple[int, str]] = []
    for info, start, body in fenced_blocks(text):
        language = info.split()[0].lower() if info.split() else ""
        if language in CODE_LANGUAGES:
            blocks.append((start, body))
    return blocks


def translation_unit(doc: str, blocks: list[tuple[int, str]], includes: tuple[str, ...]) -> str:
    """One compilation unit: the page's subject headers, then each block at its own line number."""
    lines = [f'#include "{name}"' for name in includes]
    for start, body in blocks:
        # A diagnostic then reads `docs/api/API_REFERENCE.md:30:16: error: ...` - the page and the
        # page's line, which is the only location a reader can act on.
        lines.append(f'#line {start} "{doc}"')
        lines.append(body.rstrip())
    return "\n".join(lines) + "\n"


def run_compiler(unit: str, roots: tuple[str, ...], compiler: str, root: Path) -> tuple[int, str]:
    """(exit code, diagnostics) for one unit, read from stdin so no temporary file is written."""
    command = [compiler, *COMPILE_ARGS]
    command += [f"-I{(root / name).as_posix()}" for name in roots]
    command += ["-x", "c", "-"]
    try:
        finished = subprocess.run(command, input=unit, capture_output=True, text=True, check=False)
    except OSError as error:
        return 1, f"cannot run `{compiler}`: {error}"
    return finished.returncode, (finished.stdout + finished.stderr).strip()


def findings(samples: tuple[Sample, ...] | None = None, root: Path | None = None,
             compiler: str = "cc") -> list[str]:
    """Every document's blocks must compile against the headers that document is about."""
    tree = ROOT if root is None else root
    judged = SAMPLES if samples is None else samples
    problems: list[str] = []
    inspected = 0
    for sample in judged:
        path = tree / sample.doc
        if not path.is_file():
            problems.append(
                f"{sample.doc}: listed here as a document that makes a compile claim, but the file "
                f"is gone - the list has drifted from the tree")
            continue
        blocks = code_blocks(path)
        if not blocks:
            problems.append(
                f"{sample.doc}: no fenced C block was found - the extractor is broken, and a page "
                f"with no blocks compiles exactly as cleanly as a correct one")
            continue
        inspected += len(blocks)
        status, output = run_compiler(translation_unit(sample.doc, blocks, sample.includes),
                                      sample.roots, compiler, tree)
        if status != 0:
            problems.append(
                f"{sample.doc}: its {len(blocks)} C blocks do not compile against the headers it "
                f"documents - a signature the header contradicts is a call the reader cannot write:\n"
                f"{output}")
    if inspected == 0:
        problems.append(
            "no fenced C block was compiled - the walk is broken, and a walk that inspects nothing "
            "reports the same result as a clean one")
    return problems


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    compiler = "cc"
    for index, argument in enumerate(args):
        if argument == "--cc" and index + 1 < len(args):
            compiler = args[index + 1]
        elif argument.startswith("--cc="):
            compiler = argument.split("=", 1)[1]
    problems = findings(compiler=compiler)
    if problems:
        for problem in problems:
            print(problem)
        return 1
    blocks = sum(len(code_blocks(ROOT / sample.doc)) for sample in SAMPLES)
    print(f"doc_samples_compile ok ({len(SAMPLES)} documents, {blocks} C blocks compiled with "
          f"`{compiler}` against {sum(len(s.roots) for s in SAMPLES)} include roots)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
