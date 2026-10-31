#!/usr/bin/env python3
"""A name defined twice at module level means the second definition is the one that runs.

The guards are the part of this repository that decides whether everything else is correct, so a guard
whose behaviour silently differs from what a reader sees at the top of the file is worse than no guard.
One of them carried eleven constants defined twice - the same block pasted in again by successive edits -
and every one of them happened to be identical, which is exactly why nobody noticed. Had one line differed,
the later copy would have been in force while the earlier one was the one a reader trusted.

This checks the shape, not the values: a module-level name may be bound once.
"""

from __future__ import annotations

import ast
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
GUARDS = ROOT / "tests"
CMAKE_DIRS = ("tests",)  # every CMakeLists.txt under these, plus the Makefile, may register a test

#: Support checkers that no gate target names, each with the reason it still earns its place. Empty on
#: purpose: a checker either runs in the gate or it does not, and only the first kind is a guard.
GUARD_EXEMPT: dict[str, str] = {}


def names_file(name: str, blob: str) -> bool:
    """Whether ``blob`` names the file ``name`` as a whole token.

    A plain substring test is wrong here: ``test_check_dead_cli_options.py`` contains
    ``check_dead_cli_options.py``, so registering only the self-test would make the checker itself look
    registered while nothing ran it. The lookarounds reject a preceding or trailing word character, which
    covers the ``_`` that glues the two names together.
    """
    return re.search(rf"(?<![\w-]){re.escape(name)}(?![\w-])", blob) is not None



def bound_names(tree: ast.Module) -> dict[str, list[int]]:
    """name -> the line numbers where the module binds it, at module level only."""
    seen: dict[str, list[int]] = {}

    def note(name: str, line: int) -> None:
        seen.setdefault(name, []).append(line)

    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    note(target.id, node.lineno)
                elif isinstance(target, (ast.Tuple, ast.List)):
                    for element in target.elts:
                        if isinstance(element, ast.Name):
                            note(element.id, node.lineno)
        elif isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            note(node.target.id, node.lineno)
        elif isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
            note(node.name, node.lineno)
    return seen


def runs_something(tree: ast.Module, text: str) -> bool:
    """Whether importing or running this file actually exercises anything.

    A test file with no entry point exits 0 having tested nothing, which reads the same as a passing file -
    the same shape as any other empty result. This case exists because the lint below was written after
    writing a test file that had no __main__ and silently ran zero cases.
    """
    if "unittest.main(" in text or "pytest" in text:
        return True
    for node in tree.body:
        if isinstance(node, ast.Assert):
            return True
        if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
            return True
        if isinstance(node, ast.If) and "__main__" in ast.dump(node.test) and node.body:
            return True
    return False


def entry_point_findings(root: Path = GUARDS, files: list[Path] | None = None) -> list[str]:
    problems: list[str] = []
    inspected = 0
    for path in files if files is not None else sorted(root.rglob("test_*.py")):
        inspected += 1
        text = path.read_text(encoding="utf-8", errors="replace")
        try:
            tree = ast.parse(text)
        except SyntaxError:
            continue  # reported by findings()
        if not runs_something(tree, text):
            problems.append(
                f"{path.relative_to(root)}: runs nothing - no __main__, no unittest.main, no top-level "
                f"assert or call, so it exits 0 having tested nothing and looks like a pass")
    if inspected == 0:
        problems.append("no test files were inspected - the walk is broken")
    return problems


def registration_text(root: Path = ROOT) -> str:
    """The gates that can name a test: the root CMakeLists, the Makefile, and every CMakeLists under a gate tree.

    Deliberately not every CMakeLists in the repository. A vendor or component CMakeLists that happens to
    contain a name is not a gate - counting it would let a file look registered while nothing runs it.
    """
    texts: list[str] = []
    for path in (root / "CMakeLists.txt", root / "Makefile"):
        if path.exists():
            texts.append(path.read_text(encoding="utf-8", errors="replace"))
    for tree in CMAKE_DIRS:
        for path in sorted((root / tree).rglob("CMakeLists.txt")):
            texts.append(path.read_text(encoding="utf-8", errors="replace"))
    return "\n".join(texts)


def make_variables(text: str) -> dict[str, str]:
    """`NAME := value` / `NAME = value`. The Makefile names its test directory this way, so a token like
    `$(TOOLS_TEST_DIR)` has to be resolved or the discovery line reads as covering nothing."""
    variables: dict[str, str] = {}
    for line in text.splitlines():
        name, sep, value = line.partition(":=")
        if not sep:
            name, sep, value = line.partition("=")
        name = name.strip()
        if sep and name and " " not in name and not name.startswith("\t"):
            variables.setdefault(name, value.strip())
    return variables


def expand(token: str, variables: dict[str, str]) -> str:
    """Substitute Makefile variables until the token stops changing (definitions can nest)."""
    for _ in range(8):
        before = token
        for name, value in variables.items():
            token = token.replace(f"$({name})", value).replace(f"${{{name}}}", value)
        if token == before:
            break
    return token


def discovered_dirs(text: str) -> list[str]:
    """Repository-relative directories handed to `unittest discover` by a gate."""
    variables = make_variables(text)
    dirs: list[str] = []
    for line in text.splitlines():
        if "discover" not in line or " -s " not in line:
            continue
        for token in line.split():
            candidate = expand(token.strip("\"'"), variables).rstrip("/")
            if candidate.startswith(("tests", "tools")) and "/" in candidate:
                dirs.append(candidate)
    return dirs


