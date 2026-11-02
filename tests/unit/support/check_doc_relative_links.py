#!/usr/bin/env python3
"""The documentation's front door links to a tree that was never built.

`docs/index.md` is the page a reader lands on, and it was written against a documentation layout
that only ever existed as a plan: `build-system/cmake.md`, `testing/strategy.md`,
`architecture/overview.md`, `components/hal/xy_hal_gpio.md`, one page per peripheral, none of them
ever written. Measured against the tree, **66 of its links** could not be opened - and nothing in
the repository noticed, because a link is prose to every other guard.

The second class is subtler and larger: links written from the **repository root** while the page
sits somewhere else. `docs/components/index.md` says `components/hal/introduction.md`; from that
page a browser resolves `docs/components/components/hal/introduction.md`. The author was reading
the tree, not the link. So the rule here is the reader's rule - a target must exist relative to the
**containing file**, and a target that only resolves from the repository root is reported as
exactly that mistake, with the path it produced, because the fix is to prefix the `../` rather than
to write the page again.

Three things are not links and must not be judged:

  - a fenced block. `CONTRIBUTING.md` prints `[相关文档 1](link1.md)` inside the document template it
    is teaching a contributor to copy; measured as a link, the template is a broken link.
  - an inline code span. `` `![alt](url)` `` is a sentence about markdown syntax.
  - a placeholder. `<name>`, `{id}`, `*.md` are targets still being filled in - the same category
    error `check_doc_api_symbols.py` already handles with its own `PLACEHOLDERS`.

Scope is the documents that instruct, the same set the API-symbol guard sweeps: the entry pages
(`README.md`, `CONTRIBUTING.md`, `docs/README.md`, `docs/index.md`), the getting-started, API, HAL,
hardware and component guide trees, the undated pages of `docs/design`, and first-party component
READMEs.

`docs/history` and `docs/plans` are records - a dated page says what was true on that day, and
rewriting its links falsifies the record instead of fixing a defect. Dated pages inside
`docs/design` are skipped for the same reason, by the same filename test the symbol guard uses.
Imported vendor trees are skipped because their links are upstream's (coreJSON's README points at
`.gitmodules` and `source/include`, which a submodule checkout has and this vendored copy does not).
`docs/reference` holds TAOCP study notes, not claims about this tree, and `docs/rules` is a corpus
of standards whose own cross-links are a separate slice.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[3]

#: Pages a reader arrives on, judged as one set because they are the same claim: where to go next.
ENTRY_PAGES = ("README.md", "CONTRIBUTING.md", "docs/index.md", "docs/README.md")
#: Guide trees that describe the tree as it is today.
DOC_ROOTS = ("docs/getting-started", "docs/api", "docs/hal", "docs/hardware", "docs/components")
#: Only the undated pages here; a date in the filename is the page announcing it is a record.
DESIGN_ROOT = "docs/design"
#: First-party component documents, minus the imported trees listed below.
README_ROOTS = ("components",)
#: Imported trees. Their READMEs belong to upstream and their links describe a checkout this
#: repository does not make - `components/dm/coreJSON` points at `.gitmodules` and `source/include`.
VENDOR_ROOTS = ("components/third_party", "components/dm/coreJSON", "components/net/at")
DATED_RECORD = re.compile(r"\d{4}-\d{2}-\d{2}")

FENCE = re.compile(r"^\s*(`{3,}|~{3,})\s*(\S*)\s*$")
INLINE_CODE = re.compile(r"`[^`\n]*`")
#: `[text](inside)` and `![alt](inside)`. The inside is parsed separately because markdown has three
#: ways to write a destination and only one of them tolerates a space: `<a b.md>`, a bare path, or a
#: bare path followed by a `"title"`. `[6A 工作流](misc/6A 工作流详解.md)` was measured in
#: `docs/README.md` - a reader gets no link at all, only the literal brackets, so the file it names
#: is unreachable no matter that the file exists.
LINK = re.compile(r"!?\[[^\]\n]*\]\(([^)\n]*)\)")
SCHEMES = ("http://", "https://", "mailto:", "ftp://", "tel:", "file:", "#")
#: A target still being filled in is not a target: `<name>`, `{id}`, `*.md`, `path/...`.
PLACEHOLDERS = ("<", ">", "{", "}", "*", "...")
#: Titles a link may carry after the destination.
TITLE_OPENERS = ('"', "'", "(")


def destination(inside: str) -> tuple[str, str]:
    """(target, problem) for the text between a link's parentheses.

    ``problem`` is ``"space"`` when the destination carries a raw space without the ``<...>`` that
    would make markdown accept it, and ``""`` otherwise.
    """
    inside = inside.strip()
    if inside.startswith("<"):
        # The angle-bracket form wraps the whole destination. `<name>.md` is therefore malformed -
        # the `>` sits inside the path - and markdown renders it as text, the same category as a
        # placeholder still being filled in.
        if ">" not in inside:
            return "", ""
        target = inside[1:inside.index(">")].strip()
        rest = inside[inside.index(">") + 1:].strip()
        if rest and not rest.startswith(TITLE_OPENERS):
            return "", ""
        return target, ""
    parts = inside.split(None, 1)
    if not parts:
        return "", ""
    target, rest = parts[0], (parts[1].strip() if len(parts) > 1 else "")
    if rest and not rest.startswith(TITLE_OPENERS):
        return target, "space"
    return target, ""


def documents(root: Path | None = None) -> list[Path]:
    """Every markdown page this guard judges, as absolute paths."""
    root = ROOT if root is None else root
    found: list[Path] = []
    for relative in ENTRY_PAGES:
        path = root / relative
        if path.is_file():
            found.append(path)
    for tree in DOC_ROOTS:
        directory = root / tree
        if directory.is_dir():
            found.extend(sorted(directory.rglob("*.md")))
    design = root / DESIGN_ROOT
    if design.is_dir():
        for path in sorted(design.rglob("*.md")):
            if not DATED_RECORD.search(path.name):
                found.append(path)
    for tree in README_ROOTS:
        directory = root / tree
        if not directory.is_dir():
            continue
        for path in sorted(directory.rglob("README.md")):
            relative = path.relative_to(root).as_posix()
            if any(relative == vendor or relative.startswith(vendor + "/") for vendor in VENDOR_ROOTS):
                continue
            found.append(path)
    return sorted(set(found))


def targets(text: str):
    """(line number, target, problem) for every relative link a reader could click.

    Fenced blocks and inline code spans are removed first, so a document teaching markdown - or a
    document quoting a shell command that contains brackets - is not read as a set of links.
    ``problem`` is empty for a well-formed destination.

    The fence is tracked the way CommonMark tracks it: the closing line must carry **no** info
    string. `docs/index.md` prints a markdown template whose body is itself a ```c block, and a
    toggle on the three backticks alone flips on the inner line and leaves every later link in the
    document being read as fenced - or, in the other direction, the template's own links being read
    as claims.
    """
    opening = ""
    for number, line in enumerate(text.splitlines(), 1):
        fence = FENCE.match(line)
        if fence:
            marker, info = fence.group(1), fence.group(2)
            if not opening:
                opening = marker
            elif marker[0] == opening[0] and len(marker) >= len(opening) and not info:
                opening = ""
            continue
        if opening:
            continue
        for match in LINK.finditer(INLINE_CODE.sub("", line)):
            target, problem = destination(match.group(1))
            if not target or target.startswith(SCHEMES):
                continue
            if any(marker in target for marker in PLACEHOLDERS):
                continue
            # `page.md#section` and `page.md?plain=1` are the same file; percent-encoding is a
            # filename with a space or a CJK character in it, not a different name.
            yield number, unquote(target.split("#", 1)[0].split("?", 1)[0]), problem


