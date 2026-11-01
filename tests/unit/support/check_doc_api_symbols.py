#!/usr/bin/env python3
"""A getting-started document that teaches an API the repository does not have.

`docs/getting-started/QUICK_START.md` opens with a "first application" whose whole point is that
a reader can copy it. It used `xy_hal_pin_config_t`, `xy_hal_pin_init()`, `xy_hal_pin_write()`,
`xy_os_thread_new` spelled `xy_os_thread_create()`, and `xy_crypto.h`. None of those names exist:
the declaration is `xy_hal_gpio_config_t` / `xy_hal_gpio_init()` / `xy_hal_gpio_write()`, threads
are created by `xy_os_thread_new()` with an attribute struct, and the crypto umbrella is
`xy_tiny_crypto.h`. Compiled as written, the sample produces twelve errors - and nothing in the
repository noticed, because prose is not built.

So this judges the three things a copied snippet asserts:

  1. an identifier - every `xy_*` token inside a fenced C block must appear in some header;
  2. an include - every `#include "name.h"` must name a header that exists;
  3. a location - every backticked repository path (`examples/...`, `components/...`, `docs/...`)
     must exist.

The first two are checked against headers read through `read_code()`, so a symbol that only appears
in a comment does not count as declared. That direction matters: a name mentioned in a header's
prose is exactly the kind of thing a doc author would copy.

The third rule exists because the first two were not enough: `QUICK_START.md` told a new reader that
the sensor example was `examples/sensor_demo` and that the OSAL guide was
`docs/components/kernel/osal/README.md`. Measured against the tree, **four of the five "reading
order" links and three of the four example rows pointed at paths that do not exist** - the component
docs had been moved one level down (into `components/<x>/`) and nothing read the document as a set of
claims. A path is a claim.

Scope is `docs/getting-started/`, the documents that instruct. Architecture notes, proposals and
the API reference legitimately describe APIs that are being designed - judging them would punish
writing the plan down, which is the mistake this repository has already made once. `docs/history`
records what was true when it was recorded and is not rewritten.

Boundaries, stated rather than hidden: only fenced ```c / ```cpp blocks are judged (prose and
shell blocks are not API claims - a shell block's `make <target>` is a different rule), and the
fences are read by the shared `source_scan.fenced_blocks()` so a `~~~` block, a nested
four-backtick block and a block whose terminator went missing are all read the way CommonMark
says rather than by one regex over the whole document. A block may declare its own helper, so a
name the block defines itself is not reported. The path rule reads prose as well, because a table
row names a location as firmly as a sentence does; a line that tells the reader to *create* the path
(`创建` / `新建` / `create`) is an instruction, not a claim that it exists, and a placeholder
(`<name>`, `...`) is not a path yet.

Presence is not correctness: this says a symbol exists, not that the sample uses it properly. The
two samples were also compiled - as written, against the real include roots, for both crypto
include orders - because the first draft of this guard would have passed a sample that included
two headers which declared `xy_crc32` with different parameter types. The stale second `xy_crc.h`
that conflict came from is now a compatibility forward to the CRC component's own header; that is
a claim about the build, and `gcc -fsyntax-only` is what proved it.
"""

from __future__ import annotations

import difflib
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import code_only, fenced_blocks, read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
DOC_ROOTS = ("docs/getting-started",)
INDEX_ROOTS = ("components", "boards")
#: Fenced-block languages that make a compile claim. A `bash` block is judged by a different rule
#: (its `make <target>` names a target, not a symbol).
CODE_LANGUAGES = ("c", "cpp", "c++")
#: Documents exempted from the sweep, each with the reason. Empty on purpose: the scope is
#: already the narrow set of documents that instruct.
EXEMPT: dict[str, str] = {}

TOKEN = re.compile(r"\bxy_[A-Za-z0-9_]+\b")
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
DEFINES_ITSELF = re.compile(r"^\s*#\s*define\s+(xy_[A-Za-z0-9_]+)")

#: Top-level directories a document may point a reader at. Restricting the prefix to these keeps
#: the rule off prose that merely contains a slash.
PATH_ROOTS = ("examples", "components", "boards", "tests", "projects", "tools", "docs")
PATH_REFERENCE = re.compile(
    r"`((?:" + "|".join(PATH_ROOTS) + r")/[A-Za-z0-9_./-]+)`")
#: A line that tells the reader to create the path is not a claim that it exists today.
CREATION_WORDS = ("创建", "新建", "create", "Create", "your")
#: ... and a placeholder is not a path yet.
PLACEHOLDERS = ("...", "<", ">", "*")


