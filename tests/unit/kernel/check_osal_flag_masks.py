#!/usr/bin/env python3
"""Event-group masks must be validated before they reach the flag word.

These calls return a flag word, so an error is reported inside that word - bit 31 of it - and the same
byte is where FreeRTOS keeps its control bits. A mask that reaches the word unchecked therefore does
two bad things at once: it lets a caller store the error answer as a flag (after which a real failure
cannot be told from a "flag"), and it silently accepts FreeRTOS control bits as data.

The bare-metal backend did exactly that: xy_os_event_flags_set(ef, 0x80000000) made
xy_os_event_flags_get return 0x80000000, which is what the same call returns when it fails. A mask of
zero went through too, asking to set nothing.

So each of set, clear and wait must refuse a mask that is empty or touches the reserved range. The
check follows one hop into a local helper, because whether the rule is written inline or in
bm_event_mask_valid is a style choice, and a rule that only accepted the inline form would report
working code as broken.
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
TARGETS = ("xy_os_event_flags_set", "xy_os_event_flags_clear", "xy_os_event_flags_wait")
RESERVED = "XY_OS_EVENT_FLAGS_RESERVED"
#: Both polarities: `if (!flags) refuse` and `if (flags != 0 && valid) accept` say the same thing.
NOT_EMPTY = re.compile(r"flags\s*==\s*0U?|!flags\b|flags\s*==\s*0x0U?|flags\s*!=\s*0U?")

#: Backends that still take a raw mask. None of them compiles in this build, so a change here would be
#: an edit no compiler and no test can check - recorded rather than guessed at.
PINNED = {
    "cmsis_rtx": "compiled by no target in this build; the vendor call is the only validation path",
    "rtthread": "compiled by no target in this build",
    "rtx5": "orphaned backend: no OSAL_BACKEND branch names this directory",
}


def bodies(source: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for match in DEFINITION.finditer(source):
        body = braced_body(source[match.end():])
        if body:
            out.setdefault(match.group(1), body)
    return out


def with_helpers(name: str, all_bodies: dict[str, str], depth: int = 1, seen: set[str] | None = None) -> str:
    """The function plus the bodies of the local helpers it calls, to the given depth."""
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


def findings(backend_dir: Path = BACKENDS, pinned: dict[str, str] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    problems: list[str] = []
    seen_targets = 0
    unpinned_files = 0
    for path in sorted(backend_dir.glob("*/xy_os_*.c")):
        name = path.parent.name
        if name in pinned:
            continue
        unpinned_files += 1
        all_bodies = bodies(read_code(path))
        for target in TARGETS:
            if target not in all_bodies:
                continue
            seen_targets += 1
            text = with_helpers(target, all_bodies)
            if RESERVED not in text:
                problems.append(f"{name}:{target}: does not test the mask against {RESERVED}, so a "
                                f"caller can store the error answer or a control bit as a flag")
            elif not NOT_EMPTY.search(text):
                problems.append(f"{name}:{target}: never refuses an empty mask, which asks for nothing")
    # Counting zero targets and counting "all compliant" look identical, so say which happened - but
    # only when there was an unpinned backend to look in.
    if unpinned_files and seen_targets == 0:
        problems.append("no event-group entries were found - the walk is broken")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("osal_flag_masks failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    checked = sum(1 for path in sorted(BACKENDS.glob("*/xy_os_*.c"))
                  if path.parent.name not in PINNED
                  for target in TARGETS if target in read_code(path))
    print(f"osal_flag_masks ok ({checked} event-group entries validate their mask; pinned backends: "
          f"{', '.join(sorted(PINNED)) or 'none'})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
