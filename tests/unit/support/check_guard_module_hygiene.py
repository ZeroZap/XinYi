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
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
GUARDS = ROOT / "tests" / "unit"


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
    # Two independent questions, so each has its own function and its own tests: names bound once, and test
    # files that actually run something.
    problems = findings() + entry_point_findings()
    if problems:
        print("guard_module_hygiene failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"guard_module_hygiene ok ({len(list(GUARDS.rglob('check_*.py')))} guard files, each name bound once; "
          f"{len(list(GUARDS.rglob('test_*.py')))} test files, each running something)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
