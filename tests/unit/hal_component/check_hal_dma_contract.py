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


def source_of(value) -> str:
    return read_code(value) if isinstance(value, Path) else value


def findings(backends: dict | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    header = HEADER if header is None else header
    problems: list[str] = []

    for name, path in sorted(backends.items()):
        source = source_of(path)
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
