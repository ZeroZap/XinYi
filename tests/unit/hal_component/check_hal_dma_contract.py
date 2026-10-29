#!/usr/bin/env python3
"""The DMA item count is 16 bits wide on every backend, and one unit meaning.

HAL_DMA_Start programs DMA_SxNDTR, which holds 16 bits. The WCH backend refused data_len > 0xFFFF
from the start; L4, U5 and F4 passed any size_t through, so 70000 items became 4464 and the transfer
stopped early - with every call reporting success. The same 16-bit counter exists on both families,
so "the vendor will complain" was never true.

The unit is items of the configured width, not bytes, and the header said only "Data length".
Pandora passes 8 for eight uint32 words; openCH passes a byte count with byte widths. Either is
correct as an item count and neither is discoverable from the documentation.

Also asks each backend what it says about a channel it was never given: U5 and F4 answer NOT_INIT,
L4 used to report the vendor abort failure, so "never set up" and "abort failed" were the same
answer.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_dma.h"

BACKENDS = {
    "stm32l4": ROOT / "components/hal/stm32/stm32l4/xy_hal_dma.c",
    "stm32u5": ROOT / "components/hal/stm32/stm32u5/xy_hal_dma.c",
    "stm32f4": ROOT / "components/hal/stm32/stm32f4/xy_hal_dma.c",
    "wch": ROOT / "components/hal/wch/ch32x/src/xy_hal_dma.c",
}

REFUSES_ZERO = re.compile(r"data_len\s*==\s*0U?")
CAPS_COUNT = re.compile(r"data_len\s*>\s*(?:0xFFFFU?|UINT16_MAX|65535U?)")
NOT_INIT = "XY_HAL_ERROR_NOT_INIT"
# Each enum in the config is mapped through a helper with a `default:` branch, so an out-of-range
# value is not an error - it becomes a concrete hardware setting. Refusing the config is the only
# thing that stops a typo from changing the transfer shape.
CONFIG_FIELDS = ("direction", "mode", "priority", "periph_width", "mem_width", "periph_incr",
                 "mem_incr")
LOCAL_CALL = re.compile(r"\b(\w+)\s*\(")
# A mode written as a constant while the config validator accepts that mode: the caller's choice is
# silently replaced. Either honour the flag or refuse it.
CONSTANT_NORMAL_MODE = re.compile(r"Init\.Mode\s*=\s*DMA_NORMAL\s*;")
REFUSES_CIRCULAR = re.compile(r"XY_HAL_DMA_MODE_CIRCULAR[\s\S]{0,200}?XY_HAL_ERROR_NOT_SUPPORTED")
# A vendor callback registration whose result nobody looks at.
UNCHECKED_REGISTRATION = re.compile(
    r"(?<![=!]=)\s*HAL_DMA_RegisterCallback\s*\(", re.M)
# Either translation of "0 waits forever": HAL's own sentinel, or an explicit zero test.
UNBOUNDED_WAIT = re.compile(r"HAL_MAX_DELAY|timeout\s*!=\s*0U?")
# The vendor poll taking the caller's value unchanged - which means 0 fails at once.
BARE_TIMEOUT = re.compile(
    r"PollForTransfer\s*\([^;]*?,\s*timeout\s*\)", re.S)


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


def function_body_local(source: str, signature: str) -> str:
    return function_body(source, signature)


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


def mode_findings(source: str, name: str) -> list[str]:
    """Circular mode must be honoured or refused, never replaced by normal mode."""
    body = function_body(source, "xy_hal_dma_init(")
    resolved = with_delegation(body, local_definitions(source)) if body else ""
    if not resolved:
        return []
    if CONSTANT_NORMAL_MODE.search(resolved) and not REFUSES_CIRCULAR.search(source):
        return [f"{name}: init writes a constant normal mode while accepting circular mode, so a "
                f"caller that asks for circular gets a normal transfer and success"]
    return []


def callback_findings(source: str, name: str) -> list[str]:
    """A failed vendor registration leaves every registered callback unreachable.

    The statement around each call decides it: `if (HAL_DMA_RegisterCallback(...) != HAL_OK)` or an
    assignment looks at the result, a bare call statement does not. Reading only the token before the
    call marks the checked shape as unchecked - it did, on all three backends.
    """
    for match in UNCHECKED_REGISTRATION.finditer(source):
        line_start = source.rfind("\n", 0, match.start()) + 1
        end = source.find(";", match.end())
        statement = source[line_start:end if end > 0 else len(source)]
        if "HAL_OK" not in statement:
            return [f"{name}: HAL_DMA_RegisterCallback result is ignored, so the interrupt shims may "
                    f"be absent while init reports success and callbacks never fire"]
    return []


ROUTING_FIELD = re.compile(r"\bInit\.(?:Request|Channel)\b")
COMMENT = re.compile(r"/\*[\s\S]*?\*/|//[^\n]*")


def routing_findings(source: str, name: str, raw: str | None = None) -> list[str]:
    """Whoever programs a transfer must say who owns the routing.

    A backend that writes Init.Request or Init.Channel conditionally, or leaves it alone entirely, is
    deciding something about peripheral routing - and a request left at zero on a DMAMUX part is
    muxed onto another peripheral's request with no error. The note is the only thing that tells a
    caller to fill it in; the backend that had no note was the one that silently went without.
    """
    # The note lives in a comment, and read_code() strips comments - this rule has to look at the
    # text as written or it can never see the thing it asks for.
    text = source if raw is None else raw
    if not ROUTING_FIELD.search(source):
        return []
    for comment in COMMENT.findall(text):
        if ROUTING_FIELD.search(comment.replace("hdma->Init.", "Init.")) or (
                "Request" in comment or "Channel" in comment):
            if "caller" in comment.lower() or "owned by" in comment.lower():
                return []
    return [f"{name}: programs or leaves Init.Request/Init.Channel alone without saying that routing "
            f"is the caller's, so a transfer can be muxed onto another peripheral's request"]


def counter_findings(source: str, name: str) -> list[str]:
    """Reading a counter from a channel that was never set up returns a plausible number."""
    body = function_body(source, "xy_hal_dma_get_counter(")
    if not body:
        return [f"{name}: xy_hal_dma_get_counter is missing"]
    if NOT_INIT not in body:
        return [f"{name}: get_counter answers a counter register for a channel that was never set "
                f"up, so an uninitialised read is indistinguishable from a real remaining count"]
    return []


def config_findings(source: str, name: str) -> list[str]:
    """An out-of-range enum must be refused, not defaulted by a mapping helper."""
    body = function_body(source, "xy_hal_dma_init(")
    if not body:
        return []
    resolved = with_delegation(body, local_definitions(source))
    problems: list[str] = []
    missing = [field for field in CONFIG_FIELDS
               if not re.search(rf"\b{field}\s*(?:>=|>)\s*\w", resolved)]
    if missing:
        problems.append(
            f"{name}: init does not refuse an out-of-range config field ({', '.join(missing)}); "
            f"the mapping helpers default instead, so a typo silently changes the transfer")
    return problems


def source_of(value) -> str:
    return read_code(value) if isinstance(value, Path) else value


def findings(backends: dict | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    header = HEADER if header is None else header
    problems: list[str] = []

    for name, path in sorted(backends.items()):
        source = source_of(path)
        raw = read_raw(path) if isinstance(path, Path) else source
        problems.extend(config_findings(source, name))
        problems.extend(mode_findings(source, name))
        problems.extend(callback_findings(source, name))
        problems.extend(counter_findings(source, name))
        problems.extend(routing_findings(source, name, raw))
        start_body = function_body(source, "xy_hal_dma_start(")
        if not start_body:
            problems.append(f"{name}: xy_hal_dma_start is missing")
        else:
            if not REFUSES_ZERO.search(start_body):
                problems.append(f"{name}: start accepts a zero item count")
            if not CAPS_COUNT.search(start_body):
                problems.append(
                    f"{name}: start does not refuse an item count above the hardware counter, so a "
                    f"larger count is truncated and the transfer stops early while reporting success")
        for entry in ("xy_hal_dma_stop(", "xy_hal_dma_poll_complete("):
            body = function_body(source, entry)
            if not body:
                problems.append(f"{name}: {entry.rstrip('(')} is missing")
            elif NOT_INIT not in body:
                problems.append(
                    f"{name}: {entry.rstrip('(')} does not answer an uninitialised channel with "
                    f"NOT_INIT, so 'never set up' looks like a vendor failure")

    for name, path in sorted(backends.items()):
        source = source_of(path)
        body = function_body(source, "xy_hal_dma_poll_complete(")
        if not body:
            continue
        if BARE_TIMEOUT.search(body):
            problems.append(
                f"{name}: poll_complete hands the caller's timeout to the vendor poll unchanged, so "
                f"0 means an immediate timeout here and an unbounded wait on WCH")
        elif not UNBOUNDED_WAIT.search(body):
            problems.append(
                f"{name}: poll_complete never states what a zero timeout means, so an unbounded "
                f"wait cannot be requested portably")

    text = read_raw(header)
    if "owned by the caller" not in text:
        problems.append(
            "xy_hal_dma.h: init does not say that the channel and peripheral request are the "
            "caller's, so a transfer can be routed to the wrong request without an error")
    if "does not enable the interrupt" not in text:
        problems.append(
            "xy_hal_dma.h: register_callback does not say that registering a callback leaves the "
            "interrupt line alone, so a callback that never runs looks like a broken callback")
    if "waits indefinitely" not in text:
        problems.append("xy_hal_dma.h: poll_complete's zero timeout is not documented")
    if "Number of items" not in text:
        problems.append(
            "xy_hal_dma.h: data_len is not documented as a count of items of the configured width")
    if "0xFFFF" not in text:
        problems.append("xy_hal_dma.h: the item counter limit is not documented")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_dma_contract failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"hal_dma_contract ok ({len(BACKENDS)} backends: item count capped, uninitialised "
          f"channel named)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
