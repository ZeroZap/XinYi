#!/usr/bin/env python3
"""SPI transfers count bytes, and HAL counts frames.

`len` is a byte count in the public API - WCH walks bytes one at a time, and 16-bit data still
arrives as a byte buffer. HAL's Size parameter counts *frames*, and a frame is two bytes when the
peripheral is configured for 16-bit data. L4 and U5 handed the byte count straight to
HAL_SPI_Transmit/Receive, so a 4-byte transfer of 16-bit data asked for 4 frames and read 8 bytes
out of a 4-byte buffer - a buffer overread that only appears once a caller uses 16-bit frames.
F4 already converted; so did the F4 validator's odd-length rule, which nobody else had written
down.

Both also compared the byte count against UINT16_MAX. That is the wrong side of the conversion:
65536 bytes is 32768 frames and perfectly legal, while the check rejected it.

This guard asks each backend the same questions: does a converted frame count reach HAL, is an odd
byte count refused at 16-bit, is zero refused, and does the header say `len` is bytes. A function
that delegates is followed one hop, because two of these backends do their checking in a helper
(WCH receive -> transmit_receive, F4 -> spi_validate_transfer) and a guard that reads only the
entry point's own lines would call them unchecked.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_spi.h"

BACKENDS = {
    "stm32l4": ROOT / "components/hal/stm32/stm32l4/xy_hal_spi.c",
    "stm32u5": ROOT / "components/hal/stm32/stm32u5/xy_hal_spi.c",
    "stm32f4": ROOT / "components/hal/stm32/stm32f4/xy_hal_spi.c",
    "wch": ROOT / "components/hal/wch/ch32x/src/xy_hal_spi.c",
    # The host backend models the same contract and programs no vendor data size at all.
    "pc": ROOT / "components/hal/PC/xy_hal_spi_pc.c",
}

ENTRY_POINTS = (
    "xy_hal_spi_transmit(", "xy_hal_spi_receive(", "xy_hal_spi_transmit_receive(",
    "xy_hal_spi_transmit_dma(", "xy_hal_spi_receive_dma(",
    "xy_hal_spi_transmit_receive_dma(",
)

# A HAL call whose size argument is the caller's byte count rather than a converted frame count.
RAW_BYTE_COUNT = re.compile(r"HAL_SPI_\w+\s*\([^;]*?\(uint16_t\)\s*len\b")
ZERO_REFUSED = re.compile(r"len\s*==\s*0U?|len\s*!=\s*0U?|len\s*<\s*1U?")
ODD_REFUSED = re.compile(r"len\s*&\s*1U?|len\s*%\s*2U?")
HALVES_FRAMES = re.compile(r"len\s*>>=\s*1U?|len\s*/=\s*2U?")
SIXTEEN_BIT = re.compile(r"DATASIZE_16BIT")
FRAME_LIMIT = re.compile(r"frames?\s*>\s*UINT16_MAX|len\s*>\s*UINT16_MAX")
LOCAL_CALL = re.compile(r"\b(\w+)\s*\(")
# The public enums are XY_HAL_SPI_*; matching "HAL_SPI_" found those and made the byte-oriented
# backend look HAL-backed, so the rule asked it about a frame register it does not have.
HAL_CALL = re.compile(r"\bHAL_SPI_(?:Transmit|Receive|TransmitReceive|Init|DeInit)\w*\s*\(")
# Two shapes program the mode: the constant assigned in init, or returned from a mapping
# helper (F4 maps through spi_datasize(), so the constant is not on the assignment line).
PROGRAMS_SIXTEEN_BIT = re.compile(
    r"return\s+SPI_DATASIZE_16BIT|DataSize\s*=\s*[^;]*16(?:b|BIT)")
REFUSES_SIXTEEN_BIT = re.compile(
    r"DATASIZE_16BIT[\s\S]{0,200}?XY_HAL_ERROR_NOT_SUPPORTED")


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
    """Functions defined in this file, so a delegating entry point can be followed."""
    names = set(re.findall(r"^static\s+\w[\w\s\*]*?\b(\w+)\s*\(", source, re.M))
    return {name: function_body(source, name + "(") for name in names}


def with_delegation(body: str, definitions: dict[str, str]) -> str:
    """A body plus the bodies of up to two local functions it calls."""
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


def byte_count_findings(body: str, where: str) -> list[str]:
    problems: list[str] = []
    if not body:
        return problems
    if RAW_BYTE_COUNT.search(body):
        problems.append(
            f"{where}: passes the caller's byte count to HAL as a frame count; with 16-bit data "
            f"HAL reads twice the buffer")
    return problems


def validation_findings(body: str, where: str, hal_backed: bool) -> list[str]:
    problems: list[str] = []
    if not ZERO_REFUSED.search(body):
        problems.append(f"{where}: does not refuse a zero length")
    if hal_backed and not (SIXTEEN_BIT.search(body) and ODD_REFUSED.search(body)):
        problems.append(
            f"{where}: never checks an odd byte count against 16-bit frames, so the conversion "
            f"either does not exist or rounds silently")
    if hal_backed and not FRAME_LIMIT.search(body):
        problems.append(
            f"{where}: never compares the converted frame count with UINT16_MAX; comparing the "
            f"byte count instead rejects legal 16-bit transfers")
    return problems


REFUSES_PARTIAL_FRAME = re.compile(r"%\s*2U?\s*\)\s*!=\s*0U?|&\s*1U\s*\)\s*!=\s*0U?")


def sixteen_bit_findings(source: str, where: str) -> list[str]:
    """A backend may convert 16-bit frames or refuse the mode - but not ignore which it is.

    WCH programmed SPI_DataSize_16b and then sent one byte per frame: `len` frames on the wire
    where `len / 2` were meant, each carrying one byte and a zero high byte. Convert, or do not
    program the mode.
    """
    programms = bool(PROGRAMS_SIXTEEN_BIT.search(source))
    refuses = bool(REFUSES_SIXTEEN_BIT.search(source))
    converts = bool(ODD_REFUSED.search(source) and HALVES_FRAMES.search(source))
    if programms and not converts:
        return [f"{where}: programs a 16-bit data size but never converts the byte count into "
                f"frames, so each byte goes out as its own 16-bit frame"]
    if not programms and not refuses:
        # A backend with no vendor data-size register - the host one - has nothing to program. What
        # it can still get wrong is accepting a count that is not whole frames, so that refusal is
        # the shape it must show.
        if REFUSES_PARTIAL_FRAME.search(source):
            return []
        return [f"{where}: neither converts 16-bit frames nor refuses the mode, so a 16-bit "
                f"request is accepted and silently sent as bytes"]
    return []


def findings(backends: dict[str, Path] | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    header = HEADER if header is None else header
    problems: list[str] = []
    if "bytes" not in read_raw(header) or "frame" not in read_raw(header).lower():
        problems.append("xy_hal_spi.h: len is not documented as a byte count that backends "
                        "convert to frames")
    for name, path in sorted(backends.items()):
        source = read_code(path)
        problems.extend(sixteen_bit_findings(source, name))
        definitions = local_definitions(source)
        hal_backed = bool(HAL_CALL.search(source))
        for entry in ENTRY_POINTS:
            body = function_body(source, entry)
            if not body:
                continue
            resolved = with_delegation(body, definitions)
            where = f"{name}:{entry.rstrip('(')}"
            problems.extend(byte_count_findings(resolved, where))
            problems.extend(validation_findings(resolved, where, hal_backed))
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_spi_contract failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"hal_spi_contract ok ({len(BACKENDS)} backends, byte counts converted to frames once)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