def resolves_from_root(page: Path, target: str, root: Path | None = None) -> bool:
    """Whether the target would exist if the page were the repository root.

    Only used to name the mistake precisely. A link that resolves from the root and not from the
    page is not a missing document - it is a correct path measured from the wrong place, and the
    reader still gets a 404.
    """
    root = ROOT if root is None else root
    return (root / target).exists()


def findings(root: Path | None = None, files: list[Path] | None = None) -> list[str]:
    """Every relative link that cannot be opened from the page that carries it."""
    root = ROOT if root is None else root
    problems: list[str] = []
    inspected = 0
    for path in files if files is not None else documents(root):
        inspected += 1
        shown = shown_path(path, root)
        for number, target, problem in targets(path.read_text(encoding="utf-8", errors="replace")):
            if problem == "space":
                problems.append(
                    f"{shown}:{number}: {target} has a raw space in the destination - markdown does "
                    f"not parse that as a link at all, so a reader sees the brackets and reaches "
                    f"nothing; percent-encode the space or wrap the destination in <...>")
                continue
            landing = (path.parent / target).resolve()
            if landing.exists():
                continue
            if resolves_from_root(path, target, root):
                problems.append(
                    f"{shown}:{number}: {target} is written from the repository root - a reader on "
                    f"this page gets {shown_landing(path, target)}, which does not exist; the "
                    f"target does exist, so this is the path that needs the `../`, not the page")
            else:
                problems.append(
                    f"{shown}:{number}: {target} does not exist, from this page or from the "
                    f"repository root - a reader who clicks it gets a 404")
    if inspected == 0:
        problems.append("no documents were inspected - the walk is broken")
    return problems


