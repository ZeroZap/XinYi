#!/usr/bin/env python3
"""A workflow must not select the OSAL backend through a CMake variable.

The build derives the backend from Kconfig and refuses a request that cannot take effect:

    OSAL_BACKEND=freertos was selected, but this build derives the OSAL backend from Kconfig and is using
    baremetal. The request cannot take effect, and quietly substituting a different kernel would make every
    downstream result ambiguous.

So a workflow step that passes -DOSAL_BACKEND does not build the backend it names - it fails to configure at
all. This was written because I did exactly that: the local gate used a build directory configured long ago,
the workflow recipe was never run from scratch, and the mistake survived into a commit that claimed the
variant built.

The variable name is matched with its `=`, so the reason above and any prose about the trap does not trip it.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
WORKFLOWS = ROOT / ".github" / "workflows"

#: (workflow file, flag) pairs that are allowed with a reason. Empty on purpose.
PINNED: dict[tuple[str, str], str] = {}

#: flag -> why it cannot work here.
REJECTED = {
    "-DOSAL_BACKEND=":
        "the backend comes from Kconfig and this flag is rejected with a FATAL_ERROR, so the step fails to "
        "configure instead of building the backend it names - use -DKCONFIG_OVERRIDES=OSAL_BACKEND_<X>=ON",
}


def findings(paths: list[Path] | None = None, pinned: dict[tuple[str, str], str] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    candidates = paths if paths is not None else sorted(WORKFLOWS.glob("*.yml"))
    problems: list[str] = []
    inspected = 0
    for path in candidates:
        inspected += 1
        text = path.read_text(encoding="utf-8", errors="replace")
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        for line_number, line in enumerate(text.split("\n"), start=1):
            if line.lstrip().startswith("#"):
                continue
            for flag, reason in REJECTED.items():
                if flag not in line:
                    continue
                if (relative, flag) in pinned:
                    continue
                problems.append(f"{relative}:{line_number}: passes {flag} - {reason}")
    if inspected == 0:
        problems.append("no workflows were inspected - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("ci_build_flags failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"ci_build_flags ok ({len(list(WORKFLOWS.glob('*.yml')))} workflows, no Kconfig-derived variable "
          f"passed as a CMake flag)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