def documents() -> list[Path]:
    found: list[Path] = []
    for root in DOC_ROOTS:
        directory = ROOT / root
        if not directory.is_dir():
            continue
        for path in sorted(directory.rglob("*.md")):
            if path.relative_to(ROOT).as_posix() in EXEMPT:
                continue
            found.append(path)
    return found


def header_index(roots: tuple[str, ...] = INDEX_ROOTS) -> tuple[set[str], set[str]]:
    """(every identifier a header declares or uses, every header basename)."""
    symbols: set[str] = set()
    names: set[str] = set()
    for root in roots:
        directory = ROOT / root
        if not directory.is_dir():
            continue
        for path in directory.rglob("*.h"):
            names.add(path.name)
            symbols.update(TOKEN.findall(read_code(path)))
    return symbols, names


def code_blocks(text: str) -> list[tuple[int, list[str]]]:
    """(first line number, lines) for every fenced C/C++ block, read by the shared reader."""
    blocks: list[tuple[int, list[str]]] = []
    for info, start, body in fenced_blocks(text):
        language = info.split()[0].lower() if info.split() else ""
        if language in CODE_LANGUAGES:
            blocks.append((start, body.splitlines()))
    return blocks


def path_references(text: str) -> list[tuple[int, str]]:
    """(line number, path) for every repository path this document points a reader at.

    Placeholders and lines that ask the reader to create something are skipped: neither asserts
    that the path is there today, and reporting them would punish a document for explaining how to
    start a new one.
    """
    found: list[tuple[int, str]] = []
    for number, line in enumerate(text.splitlines(), start=1):
        if any(word in line for word in CREATION_WORDS):
            continue
        for match in PATH_REFERENCE.finditer(line):
            token = match.group(1).rstrip("/")
            if any(mark in token for mark in PLACEHOLDERS):
                continue
            found.append((number, token))
    return found


def findings(paths: list[Path] | None = None,
             symbols: set[str] | None = None,
             names: set[str] | None = None,
             root: Path | None = None) -> list[str]:
    walked_symbols: set[str] = set()
    walked_names: set[str] = set()
    if symbols is None or names is None:
        walked_symbols, walked_names = header_index()
    index_symbols = symbols if symbols is not None else walked_symbols
    index_names = names if names is not None else walked_names
    tree = ROOT if root is None else root
    sweep = paths is None
    candidates = documents() if paths is None else paths
    problems: list[str] = []
    inspected_blocks = 0
    inspected_paths = 0
    for path in candidates:
        text = path.read_text(encoding="utf-8", errors="replace")
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        for number, token in path_references(text):
            inspected_paths += 1
            if not (tree / token).exists():
                problems.append(
                    f"{relative}:{number}: points readers at `{token}`, which does not exist in "
                    f"this tree - a reader who follows it finds nothing")
        for start, lines in code_blocks(text):
            inspected_blocks += 1
            local = {m.group(1) for line in lines for m in [DEFINES_ITSELF.match(line)] if m}
            # A sample's own comments are prose: a block that writes down "the old spelling was
            # xy_hal_pin_write()" is explaining, not calling. Same rule as read_code() for headers.
            code = code_only("\n".join(lines)).splitlines()
            for offset, line in enumerate(code):
                where = f"{relative}:{start + offset}"
                include = INCLUDE.match(line)
                if include:
                    header = Path(include.group(1)).name
                    if header not in index_names:
                        problems.append(
                            f"{where}: includes \"{header}\", which is not a header in this repository - "
                            f"the reader cannot compile it")
                    continue
                for token in TOKEN.findall(line):
                    if token in index_symbols or token in local:
                        continue
                    near = difflib.get_close_matches(token, index_symbols, n=1)
                    hint = f" (did you mean {near[0]}?)" if near else ""
                    problems.append(
                        f"{where}: calls {token}{hint}, which no header declares - a copied sample that "
                        f"does not compile is not an example")
    if inspected_blocks == 0:
        problems.append(
            f"no fenced C block was found under {', '.join(DOC_ROOTS)} - the walk is broken, and a "
            f"walk that inspects nothing reports the same result as a clean one")
    if sweep and inspected_paths == 0:
        problems.append(
            f"no repository path reference was found under {', '.join(DOC_ROOTS)} - the path walk "
            f"is broken, and a walk that inspects nothing reports the same result as a clean one")
    return problems


def main() -> int:
    symbols, names = header_index()
    problems = findings(symbols=symbols, names=names)
    if problems:
        for problem in problems:
            print(problem)
        return 1
    print(f"doc_api_symbols ok ({len(documents())} documents, {len(symbols)} declared identifiers, "
          f"{len(names)} headers)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