def shown_path(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def shown_landing(page: Path, target: str) -> str:
    """The path a reader's browser would request, as the page's own directory would produce it."""
    return page.parent.as_posix() + "/" + target


#: A navigation entry: `- Title: path.md`. A group header (`- 组件文档:`) has no path and is not
#: an entry, which is why the target must end in `.md`.
NAV_ENTRY = re.compile(r"^\s*-\s+(.+?):\s+(\S+\.md)\s*$")


def nav_findings(root: Path | None = None, config: Path | None = None) -> list[str]:
    """Every page the site navigation names must exist under `docs/`.

    The same planned tree reached the site config: `docs/mkdocs.yml` navigated to
    `getting-started/features.md`, `api/by-component.md`, one `components/<x>/index.md` per component
    and a whole `toolchain/` section - 39 of its 58 entries named pages that were never written.
    mkdocs either fails the build or drops the page, and either way the reader's menu lies. This is
    judged separately from the markdown sweep because it is a different file, not a different claim.
    """
    root = ROOT if root is None else root
    config = config or (root / "docs" / "mkdocs.yml")
    if not config.is_file():
        return []
    problems: list[str] = []
    inspected = 0
    for number, line in enumerate(config.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        match = NAV_ENTRY.match(line)
        if not match:
            continue
        inspected += 1
        target = match.group(2)
        if not (root / "docs" / target).exists():
            problems.append(
                f"docs/mkdocs.yml:{number}: the navigation entry {match.group(1)!r} points at "
                f"docs/{target}, which does not exist - the site either fails to build or loses "
                f"the page, and the menu still offers it")
    if inspected == 0:
        problems.append("docs/mkdocs.yml names no pages - the navigation was not parsed")
    return problems


def main() -> int:
    problems = findings() + nav_findings()
    pages = documents()
    links = sum(1 for path in pages
                for _ in targets(path.read_text(encoding="utf-8", errors="replace")))
    if problems:
        print("doc_relative_links failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    nav = len([line for line in (ROOT / "docs" / "mkdocs.yml").read_text(encoding="utf-8").splitlines()
               if NAV_ENTRY.match(line)]) if (ROOT / "docs" / "mkdocs.yml").is_file() else 0
    print(f"doc_relative_links ok ({len(pages)} documents, {links} relative links, each resolving "
          f"from the page that carries it; {nav} navigation entries, each naming a page that exists)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