def registration_findings(root: Path = GUARDS, files: list[Path] | None = None,
                          text: str | None = None) -> list[str]:
    """A test file no target names is a test that never runs - green because nobody looked.

    A file counts as registered when a gate CMakeLists mentions it by name, or when the Makefile runs
    unittest discovery over the directory it lives in.
    """
    blob = registration_text() if text is None else text
    directories = discovered_dirs(blob)

    problems: list[str] = []
    inspected = 0
    for path in files if files is not None else sorted(root.rglob("test_*.py")):
        inspected += 1
        if path.name in blob:
            continue
        try:
            relative = path.resolve().relative_to(ROOT).as_posix()
        except ValueError:
            relative = ""
        if any(relative == found or relative.startswith(found + "/") for found in directories):
            continue
        problems.append(
            f"{shown(path, root)}: no gate target names this file and no discovery covers its "
            f"directory - whatever it tests is not being tested")
    if inspected == 0:
        problems.append("no test files were inspected - the walk is broken")
    return problems


def guard_registration_findings(root: Path = GUARDS, text: str | None = None,
                                files: list[Path] | None = None) -> list[str]:
    """A support checker no gate names is a gate only for whoever remembers to type it by hand.

    ``registration_findings()`` holds ``test_*.py`` to this rule and never held ``check_*.py`` to it, so a
    checker could be written, reviewed and merged with no gate target running it. The tree then stays
    green for the one reason that must never count: nothing executes the file. ``check_doc_api_symbols.py``
    sat in exactly that state - a finished guard against a getting-started sample that does not compile.
    """
    blob = registration_text() if text is None else text
    directories = discovered_dirs(blob)
    problems: list[str] = []
    inspected = 0
    for path in files if files is not None else sorted(root.rglob("check_*.py")):
        inspected += 1
        try:
            relative = path.resolve().relative_to(ROOT).as_posix()
        except ValueError:
            relative = path.as_posix()
        if names_file(path.name, blob):
            continue
        exempted = next((GUARD_EXEMPT[key] for key in (relative, path.name) if key in GUARD_EXEMPT), None)
        if exempted is not None:
            if not exempted.strip():
                problems.append(
                    f"{shown(path, root)}: exempted from the registration rule without a stated reason - "
                    f"an exemption nobody can audit is a silent skip")
            continue
        if any(relative == found or relative.startswith(f"{found}/") for found in directories):
            continue
        problems.append(
            f"{shown(path, root)}: no gate target names this checker - it never runs, and a guard that "
            f"never runs reports the same result as one that passes")
    if inspected == 0:
        problems.append("no checkers were inspected - the walk is broken")
    return problems


def shown(path: Path, root: Path) -> str:
    """A path fit for a message, whether or not it lives under the tree being inspected."""
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


#: Makefile test targets no workflow runs, and why. A target listed here is a gap that is known; a target
#: absent from this table and from every workflow is a gap nobody noticed.
CI_EXEMPT = {
    "test": "aggregate target; it only calls the targets below",
    "test-unit": "run by CI as ctest --test-dir build/tests/unit, which is the same thing",
    "test-qemu-ch32v": "needs a locally built QEMU with the ch32v307 machine and the WCH RISC-V toolchain",
}


def ci_coverage_findings(makefile: Path | None = None, workflows: list[Path] | None = None,
                         exempt: dict[str, str] | None = None) -> list[str]:
    """Every Makefile test target must be run by a workflow, or be listed as a known gap."""
    exempt = CI_EXEMPT if exempt is None else exempt
    makefile = makefile or (ROOT / "Makefile")
    workflows = workflows if workflows is not None else sorted((ROOT / ".github" / "workflows").glob("*.yml"))
    ci = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in workflows)
    targets = [line.split(":")[0] for line in makefile.read_text(encoding="utf-8").split("\n")
               if line[:1].isalpha() and ":" in line and not line.startswith("\t")
               and line.split(":")[0].startswith("test")]

    problems: list[str] = []
    for target in targets:
        if target in exempt or target in ci:
            continue
        problems.append(
            f"{target}: a Makefile test target that no workflow names and that is not listed in CI_EXEMPT - "
            f"it is a gate only for whoever remembers to type it")
    if not targets:
        problems.append("no Makefile test targets were found - the walk is broken")
    return problems


def findings(root: Path = GUARDS, files: list[Path] | None = None) -> list[str]:
    problems: list[str] = []
    inspected = 0
    paths = files if files is not None else sorted(root.rglob("check_*.py"))
    for path in paths:
        inspected += 1
        try:
            tree = ast.parse(path.read_text(encoding="utf-8", errors="replace"))
        except SyntaxError as error:
            problems.append(f"{path.relative_to(root)}: does not parse ({error.msg}) - the lint cannot see it")
            continue
        for name, lines in sorted(bound_names(tree).items()):
            if len(lines) > 1:
                problems.append(
                    f"{path.relative_to(root)}: {name} is defined at lines {lines}; the last one wins and a "
                    f"reader will assume the first - keep one, or name them differently")
    if inspected == 0:
        problems.append("no guard files were inspected - the walk is broken")
    return problems


def main() -> int:
    # Three independent questions, so each has its own function and its own tests: names bound once, test
    # files that actually run something, and checkers a gate target actually names.
    problems = (findings() + entry_point_findings() + registration_findings() + ci_coverage_findings()
                + guard_registration_findings())
    if problems:
        print("guard_module_hygiene failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"guard_module_hygiene ok ({len(list(GUARDS.rglob('check_*.py')))} guard files, each name bound once "
          f"and each named by a gate target; "
          f"{len(list(GUARDS.rglob('test_*.py')))} test files, each running something and each named by a "
          f"gate target or covered by a discovery walk)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
