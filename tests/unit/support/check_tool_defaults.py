#!/usr/bin/env python3
"""A command-line default that is an absolute path is a default for one machine.

The board tools in tools/ are run by whoever is at the bench, and a default of D:/MounRiver/... or
/home/<someone>/... silently means "this tool was written here". On any other machine the default is wrong
before the user types anything, and the resulting error names a path they never asked for.

PATH lookup is the fix where the tool exists in more than one place (st-flash, openocd). Where it does not -
a vendor IDE that installs into a fixed directory - the absolute path is the only option, and it has to be
named here with the reason, so that it is a decision rather than an accident.
"""

from __future__ import annotations

import ast
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TOOLS = ROOT / "tools"

ABSOLUTE = re.compile(r"^(?:[A-Za-z]:[\\/]|/)")

#: (tool, option) -> why an absolute default is the only sensible one.
PINNED = {
    ("tools/program_opench_ch32v307.py", "--mrs-root"):
        "MounRiver Studio installs itself where its installer decides; the root is only used to derive the "
        "WCH OpenOCD config, and --mrs-root overrides it",
}


def module_literals(tree: ast.Module) -> dict[str, str]:
    """name -> literal text for module-level assignments of a string or Path('...')."""
    found: dict[str, str] = {}
    pending: list[tuple[str, ast.AST]] = []
    for node in tree.body:
        if not isinstance(node, ast.Assign) or len(node.targets) != 1:
            continue
        target = node.targets[0]
        if not isinstance(target, ast.Name):
            continue
        value = node.value
        if isinstance(value, ast.Constant) and isinstance(value.value, str):
            found[target.id] = value.value
        elif (isinstance(value, ast.Call) and isinstance(value.func, ast.Name)
              and value.func.id == "Path" and value.args
              and isinstance(value.args[0], ast.Constant) and isinstance(value.args[0].value, str)):
            found[target.id] = value.args[0].value
        else:
            pending.append((target.id, value))
    # A constant aliased to another constant is still that constant. Without this the guard found nothing at
    # all: DEFAULT_MRS_ROOT = _VENDOR_MRS_ROOT is a name, not a literal, and the absolute path behind it was
    # never seen - the answer "no unpinned absolute default" was true only because nothing was looked at.
    for _ in range(3):
        progressed = False
        for name, value in list(pending):
            if isinstance(value, ast.Name) and value.id in found:
                found[name] = found[value.id]
                pending.remove((name, value))
                progressed = True
        if not progressed:
            break
    return found


def default_text(node: ast.AST, literals: dict[str, str]) -> str | None:
    if isinstance(node, ast.Constant) and isinstance(node.value, str):
        return node.value
    if isinstance(node, ast.Name):
        return literals.get(node.id)
    if (isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "Path"
            and node.args and isinstance(node.args[0], ast.Constant)
            and isinstance(node.args[0].value, str)):
        return node.args[0].value
    return None


def findings(tool: Path | None = None, pinned: dict[tuple[str, str], str] | None = None,
             paths: list[Path] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    problems: list[str] = []
    inspected = 0
    for path in paths if paths is not None else sorted(TOOLS.glob("*.py")):
        inspected += 1
        try:
            tree = ast.parse(path.read_text(encoding="utf-8", errors="replace"))
        except SyntaxError:
            problems.append(f"{path.name}: does not parse - the lint cannot see it")
            continue
        literals = module_literals(tree)
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        for node in ast.walk(tree):
            if not isinstance(node, ast.Call) or not node.args:
                continue
            func = node.func
            name = func.attr if isinstance(func, ast.Attribute) else getattr(func, "id", "")
            if name != "add_argument":
                continue
            option = None
            for arg in node.args:
                if isinstance(arg, ast.Constant) and isinstance(arg.value, str) and arg.value.startswith("-"):
                    option = arg.value
                    break
            if not option:
                continue
            default = next((kw.value for kw in node.keywords if kw.arg == "default"), None)
            if default is None:
                continue
            text = default_text(default, literals)
            if text is None or not ABSOLUTE.match(text):
                continue
            if (relative, option) in pinned:
                continue
            problems.append(
                f"{relative} {option}: defaults to the absolute path {text} - that is a default for whichever "
                f"machine has it; look the executable up on PATH, or name it in PINNED with the reason")
    if inspected == 0:
        problems.append("no tools were inspected - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("tool_defaults failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"tool_defaults ok ({len(list(TOOLS.glob('*.py')))} tools, no unpinned absolute-path default)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
