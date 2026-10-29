#!/usr/bin/env python3
"""RTC contract guard: alarm ids and formats are arguments, not decoration.

Two defects in this family have already been found and fixed by hand - U5 answered a request for
alarm B out of alarm A's slot, and a backend that cannot produce BCD would hand back binary digits a
caller reads as BCD. Neither was caught by anything; both were found by reading the code. This guard
is what reads it from now on.

A backend may refuse what it cannot do. What it may not do is accept the argument, ignore it, and
report success.

Coverage is pinned per backend rather than required, because it is not a decision: the L4 backend
implements two of the fifteen entries and that is the Pandora's own RTC. Pinning keeps the gap on the
record and makes progress visible - the same both-ways rule the PC coverage guard uses.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components/hal/inc/xy_hal_rtc.h"

BACKENDS = {
    "stm32l4": ROOT / "components/hal/stm32/stm32l4/xy_hal_rtc.c",
    "stm32u5": ROOT / "components/hal/stm32/stm32u5/xy_hal_rtc.c",
    "stm32f4": ROOT / "components/hal/stm32/stm32f4/xy_hal_rtc.c",
    "wch": ROOT / "components/hal/wch/ch32x/src/xy_hal_rtc.c",
    "pc": ROOT / "components/hal/PC/xy_hal_rtc_pc.c",
}

# How many of the public entries each backend defines today. Not a target - a record. An entry gained
# fails the guard until the number is raised, and an entry lost fails it because it dropped.
PINNED_COVERAGE = {
    "stm32l4": 15,
    "stm32u5": 15,
    "stm32f4": 15,
    "wch": 15,
    "pc": 15,
}

ALARM_ENTRIES = (
    "xy_hal_rtc_set_alarm",
    "xy_hal_rtc_get_alarm",
    "xy_hal_rtc_enable_alarm",
    "xy_hal_rtc_disable_alarm",
)
FORMAT_ENTRIES = (
    "xy_hal_rtc_set_time",
    "xy_hal_rtc_get_time",
    "xy_hal_rtc_set_date",
    "xy_hal_rtc_get_date",
)
BACKUP_ENTRIES = ("xy_hal_rtc_backup_read", "xy_hal_rtc_backup_write")

DISCARDS_ALARM_ID = re.compile(r"\(\s*void\s*\)\s*alarm_id|XY_UNUSED\s*\(\s*alarm_id\s*\)")
DISCARDS_FORMAT = re.compile(r"\(\s*void\s*\)\s*format|XY_UNUSED\s*\(\s*format\s*\)")
SELECTS_ID = re.compile(r"alarm_id\s*(?:==|!=)")
HONOURS_FORMAT = re.compile(r"format\s*(?:==|!=)")
REFUSES = re.compile(r"XY_HAL_(?:ERROR_)?NOT_SUPPORTED")
BOUNDS = re.compile(r"XY_HAL_RTC_BACKUP_REGISTER_COUNT")
# The target has no dependable notion of local time, and both of these signal failure with a value a
# caller cannot distinguish from a real one: mktime returns -1, localtime returns NULL.
FORBIDDEN_TIME_LIB = re.compile(r"\b(?:mktime|localtime|gmtime)\s*\(")
# Returning a plausible-looking number that is wrong. The U5 backend did exactly this until the
# arithmetic moved into a shared helper that host tests compare against known dates.
APPROXIMATE_CALENDAR = re.compile(r"\*\s*365\b|\*\s*30\s*\*\s*24|\(\s*\w+\s*-\s*70\s*\)\s*\*")
USES_CIVIL = re.compile(r"xy_civil_(?:days_from_epoch|seconds_from_epoch|from_days)")
DERIVES_FROM_DATE = re.compile(r"xy_hal_rtc_get_date|date\.|->date")
RAW_COUNTER = re.compile(r"RTC_GetCounter\s*\(")
COUNTER_DOCUMENTED = re.compile(r"seconds since the epoch")


LOCAL_CALL = re.compile(r"\b(\w+)\s*\(")


def braced(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        return ""
    brace = source.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace:index + 1]
    return ""


def local_definitions(source: str) -> dict[str, str]:
    names = set(re.findall(r"^static\s+[^;{=]*?\b(\w+)\s*\(", source, re.M))
    return {name: braced(source, name + "(") for name in names}


def with_delegation(source: str, body: str, hops: int = 2) -> str:
    definitions = local_definitions(source)
    seen: set[str] = set()
    for _ in range(hops):
        added = False
        for name in LOCAL_CALL.findall(body):
            if name in definitions and name not in seen and definitions[name]:
                seen.add(name)
                body = body + "\n" + definitions[name]
                added = True
        if not added:
            break
    return body


def declared_entries(header: Path | None = None) -> set[str]:
    text = read_code(header or HEADER)
    names = set(re.findall(r"\b(xy_hal_rtc_\w+)\s*\(", text))
    return {name for name in names if name != "xy_hal_rtc_callback_t"}


def defined_entries(source: str, declared: set[str] | None = None) -> set[str]:
    """Only the public entries count. A backend may have helpers of its own; they are not coverage."""
    names = set(re.findall(r"\b(xy_hal_rtc_\w+)\s*\([^;{]*?\)\s*\{", source))
    return names if declared is None else names & declared


def coverage_findings(source: str, name: str, pinned: int,
                      declared: set[str] | None = None) -> list[str]:
    defined = defined_entries(source, declared)
    actual = len(defined)
    if actual > pinned:
        gained = sorted(defined)[:3]
        return [f"{name}: defines {actual} RTC entries, pin says {pinned} "
                f"({', '.join(gained)} …) - raise the pin so the gain is on the record"]
    if actual < pinned:
        return [f"{name}: defines {actual} RTC entries, pin says {pinned} - an entry was lost"]
    return []


def call_arguments(body: str, name: str) -> list[str]:
    """Argument text of every call to `name` in `body`, by paren balance.

    A regex over the arguments cannot do this: `[^;{}]*` runs straight past the closing paren of the
    first call and reports a later sibling's argument as this call's, which produced findings against
    helpers that were plainly correct.
    """
    found: list[str] = []
    index = 0
    while True:
        index = body.find(name + "(", index)
        if index < 0:
            return found
        open_at = index + len(name)
        depth = 0
        for position in range(open_at, len(body)):
            if body[position] == "(":
                depth += 1
            elif body[position] == ")":
                depth -= 1
                if depth == 0:
                    found.append(body[open_at + 1:position])
                    index = position + 1
                    break
        else:
            return found


def argument_findings(source: str, name: str, entries: tuple[str, ...],
                      argument: str, selects: re.Pattern[str], discards: re.Pattern[str],
                      noun: str) -> list[str]:
    """The argument must change what happens, or the call must be refused.

    Handing the argument to a local helper is not the same as acting on it. with_delegation merges
    the helper's text into the entry, so a validator that compares the value and then always returns
    the same status satisfies any rule that only looks for a comparison - which is how a backend that
    accepted BCD and answered in binary stayed green through two attempts at this rule. The helper
    that receives the argument has to branch on it.
    """
    problems: list[str] = []
    local = local_definitions(source)
    for entry in entries:
        body = braced(source, entry + "(")
        if not body:
            continue  # absent entries are the coverage question, not this one
        resolved = with_delegation(source, body)
        discards_it = discards.search(resolved) is not None
        uses_it = selects.search(resolved) is not None
        refuses = REFUSES.search(resolved) is not None
        # Follow the argument into any local helper that receives it, whatever else the entry does.
        # This has to run before the classification below: a helper that refuses for an unrelated
        # reason (not initialised, say) satisfies the "refuses" escape hatch while the argument itself
        # still changes nothing, which is how the mutated validator stayed green.
        followed = False
        for helper, helper_body in local.items():
            if not any(re.search(rf"\b{argument}\b", args)
                       for args in call_arguments(body, helper)):
                continue
            if selects.search(helper_body) is not None:
                continue
            problems.append(
                f"{name}:{entry}: hands {argument} to {helper}() which "
                f"{'discards it' if discards.search(helper_body) else 'never branches on it'}, "
                f"so every request is answered the same way while reporting success")
            followed = True
        if followed:
            continue
        if uses_it:
            continue
        if discards_it and refuses:
            continue  # an honest "I cannot do this", not a silent success
        problems.append(
            f"{name}:{entry}: does not let {noun} change what happens"
            if not discards_it else
            f"{name}:{entry}: discards {argument} and does not refuse, so every request is "
            f"answered the same way while reporting success")
    return problems


def timestamp_findings(source: str, name: str, raw: str | None = None) -> list[str]:
    """A timestamp is either the epoch-based value every other backend gives, or it is refused."""
    body = with_delegation(source, braced(source, "xy_hal_rtc_get_timestamp("))
    if not body:
        return []
    problems: list[str] = []
    if FORBIDDEN_TIME_LIB.search(body):
        problems.append(
            f"{name}:xy_hal_rtc_get_timestamp: uses the C time library, whose local-time dependency "
            f"nothing sets on this target and whose error value is a valid timestamp")
    if APPROXIMATE_CALENDAR.search(body):
        problems.append(
            f"{name}:xy_hal_rtc_get_timestamp: computes the calendar by approximation (365-day years "
            f"or 30-day months), which returns a plausible number that is wrong")
    if DERIVES_FROM_DATE.search(body) and not USES_CIVIL.search(body):
        problems.append(
            f"{name}:xy_hal_rtc_get_timestamp: derives seconds from the date registers without the "
            f"shared calendar arithmetic, so this backend's timestamps differ from the others'")
    if RAW_COUNTER.search(body) and not USES_CIVIL.search(body):
        # A counter is a legitimate answer only if the caller is told what it counts; otherwise two
        # backends return different things under one name.
        if raw is None or not COUNTER_DOCUMENTED.search(raw):
            problems.append(
                f"{name}:xy_hal_rtc_get_timestamp: returns the hardware counter without saying what "
                f"it counts, so its value cannot be compared with another backend's timestamp")
    return problems


def backup_findings(source: str, name: str) -> list[str]:
    problems: list[str] = []
    for entry in BACKUP_ENTRIES:
        body = braced(source, entry + "(")
        if not body:
            continue
        if not BOUNDS.search(body):
            problems.append(
                f"{name}:{entry}: does not bound the register index, so an out-of-range index "
                f"reaches the hardware")
    return problems


def findings(backends: dict[str, Path] | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    declared = declared_entries(header)
    problems: list[str] = []
    for name, path in sorted(backends.items()):
        source = read_code(path)
        problems.extend(coverage_findings(source, name, PINNED_COVERAGE.get(name, 0), declared))
        problems.extend(argument_findings(source, name, ALARM_ENTRIES, "alarm_id", SELECTS_ID,
                                          DISCARDS_ALARM_ID, "the alarm id"))
        problems.extend(argument_findings(source, name, FORMAT_ENTRIES, "format", HONOURS_FORMAT,
                                          DISCARDS_FORMAT, "the requested format"))
        problems.extend(timestamp_findings(source, name, read_raw(path)))
        problems.extend(backup_findings(source, name))

    text = read_raw(header or HEADER)
    if "alarm_id" not in text or "selects" not in text.lower():
        problems.append("xy_hal_rtc.h: does not document that alarm_id selects which alarm is used")
    if "BCD" not in text or "refus" not in text.lower():
        problems.append("xy_hal_rtc.h: does not document that a backend which cannot produce the "
                        "requested format must refuse rather than convert silently")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_rtc_contract failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    counts = ", ".join(f"{name} {count}/{len(declared_entries())}"
                       for name, count in sorted(PINNED_COVERAGE.items()))
    print(f"hal_rtc_contract ok ({len(BACKENDS)} backends: alarm id selects or is refused, "
          f"format honoured or refused, backup index bounded; coverage {counts})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
