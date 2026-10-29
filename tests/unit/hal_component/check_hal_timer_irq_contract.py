#!/usr/bin/env python3
"""Arming a timer interrupt must prepare the interrupt, and only the interrupt.

Two shapes were wrong and both reported XY_HAL_OK:

  * L4 and F4 armed the interrupt by calling HAL_TIM_Base_Start_IT(), which starts the counter, and
    disarmed it with HAL_TIM_Base_Stop_IT(), which stops timekeeping. F4 ignored `event`
    altogether, so asking for a compare match got the update interrupt.
  * No STM32 backend enabled the NVIC line for the timer it had just enabled in the peripheral.
    The interrupt was armed where nothing could deliver it - the same defect the UART receive path
    had, found the same way.

So the questions asked here are: does `enable_irq` start or stop the timer, does the resolved body
reach the NVIC (following one hop, because each backend arms through a helper), and does it look at
`event` at all.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_timer.h"

BACKENDS = {
    "stm32l4": ROOT / "components/hal/stm32/stm32l4/xy_hal_timer.c",
    "stm32u5": ROOT / "components/hal/stm32/stm32u5/xy_hal_timer.c",
    "stm32f4": ROOT / "components/hal/stm32/stm32f4/xy_hal_timer.c",
    "wch": ROOT / "components/hal/wch/ch32x/src/xy_hal_timer.c",
    # The host backend models the same contract; it must be held to it too.
    "pc": ROOT / "components/hal/PC/xy_hal_timer_pc.c",
}

# Starting or stopping the counter as part of arming an interrupt.
STARTS_COUNTER = re.compile(r"HAL_TIM_Base_Start\w*\s*\(")
STOPS_COUNTER = re.compile(r"HAL_TIM_Base_Stop\w*\s*\(")
# Either NVIC idiom the tree uses: CubeHAL's HAL_NVIC_EnableIRQ, or the CH32 NVIC_Init path.
ARMS_NVIC = re.compile(r"HAL_NVIC_EnableIRQ\s*\(|NVIC_Init\s*\(|NVIC->ISER")
# Two ways to discard an argument, and the guard only knew the first: the tree's XY_UNUSED macro, and
# a plain cast-to-void. A backend that wrote `(void)event;` armed whatever it liked and read clean.
DISCARDS_EVENT = re.compile(r"XY_UNUSED\s*\(\s*event\s*\)|\(\s*void\s*\)\s*event\s*;")
# Starting the counter has a vendor idiom (HAL_TIM_Base_Start) and a software one (setting the flag
# the backend's own start() sets). The guard knew only the vendor spelling, so a host backend could
# start timekeeping from enable_irq and stay green.
STARTS_IN_SOFTWARE = re.compile(r"\b(?:running|started|enabled_running)\s*=\s*1U?\s*;")
# A backend with no NVIC still has to make arming mean something: it sets a per-event flag, and some
# delivery path reads that flag and calls the callback table. Without the second half the flag is
# decoration, which is the same defect as enabling a peripheral interrupt nothing can deliver.
ARMS_FLAG = re.compile(r"armed\s*\[[^\]]*\]\s*=\s*1U?")
READS_FLAG = re.compile(r"armed\s*\[")
CALLS_BACK = re.compile(r"callbacks\s*\[")
USES_EVENT = re.compile(r"\bevent\b")
LOCAL_CALL = re.compile(r"\b(\w+)\s*\(")


def function_body(source: str, signature: str) -> str:
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
    names = set(re.findall(r"^static\s+\w[\w\s\*]*?\b(\w+)\s*\(", source, re.M))
    return {name: function_body(source, name + "(") for name in names}


def with_delegation(body: str, definitions: dict[str, str]) -> str:
    seen: list[str] = []
    for _ in range(2):
        added = False
        for name in LOCAL_CALL.findall(body):
            if name in definitions and name not in seen:
                seen.append(name)
                body += "\n" + definitions[name]
                added = True
        if not added:
            break
    return body


def source_of(value) -> str:
    """Path or literal source text - cases hand in snippets, the repo hands in files."""
    return read_code(value) if isinstance(value, Path) else value


def findings(backends: dict | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    header = HEADER if header is None else header
    problems: list[str] = []

    for name, path in sorted(backends.items()):
        source = source_of(path)
        definitions = local_definitions(source)
        for entry, forbidden, label in (
            ("xy_hal_timer_enable_irq(", STARTS_COUNTER, "starts the counter"),
            ("xy_hal_timer_disable_irq(", STOPS_COUNTER, "stops the counter"),
        ):
            body = function_body(source, entry)
            if not body:
                problems.append(f"{name}: {entry.rstrip('(')} is missing")
                continue
            resolved = with_delegation(body, definitions)
            where = f"{name}:{entry.rstrip('(')}"
            starts = forbidden.search(resolved) or (
                entry == "xy_hal_timer_enable_irq(" and STARTS_IN_SOFTWARE.search(resolved))
            if starts:
                problems.append(
                    f"{where}: {label} - arming or masking an interrupt is not starting or "
                    f"stopping the timer, and a caller that only wanted the interrupt changes "
                    f"timekeeping as a side effect")
            if DISCARDS_EVENT.search(resolved):
                problems.append(
                    f"{where}: discards `event`, so every request is answered as whichever "
                    f"interrupt the implementation happens to enable")
            elif not USES_EVENT.search(resolved):
                problems.append(f"{where}: never reads `event`")
        arm = with_delegation(function_body(source, "xy_hal_timer_enable_irq("), definitions)
        if arm and not ARMS_NVIC.search(arm):
            software = (ARMS_FLAG.search(arm) is not None
                        and READS_FLAG.search(source) is not None
                        and CALLS_BACK.search(source) is not None)
            if not software:
                problems.append(
                    f"{name}: enable_irq never reaches the NVIC, so the interrupt is enabled in the "
                    f"peripheral and can never reach the CPU - reported as success, and no "
                    f"delivery path reads what it armed")

    text = read_raw(header)
    if "does not start" not in text or "NVIC" not in text:
        problems.append(
            "xy_hal_timer.h: arming an interrupt is not documented as separate from starting the "
            "timer, nor as owning the NVIC line")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_timer_irq_contract failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"hal_timer_irq_contract ok ({len(BACKENDS)} backends: event honoured, NVIC armed, "
          f"counter untouched)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
