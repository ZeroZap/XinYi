#!/usr/bin/env python3
"""Two backends of one interface should refuse in the same vocabulary.

Every real defect this code has had in the OSAL layer was a difference between the two backends that
nothing compared: semaphore counts that one refused and the other reinterpreted, attribute bits one
honoured and the other stored and ignored, a mutex or a pool one refused to delete while in use and the
other deleted anyway. Each was found by reading; this makes the reading mechanical.

It compares, per entry, the set of refusal codes each backend can answer with. A difference is the
cheapest available pointer to a bug, and it is also allowed to be correct - which is why every entry that
is allowed to differ has to say why, in PINNED, in a sentence a reader can check.

Refusals are followed one level of indirection: a body that returns pdstatus_to_xy_wait(result) refuses
with whatever that helper refuses with, and a comparison that stopped at the call would report a
difference that is not there.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import braced_body, read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BACKENDS = ROOT / "components" / "kernel" / "osal" / "backend"

#: The two backends that compile and run in this project's builds.
COMPARED = ("baremetal", "freertos")

# Captures every function, not just the xy_os_ ones: an entry that answers through a local helper refuses
# with whatever the helper refuses with, and a pattern anchored on the xy_os_ prefix silently dropped helpers
# named pdstatus_to_xy_wait - which is how four entries were reported as differing when they do not.
DEFINITION = re.compile(r"^(?:[A-Za-z_][\w \t\*]*?)\b([A-Za-z_]\w*)\s*\([^;{]*\)\s*$", re.M)
CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
CODE = re.compile(r"\b(XY_OS_ERROR_\w+|XY_OS_[A-Z_]+_ERROR)\b")

#: entry -> why these two are allowed to refuse differently. Written as a checkable claim, not a name.
PINNED = {
    "xy_os_delay":
        "the FreeRTOS backend validates the tick count before handing it to vTaskDelay and refuses zero "
        "explicitly; the bare-metal one has no scheduler to delay",
    "xy_os_delay_until":
        "same as xy_os_delay: the FreeRTOS backend validates its argument, the bare-metal one has nothing "
        "to wait for",
    "xy_os_kernel_get_state":
        "the FreeRTOS backend can detect that the scheduler is not running and says so; the bare-metal "
        "one is always 'started'",
    "xy_os_timer_start":
        "the resources are different: the bare-metal timers live in a fixed pool of eight that can run "
        "out, the FreeRTOS ones are allocated per timer",
    "xy_os_thread_detach":
        "the bare-metal backend has no scheduler, so it answers NOT_SUPPORTED; the FreeRTOS one has a "
        "scheduler and refuses a bad handle instead",
    "xy_os_thread_join":
        "as xy_os_thread_detach",
    "xy_os_thread_resume":
        "as xy_os_thread_detach",
    "xy_os_thread_set_priority":
        "as xy_os_thread_detach",
    "xy_os_thread_suspend":
        "as xy_os_thread_detach",
    "xy_os_thread_terminate":
        "as xy_os_thread_detach",
}


def functions(source: str) -> dict[str, str]:
    """name -> body, for every function definition in a backend file."""
    found: dict[str, str] = {}
    for match in DEFINITION.finditer(source):
        body = braced_body(source[match.end():])
        if body:
            found[match.group(1)] = body
    return found


def refusals(bodies: dict[str, str], body: str) -> set[str]:
    """Codes a body can answer with, following one level of local helper calls."""
    codes = set(CODE.findall(body))
    for name in CALL.findall(body):
        helper = bodies.get(name)
        if helper is not None and helper != body:
            codes |= set(CODE.findall(helper))
    return codes


def findings(backend_dir: Path = BACKENDS, compared: tuple[str, str] = COMPARED,
             pinned: dict[str, str] | None = None) -> list[str]:
    pinned = PINNED if pinned is None else pinned
    parsed: dict[str, dict[str, str]] = {}
    for name in compared:
        files = sorted((backend_dir / name).glob("xy_os_*.c"))
        if not files:
            return [f"backend {name} has no source - the walk is broken"]
        merged: dict[str, str] = {}
        for path in files:
            merged.update(functions(read_code(path)))
        parsed[name] = merged

    left, right = (parsed[name] for name in compared)
    shared = sorted(name for name in set(left) & set(right) if name.startswith("xy_os_"))
    if not shared:
        return ["the two backends share no entry - the walk is broken"]

    problems: list[str] = []
    for entry in shared:
        if entry in pinned:
            continue
        only_left = refusals(left, left[entry]) - refusals(right, right[entry])
        only_right = refusals(right, right[entry]) - refusals(left, left[entry])
        if only_left or only_right:
            detail = []
            if only_left:
                detail.append(f"{compared[0]} only: {sorted(only_left)}")
            if only_right:
                detail.append(f"{compared[1]} only: {sorted(only_right)}")
            problems.append(f"{entry}: the backends refuse differently ({'; '.join(detail)}) - either "
                            f"align them or pin the entry with a reason")
    for entry in sorted(pinned):
        if entry not in shared:
            problems.append(f"{entry} is pinned but is not an entry both backends define - the pin is stale")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("osal_backend_error_codes failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"osal_backend_error_codes ok (comparing {' vs '.join(COMPARED)}; "
          f"{len(PINNED)} entries pinned as allowed to differ)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
